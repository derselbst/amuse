/**
 * fluidsyX - MusyX player using FluidSynth
 *
 * Plays (synthesizes) MusyX SoundMacro sequences by translating them into
 * FluidSynth sequencer events. Uses amuse's ContainerRegistry and
 * AudioGroupProject/Pool parsing to read MusyX data, and FluidSynth's
 * sequencer + audio driver for playback.
 */

#include "amuse/ContainerRegistry.hpp"
#include "amuse/AudioGroupPool.hpp"
#include "amuse/AudioGroupProject.hpp"
#include "amuse/AudioGroupData.hpp"
#include "amuse/AudioGroupSampleDirectory.hpp"
#include "amuse/DSPCodec.hpp"
#include "amuse/N64MusyXCodec.hpp"
#include "amuse/Envelope.hpp"
#include "amuse/Common.hpp"
#include "amuse/SongState.hpp"

#include <fluidsynth.h>

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <climits>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <list>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#endif

#define VERSION(major,minor,patch) \
  (((major) << 16) | \
   ((minor) << 8) | \
   (patch))

#if defined(FLUIDSYNTH_VERSION_MAJOR) && defined(FLUIDSYNTH_VERSION_MINOR) && defined(FLUIDSYNTH_VERSION_MICRO)
#define FLUID_VERSION_AT_LEAST(major,minor,patch) \
  (VERSION(FLUIDSYNTH_VERSION_MAJOR, FLUIDSYNTH_VERSION_MINOR, FLUIDSYNTH_VERSION_MICRO) >= VERSION(major,minor,patch))
#else
#define FLUID_VERSION_AT_LEAST(major,minor,patch) 0
#endif

/* ── RAII wrappers for FluidSynth C objects ── */
using FluidSettingsPtr    = std::unique_ptr<fluid_settings_t, decltype(&delete_fluid_settings)>;
using FluidSynthPtr       = std::unique_ptr<fluid_synth_t, decltype(&delete_fluid_synth)>;
using FluidAudioDriverPtr = std::unique_ptr<fluid_audio_driver_t, decltype(&delete_fluid_audio_driver)>;
using FluidSequencerPtr   = std::unique_ptr<fluid_sequencer_t, decltype(&delete_fluid_sequencer)>;
using FluidModPtr         = std::unique_ptr<fluid_mod_t, decltype(&delete_fluid_mod)>;
using FluidEventPtr       = std::unique_ptr<fluid_event_t, decltype(&delete_fluid_event)>;

using namespace amuse;

/* ──────────────────── Terminal helpers ──────────────────── */

#ifndef _WIN32
static struct termios g_origTermios;
static bool g_termRawMode = false;

static void disableRawMode() {
  if (g_termRawMode) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_origTermios);
    g_termRawMode = false;
  }
}

static void enableRawMode() {
  if (g_termRawMode)
    return;
  tcgetattr(STDIN_FILENO, &g_origTermios);
  atexit(disableRawMode);
  struct termios raw = g_origTermios;
  raw.c_lflag &= ~static_cast<tcflag_t>(ECHO | ICANON);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
  g_termRawMode = true;
}

static int readKey() {
  char c = 0;
  if (read(STDIN_FILENO, &c, 1) == 1)
    return static_cast<unsigned char>(c);
  return -1;
}
#else
static void enableRawMode() { /* conio.h handles this on Windows */ }
static void disableRawMode() {}

static int readKey() {
  if (_kbhit())
    return _getch();
  return -1;
}
#endif

/* ──────────────────── Global running flag ──────────────────── */

static std::atomic<bool> g_running{true};

static void signalHandler(int) { g_running.store(false); }

/** Maximum number of SoundMacro commands processed in a single burst
 *  (without any delay). Prevents infinite loops from freezing the app. */
static constexpr int kMaxMacroCmdsPerBurst = 4096;

/* ── Audio codec frame constants ── */
static constexpr unsigned kDSPFrameSamples = 14;   /**< DSP ADPCM: 14 samples per frame */
static constexpr unsigned kDSPFrameBytes   = 8;    /**< DSP ADPCM: 8 bytes per frame */
static constexpr unsigned kN64FrameSamples = 64;   /**< N64 VADPCM: 64 samples per frame */
static constexpr unsigned kN64FrameBytes   = 40;   /**< N64 VADPCM: 40 bytes per frame */

/* ── MIDI pitch bend constants (14-bit) ── */
static constexpr int kPitchBendCenter = 0x2000;    /**< MIDI pitch bend center (8192) */
static constexpr int kPitchBendMax    = 0x3FFF;    /**< MIDI 14-bit pitch bend maximum (16383) */

/* ── SNG modulation → MIDI CC conversion ── */
static constexpr int kSngModToCCDivisor = 128;     /**< SNG internal mod range → 7-bit MIDI CC */

/* ────────────── Seconds ↔ timecents conversion ────────────── */

/** Convert seconds to SF2 timecents (duplicated from FluidSynth's fluid_conv.c).
 *  timecents = 1200 / ln(2) * ln(sec). */
static double secondsToTimecents(double sec) {
  if (sec <= 0.0)
    return -32768.0;
  double res = (1200.0 / M_LN2) * std::log(sec);
  if (res < -32768.0)
    res = -32768.0;
  return res;
}

/** NRPN scale factors for SF2 generators, from SFSpec21 §8.1.3.
 *  Used when sending generator values via NRPN CC messages. */
static constexpr int kGenNrpnScale_VolEnvAttack  = 2;
static constexpr int kGenNrpnScale_VolEnvDecay   = 2;
static constexpr int kGenNrpnScale_VolEnvSustain = 1;
static constexpr int kGenNrpnScale_VolEnvRelease = 2;

static constexpr float kInstantReleaseTimecents = -12000.0f; /**< Near-instant release (~1 ms) */
static constexpr uint16_t kInfiniteLoopSentinel = 65535;     /**< CmdLoop::times value for endless loop */

/* ═══════════════════ SNG binary format structures ═══════════════════
 *
 * Replicated locally from SongState (whose members are private) so that
 * we can parse the raw SNG binary and schedule FluidSynth sequencer events
 * directly, without going through amuse::Sequencer.
 * ═════════════════════════════════════════════════════════════════════ */

/** SNG file header (immediately at offset 0). */
struct SngHeader {
  uint32_t trackIdxOff;
  uint32_t regionIdxOff;
  uint32_t chanMapOff;
  uint32_t tempoTableOff;
  uint32_t initialTempo; /* top bit = per-channel looping */
  uint32_t loopStartTicks[16];
  uint32_t chanMapOff2;

  void swapFromBig() {
    trackIdxOff  = SBig(trackIdxOff);
    regionIdxOff = SBig(regionIdxOff);
    chanMapOff   = SBig(chanMapOff);
    tempoTableOff = SBig(tempoTableOff);
    initialTempo  = SBig(initialTempo);
    if ((initialTempo & 0x80000000) != 0u) {
      for (auto& t : loopStartTicks)
        t = SBig(t);
      chanMapOff2 = SBig(chanMapOff2);
    } else {
      loopStartTicks[0] = SBig(loopStartTicks[0]);
    }
  }
};

/** Track region within the SNG track-region list. */
struct SngTrackRegion {
  uint32_t startTick;
  uint8_t  progNum;
  uint8_t  unk1;
  uint16_t unk2;
  int16_t  regionIndex;
  int16_t  loopToRegion;

  bool indexValid(bool big) const {
    return (big ? SBig(regionIndex) : regionIndex) >= 0;
  }
};

/** Tempo change entry in the SNG tempo table. */
struct SngTempoChange {
  uint32_t tick;
  uint32_t tempo;

  void swapBig() {
    tick  = SBig(tick);
    tempo = SBig(tempo);
  }
};

/** Per-region track header (12 bytes, precedes command data). */
struct SngTrackHeader {
  uint32_t type;
  uint32_t pitchOff;
  uint32_t modOff;

  void swapBig() {
    type     = SBig(type);
    pitchOff = SBig(pitchOff);
    modOff   = SBig(modOff);
  }
};

/* ── SNG variable-length decode helpers (same logic as SongState.cpp) ── */

static uint16_t sngDecodeUnsignedValue(const unsigned char*& data) {
  uint16_t ret = 0;
  if ((data[0] & 0x80) != 0) {
    ret = data[1] | ((data[0] & 0x7f) << 8);
    data += 2;
  } else {
    ret = data[0];
    data += 1;
  }
  return ret;
}

static int16_t sngDecodeSignedValue(const unsigned char*& data) {
  int16_t ret = 0;
  if ((data[0] & 0x80) != 0) {
    ret = static_cast<int16_t>(data[1] | ((data[0] & 0x7f) << 8));
    ret |= ((ret << 1) & 0x8000);
    data += 2;
  } else {
    ret = static_cast<int16_t>(data[0] | ((data[0] << 1) & 0x80));
    data += 1;
  }
  return ret;
}

static std::pair<uint32_t, int32_t> sngDecodeDelta(const unsigned char*& data) {
  std::pair<uint32_t, int32_t> ret = {};
  do {
    if (data[0] == 0x80 && data[1] == 0x00)
      break;
    ret.first += sngDecodeUnsignedValue(data);
    ret.second = sngDecodeSignedValue(data);
  } while (ret.second == 0);
  return ret;
}

/** Decode a delta-time value in SNG revision (v1) format. */
static uint32_t sngDecodeTime(const unsigned char*& data) {
  uint32_t ret = 0;
  while (true) {
    uint16_t thisPart = SBig(*reinterpret_cast<const uint16_t*>(data));
    uint16_t nextPart = *reinterpret_cast<const uint16_t*>(data + 2);
    if (nextPart == 0) {
      ret += thisPart;
      data += 4;
      continue;
    }
    ret += thisPart;
    data += 2;
    break;
  }
  return ret;
}

/* ── Collected SNG event for scheduling ── */

struct SngEvent {
  enum Type { NoteOn, NoteOff, CC, Program, PitchBend, Tempo };
  Type     type;
  uint32_t absTick;
  uint8_t  channel;
  uint8_t  data1;   /* note / ctrl / program */
  uint8_t  data2;   /* velocity / value */
  int      pitchBend14; /* 14-bit pitch bend (only for PitchBend type) */
  double   tempoScale;  /* ticks/sec (only for Tempo type) */
};

/** Information about the SNG song's loop structure.
 *  If the song has a loop (regionIndex == -2), `hasLoop` is true,
 *  `loopStartTick` is the tick the loop body begins at, and
 *  `loopEndTick` is the tick where the loop marker is hit (the point
 *  at which playback would jump back to `loopStartTick`). */
struct SngLoopInfo {
  bool     hasLoop      = false;
  uint32_t loopStartTick = 0;
  uint32_t loopEndTick   = 0;
};

/** SNG uses 384 ticks per quarter-note. */
static constexpr double kSngTicksPerQuarter = 384.0;

/** Convert BPM to FluidSynth sequencer time-scale (ticks per second). */
static double bpmToScale(uint32_t bpm) {
  if (bpm == 0)
    bpm = 120;
  return static_cast<double>(bpm) * kSngTicksPerQuarter / 60.0;
}

/* ── Parse the raw SNG binary and collect events ── */

static bool parseSngEvents(const unsigned char* sngData, bool bigEndian,
                           int sngVersion,
                           std::vector<SngEvent>& outEvents,
                           double& outInitialScale,
                           SngLoopInfo& outLoop) {
  SngHeader hdr = *reinterpret_cast<const SngHeader*>(sngData);
  if (bigEndian)
    hdr.swapFromBig();

  uint32_t initBpm = hdr.initialTempo & 0x7fffffffu;
  outInitialScale = bpmToScale(initBpm);
  outLoop = {};

  /* Collect tempo change events */
  if (hdr.tempoTableOff != 0) {
    auto* tp = reinterpret_cast<const SngTempoChange*>(sngData + hdr.tempoTableOff);
    while (true) {
      SngTempoChange tc = *tp;
      if (bigEndian)
        tc.swapBig();
      if (tc.tick == 0xffffffffu)
        break;
      uint32_t bpm = tc.tempo & 0x7fffffffu;
      SngEvent ev{};
      ev.type = SngEvent::Tempo;
      ev.absTick = tc.tick;
      ev.tempoScale = bpmToScale(bpm);
      outEvents.push_back(ev);
      ++tp;
    }
  }

  const auto* trackIdx = reinterpret_cast<const uint32_t*>(sngData + hdr.trackIdxOff);
  const auto* regionIdx = reinterpret_cast<const uint32_t*>(sngData + hdr.regionIdxOff);
  const auto* chanMap = reinterpret_cast<const uint8_t*>(sngData + hdr.chanMapOff);

  for (int i = 0; i < 64; ++i) {
    uint32_t trkOff = (bigEndian ? SBig(trackIdx[i]) : trackIdx[i]);
    if (trkOff == 0)
      continue;

    uint8_t midiChan = chanMap[i];
    const auto* firstRegion =
        reinterpret_cast<const SngTrackRegion*>(sngData + trkOff);
    const auto* nextRegion = firstRegion;

    /* Iterate all regions of this track */
    while (nextRegion->indexValid(bigEndian)) {
      const SngTrackRegion* region = nextRegion;
      uint32_t regStart = bigEndian ? SBig(region->startTick) : region->startTick;
      uint32_t regIdx = static_cast<uint32_t>(
          bigEndian ? SBig(region->regionIndex) : region->regionIndex);
      nextRegion = &region[1];

      /* Program change at region start */
      if (region->progNum != 0xff) {
        outEvents.push_back({SngEvent::Program, regStart, midiChan,
                             region->progNum, 0, 0});
      }

      /* Locate region data via regionIdx table */
      uint32_t dataOff = bigEndian ? SBig(regionIdx[regIdx]) : regionIdx[regIdx];
      const unsigned char* data = sngData + dataOff;

      SngTrackHeader thdr = *reinterpret_cast<const SngTrackHeader*>(data);
      if (bigEndian)
        thdr.swapBig();
      data += 12;

      /* --- Continuous pitch wheel data --- */
      if (thdr.pitchOff != 0) {
        const unsigned char* pdata = sngData + thdr.pitchOff;
        int32_t pitchVal = 0;
        uint32_t pitchTick = 0;
        while (pdata[0] != 0x80 || pdata[1] != 0x00) {
          auto delta = sngDecodeDelta(pdata);
          pitchTick += delta.first;
          pitchVal  += delta.second;
          int bend14 = std::clamp(pitchVal + kPitchBendCenter, 0, kPitchBendMax);
          outEvents.push_back({SngEvent::PitchBend, regStart + pitchTick,
                               midiChan, 0, 0, bend14});
        }
      }

      /* --- Continuous modulation data --- */
      if (thdr.modOff != 0) {
        const unsigned char* mdata = sngData + thdr.modOff;
        int32_t modVal = 0;
        uint32_t modTick = 0;
        while (mdata[0] != 0x80 || mdata[1] != 0x00) {
          auto delta = sngDecodeDelta(mdata);
          modTick += delta.first;
          modVal  += delta.second;
          uint8_t ccVal = static_cast<uint8_t>(
              std::clamp(modVal / kSngModToCCDivisor, 0, 127));
          outEvents.push_back({SngEvent::CC, regStart + modTick,
                               midiChan, 1 /*mod wheel*/, ccVal, 0});
        }
      }

      /* --- Note / CC / Program commands --- */
      if (sngVersion == 1) {
        /* Revision format */
        int32_t waitCountdown = static_cast<int32_t>(sngDecodeTime(data));
        while (true) {
          if (*reinterpret_cast<const uint16_t*>(data) == 0xffff)
            break;

          uint32_t evTick = regStart + static_cast<uint32_t>(waitCountdown);

          if ((data[0] & 0x80) != 0 && (data[1] & 0x80) != 0) {
            /* Control change */
            uint8_t val  = data[0] & 0x7f;
            uint8_t ctrl = data[1] & 0x7f;
            outEvents.push_back({SngEvent::CC, evTick, midiChan, ctrl, val, 0});
            data += 2;
          } else if ((data[0] & 0x80) != 0) {
            /* Program change */
            uint8_t prog = data[0] & 0x7f;
            outEvents.push_back({SngEvent::Program, evTick, midiChan,
                                 prog, 0, 0});
            data += 2;
          } else {
            /* Note */
            uint8_t note = data[0] & 0x7f;
            uint8_t vel  = data[1] & 0x7f;
            uint16_t length = bigEndian
                ? SBig(*reinterpret_cast<const uint16_t*>(data + 2))
                : *reinterpret_cast<const uint16_t*>(data + 2);
            outEvents.push_back({SngEvent::NoteOn, evTick, midiChan,
                                 note, vel, 0});
            if (length == 0) {
              outEvents.push_back({SngEvent::NoteOff, evTick, midiChan,
                                   note, 0, 0});
            } else {
              outEvents.push_back({SngEvent::NoteOff,
                                   evTick + length, midiChan,
                                   note, 0, 0});
            }
            data += 4;
          }
          waitCountdown += static_cast<int32_t>(sngDecodeTime(data));
        }
      } else {
        /* Legacy (N64) format */
        int32_t absTick = bigEndian
            ? SBig(*reinterpret_cast<const int32_t*>(data))
            : *reinterpret_cast<const int32_t*>(data);
        int32_t waitCountdown = absTick;
        int32_t lastN64Tick = absTick;
        data += 4;

        while (true) {
          if (*reinterpret_cast<const uint16_t*>(&data[2]) == 0xffff)
            break;

          uint32_t evTick = regStart + static_cast<uint32_t>(waitCountdown);

          if ((data[2] & 0x80) != 0x80) {
            /* Note */
            uint16_t length = bigEndian
                ? SBig(*reinterpret_cast<const uint16_t*>(data))
                : *reinterpret_cast<const uint16_t*>(data);
            uint8_t note = data[2] & 0x7f;
            uint8_t vel  = data[3] & 0x7f;
            outEvents.push_back({SngEvent::NoteOn, evTick, midiChan,
                                 note, vel, 0});
            if (length == 0) {
              outEvents.push_back({SngEvent::NoteOff, evTick, midiChan,
                                   note, 0, 0});
            } else {
              outEvents.push_back({SngEvent::NoteOff,
                                   evTick + length, midiChan,
                                   note, 0, 0});
            }
          } else if ((data[2] & 0x80) != 0 && (data[3] & 0x80) != 0) {
            /* Control change */
            uint8_t val  = data[2] & 0x7f;
            uint8_t ctrl = data[3] & 0x7f;
            outEvents.push_back({SngEvent::CC, evTick, midiChan, ctrl, val, 0});
          } else if ((data[2] & 0x80) != 0) {
            /* Program change */
            uint8_t prog = data[2] & 0x7f;
            outEvents.push_back({SngEvent::Program, evTick, midiChan,
                                 prog, 0, 0});
          }
          data += 4;

          int32_t nextAbsTick = bigEndian
              ? SBig(*reinterpret_cast<const int32_t*>(data))
              : *reinterpret_cast<const int32_t*>(data);
          waitCountdown += nextAbsTick - lastN64Tick;
          lastN64Tick = nextAbsTick;
          data += 4;
        }
      }
    }

    /* Detect loop marker: regionIndex == -2 means "loop back to loopToRegion".
     * The loop end tick is the terminating region's startTick, and the loop
     * start tick is the startTick of the target region. */
    if (!outLoop.hasLoop) {
      int16_t termIdx = bigEndian ? SBig(nextRegion->regionIndex) : nextRegion->regionIndex;
      if (termIdx == -2) {
        int16_t loopTo = bigEndian ? SBig(nextRegion->loopToRegion) : nextRegion->loopToRegion;
        uint32_t loopEndTick = bigEndian ? SBig(nextRegion->startTick) : nextRegion->startTick;
        uint32_t loopStartTick = 0;
        if (loopTo >= 0) {
          loopStartTick = bigEndian ? SBig(firstRegion[loopTo].startTick)
                                    : firstRegion[loopTo].startTick;
        }
        /* Also consult the header's loopStartTicks (per-channel or global) */
        if ((hdr.initialTempo & 0x80000000u) != 0u) {
          uint32_t hdrLoop = hdr.loopStartTicks[midiChan];
          if (hdrLoop != 0 && hdrLoop < loopEndTick)
            loopStartTick = std::max(loopStartTick, hdrLoop);
        } else {
          uint32_t hdrLoop = hdr.loopStartTicks[0];
          if (hdrLoop != 0 && hdrLoop < loopEndTick)
            loopStartTick = std::max(loopStartTick, hdrLoop);
        }
        if (loopEndTick > loopStartTick) {
          outLoop.hasLoop = true;
          outLoop.loopStartTick = loopStartTick;
          outLoop.loopEndTick   = loopEndTick;
        }
      }
    }
  }

  /* Sort events by tick (stable, so same-tick ordering is preserved) */
  std::stable_sort(outEvents.begin(), outEvents.end(),
                   [](const SngEvent& a, const SngEvent& b) {
                     return a.absTick < b.absTick;
                   });
  return true;
}

/* ────────────── Runtime state for a single SoundMacro VM ────────────── */

struct MacroExecContext {
  const SoundMacro* macro = nullptr;
  int pc = 0; /* program counter (command index) */
  uint8_t midiKey = 60;
  uint8_t midiVel = 100;
  uint8_t initKey = 60;  /**< Original trigger key (never modified by SetNote etc.) */
  int channel = 0;
  double ticksPerSec = 1000.0; /* default: 1 tick = 1 ms */
  int loopCountdown = -1;
  int loopStep = 0;
  bool ended = false;
  bool keyoffReceived = false;    /**< True once a key-off event was delivered to this voice */
  bool sampleEndReceived = false; /**< True once the sample-end event was delivered */
  bool waitingKeyoff = false;
  bool waitingSampleEnd = false;
  bool inIndefiniteWait = false;  /**< Macro paused in an indefinite wait (ticks=0, keyOff/sampleEnd) */
  int lastPlayMacroId = -1;      /**< activeMacros key of the last child spawned by PlayMacro */
  /* variable bank (32 × 32-bit) */
  std::array<int32_t, 32> vars = {};

  /* ── FluidSynth voice tracking ──
   * Each SoundMacro owns a single voice.  CmdStartSample routes voice
   * creation through fluid_synth_start() (via dummyPreset) so that the
   * voice receives a unique user-controlled ID (fluid_synth_alloc_voice()
   * alone leaves ID=0, which is treated as "not started").
   * Subsequent commands look up the voice by ID via findVoiceById(). */
  unsigned int voiceId = 0;
  uint8_t allocKey = 60;      /**< MIDI key at voice allocation time */
  uint8_t triggerNote = 60;   /**< Original SNG note (before keymap/layer transpose), used for NoteOff matching */

  /* Pending voice state – set by commands that run BEFORE CmdStartSample
   * creates the voice.  Applied between alloc and start. */
  float pendingPanGen = 0.0f;   /**< GEN_PAN (-500..500, 0=center) */
  bool  hasPendingPan = false;
  float pendingAttnGen = 0.0f;  /**< GEN_ATTENUATION (centibels, 0..1440) */
  bool  hasPendingAttn = false;

  /* ADSR controller mapping (CmdSetAdsrCtrl).
   * NOTE: In MusyX, ADSR is per-voice (bound to the SoundMacro).
   * With voice-level generators this is now per-voice in FluidSynth too. */
  bool useAdsrControllers = false;
  bool adsrBootstrapped = false;
  uint8_t midiAttack  = 0;
  uint8_t midiDecay   = 0;
  uint8_t midiSustain = 0;
  uint8_t midiRelease = 0;

  /* Controller value storage (0-127 for standard MIDI CCs) */
  std::array<int8_t, 128> ctrlVals = {};

  /* Accumulated fine-tune in cents, from SoundMacro detune commands.
   * MusyX samples do not carry per-sample sub-semitone fine-tuning;
   * fine-tuning is applied entirely at the command level via SetNote,
   * AddNote, LastNote, RndNote (all have a ±99-cent 'detune' field)
   * and SetPitch (absolute Hz + 1/65536 Hz fine). */
  int curDetune = 0;
};

/* ═══════════════ Custom SoundFont loader for MusyX samples ═══════════════
 *
 * We decode MusyX compressed samples (DSP ADPCM, N64 VADPCM, PCM) to
 * 16-bit signed PCM, wrap them in fluid_sample_t objects, and create
 * a virtual SoundFont so FluidSynth can synthesise the original sounds.
 *
 * The mapping is:
 *   bank 0, program N  →  MusyX SoundMacro lookup for program N
 *   noteon callback     →  finds CmdStartSample in the macro, plays
 *                          the corresponding decoded fluid_sample_t
 *
 * Limitations:
 *   - Only the *first* CmdStartSample in a SoundMacro is used here;
 *     the macro VM still runs to handle control flow, envelopes, etc.
 *   - One preset per program number (bank 0 only).
 * ═════════════════════════════════════════════════════════════════════════ */

/** Decoded PCM sample data kept alive for the lifetime of the soundfont. */
struct DecodedSample {
  SampleId id;
  std::string name;
  std::vector<int16_t> pcmData;   /**< 16-bit signed mono PCM */
  uint32_t sampleRate = 32000;
  uint8_t rootKey = 60;           /**< MIDI root key */
  bool looped = false;
  uint32_t loopStart = 0;
  uint32_t loopEnd = 0;
  fluid_sample_t* flSample = nullptr; /**< Owned by FluidSynth after loading */
};

/** Per-preset data: which fluid_sample to play for a given MIDI note. */
struct MusyXPresetData {
  int bank = 0;
  int program = 0;
  /** Default sample for this preset (from the first CmdStartSample in the
   *  macro associated with this program number).  May be null. */
  fluid_sample_t* defaultSample = nullptr;
  uint8_t rootKey = 60;
  bool looped = false;
  uint32_t loopStart = 0;
  uint32_t loopEnd = 0;
};

/** The entire virtual soundfont: owns decoded samples + presets. */
struct MusyXSoundFontData {
  std::string name;
  std::vector<DecodedSample> samples;
  /** Map SampleId → index in `samples` */
  std::unordered_map<uint16_t, size_t> sampleIndex;
  /** Preset list (one per program number that has a SoundMacro). */
  std::vector<MusyXPresetData> presets;
  /** fluid_preset_t objects kept alive */
  std::vector<fluid_preset_t*> flPresets;
  int iterIdx = 0; /**< for iteration */
};

/* ── Decode helpers ── */

/** Decode a single MusyX sample entry to 16-bit PCM. */
static std::vector<int16_t> decodeSampleToPCM(
    const AudioGroupSampleDirectory::EntryData& ent,
    const unsigned char* sampBase)
{
  const unsigned char* samp = sampBase + ent.m_sampleOff;
  SampleFormat fmt = ent.getSampleFormat();
  uint32_t numSamples = ent.getNumSamples();
  std::vector<int16_t> out(numSamples);

  if (fmt == SampleFormat::DSP || fmt == SampleFormat::DSP_DRUM) {
    uint32_t remSamples = numSamples;
    const unsigned char* cur = samp;
    int16_t prev1 = ent.m_ADPCMParms.dsp.m_hist1;
    int16_t prev2 = ent.m_ADPCMParms.dsp.m_hist2;
    size_t outOff = 0;
    while (remSamples > 0) {
      int16_t decomp[kDSPFrameSamples] = {};
      unsigned thisSamples = std::min(remSamples, kDSPFrameSamples);
      DSPDecompressFrame(decomp, cur, ent.m_ADPCMParms.dsp.m_coefs,
                         &prev1, &prev2, thisSamples);
      std::memcpy(&out[outOff], decomp, thisSamples * sizeof(int16_t));
      outOff += thisSamples;
      remSamples -= thisSamples;
      cur += kDSPFrameBytes;
    }
  } else if (fmt == SampleFormat::N64) {
    uint32_t remSamples = numSamples;
    /* N64: codebook is at the beginning of the sample data */
    const unsigned char* cur = samp + sizeof(AudioGroupSampleDirectory::ADPCMParms::VADPCMParms);
    size_t outOff = 0;
    while (remSamples > 0) {
      int16_t decomp[kN64FrameSamples] = {};
      unsigned thisSamples = std::min(remSamples, kN64FrameSamples);
      N64MusyXDecompressFrame(decomp, cur, ent.m_ADPCMParms.vadpcm.m_coefs,
                              thisSamples);
      std::memcpy(&out[outOff], decomp, thisSamples * sizeof(int16_t));
      outOff += thisSamples;
      remSamples -= thisSamples;
      cur += kN64FrameBytes;
    }
  } else if (fmt == SampleFormat::PCM) {
    /* Big-endian 16-bit PCM */
    const uint8_t* cur = samp;
    for (uint32_t i = 0; i < numSamples; ++i) {
      int16_t s = static_cast<int16_t>((cur[0] << 8) | cur[1]);
      out[i] = s;
      cur += 2;
    }
  } else {
    /* PCM_PC: little-endian 16-bit PCM (native on x86) */
    std::memcpy(out.data(), samp, numSamples * sizeof(int16_t));
  }
  return out;
}

/* ── FluidSynth SoundFont loader callbacks ── */

/* -- sfont callbacks -- */
static const char* musyx_sfont_get_name(fluid_sfont_t* sfont) {
  auto* d = static_cast<MusyXSoundFontData*>(fluid_sfont_get_data(sfont));
  return d ? d->name.c_str() : "MusyX";
}

static fluid_preset_t* musyx_sfont_get_preset(fluid_sfont_t* sfont,
                                               int bank, int prenum) {
  auto* d = static_cast<MusyXSoundFontData*>(fluid_sfont_get_data(sfont));
  if (!d)
    return nullptr;
  for (size_t i = 0; i < d->presets.size(); ++i) {
    if (d->presets[i].bank == bank && d->presets[i].program == prenum)
      return d->flPresets[i];
  }
  return nullptr;
}

static void musyx_sfont_iter_start(fluid_sfont_t* sfont) {
  auto* d = static_cast<MusyXSoundFontData*>(fluid_sfont_get_data(sfont));
  if (d)
    d->iterIdx = 0;
}

static fluid_preset_t* musyx_sfont_iter_next(fluid_sfont_t* sfont) {
  auto* d = static_cast<MusyXSoundFontData*>(fluid_sfont_get_data(sfont));
  if (!d || d->iterIdx >= static_cast<int>(d->flPresets.size()))
    return nullptr;
  return d->flPresets[d->iterIdx++];
}

static int musyx_sfont_free(fluid_sfont_t* sfont) {
  auto* d = static_cast<MusyXSoundFontData*>(fluid_sfont_get_data(sfont));
  /* Presets are owned by their parent sfont in FluidSynth - just clean up
   * our data.  The fluid_sample_t objects are destroyed with the synth. */
  delete d;
  delete_fluid_sfont(sfont);
  return 0;
}

/* -- preset callbacks -- */
static const char* musyx_preset_get_name(fluid_preset_t* preset) {
  auto* d = static_cast<MusyXPresetData*>(fluid_preset_get_data(preset));
  (void)d;
  return "MusyX Preset";
}

static int musyx_preset_get_bank(fluid_preset_t* preset) {
  auto* d = static_cast<MusyXPresetData*>(fluid_preset_get_data(preset));
  return d ? d->bank : 0;
}

static int musyx_preset_get_num(fluid_preset_t* preset) {
  auto* d = static_cast<MusyXPresetData*>(fluid_preset_get_data(preset));
  return d ? d->program : 0;
}

static int musyx_preset_noteon(fluid_preset_t* preset, fluid_synth_t* synth,
                                int chan, int key, int vel) {
  auto* d = static_cast<MusyXPresetData*>(fluid_preset_get_data(preset));
  if (!d || !d->defaultSample)
    return FLUID_FAILED;

  fluid_voice_t* voice = fluid_synth_alloc_voice(synth, d->defaultSample,
                                                  chan, key, vel);
  if (!voice)
    return FLUID_FAILED;

  /* Set sample mode: 0=no loop, 1=loop continuously, 3=loop until noteoff */
  if (d->looped)
    fluid_voice_gen_set(voice, GEN_SAMPLEMODE, 1);

  fluid_synth_start_voice(synth, voice);
  return FLUID_OK;
}

static void musyx_preset_free(fluid_preset_t* preset) {
  /* MusyXPresetData is owned by MusyXSoundFontData; don't free it here. */
  delete_fluid_preset(preset);
}

/* -- sfloader callbacks -- */
static fluid_sfont_t* musyx_sfloader_load(fluid_sfloader_t* loader,
                                           const char* filename) {
  auto* sfData = static_cast<MusyXSoundFontData*>(fluid_sfloader_get_data(loader));
  if (!sfData)
    return nullptr;

  /* Create the FluidSynth SoundFont object */
  fluid_sfont_t* sfont = new_fluid_sfont(
      musyx_sfont_get_name,
      musyx_sfont_get_preset,
      musyx_sfont_iter_start,
      musyx_sfont_iter_next,
      musyx_sfont_free);
  fluid_sfont_set_data(sfont, sfData);

  /* Create presets */
  sfData->flPresets.resize(sfData->presets.size(), nullptr);
  for (size_t i = 0; i < sfData->presets.size(); ++i) {
    fluid_preset_t* preset = new_fluid_preset(
        sfont,
        musyx_preset_get_name,
        musyx_preset_get_bank,
        musyx_preset_get_num,
        musyx_preset_noteon,
        musyx_preset_free);
    fluid_preset_set_data(preset, &sfData->presets[i]);
    sfData->flPresets[i] = preset;
  }

  fmt::print("fluidsyX: MusyX SoundFont loaded with {} samples, {} presets\n",
         sfData->samples.size(), sfData->presets.size());
  return sfont;
}

static void musyx_sfloader_free(fluid_sfloader_t* loader) {
  /* The MusyXSoundFontData is transferred to the sfont on load, so the
   * loader does not own it any more. */
  delete_fluid_sfloader(loader);
}

/* ──────────────────── FluidSynth state ──────────────────── */

struct FluidsyXApp {
  /* FluidSynth objects */
  FluidSettingsPtr    settings{nullptr, &delete_fluid_settings};
  FluidSynthPtr       synth{nullptr, &delete_fluid_synth};
  FluidAudioDriverPtr adriver{nullptr, &delete_fluid_audio_driver};
  FluidSequencerPtr   sequencer{nullptr, &delete_fluid_sequencer};
  fluid_seq_id_t synthSeqId = -1;
  fluid_seq_id_t callbackSeqId = -1;

  FluidModPtr         modBlueprintADR{nullptr, &delete_fluid_mod};
  FluidModPtr         modBlueprintSustain{nullptr, &delete_fluid_mod};

  /* Amuse parsed data */
  std::vector<std::pair<std::string, IntrusiveAudioGroupData>> data;
  std::list<AudioGroupProject> projs;
  std::vector<AudioGroupPool> pools;
  std::vector<AudioGroupSampleDirectory> sdirs; /**< one per data entry */
  std::map<GroupId, std::pair<std::pair<std::string, IntrusiveAudioGroupData>*,
                              ObjToken<SongGroupIndex>>>
      allSongGroups;
  std::map<GroupId, std::pair<std::pair<std::string, IntrusiveAudioGroupData>*,
                              ObjToken<SFXGroupIndex>>>
      allSFXGroups;
  size_t totalGroups = 0;

  /* Currently-selected group */
  int groupId = -1;
  bool sfxGroup = false;
  int setupId = -1;

  /* The pool that belongs to the active group */
  const AudioGroupPool* activePool = nullptr;

  /** Pointer to the MusyX virtual SoundFont data.
   *  Kept here so processMacroCmd can look up specific samples by SampleId
   *  when CmdStartSample fires.  Ownership is held by FluidSynth's sfont. */
  MusyXSoundFontData* musyxSfData = nullptr;

  /* Interactive playback state */
  int chanId = 0;
  int8_t octave = 4;
  int8_t velocity = 64;
  float volume = 0.8f;

  /* Active macro execution contexts (for timer-driven processing).
   * Uses a map with stable integer IDs so that vector reallocation
   * cannot invalidate references stored in timer events. */
  std::map<int, MacroExecContext> activeMacros;
  int nextMacroId = 0;

  /* Monotonic counter for unique voice IDs (passed to fluid_synth_start).
   * 0 is reserved as "not started". */
  unsigned int nextVoiceId = 1;

  /** Context passed to the dummy preset's noteon callback so CmdStartSample
   *  can alloc a voice with the specific MusyX sample and initial state. */
  struct PendingVoiceStart {
    fluid_sample_t* flSamp  = nullptr;
    bool            looped  = false;
    MacroExecContext *macroContext = nullptr;
  };
  PendingVoiceStart pendingVoiceStart;

  /** Dummy preset used to route voice creation through fluid_synth_start()
   *  so that voices receive a proper unique ID (set via nextVoiceId). */
  fluid_preset_t* dummyPreset = nullptr;

  /* Per-channel CC state tracking (propagated to new MacroExecContexts).
   * Updated when MIDI setup is applied and when CC-manipulating commands
   * modify values. */
  std::array<std::array<int8_t, 128>, 16> channelCtrlVals = {};

  /* RNG */
  std::mt19937 rng{std::random_device{}()};

  /* Selected song data (set from main() when a song is chosen) */
  const ContainerRegistry::SongData* selectedSong = nullptr;

  /** Maximum playback duration in SNG ticks.  When > 0 and the song
   *  has a loop, events from the loop body are re-scheduled until this
   *  duration is reached.  Set from the --duration CLI flag. */
  uint32_t maxDurationTicks = 0;

  /** When true, log every SoundMacro command as it executes (with all
   *  introspection fields).  Unimplemented commands are always logged
   *  regardless of this flag.  Set from the --verbose CLI flag. */
  bool verbose = false;

  /* Active SongGroupIndex for note→SoundMacro resolution during playback */
  const SongGroupIndex* activeSongGroup = nullptr;

  /* Per-channel program number (set from MIDI setup and SNG program changes) */
  std::array<uint8_t, 16> channelPrograms = {};

  /* Per-channel ADSR controller mapping.  When a SoundMacro executes
   * SetAdsrCtrl, it records which CC numbers control Attack/Decay/Sustain/
   * Release for that channel.  Subsequent CC events matching those numbers
   * are intercepted and converted to NRPN generator changes so FluidSynth's
   * volume envelope is adjusted.
   * NOTE: MusyX ADSR is per-voice; FluidSynth NRPN is per-channel. */
  struct ChannelAdsrMapping {
    bool active = false;
    uint8_t attackCC  = 0;
    uint8_t decayCC   = 0;
    uint8_t sustainCC = 0;
    uint8_t releaseCC = 0;
  };
  std::array<ChannelAdsrMapping, 16> channelAdsrMap = {};

  /* Pending SNG events dispatched via timer callbacks.
   * Timer data encodes a negative index into this vector: data = -(1 + idx). */
  struct PendingSngNoteEvent {
    enum Type { NoteOn, NoteOff, ProgramChange, CC };
    Type    type;
    uint8_t channel;
    uint8_t note;     /* also: CC number for Type::CC */
    uint8_t velocity; /* also: CC value for Type::CC */
  };
  std::vector<PendingSngNoteEvent> pendingSngEvents;

  /* ── lifecycle helpers ── */

  [[nodiscard]] bool initFluidSynth();
  void shutdownFluidSynth();

  [[nodiscard]] bool loadMusyXData(const char* path);
  [[nodiscard]] int selectGroup();
  const AudioGroupPool* findPoolForGroup();

  void songLoop(const SongGroupIndex& index);
  void sfxLoop(const SFXGroupIndex& index);

  /** Parse SNG song data and schedule all events on the FluidSynth sequencer.
   *  Returns the total duration in SNG ticks, or 0 on failure. */
  [[nodiscard]] double scheduleSongEvents(const uint8_t* sngData, size_t sngSize);

  /** Build a custom MusyX SoundFont from the parsed sample directory
   *  and pool, register it with FluidSynth. */
  [[nodiscard]] bool buildMusyXSoundFont();

  /* ── SoundMacro → FluidSynth translation ── */

  /** Resolve a note-on through the SongGroupIndex page→keymap/layer→SoundMacro
   *  chain and enqueue the resulting SoundMacro(s) for execution. */
  void resolveAndEnqueueNote(uint8_t channel, uint8_t note, uint8_t vel,
                             unsigned int tick);

  /** Enqueue all commands of a SoundMacro, starting at \p step.
   *  Returns the activeMacros key if the macro was stored (for timer-driven
   *  continuation), or -1 if it ran to completion synchronously. */
  int enqueueSoundMacro(const SoundMacro* sm, int step, int channel,
                         uint8_t key, uint8_t vel, unsigned int startTick,
                         uint8_t triggerNote = 0xff);

  /** Process one command of a MacroExecContext, scheduling FluidSynth events.
   *  Returns the additional tick delay introduced by the command. */
  unsigned int processMacroCmd(MacroExecContext& ctx, unsigned int curTick);

  /** Timer callback trampoline */
  static void timerCallback(unsigned int time, fluid_event_t* event,
                            fluid_sequencer_t* seq, void* data);

  /** Apply ADSR generator values directly to a FluidSynth voice.
   *  If \p started is true, also calls fluid_voice_update_param() for
   *  each generator so the change takes effect on an already-playing voice. */
  void applyAdsrToVoice(fluid_voice_t* v, MacroExecContext& ctx, bool started);

  /** Apply pitch offset (GEN_COARSETUNE + GEN_FINETUNE) to the macro's voice.
   *  Computes the offset from allocKey and curDetune. */
  void applyVoicePitch(MacroExecContext& ctx);

  /** Kick off the next pending timer step for all active macros */
  void scheduleNextTimerStep();
};

/* ═══════════════════ Dummy preset callbacks ═══════════════════ */

static int dummy_preset_noteon(fluid_preset_t* preset, fluid_synth_t* synth,
                                int chan, int key, int vel) {
  auto* app = static_cast<FluidsyXApp*>(fluid_preset_get_data(preset));
  if (!app || !app->pendingVoiceStart.flSamp)
    return FLUID_FAILED;

  auto& p = app->pendingVoiceStart;
  fluid_voice_t* voice = fluid_synth_alloc_voice(synth, p.flSamp, chan, key, vel);
  if (!voice)
    return FLUID_FAILED;

  auto& ctx = *p.macroContext;
  if (p.looped)
    fluid_voice_gen_set(voice, GEN_SAMPLEMODE, 1);
  if (ctx.hasPendingPan)
    fluid_voice_gen_set(voice, GEN_PAN, ctx.pendingPanGen);
  if (ctx.hasPendingAttn)
    fluid_voice_gen_set(voice, GEN_ATTENUATION, ctx.pendingAttnGen);
  if (ctx.curDetune != 0.0f)
    fluid_voice_gen_set(voice, GEN_FINETUNE, ctx.curDetune);

  app->applyAdsrToVoice(voice, ctx, false);

  fluid_synth_start_voice(synth, voice);
  return FLUID_OK;
}

static void dummy_preset_free(fluid_preset_t* preset) {
  delete_fluid_preset(preset);
}

/* ═══════════════════ FluidSynth init / shutdown ═══════════════════ */

bool FluidsyXApp::initFluidSynth() {
#if FLUID_VERSION_AT_LEAST(2,5,0)
    fluid_mod_t *blueprint = new_fluid_mod();

    fluid_mod_set_source1(blueprint, FLUID_MOD_NONE,
                          FLUID_MOD_CC | FLUID_MOD_CUSTOM | FLUID_MOD_UNIPOLAR | FLUID_MOD_POSITIVE);
    fluid_mod_set_source2(blueprint, FLUID_MOD_NONE, 0);
    fluid_mod_set_amount(blueprint, 1);
    fluid_mod_set_custom_mapping(blueprint, [](const fluid_mod_t* mod, int value, int range, int is_src1, void* data)
    {
        // The volenv* generators all use -12000 timecents as default, which we need to get rid of.
        return secondsToTimecents(MIDItoTIME[std::clamp(value,  0, 103)] / 1000.0) + 12000;
    }, nullptr);

    modBlueprintADR.reset(new_fluid_mod());
    fluid_mod_clone(modBlueprintADR.get(), blueprint);

    fluid_mod_set_custom_mapping(blueprint, [](const fluid_mod_t* mod, int value, int range, int is_src1, void* data)
    {
        double sustainFactor = value * 1.0 / range;
        return (1.0 - sustainFactor) * 1440.0;
    }, nullptr);


    modBlueprintSustain.reset(new_fluid_mod());
    fluid_mod_clone(modBlueprintSustain.get(), blueprint);

    delete_fluid_mod(blueprint);
#endif

  settings.reset(new_fluid_settings());
  if (!settings) {
    fmt::print(stderr, "fluidsyX: failed to create FluidSynth settings\n");
    return false;
  }

  fluid_settings_setnum(settings.get(), "synth.gain", 0.9);
  /* Use FluidSynth's linear portamento mode via the portamento-time
   * setting.  NOTE: synth.portamento-time is a global setting, not
   * per-channel.  If multiple channels set different portamento times
   * concurrently, only the last value will take effect. */
  fluid_settings_setstr(settings.get(), "synth.portamento-time", "linear");

  synth.reset(new_fluid_synth(settings.get()));
  if (!synth) {
    fmt::print(stderr, "fluidsyX: failed to create FluidSynth synthesizer\n");
    return false;
  }
  fluid_synth_set_channel_type(synth.get(), 9, CHANNEL_TYPE_MELODIC);
  fluid_synth_bank_select(synth.get(), 9, 0); // try to force drum channel to bank 0 rather than bank 128


  /* Create sequencer (use system timer so it advances in real-time) */
  sequencer.reset(new_fluid_sequencer2(0));
  if (!sequencer) {
    fmt::print(stderr, "fluidsyX: failed to create FluidSynth sequencer\n");
    return false;
  }

  /* Register the synth as a sequencer destination */
  synthSeqId = fluid_sequencer_register_fluidsynth(sequencer.get(), synth.get());

  /* Register our application callback client for timer events */
  callbackSeqId = fluid_sequencer_register_client(
      sequencer.get(), "fluidsyX", &FluidsyXApp::timerCallback, this);

  adriver.reset(new_fluid_audio_driver(settings.get(), synth.get()));
  if (!adriver) {
    fmt::print(stderr, "fluidsyX: failed to create FluidSynth audio driver\n");
    return false;
  }
  return true;
}

void FluidsyXApp::shutdownFluidSynth() {
    adriver.reset();
    if (callbackSeqId >= 0)
      fluid_sequencer_unregister_client(sequencer.get(), callbackSeqId);
    sequencer.reset();
    dummy_preset_free(dummyPreset);
    dummyPreset = nullptr;
    synth.reset();
    settings.reset();
    modBlueprintADR.reset();
    modBlueprintSustain.reset();
}

/* ═══════════════════ MusyX data loading ═══════════════════ */

bool FluidsyXApp::loadMusyXData(const char* path) {
  ContainerRegistry::Type cType = ContainerRegistry::DetectContainerType(path);
  if (cType == ContainerRegistry::Type::Invalid) {
    fmt::print(stderr, "fluidsyX: invalid or missing data at '{}'\n", path);
    return false;
  }
  fmt::print("fluidsyX: found '{}' Audio Group data\n",
         ContainerRegistry::TypeToName(cType));

  data = ContainerRegistry::LoadContainer(path);
  if (data.empty()) {
    fmt::print(stderr, "fluidsyX: no groups loaded from '{}'\n", path);
    return false;
  }

  for (auto& grp : data) {
    projs.push_back(AudioGroupProject::CreateAudioGroupProject(grp.second));
    AudioGroupProject& proj = projs.back();
    totalGroups += proj.sfxGroups().size() + proj.songGroups().size();

    for (const auto& [gid, entry] : proj.songGroups())
      allSongGroups[gid] = std::make_pair(&grp, entry);

    for (const auto& [gid, entry] : proj.sfxGroups())
      allSFXGroups[gid] = std::make_pair(&grp, entry);

    /* Also parse the pool so we can access SoundMacro commands */
    pools.push_back(AudioGroupPool::CreateAudioGroupPool(grp.second));

    /* Parse the sample directory */
    sdirs.push_back(AudioGroupSampleDirectory::CreateAudioGroupSampleDirectory(grp.second));
  }

  return true;
}

/* ═══════════════════ Group selection (interactive) ═══════════════════ */

int FluidsyXApp::selectGroup() {
  if (totalGroups == 0) {
    fmt::print(stderr, "fluidsyX: empty project\n");
    return 1;
  }

  if (groupId != -1) {
    /* Already set (e.g. from song data) */
    if (allSongGroups.find(groupId) != allSongGroups.end())
      sfxGroup = false;
    else if (allSFXGroups.find(groupId) != allSFXGroups.end())
      sfxGroup = true;
    else {
      fmt::print(stderr, "fluidsyX: unable to find Group {}\n", groupId);
      return 1;
    }
  } else if (totalGroups > 1) {
    fmt::print("Multiple Audio Groups discovered:\n");
    for (const auto& pair : allSFXGroups) {
      fmt::print("    {} {} (SFXGroup)  {} sfx-entries\n", pair.first.id,
             pair.second.first->first,
             pair.second.second->m_sfxEntries.size());
    }
    for (const auto& pair : allSongGroups) {
      fmt::print("    {} {} (SongGroup)  {} normal-pages, {} drum-pages, "
             "{} MIDI-setups\n",
             pair.first.id, pair.second.first->first,
             pair.second.second->m_normPages.size(),
             pair.second.second->m_drumPages.size(),
             pair.second.second->m_midiSetups.size());
    }
    int userSel = 0;
    fmt::print("Enter Group Number: ");
    if (scanf("%d", &userSel) <= 0) {
      fmt::print(stderr, "fluidsyX: unable to parse prompt\n");
      return 1;
    }
    /* consume trailing newline */
    while (getchar() != '\n')
      ;

    if (allSongGroups.find(userSel) != allSongGroups.end()) {
      groupId = userSel;
      sfxGroup = false;
    } else if (allSFXGroups.find(userSel) != allSFXGroups.end()) {
      groupId = userSel;
      sfxGroup = true;
    } else {
      fmt::print(stderr, "fluidsyX: unable to find Group {}\n", userSel);
      return 1;
    }
  } else {
    /* Single group – use it */
    if (!allSongGroups.empty()) {
      groupId = allSongGroups.cbegin()->first.id;
      sfxGroup = false;
    } else {
      groupId = allSFXGroups.cbegin()->first.id;
      sfxGroup = true;
    }
  }
  return 0;
}

/* ═══════════════════ Pool lookup ═══════════════════ */

const AudioGroupPool* FluidsyXApp::findPoolForGroup() {
  /* The pools vector is 1:1 with the data vector; look up which data entry
   * owns the active group. */
  IntrusiveAudioGroupData* selData = nullptr;
  {
    auto songIt = allSongGroups.find(groupId);
    if (songIt != allSongGroups.end())
      selData = &songIt->second.first->second;
    else {
      auto sfxIt = allSFXGroups.find(groupId);
      if (sfxIt != allSFXGroups.end())
        selData = &sfxIt->second.first->second;
    }
  }
  if (!selData)
    return nullptr;

  for (size_t i = 0; i < data.size(); ++i) {
    if (&data[i].second == selData)
      return &pools[i];
  }
  return nullptr;
}

/* ═══════════════════ Build MusyX SoundFont ═══════════════════ */

bool FluidsyXApp::buildMusyXSoundFont() {
  if (!activePool)
    return false;

  /* Find which data entry & sdir belongs to the active group */
  IntrusiveAudioGroupData* selData = nullptr;
  size_t dataIdx = 0;
  {
    auto songIt = allSongGroups.find(groupId);
    if (songIt != allSongGroups.end())
      selData = &songIt->second.first->second;
    else {
      auto sfxIt = allSFXGroups.find(groupId);
      if (sfxIt != allSFXGroups.end())
        selData = &sfxIt->second.first->second;
    }
    if (!selData)
      return false;
    for (size_t i = 0; i < data.size(); ++i) {
      if (&data[i].second == selData) {
        dataIdx = i;
        break;
      }
    }
  }

  const AudioGroupSampleDirectory& sdir = sdirs[dataIdx];
  const unsigned char* sampBase = selData->getSamp();
  if (!sampBase)
    return false;

  auto* sfData = new MusyXSoundFontData;
  sfData->name = "MusyX:" + data[dataIdx].first;

  /* 1. Decode all samples to PCM and create fluid_sample_t objects */
  for (const auto& [sampleId, entry] : sdir.sampleEntries()) {
    if (!entry || !entry->m_data)
      continue;
    const auto& ent = *entry->m_data;
    uint32_t numSamples = ent.getNumSamples();
    if (numSamples == 0)
      continue;

    DecodedSample ds;
    ds.id = sampleId;
    ds.name = "sample_" + std::to_string(sampleId.id);
    ds.pcmData = decodeSampleToPCM(ent, sampBase);
    ds.sampleRate = ent.m_sampleRate ? ent.m_sampleRate : 32000;
    ds.rootKey = ent.getPitch();
    ds.looped = ent.isLooped();
    if (ds.looped) {
      ds.loopStart = ent.getLoopStartSample();
      ds.loopEnd = ent.getLoopEndSample();
    }

    /* Create FluidSynth sample */
    fluid_sample_t* flSamp = new_fluid_sample();
    fluid_sample_set_name(flSamp, ds.name.c_str());
    fluid_sample_set_sound_data(
        flSamp,
        ds.pcmData.data(),
        nullptr,                          /* no 24-bit extension */
        static_cast<unsigned int>(ds.pcmData.size()),
        ds.sampleRate,
        1 /* copy_data */);
    /* Set sample root key.  fine_tune is 0 because MusyX samples do not
     * carry per-sample sub-semitone tuning; fine-tuning is applied at the
     * SoundMacro command level via SetNote/AddNote/LastNote/RndNote detune
     * fields (±99 cents) and SetPitch (absolute Hz).  Those detune values
     * are sent to FluidSynth as pitch bend events at playback time.
     * SF2 allows ±100 cents fine-tune per sample, but MusyX has no
     * equivalent stored metadata. */
    fluid_sample_set_pitch(flSamp, ds.rootKey, 0);
    if (ds.looped) {
      fluid_sample_set_loop(flSamp, ds.loopStart, ds.loopEnd);
    }
    ds.flSample = flSamp;
    sfData->sampleIndex[sampleId.id] = sfData->samples.size();
    sfData->samples.push_back(std::move(ds));
  }

  fmt::print("fluidsyX: decoded {} samples from MusyX data\n",
         sfData->samples.size());

  /* 2. Build presets from the group's page entries or SFX entries.
   *    For each program number, find the SoundMacro, scan for the first
   *    CmdStartSample, and map it to the decoded fluid_sample_t. */

  auto findFirstSampleInMacro = [&](const SoundMacro* sm) -> fluid_sample_t* {
    if (!sm)
      return nullptr;
    for (const auto& cmd : sm->m_cmds) {
      if (cmd->Isa() == SoundMacro::CmdOp::StartSample) {
        auto& startCmd = static_cast<const SoundMacro::CmdStartSample&>(*cmd);
        auto sIt = sfData->sampleIndex.find(startCmd.sample.id.id);
        if (sIt != sfData->sampleIndex.end())
          return sfData->samples[sIt->second].flSample;
      }
    }
    return nullptr;
  };

  auto addPresetForMacro = [&](int program, const SoundMacro* sm) {
    fluid_sample_t* flSamp = findFirstSampleInMacro(sm);
    if (!flSamp)
      return;

    /* Find sample metadata for loop info */
    MusyXPresetData pd;
    pd.bank = 0;
    pd.program = program;
    pd.defaultSample = flSamp;

    /* Look up decoded sample for metadata */
    for (const auto& ds : sfData->samples) {
      if (ds.flSample == flSamp) {
        pd.rootKey = ds.rootKey;
        pd.looped = ds.looped;
        pd.loopStart = ds.loopStart;
        pd.loopEnd = ds.loopEnd;
        break;
      }
    }
    sfData->presets.push_back(pd);
  };

  if (sfxGroup) {
    /* SFX group: map SFX entries as presets (program = SFX index) */
    auto sfxIt = allSFXGroups.find(groupId);
    if (sfxIt != allSFXGroups.end()) {
      int prog = 0;
      for (const auto& [sfxId, entry] : sfxIt->second.second->m_sfxEntries) {
        const SoundMacro* sm = activePool->soundMacro(entry.objId);
        addPresetForMacro(prog, sm);
        ++prog;
      }
    }
  } else {
    /* Song group: map normal pages to presets */
    auto songIt = allSongGroups.find(groupId);
    if (songIt != allSongGroups.end()) {
      for (const auto& [prog, pageEntry] : songIt->second.second->m_normPages) {
        /* PageEntry objId can be a keymap or layer.  Try keymap first. */
        const auto* keymap = activePool->keymap(pageEntry.objId);
        if (keymap) {
          /* Use the macro from key 60 (middle C) as the default */
          const SoundMacro* sm = activePool->soundMacro(keymap[60].macro);
          addPresetForMacro(prog, sm);
        } else {
          /* Try layers */
          const auto* layers = activePool->layer(pageEntry.objId);
          if (layers && !layers->empty()) {
            const SoundMacro* sm = activePool->soundMacro((*layers)[0].macro);
            addPresetForMacro(prog, sm);
          }
        }
      }
      /* Also map drum pages (bank 128 in MIDI, but we use bank 0 prog 128+) */
      for (const auto& [prog, pageEntry] : songIt->second.second->m_drumPages) {
        const auto* keymap = activePool->keymap(pageEntry.objId);
        if (keymap) {
          const SoundMacro* sm = activePool->soundMacro(keymap[60].macro);
          addPresetForMacro(128 + prog, sm);
        }
      }
    }
  }

  fmt::print("fluidsyX: created {} presets from MusyX data\n",
         sfData->presets.size());

  /* Keep a pointer so processMacroCmd can look up samples by SampleId */
  musyxSfData = sfData;

  /* 3. Register the custom loader and load the soundfont */
  fluid_sfloader_t* loader = new_fluid_sfloader(
      musyx_sfloader_load, musyx_sfloader_free);
  fluid_sfloader_set_data(loader, sfData);
  fluid_synth_add_sfloader(synth.get(), loader);

  /* Trigger the load via sfload with our magic filename */
  int sfId = fluid_synth_sfload(synth.get(), "MusyX_Virtual", /*reset_presets=*/1);
  if (sfId < 0) {
    fmt::print(stderr, "fluidsyX: warning: failed to activate MusyX SoundFont\n");
    return false;
  }

  fmt::print("fluidsyX: MusyX SoundFont activated (id={})\n", sfId);

  /* Create the dummy preset used to route CmdStartSample through
   * fluid_synth_start() so that voices receive a user-controlled unique ID. */
  fluid_sfont_t* musyxSfont = fluid_synth_get_sfont_by_id(synth.get(), sfId);
  dummyPreset = new_fluid_preset(
      musyxSfont,
      [](fluid_preset_t*) -> const char* { return "MusyX Voice"; },
      [](fluid_preset_t*) -> int { return 0; },
      [](fluid_preset_t*) -> int { return 128; },
      dummy_preset_noteon,
      dummy_preset_free);
  if (dummyPreset)
    fluid_preset_set_data(dummyPreset, this);

  return true;
}

/* ═══════════════════ Voice-level helpers ═══════════════════ */

/** Find a FluidSynth voice by its unique ID. Scans the voice list via
 *  fluid_synth_get_voicelist() — safe because we never store raw voice
 *  pointers across scheduling boundaries. Returns nullptr if not found. */
static fluid_voice_t* findVoiceById(fluid_synth_t* synth, unsigned int voiceId) {
  if (voiceId == 0) return nullptr;
  int poly = fluid_synth_get_polyphony(synth);
  std::vector<fluid_voice_t*> voices(static_cast<size_t>(poly));
  fluid_synth_get_voicelist(synth, voices.data(), poly, voiceId);
  
  if(voices[0] != nullptr && voices[1] != nullptr)
  {
    fmt::print(stderr, "Warning: multiple voices with the same ID {} found!\n", voiceId);
  }
  return voices[0];
}

/** Return the live FluidSynth voice for this macro context, or nullptr. */
static fluid_voice_t* getActiveVoice(fluid_synth_t* synth, MacroExecContext& ctx) {
  return findVoiceById(synth, ctx.voiceId);
}

/** Trigger the ADSR release phase of a voice (equivalent to note-off).
 *  fluid_synth_stop() sends a note-off to the voice identified by the unique
 *  ID, causing the volume envelope to enter the release phase.  The voice
 *  continues playing (fading out) until the release completes.
 *  voiceId is NOT cleared – the voice may still be looked up for further
 *  manipulation while it is in the release phase. */
static void releaseVoice(fluid_synth_t* synth, MacroExecContext& ctx) {
  if (ctx.voiceId != 0)
    fluid_synth_stop(synth, ctx.voiceId);
}

/** Immediately silence a voice (near-instant release).
 *  Sets the volume envelope release to the minimum timecent value before
 *  calling fluid_synth_stop(), making the release effectively instant.
 *  fluid_voice_off() is only available in FluidSynth 2.4+. */
static void killVoice(fluid_synth_t* synth, MacroExecContext& ctx) {
  if (ctx.voiceId != 0) {
    fluid_voice_t* v = findVoiceById(synth, ctx.voiceId);
    if (v) {
      /* Set release to near-instant */
      fluid_voice_gen_set(v, GEN_VOLENVRELEASE, kInstantReleaseTimecents);
      fluid_voice_update_param(v, GEN_VOLENVRELEASE);
    }
    fluid_synth_stop(synth, ctx.voiceId);
  }
  ctx.voiceId = 0;
}

void FluidsyXApp::applyAdsrToVoice(fluid_voice_t* v, MacroExecContext& ctx,
                                    bool started) {
  if (!ctx.useAdsrControllers || !v)
    return;

#if FLUID_VERSION_AT_LEAST(2,5,0)
  /* Attack */
  fluid_mod_set_source1(modBlueprintADR.get(), ctx.midiAttack, fluid_mod_get_flags1(modBlueprintADR.get()));
  fluid_mod_set_dest(modBlueprintADR.get(), GEN_VOLENVATTACK);
  fluid_voice_add_mod(v, modBlueprintADR.get(), FLUID_VOICE_OVERWRITE);

  /* Decay */
  fluid_mod_set_source1(modBlueprintADR.get(), ctx.midiDecay, fluid_mod_get_flags1(modBlueprintADR.get()));
  fluid_mod_set_dest(modBlueprintADR.get(), GEN_VOLENVDECAY);
  fluid_voice_add_mod(v, modBlueprintADR.get(), FLUID_VOICE_OVERWRITE);

  /* Release */
  fluid_mod_set_source1(modBlueprintADR.get(), ctx.midiRelease, fluid_mod_get_flags1(modBlueprintADR.get()));
  fluid_mod_set_dest(modBlueprintADR.get(), GEN_VOLENVRELEASE);
  fluid_voice_add_mod(v, modBlueprintADR.get(), FLUID_VOICE_OVERWRITE);
  /* Sustain – SF2: 0 cB = max volume, 1440 cB = silence */
  fluid_mod_set_source1(modBlueprintSustain.get(), ctx.midiSustain, fluid_mod_get_flags1(modBlueprintSustain.get()));
  fluid_mod_set_dest(modBlueprintSustain.get(), GEN_VOLENVSUSTAIN);
  fluid_voice_add_mod(v, modBlueprintSustain.get(), FLUID_VOICE_OVERWRITE);

  if (started) {
    fluid_voice_update_param(v, GEN_VOLENVATTACK);
    fluid_voice_update_param(v, GEN_VOLENVDECAY);
    fluid_voice_update_param(v, GEN_VOLENVSUSTAIN);
    fluid_voice_update_param(v, GEN_VOLENVRELEASE);
  }
#else
#warning "FluidSynth 2.5 or later is required for per-voice ADSR control. ADSR commands will be ignored."
#endif
}

void FluidsyXApp::applyVoicePitch(MacroExecContext& ctx) {
  fluid_voice_t* v = getActiveVoice(synth.get(), ctx);
  if (!v) return;
  int offsetCents = (ctx.midiKey - ctx.allocKey) * 100 + ctx.curDetune;
  int coarse = offsetCents / 100;
  int fine   = offsetCents % 100;
  fluid_voice_gen_set(v, GEN_COARSETUNE, static_cast<float>(coarse));
  fluid_voice_gen_set(v, GEN_FINETUNE,   static_cast<float>(fine));
  fluid_voice_update_param(v, GEN_COARSETUNE);
  fluid_voice_update_param(v, GEN_FINETUNE);
}

/* ═══════════════════ SoundMacro → FluidSynth translation ═══════════════════ */

/** Format a SoundMacro command using its CmdIntrospection metadata.
 *  Produces a string like: "SetNote(Key=60, Detune=0, Use Millisec=1, Ticks/Millisec=0)"
 *  If no introspection is available, falls back to the numeric CmdOp value. */
static std::string formatMacroCmd(const SoundMacro::ICmd& cmd) {
  using Field = SoundMacro::CmdIntrospection::Field;
  const SoundMacro::CmdOp op = cmd.Isa();
  const auto* intro = SoundMacro::GetCmdIntrospection(op);

  std::string result;
  if (intro) {
    result = std::string(intro->m_name);
  } else {
    result = fmt::format("CmdOp({})", static_cast<int>(op));
  }

  if (!intro)
    return result;

  /* Collect formatted fields */
  std::string fields;
  for (const auto& f : intro->m_fields) {
    if (f.m_name.empty())
      continue;

    const auto* base = reinterpret_cast<const char*>(&cmd);
    std::string val;

    switch (f.m_tp) {
    case Field::Type::Bool: {
      bool v{};
      std::memcpy(&v, base + f.m_offset, sizeof(v));
      val = v ? "true" : "false";
      break;
    }
    case Field::Type::Int8: {
      int8_t v{};
      std::memcpy(&v, base + f.m_offset, sizeof(v));
      val = fmt::format("{}", static_cast<int>(v));
      break;
    }
    case Field::Type::UInt8: {
      uint8_t v{};
      std::memcpy(&v, base + f.m_offset, sizeof(v));
      val = fmt::format("{}", static_cast<unsigned>(v));
      break;
    }
    case Field::Type::Int16: {
      int16_t v{};
      std::memcpy(&v, base + f.m_offset, sizeof(v));
      val = fmt::format("{}", v);
      break;
    }
    case Field::Type::UInt16: {
      uint16_t v{};
      std::memcpy(&v, base + f.m_offset, sizeof(v));
      val = fmt::format("{}", v);
      break;
    }
    case Field::Type::Int32: {
      int32_t v{};
      std::memcpy(&v, base + f.m_offset, sizeof(v));
      val = fmt::format("{}", v);
      break;
    }
    case Field::Type::UInt32: {
      uint32_t v{};
      std::memcpy(&v, base + f.m_offset, sizeof(v));
      val = fmt::format("{}", v);
      break;
    }
    case Field::Type::SoundMacroId: {
      uint16_t v{};
      std::memcpy(&v, base + f.m_offset, sizeof(v));
      val = fmt::format("macro:{}", v);
      break;
    }
    case Field::Type::SoundMacroStep: {
      uint16_t v{};
      std::memcpy(&v, base + f.m_offset, sizeof(v));
      val = fmt::format("step:{}", v);
      break;
    }
    case Field::Type::TableId: {
      uint16_t v{};
      std::memcpy(&v, base + f.m_offset, sizeof(v));
      val = fmt::format("table:{}", v);
      break;
    }
    case Field::Type::SampleId: {
      uint16_t v{};
      std::memcpy(&v, base + f.m_offset, sizeof(v));
      val = fmt::format("sample:{}", v);
      break;
    }
    case Field::Type::Choice: {
      uint8_t v{};
      std::memcpy(&v, base + f.m_offset, sizeof(v));
      if (v < f.m_choices.size() && !f.m_choices[v].empty())
        val = fmt::format("{}({})", std::string_view(f.m_choices[v]),
                          static_cast<unsigned>(v));
      else
        val = fmt::format("{}", static_cast<unsigned>(v));
      break;
    }
    default:
      val = "?";
      break;
    }

    if (!fields.empty())
      fields += ", ";
    fields += fmt::format("{}={}", std::string_view(f.m_name), val);
  }

  if (!fields.empty())
    result += fmt::format("({})", fields);
  return result;
}

unsigned int FluidsyXApp::processMacroCmd(MacroExecContext& ctx,
                                          unsigned int curTick) {
  if (ctx.ended || !ctx.macro)
    return 0;
  if (ctx.pc < 0 || ctx.pc >= static_cast<int>(ctx.macro->m_cmds.size())) {
    ctx.ended = true;
    return 0;
  }

  const SoundMacro::ICmd& cmd = *ctx.macro->m_cmds[ctx.pc];
  SoundMacro::CmdOp op = cmd.Isa();
  unsigned int delay = 0;
  FluidEventPtr evt(new_fluid_event(), &delete_fluid_event);

  if (verbose)
    fmt::print("fluidsyX: [ch{} pc{}] {}\n", ctx.channel, ctx.pc,
               formatMacroCmd(cmd));

  fluid_event_set_source(evt.get(), callbackSeqId);
  fluid_event_set_dest(evt.get(), synthSeqId);

  switch (op) {
  /* ── Termination ── */
  case SoundMacro::CmdOp::End:
  case SoundMacro::CmdOp::Stop: {
    /* Original amuse: CmdEnd/CmdStop just set PC=-1 (stop macro execution).
     * The voice is NOT stopped – it continues playing until its ADSR envelope
     * completes naturally or an external note-off arrives. */
    ctx.ended = true;
    break;
  }

  /* ── Wait / Timing ── */
  case SoundMacro::CmdOp::WaitTicks: {
    auto& c = static_cast<const SoundMacro::CmdWaitTicks&>(cmd);
    if (c.keyOff)
      ctx.waitingKeyoff = true;
    if (c.sampleEnd)
      ctx.waitingSampleEnd = true;

    /* If the wait condition is already satisfied, skip the wait entirely
     * (matches original amuse: m_keyoffWait && m_keyoff → no wait). */
    if ((ctx.waitingKeyoff && ctx.keyoffReceived) ||
        (ctx.waitingSampleEnd && ctx.sampleEndReceived)) {
      ctx.waitingKeyoff = false;
      ctx.waitingSampleEnd = false;
      ctx.pc++;
      break;
    }

    uint16_t ticks = c.ticksOrMs;
    if (ticks == 0) {
      /* Indefinite wait – macro pauses until keyOff or sampleEnd.
       * Original amuse: m_indefiniteWait = true, m_inWait = true.
       * We use UINT_MAX as sentinel; the processing loop will store the
       * context but NOT schedule a timer. */
      ctx.inIndefiniteWait = true;
      ctx.pc++;
      delay = UINT_MAX;
    } else if (c.msSwitch) {
      /* value is already in ms */
      delay = ticks;
      ctx.pc++;
    } else {
      delay = static_cast<unsigned int>(ticks * 1000.0 / ctx.ticksPerSec);
      ctx.pc++;
    }
    break;
  }
  case SoundMacro::CmdOp::WaitMs: {
    auto& c = static_cast<const SoundMacro::CmdWaitMs&>(cmd);
    if (c.keyOff)
      ctx.waitingKeyoff = true;
    if (c.sampleEnd)
      ctx.waitingSampleEnd = true;

    /* Already satisfied? Skip immediately. */
    if ((ctx.waitingKeyoff && ctx.keyoffReceived) ||
        (ctx.waitingSampleEnd && ctx.sampleEndReceived)) {
      ctx.waitingKeyoff = false;
      ctx.waitingSampleEnd = false;
      ctx.pc++;
      break;
    }

    if (c.ms == 0) {
      ctx.inIndefiniteWait = true;
      ctx.pc++;
      delay = UINT_MAX;
    } else {
      delay = c.ms;
      ctx.pc++;
    }
    break;
  }

  /* ── Control flow ── */
  case SoundMacro::CmdOp::Goto: {
    auto& c = static_cast<const SoundMacro::CmdGoto&>(cmd);
    /* If macro ID references a different macro, spawn it */
    if (c.macro.id.id != 0xffff && c.macro.id.id != 0) {
      const SoundMacro* target = activePool ? activePool->soundMacro(c.macro.id) : nullptr;
      if (target) {
        ctx.macro = target;
        ctx.pc = c.macroStep.step;
      } else {
        ctx.pc++;
      }
    } else {
      ctx.pc = c.macroStep.step;
    }
    break;
  }
  case SoundMacro::CmdOp::Loop: {
    auto& c = static_cast<const SoundMacro::CmdLoop&>(cmd);
    /* Original amuse: if keyOff/sampleEnd conditions are met, break out
     * of the loop immediately (st.m_keyoff / st.m_sampleEnd). */
    if ((c.keyOff && ctx.keyoffReceived) || (c.sampleEnd && ctx.sampleEndReceived)) {
      ctx.loopCountdown = -1;
      ctx.pc++;
      break;
    }
    if (ctx.loopCountdown < 0) {
      /* First encounter: initialize loop.
       * times==65535 → infinite loop (original amuse convention). */
      uint16_t useTimes = c.times;
      if (useTimes == kInfiniteLoopSentinel) {
        ctx.loopCountdown = -2; /* infinite */
      } else {
        ctx.loopCountdown = static_cast<int>(useTimes);
      }
      ctx.loopStep = c.macroStep.step;
    }
    if (ctx.loopCountdown == -2) {
      /* Infinite loop */
      ctx.pc = ctx.loopStep;
    } else if (ctx.loopCountdown > 0) {
      ctx.loopCountdown--;
      ctx.pc = ctx.loopStep;
    } else {
      ctx.loopCountdown = -1;
      ctx.pc++;
    }
    break;
  }
  case SoundMacro::CmdOp::Return: {
    /* For now just end; full subroutine support would need a call stack */
    ctx.ended = true;
    break;
  }
  case SoundMacro::CmdOp::GoSub: {
    auto& c = static_cast<const SoundMacro::CmdGoSub&>(cmd);
    /* Simplified: just goto the subroutine target */
    if (c.macro.id.id != 0xffff && c.macro.id.id != 0) {
      const SoundMacro* target = activePool ? activePool->soundMacro(c.macro.id) : nullptr;
      if (target) {
        ctx.macro = target;
        ctx.pc = c.macroStep.step;
      } else {
        ctx.pc++;
      }
    } else {
      ctx.pc = c.macroStep.step;
    }
    break;
  }

  /* ── Note / Pitch ── */
  case SoundMacro::CmdOp::SetNote: {
    auto& c = static_cast<const SoundMacro::CmdSetNote&>(cmd);
    ctx.midiKey = static_cast<uint8_t>(std::clamp(static_cast<int>(c.key), 0, 127));
    ctx.curDetune = c.detune; /* ±99 cents */
    /* Apply pitch via voice-level generators (per-voice, not per-channel). */
    applyVoicePitch(ctx);
    if (c.msSwitch)
      delay = c.ticksOrMs;
    else
      delay = static_cast<unsigned int>(c.ticksOrMs * 1000.0 / ctx.ticksPerSec);
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::AddNote: {
    auto& c = static_cast<const SoundMacro::CmdAddNote&>(cmd);
    int newKey = ctx.midiKey + c.add;
    ctx.midiKey = static_cast<uint8_t>(std::clamp(newKey, 0, 127));
    ctx.curDetune = c.detune;
    applyVoicePitch(ctx);
    if (c.msSwitch)
      delay = c.ticksOrMs;
    else
      delay = static_cast<unsigned int>(c.ticksOrMs * 1000.0 / ctx.ticksPerSec);
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::LastNote: {
    auto& c = static_cast<const SoundMacro::CmdLastNote&>(cmd);
    /* Original amuse: m_curPitch = (add + vox.getLastNote()) * 100 + detune
     * getLastNote() returns m_state.m_initKey — the original trigger key,
     * NOT the current midiKey (which may have been modified by SetNote). */
    int newKey = static_cast<int>(ctx.initKey) + c.add;
    ctx.midiKey = static_cast<uint8_t>(std::clamp(newKey, 0, 127));
    ctx.curDetune = c.detune;
    applyVoicePitch(ctx);
    if (c.msSwitch)
      delay = c.ticksOrMs;
    else
      delay = static_cast<unsigned int>(c.ticksOrMs * 1000.0 / ctx.ticksPerSec);
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::RndNote: {
    auto& c = static_cast<const SoundMacro::CmdRndNote&>(cmd);
    int lo = c.noteLo;
    int hi = c.noteHi;

    /* absRel mode: range is relative to initial key (m_initKey in amuse). */
    if (c.absRel) {
      lo = ctx.initKey - c.noteLo;
      hi = ctx.initKey + c.noteHi;
    }

    if (lo > hi)
      std::swap(lo, hi);
    std::uniform_int_distribution<int> dist(lo, hi);
    int note = dist(rng);
    if (c.fixedFree) {
      ctx.midiKey = static_cast<uint8_t>(std::clamp(note, 0, 127));
    } else {
      ctx.midiKey = static_cast<uint8_t>(
          std::clamp(ctx.midiKey + note, 0, 127));
    }
    /* Apply RndNote detune (same +/-99 cent field as SetNote et al.) */
    ctx.curDetune = c.detune;
    applyVoicePitch(ctx);
    ctx.pc++;
    break;
  }

  /* ── Pitch control ── */
  case SoundMacro::CmdOp::SetPitch: {
    /* SetPitch sets an absolute frequency (hz + fine/65536 Hz).
     * Convert to MIDI note + cent offset, then apply via voice generators. */
    auto& c = static_cast<const SoundMacro::CmdSetPitch&>(cmd);
    double freq = static_cast<double>(c.hz) + c.fine / 65536.0;
    if (freq > 0.0) {
      double midiNote = 69.0 + 12.0 * std::log2(freq / 440.0);
      int noteInt = static_cast<int>(std::round(midiNote));
      noteInt = std::clamp(noteInt, 0, 127);
      double centFrac = (midiNote - noteInt) * 100.0;
      ctx.midiKey = static_cast<uint8_t>(noteInt);
      ctx.curDetune = static_cast<int>(std::round(centFrac));
      applyVoicePitch(ctx);
    }
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::PitchSweep1:
  case SoundMacro::CmdOp::PitchSweep2: {
    /* Timer-driven pitch sweep; for now just advance */
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::PitchWheelR: {
    auto& c = static_cast<const SoundMacro::CmdPitchWheelR&>(cmd);
    if (c.rangeUp != c.rangeDown) {
      fmt::print(stderr,
              "fluidsyX: warning: PitchWheelR has asymmetric range "
              "(up={}, down={}); FluidSynth only supports symmetric "
              "pitch bend range, using rangeUp={}\n",
              static_cast<int>(c.rangeUp), static_cast<int>(c.rangeDown),
              static_cast<int>(c.rangeUp));
    }
    fluid_event_pitch_wheelsens(evt.get(), ctx.channel, c.rangeUp);
    fluid_sequencer_send_at(sequencer.get(), evt.get(), curTick, 1);
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::SetPitchAdsr: {
    /* Timer-driven pitch ADSR; advance for now */
    ctx.pc++;
    break;
  }

  /* ── Volume / Envelope ── */
  case SoundMacro::CmdOp::ScaleVolume: {
    auto& c = static_cast<const SoundMacro::CmdScaleVolume&>(cmd);
    int vol = (ctx.midiVel * (c.scale + 128)) / 256 + c.add;
    vol = std::clamp(vol, 0, 127);
    /* GEN_ATTENUATION in centibels: 0=full volume, 1440=silence.
     * Convert MIDI velocity-scaled volume to centibel attenuation. */
    float attn = (vol > 0)
        ? static_cast<float>(-200.0 * std::log10(vol / 127.0))
        : 1440.0f;
    attn = std::clamp(attn, 0.0f, 1440.0f);
    if (auto* v = getActiveVoice(synth.get(), ctx)) {
      fluid_voice_gen_set(v, GEN_ATTENUATION, attn);
      fluid_voice_update_param(v, GEN_ATTENUATION);
    } else {
      ctx.pendingAttnGen = attn;
      ctx.hasPendingAttn = true;
    }
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::ScaleVolumeDLS: {
    auto& c = static_cast<const SoundMacro::CmdScaleVolumeDLS&>(cmd);
    int vol = (ctx.midiVel * (c.scale + 32768)) / 65536;
    vol = std::clamp(vol, 0, 127);
    float attn = (vol > 0)
        ? static_cast<float>(-200.0 * std::log10(vol / 127.0))
        : 1440.0f;
    attn = std::clamp(attn, 0.0f, 1440.0f);
    if (auto* v = getActiveVoice(synth.get(), ctx)) {
      fluid_voice_gen_set(v, GEN_ATTENUATION, attn);
      fluid_voice_update_param(v, GEN_ATTENUATION);
    } else {
      ctx.pendingAttnGen = attn;
      ctx.hasPendingAttn = true;
    }
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::Envelope:
  case SoundMacro::CmdOp::FadeIn: {
    /* Timer-driven envelope; advance for now */
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::SetAdsr: {
    /* ADSR table lookup for envelope shape – timer callback territory */
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::SetAdsrCtrl: {
    auto& c = static_cast<const SoundMacro::CmdSetAdsrCtrl&>(cmd);
    ctx.useAdsrControllers = true;
    ctx.midiAttack  = c.attack;
    ctx.midiDecay   = c.decay;
    ctx.midiSustain = c.sustain;
    ctx.midiRelease = c.release;

    /* Bootstrap ADSR defaults on first encounter, matching the amuse
     * convention in SoundMacroState.cpp. */
    if (!ctx.adsrBootstrapped) {
      ctx.adsrBootstrapped = true;
      ctx.ctrlVals[ctx.midiAttack]  = 10;
      ctx.ctrlVals[ctx.midiSustain] = 127;
      ctx.ctrlVals[ctx.midiRelease] = 10;

      /* Also bootstrap into channel-level CC state so that future macros
       * on this channel inherit the defaults. */
      channelCtrlVals[ctx.channel][ctx.midiAttack]  = 10;
      channelCtrlVals[ctx.channel][ctx.midiSustain] = 127;
      channelCtrlVals[ctx.channel][ctx.midiRelease] = 10;
    }

    /* Record the CC→ADSR mapping at channel level so that incoming SNG CC
     * events can be intercepted and converted to NRPN generator changes. */
    if (ctx.channel < 16) {
      auto& m = channelAdsrMap[ctx.channel];
      m.active    = true;
      m.attackCC  = c.attack;
      m.decayCC   = c.decay;
      m.sustainCC = c.sustain;
      m.releaseCC = c.release;
    }

    /* Apply ADSR immediately.  If the voice is already playing, use
     * voice-level generators; otherwise defer to StartSample. */
    if (auto* v = getActiveVoice(synth.get(), ctx)) {
      applyAdsrToVoice(v, ctx, /*started=*/true);
    }
    ctx.pc++;
    break;
  }

  /* ── Panning ── */
  case SoundMacro::CmdOp::Panning: {
    auto& c = static_cast<const SoundMacro::CmdPanning&>(cmd);
    if (c.panPosition == -128) {
      /* -128 = surround-channel only; no-op in FluidSynth */
    } else {
      /* GEN_PAN: -500..+500 (0.1% units).  panPosition is -127..+127 */
      float panGen = (c.panPosition / 127.0f) * 500.0f;
      if (auto* v = getActiveVoice(synth.get(), ctx)) {
        fluid_voice_gen_set(v, GEN_PAN, panGen);
        fluid_voice_update_param(v, GEN_PAN);
      } else {
        ctx.pendingPanGen = panGen;
        ctx.hasPendingPan = true;
      }
    }
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::PianoPan: {
    auto& c = static_cast<const SoundMacro::CmdPianoPan&>(cmd);
    /* Original amuse uses m_initKey (the original trigger key), not the
     * current midiKey which may have been changed by SetNote/AddNote. */
    int diff = ctx.initKey - c.centerKey;
    int panPos = c.centerPan + diff * c.scale / 127;
    panPos = std::clamp(panPos, -127, 127);
    float panGen = (panPos / 127.0f) * 500.0f;
    if (auto* v = getActiveVoice(synth.get(), ctx)) {
      fluid_voice_gen_set(v, GEN_PAN, panGen);
      fluid_voice_update_param(v, GEN_PAN);
    } else {
      ctx.pendingPanGen = panGen;
      ctx.hasPendingPan = true;
    }
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::Spanning: {
    /* Spanning controls surround panning, which FluidSynth does not
     * currently support.  Logged as a no-op. */
    ctx.pc++;
    break;
  }

  /* ── Sample commands ── */
  case SoundMacro::CmdOp::StartSample: {
    auto& c = static_cast<const SoundMacro::CmdStartSample&>(cmd);
    /* Look up the specific sample referenced by this command.
     * Route voice creation through fluid_synth_start() so the voice receives
     * a unique user-controlled ID (fluid_synth_alloc_voice() alone leaves ID=0).
     * This allows fluid_synth_stop(synth, id) to properly stop the voice. */
    fluid_sample_t* flSamp = nullptr;
    bool looped = false;
    if (musyxSfData) {
      auto sIt = musyxSfData->sampleIndex.find(c.sample.id.id);
      if (sIt != musyxSfData->sampleIndex.end()) {
        auto& ds = musyxSfData->samples[sIt->second];
        flSamp = ds.flSample;
        looped = ds.looped;
      }
    }
    if (flSamp && dummyPreset) {
      /* Populate the pending voice start context for the noteon callback */
      pendingVoiceStart.flSamp        = flSamp;
      pendingVoiceStart.looped        = looped;
      pendingVoiceStart.macroContext = &ctx;

      /* Allocate a unique non-zero ID */
      unsigned int id = nextVoiceId++;
      if (id == 0) id = nextVoiceId++;

      /* fluid_synth_start() sets synth->noteid = id before calling noteon,
       * so the voice allocated inside dummy_preset_noteon gets our custom ID. */
      if (fluid_synth_start(synth.get(), id, dummyPreset, 0, ctx.channel,
                             ctx.midiKey, ctx.midiVel) == FLUID_OK) {
        ctx.voiceId  = id;
        ctx.allocKey = ctx.midiKey;
        ctx.hasPendingPan  = false;
        ctx.hasPendingAttn = false;
      } else {
        fmt::print(stderr, "fluidsyX: warning: voice start failed for "
                        "sample {} on ch {} key {}\n",
                c.sample.id.id, ctx.channel, ctx.midiKey);
      }
      pendingVoiceStart.flSamp = nullptr; /* prevent accidental reuse */
    } else {
      /* Fallback: send note-on to trigger preset (wrong sample possible) */
      fluid_event_noteon(evt.get(), ctx.channel, ctx.midiKey, ctx.midiVel);
      fluid_sequencer_send_at(sequencer.get(), evt.get(), curTick, 1);
    }
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::StopSample: {
    /* Original amuse: vox.stopSample() = m_curSample.reset() – immediate stop.
     * In FluidSynth we kill the voice outright (no release phase). */
    killVoice(synth.get(), ctx);
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::KeyOff: {
    /* Original amuse: vox._macroKeyOff() triggers ADSR release phase.
     * The voice continues fading out; the macro continues executing.
     * fluid_synth_stop() sends a note-off triggering the release envelope. */
    releaseVoice(synth.get(), ctx);
    ctx.keyoffReceived = true;
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::SendKeyOff: {
    /* Original amuse: sends key-off to another voice (last started or by var).
     * Simplified: if we have a last-played child macro, release its voice. */
    auto& c = static_cast<const SoundMacro::CmdSendKeyOff&>(cmd);
    int targetMacroId = -1;
    if (c.lastStarted) {
      targetMacroId = ctx.lastPlayMacroId;
    } else {
      targetMacroId = ctx.vars[c.variable & 0x1f];
    }
    if (targetMacroId >= 0) {
      auto childIt = activeMacros.find(targetMacroId);
      if (childIt != activeMacros.end() && !childIt->second.ended) {
        releaseVoice(synth.get(), childIt->second);
        childIt->second.keyoffReceived = true;
      }
    }
    ctx.pc++;
    break;
  }

  /* ── PlayMacro (spawn child macro) ── */
  case SoundMacro::CmdOp::PlayMacro: {
    auto& c = static_cast<const SoundMacro::CmdPlayMacro&>(cmd);
    if (activePool && c.macro.id.id != 0xffff) {
      const SoundMacro* child = activePool->soundMacro(c.macro.id);
      if (child) {
        /* Original amuse: startChildMacro uses m_initKey + addNote */
        int childKey = std::clamp(static_cast<int>(ctx.initKey) + c.addNote, 0, 127);
        int childId = enqueueSoundMacro(child, c.macroStep.step, ctx.channel,
                          static_cast<uint8_t>(childKey), ctx.midiVel,
                          curTick);
        ctx.lastPlayMacroId = childId;
      }
    }
    ctx.pc++;
    break;
  }

  /* ── Split / Branch ── */
  case SoundMacro::CmdOp::SplitKey: {
    auto& c = static_cast<const SoundMacro::CmdSplitKey&>(cmd);
    /* Original amuse uses m_initKey for the comparison, not the current key */
    if (ctx.initKey >= static_cast<uint8_t>(c.key)) {
      if (c.macro.id.id != 0xffff && c.macro.id.id != 0 && activePool) {
        const SoundMacro* target = activePool->soundMacro(c.macro.id);
        if (target) {
          ctx.macro = target;
          ctx.pc = c.macroStep.step;
          break;
        }
      }
      ctx.pc = c.macroStep.step;
    } else {
      ctx.pc++;
    }
    break;
  }
  case SoundMacro::CmdOp::SplitVel: {
    auto& c = static_cast<const SoundMacro::CmdSplitVel&>(cmd);
    if (ctx.midiVel >= static_cast<uint8_t>(c.velocity)) {
      if (c.macro.id.id != 0xffff && c.macro.id.id != 0 && activePool) {
        const SoundMacro* target = activePool->soundMacro(c.macro.id);
        if (target) {
          ctx.macro = target;
          ctx.pc = c.macroStep.step;
          break;
        }
      }
      ctx.pc = c.macroStep.step;
    } else {
      ctx.pc++;
    }
    break;
  }
  case SoundMacro::CmdOp::SplitMod: {
    /* Skip for now (no modulation tracking in this simple version) */
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::SplitRnd: {
    auto& c = static_cast<const SoundMacro::CmdSplitRnd&>(cmd);
    std::uniform_int_distribution<int> dist(0, 255);
    if (dist(rng) >= c.rnd) {
      if (c.macro.id.id != 0xffff && c.macro.id.id != 0 && activePool) {
        const SoundMacro* target = activePool->soundMacro(c.macro.id);
        if (target) {
          ctx.macro = target;
          ctx.pc = c.macroStep.step;
          break;
        }
      }
      ctx.pc = c.macroStep.step;
    } else {
      ctx.pc++;
    }
    break;
  }

  /* ── Vibrato / Tremolo / Portamento ── */
  case SoundMacro::CmdOp::Vibrato: {
    auto& c = static_cast<const SoundMacro::CmdVibrato&>(cmd);
    if (auto* v = getActiveVoice(synth.get(), ctx)) {
      /* GEN_VIBLFOTOPITCH: vibrato depth in cents */
      float depthCents = static_cast<float>(c.levelNote * 100 + c.levelFine);
      fluid_voice_gen_set(v, GEN_VIBLFOTOPITCH, depthCents);
      fluid_voice_update_param(v, GEN_VIBLFOTOPITCH);

      /* GEN_VIBLFOFREQ: vibrato frequency in absolute cents from 8.176 Hz */
      if (c.ticksOrMs > 0) {
        float q = c.msSwitch ? 1000.0f : static_cast<float>(ctx.ticksPerSec);
        float periodSec = c.ticksOrMs / q;
        if (periodSec > 0.0f) {
          float freqHz = 1.0f / periodSec;
          float freqCents = 1200.0f * std::log2(freqHz / 8.176f);
          fluid_voice_gen_set(v, GEN_VIBLFOFREQ, freqCents);
          fluid_voice_update_param(v, GEN_VIBLFOFREQ);
        }
      }
    } else {
      /* Fallback to channel-level modulation if no voice yet */
      int modVal = std::clamp(static_cast<int>(c.levelNote) * 8, 0, 127);
      fluid_event_modulation(evt.get(), ctx.channel, modVal);
      fluid_sequencer_send_at(sequencer.get(), evt.get(), curTick, 1);
    }
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::Portamento: {
    auto& c = static_cast<const SoundMacro::CmdPortamento&>(cmd);
    bool enable = (c.portState != SoundMacro::CmdPortamento::PortState::Disable);

    /* CC 65 = portamento on/off */
    fluid_event_control_change(evt.get(), ctx.channel, 65, enable ? 127 : 0);
    fluid_sequencer_send_at(sequencer.get(), evt.get(), curTick, 1);

    if (enable) {
      /* Convert time value to milliseconds */
      uint16_t timeVal = c.ticksOrMs;
      double timeMs;
      if (c.msSwitch) {
        timeMs = static_cast<double>(timeVal);
      } else {
        timeMs = timeVal * 1000.0 / ctx.ticksPerSec;
      }
      // CC5 * 128 + CC37 = millisec
      fluid_event_control_change(evt.get(), ctx.channel, 5, static_cast<int>(timeMs) / 128);
      fluid_sequencer_send_at(sequencer.get(), evt.get(), curTick, 1);
      fluid_event_control_change(evt.get(), ctx.channel, 37, static_cast<int>(timeMs) % 128);
      fluid_sequencer_send_at(sequencer.get(), evt.get(), curTick, 1);

      /* Set portamento mode based on MusyX PortType.
       * LastPressed → legato-only mode, Always → each-note mode. */
      if (c.portType == SoundMacro::CmdPortamento::PortType::LastPressed)
        fluid_synth_set_portamento_mode(synth.get(), ctx.channel,
                                        FLUID_CHANNEL_PORTAMENTO_MODE_LEGATO_ONLY);
      else
        fluid_synth_set_portamento_mode(synth.get(), ctx.channel,
                                        FLUID_CHANNEL_PORTAMENTO_MODE_EACH_NOTE);
    }
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::SetupTremolo:
  case SoundMacro::CmdOp::Mod2Vibrange:
  case SoundMacro::CmdOp::SetupLFO:
  case SoundMacro::CmdOp::ModeSelect: {
    /* Timer-driven modulation effects; advance */
    ctx.pc++;
    break;
  }

  /* ── Event trapping ── */
  case SoundMacro::CmdOp::TrapEvent: {
    /* In a full implementation this would register event handlers;
     * for now, just advance */
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::UntrapEvent: {
    ctx.pc++;
    break;
  }

  /* ── Messages ── */
  case SoundMacro::CmdOp::SendMessage:
  case SoundMacro::CmdOp::GetMessage:
  case SoundMacro::CmdOp::GetVid: {
    ctx.pc++;
    break;
  }

  /* ── Variable operations ── */
  case SoundMacro::CmdOp::SetVar: {
    auto& c = static_cast<const SoundMacro::CmdSetVar&>(cmd);
    int idx = static_cast<int>(c.a);
    if (idx >= 0 && idx < 32)
      ctx.vars[idx] = c.imm;
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::AddVars: {
    auto& c = static_cast<const SoundMacro::CmdAddVars&>(cmd);
    int a = static_cast<int>(c.a);
    int b = static_cast<int>(c.b);
    if (a >= 0 && a < 32 && b >= 0 && b < 32)
      ctx.vars[a] += ctx.vars[b];
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::SubVars: {
    auto& c = static_cast<const SoundMacro::CmdSubVars&>(cmd);
    int a = static_cast<int>(c.a);
    int b = static_cast<int>(c.b);
    if (a >= 0 && a < 32 && b >= 0 && b < 32)
      ctx.vars[a] -= ctx.vars[b];
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::MulVars: {
    auto& c = static_cast<const SoundMacro::CmdMulVars&>(cmd);
    int a = static_cast<int>(c.a);
    int b = static_cast<int>(c.b);
    if (a >= 0 && a < 32 && b >= 0 && b < 32)
      ctx.vars[a] *= ctx.vars[b];
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::DivVars: {
    auto& c = static_cast<const SoundMacro::CmdDivVars&>(cmd);
    int a = static_cast<int>(c.a);
    int b = static_cast<int>(c.b);
    if (a >= 0 && a < 32 && b >= 0 && b < 32 && ctx.vars[b] != 0)
      ctx.vars[a] /= ctx.vars[b];
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::AddIVars: {
    auto& c = static_cast<const SoundMacro::CmdAddIVars&>(cmd);
    int a = static_cast<int>(c.a);
    if (a >= 0 && a < 32)
      ctx.vars[a] += c.imm;
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::IfEqual: {
    auto& c = static_cast<const SoundMacro::CmdIfEqual&>(cmd);
    int a = static_cast<int>(c.a);
    int b = static_cast<int>(c.b);
    bool cond = false;
    if (a >= 0 && a < 32 && b >= 0 && b < 32)
      cond = (ctx.vars[a] == ctx.vars[b]);
    if (cond) {
      if (!c.notEq)
        ctx.pc = c.macroStep.step;
      else
        ctx.pc++;
    } else {
      if (c.notEq)
        ctx.pc = c.macroStep.step;
      else
        ctx.pc++;
    }
    break;
  }
  case SoundMacro::CmdOp::IfLess: {
    auto& c = static_cast<const SoundMacro::CmdIfLess&>(cmd);
    int a = static_cast<int>(c.a);
    int b = static_cast<int>(c.b);
    bool cond = false;
    if (a >= 0 && a < 32 && b >= 0 && b < 32)
      cond = (ctx.vars[a] < ctx.vars[b]);
    if (cond) {
      if (!c.notLt)
        ctx.pc = c.macroStep.step;
      else
        ctx.pc++;
    } else {
      if (c.notLt)
        ctx.pc = c.macroStep.step;
      else
        ctx.pc++;
    }
    break;
  }

  /* ── Selector evaluators (controller → parameter mapping) ── */
  case SoundMacro::CmdOp::VolSelect: {
    auto& c = static_cast<const SoundMacro::CmdVolSelect&>(cmd);
    /* In MusyX, VolSelect binds a CC to the volume evaluator.
     * We forward the referenced CC value as a volume CC to FluidSynth. */
    if (!c.isVar && c.midiControl < 128) {
      fluid_event_control_change(evt.get(), ctx.channel, 7,
          std::clamp(static_cast<int>(ctx.ctrlVals[c.midiControl]), 0, 127));
      fluid_sequencer_send_at(sequencer.get(), evt.get(), curTick, 1);
    }
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::PanSelect: {
    auto& c = static_cast<const SoundMacro::CmdPanSelect&>(cmd);
    if (!c.isVar && c.midiControl < 128) {
      fluid_event_control_change(evt.get(), ctx.channel, 10,
          std::clamp(static_cast<int>(ctx.ctrlVals[c.midiControl]), 0, 127));
      fluid_sequencer_send_at(sequencer.get(), evt.get(), curTick, 1);
    }
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::PitchWheelSelect: {
    /* Pitchbend is handled through PitchWheelR and pitch bend events */
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::ModWheelSelect: {
    auto& c = static_cast<const SoundMacro::CmdModWheelSelect&>(cmd);
    if (!c.isVar && c.midiControl < 128) {
      fluid_event_modulation(evt.get(), ctx.channel,
          std::clamp(static_cast<int>(ctx.ctrlVals[c.midiControl]), 0, 127));
      fluid_sequencer_send_at(sequencer.get(), evt.get(), curTick, 1);
    }
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::PedalSelect: {
    auto& c = static_cast<const SoundMacro::CmdPedalSelect&>(cmd);
    /* CC 64 = sustain pedal */
    if (!c.isVar && c.midiControl < 128) {
      fluid_event_control_change(evt.get(), ctx.channel, 64,
          std::clamp(static_cast<int>(ctx.ctrlVals[c.midiControl]), 0, 127));
      fluid_sequencer_send_at(sequencer.get(), evt.get(), curTick, 1);
    }
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::PortamentoSelect: {
    /* Portamento is configured via the Portamento command;
     * the select just sets up the evaluator. */
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::ReverbSelect: {
    auto& c = static_cast<const SoundMacro::CmdReverbSelect&>(cmd);
    /* CC 91 = reverb send */
    if (!c.isVar && c.midiControl < 128) {
      fluid_event_control_change(evt.get(), ctx.channel, 91,
          std::clamp(static_cast<int>(ctx.ctrlVals[c.midiControl]), 0, 127));
      fluid_sequencer_send_at(sequencer.get(), evt.get(), curTick, 1);
    }
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::SpanSelect:      /* surround panning – no-op (FluidSynth limitation) */
  case SoundMacro::CmdOp::DopplerSelect:   /* surround doppler – no-op (FluidSynth limitation) */
  case SoundMacro::CmdOp::TremoloSelect:
  case SoundMacro::CmdOp::PreASelect:
  case SoundMacro::CmdOp::PreBSelect:
  case SoundMacro::CmdOp::PostBSelect:
  case SoundMacro::CmdOp::AuxAFXSelect:
  case SoundMacro::CmdOp::AuxBFXSelect: {
    /* Advanced controller routing – skip for now */
    if (!verbose)
      fmt::print(stderr, "fluidsyX: [ch{} pc{}] UNIMPLEMENTED {}\n",
                 ctx.channel, ctx.pc, formatMacroCmd(cmd));
    ctx.pc++;
    break;
  }

  /* ── Miscellaneous ── */
  case SoundMacro::CmdOp::SendFlag:
  case SoundMacro::CmdOp::AgeCntSpeed:
  case SoundMacro::CmdOp::AgeCntVel:
  case SoundMacro::CmdOp::AddAgeCount:
  case SoundMacro::CmdOp::SetAgeCount:
  case SoundMacro::CmdOp::SetKeygroup:
  case SoundMacro::CmdOp::WiiUnknown:
  case SoundMacro::CmdOp::WiiUnknown2: {
    if (!verbose)
      fmt::print(stderr, "fluidsyX: [ch{} pc{}] UNIMPLEMENTED {}\n",
                 ctx.channel, ctx.pc, formatMacroCmd(cmd));
    ctx.pc++;
    break;
  }

  /* ── deliberately unimplemented ── */
  case SoundMacro::CmdOp::SRCmodeSelect:
    // ignore – this is for sample rate conversion mode, which Fluidsynth only supports as global setting
    ctx.pc++;
    break;

  case SoundMacro::CmdOp::AddPriority:
  case SoundMacro::CmdOp::SetPriority:
    // ignore - this is only relevant if we would run out of polyphony, which we won't because fluidsynth has enough
    ctx.pc++;
    break;

  default: {
    /* Unknown op – always log */
    if (!verbose)
      fmt::print(stderr, "fluidsyX: [ch{} pc{}] UNIMPLEMENTED {}\n",
                 ctx.channel, ctx.pc, formatMacroCmd(cmd));
    ctx.pc++;
    break;
  }
  }

  return delay;
}

/* ── Resolve note → SoundMacro via SongGroupIndex pages ── */

void FluidsyXApp::resolveAndEnqueueNote(uint8_t channel, uint8_t note,
                                        uint8_t vel, unsigned int tick) {
  if (!activeSongGroup || !activePool)
    return;

  /* Look up the page entry for the channel's current program */
  uint8_t prog = channelPrograms[channel];
  const SongGroupIndex::PageEntry* page = nullptr;

  if (channel == 9) {
    auto it = activeSongGroup->m_drumPages.find(prog);
    if (it != activeSongGroup->m_drumPages.end())
      page = &it->second;
  } else {
    auto it = activeSongGroup->m_normPages.find(prog);
    if (it != activeSongGroup->m_normPages.end())
      page = &it->second;
  }

  if (!page)
    return;

  ObjectId objId = page->objId;

  /* Resolve the ObjectId to SoundMacro(s) via keymap/layer/direct lookup.
   * Replicates the logic from Voice::loadPageObject(). */
  if (objId.id & 0x8000) {
    /* Layer: multiple SoundMacros mapped by key range */
    const std::vector<LayerMapping>* layer = activePool->layer(objId);
    if (layer) {
      for (const auto& mapping : *layer) {
        if (note >= static_cast<uint8_t>(mapping.keyLo) &&
            note <= static_cast<uint8_t>(mapping.keyHi)) {
          const SoundMacro* sm = activePool->soundMacro(mapping.macro);
          if (sm) {
            uint8_t mappedKey = static_cast<uint8_t>(
                std::clamp(static_cast<int>(note) + mapping.transpose, 0, 127));
            /* Pass 'note' as triggerNote so NoteOff can match on the
             * original SNG note (before transpose). */
            enqueueSoundMacro(sm, 0, channel, mappedKey, vel, tick, note);
          }
        }
      }
    }
  } else if (objId.id & 0x4000) {
    /* Keymap: per-key SoundMacro lookup (128-entry array indexed by MIDI key) */
    const Keymap* keymap = activePool->keymap(objId);
    if (keymap) {
      const Keymap& km = keymap[note];
      const SoundMacro* sm = activePool->soundMacro(km.macro);
      if (sm) {
        uint8_t mappedKey = static_cast<uint8_t>(
            std::clamp(static_cast<int>(note) + km.transpose, 0, 127));
        enqueueSoundMacro(sm, 0, channel, mappedKey, vel, tick, note);
      }
    }
  } else {
    /* Direct SoundMacro reference */
    const SoundMacro* sm = activePool->soundMacro(objId);
    if (sm)
      enqueueSoundMacro(sm, 0, channel, note, vel, tick);
  }
}

int FluidsyXApp::enqueueSoundMacro(const SoundMacro* sm, int step,
                                    int channel, uint8_t key, uint8_t vel,
                                    unsigned int startTick,
                                    uint8_t triggerNote) {
  MacroExecContext ctx;
  ctx.macro = sm;
  ctx.pc = step;
  ctx.channel = channel;
  ctx.midiKey = key;
  ctx.initKey = key;
  ctx.midiVel = vel;
  /* triggerNote = the original SNG note (before keymap/layer transpose).
   * Used to match note-off events.  If not supplied, defaults to key. */
  ctx.triggerNote = (triggerNote == 0xff) ? key : triggerNote;

  /* Initialize controller values from channel state so that selectors
   * and ADSR controllers see the current MIDI setup. */
  if (channel >= 0 && channel < 16)
    ctx.ctrlVals = channelCtrlVals[channel];

  /* Walk through commands that execute instantly (no delay).
   * When a command introduces a delay, schedule a timer event and stop. */
  unsigned int tick = startTick;
  int safetyCounter = 0;
  while (!ctx.ended && safetyCounter < kMaxMacroCmdsPerBurst) {
    unsigned int d = processMacroCmd(ctx, tick);
    if (d == UINT_MAX) {
      /* Indefinite wait – store context but do NOT schedule a timer.
       * The macro will be resumed by an external event (keyoff, sampleEnd). */
      int macroId = nextMacroId++;
      activeMacros[macroId] = ctx;
      return macroId;
    }
    tick += d;
    safetyCounter++;
    if (d > 0 && !ctx.ended) {
      /* Store context in the stable map and schedule a timer callback */
      int macroId = nextMacroId++;
      activeMacros[macroId] = ctx;

      FluidEventPtr tevt(new_fluid_event(), &delete_fluid_event);
      fluid_event_set_source(tevt.get(), callbackSeqId);
      fluid_event_set_dest(tevt.get(), callbackSeqId);
      fluid_event_timer(tevt.get(), reinterpret_cast<void*>(static_cast<intptr_t>(macroId)));
      fluid_sequencer_send_at(sequencer.get(), tevt.get(), tick, /*absolute=*/1);
      return macroId;
    }
  }
  return -1;
}

/* Timer callback – resume a SoundMacro that was waiting, or dispatch a
 * pending SNG event.  Negative data values encode SNG events:
 *   data = -(1 + eventIndex)  →  pendingSngEvents[eventIndex]
 * Non-negative values are macro context IDs (existing behavior). */
void FluidsyXApp::timerCallback(unsigned int time, fluid_event_t* event,
                                fluid_sequencer_t* /*seq*/, void* data) {
  auto* app = static_cast<FluidsyXApp*>(data);
  if (!app || fluid_event_get_type(event) != FLUID_SEQ_TIMER)
    return;

  intptr_t rawData = reinterpret_cast<intptr_t>(fluid_event_get_data(event));

  if (rawData < 0) {
    /* ── SNG event dispatch ── */
    size_t idx = static_cast<size_t>(-(rawData + 1));
    if (idx >= app->pendingSngEvents.size())
      return;

    const auto& sngEvt = app->pendingSngEvents[idx];
    switch (sngEvt.type) {
    case PendingSngNoteEvent::NoteOn:
      app->resolveAndEnqueueNote(sngEvt.channel, sngEvt.note,
                                 sngEvt.velocity, time);
      break;

    case PendingSngNoteEvent::NoteOff: {
      /* Trigger ADSR release on all active macro voices triggered by this
       * (channel, note).  We match on triggerNote (the original SNG note
       * before keymap/layer transpose).
       * Original amuse: voice->keyOff() → _doKeyOff() → ADSR release +
       * keyoffNotify (sets m_keyoff flag so waiting macros can resume). */
      uint8_t ch = sngEvt.channel;
      uint8_t note = sngEvt.note;
      for (auto& [id, mctx] : app->activeMacros) {
        if (mctx.channel == ch && mctx.triggerNote == note && !mctx.ended) {
          /* Trigger ADSR release (voice fades out naturally) */
          releaseVoice(app->synth.get(), mctx);
          mctx.keyoffReceived = true;

          /* If the macro is waiting for keyoff (either indefinite or timed),
           * schedule a timer to resume it now so it can break out of the
           * wait early (original amuse: m_keyoff → m_inWait = false). */
          if (mctx.waitingKeyoff) {
            mctx.inIndefiniteWait = false;
            mctx.waitingKeyoff = false;
            FluidEventPtr resumeEvt(new_fluid_event(), &delete_fluid_event);
            fluid_event_set_source(resumeEvt.get(), app->callbackSeqId);
            fluid_event_set_dest(resumeEvt.get(), app->callbackSeqId);
            fluid_event_timer(resumeEvt.get(), reinterpret_cast<void*>(
                static_cast<intptr_t>(id)));
            fluid_sequencer_send_at(app->sequencer.get(), resumeEvt.get(), time, 1);
          }
        }
      }
      break;
    }
    case PendingSngNoteEvent::ProgramChange:
      app->channelPrograms[sngEvt.channel] = sngEvt.note; /* note field = program */
      fluid_synth_program_change(app->synth.get(), sngEvt.channel, sngEvt.note);
      break;

    case PendingSngNoteEvent::CC: {
      uint8_t ch  = sngEvt.channel;
      uint8_t cc  = sngEvt.note;     /* note field = CC number */
      uint8_t val = sngEvt.velocity; /* velocity field = CC value */

      /* Update global CC state */
      if (ch < 16 && cc < 128)
        app->channelCtrlVals[ch][cc] = static_cast<int8_t>(val);

      /* Also propagate to any active macro contexts on this channel so
       * that ADSR controllers see the updated value. */
      for (auto& [id, mctx] : app->activeMacros) {
        if (mctx.channel == ch && !mctx.ended)
          mctx.ctrlVals[cc] = static_cast<int8_t>(val);
      }

      /* Forward the raw CC to FluidSynth (for non-ADSR uses) */
      fluid_synth_cc(app->synth.get(), ch, cc, val);
      break;
    }
    }
    return;
  }

  /* ── Macro timer resume (existing behavior) ── */
  int macroId = static_cast<int>(rawData);
  auto it = app->activeMacros.find(macroId);
  if (it == app->activeMacros.end())
    return;

  MacroExecContext& ctx = it->second;
  if (ctx.ended) {
    app->activeMacros.erase(it);
    return;
  }

  unsigned int tick = time;
  int safetyCounter = 0;
  while (!ctx.ended && safetyCounter < kMaxMacroCmdsPerBurst) {
    unsigned int d = app->processMacroCmd(ctx, tick);
    if (d == UINT_MAX) {
      /* Indefinite wait – macro pauses until external event (keyoff/sampleEnd).
       * Keep in activeMacros but don't schedule a timer. */
      return;
    }
    tick += d;
    safetyCounter++;
    if (d > 0 && !ctx.ended) {
      /* Re-schedule the timer */
      FluidEventPtr tevt(new_fluid_event(), &delete_fluid_event);
      fluid_event_set_source(tevt.get(), app->callbackSeqId);
      fluid_event_set_dest(tevt.get(), app->callbackSeqId);
      fluid_event_timer(tevt.get(), reinterpret_cast<void*>(static_cast<intptr_t>(macroId)));
      fluid_sequencer_send_at(app->sequencer.get(), tevt.get(), tick, /*absolute=*/1);
      return;
    }
  }

  /* If the macro ended, clean it up */
  if (ctx.ended)
    app->activeMacros.erase(it);
}

/* ═══════════════════ SNG → FluidSynth sequencer scheduling ═══════════════════ */

double FluidsyXApp::scheduleSongEvents(const uint8_t* sngData, size_t /*sngSize*/) {
  bool bigEndian = false;
  int sngVersion = SongState::DetectVersion(sngData, bigEndian);
  if (sngVersion < 0) {
    fmt::print(stderr, "fluidsyX: failed to detect SNG version\n");
    return 0.0;
  }
  fmt::print("fluidsyX: SNG version {} ({}-endian)\n", sngVersion,
         bigEndian ? "big" : "little");

  std::vector<SngEvent> events;
  double initialScale = 1000.0;
  SngLoopInfo loopInfo;
  if (!parseSngEvents(sngData, bigEndian, sngVersion, events, initialScale,
                      loopInfo)) {
    fmt::print(stderr, "fluidsyX: failed to parse SNG events\n");
    return 0.0;
  }

  fmt::print("fluidsyX: parsed {} song events, initial tempo scale {:.1f} ticks/s\n",
         events.size(), initialScale);
  if (loopInfo.hasLoop)
    fmt::print("fluidsyX: loop detected: ticks {} → {} (body = {} ticks)\n",
           loopInfo.loopStartTick, loopInfo.loopEndTick,
           loopInfo.loopEndTick - loopInfo.loopStartTick);
  if (events.empty())
    return 0.0;

  /* Set the sequencer time-scale to match the SNG tick rate.
   * SNG ticks are 384 per quarter-note; at T BPM the tick rate is
   * T * 384 / 60  ticks per second.  Events are then scheduled at
   * their native SNG tick positions.  Tempo changes are handled by
   * scheduling FLUID_SEQ_SCALE events that adjust the time-scale. */
  fluid_sequencer_set_time_scale(sequencer.get(), initialScale);

  unsigned int baseTick = fluid_sequencer_get_tick(sequencer.get());

  /* Clear any pending SNG events from a previous playback */
  pendingSngEvents.clear();

  FluidEventPtr evt(new_fluid_event(), &delete_fluid_event);
  fluid_event_set_source(evt.get(), -1);

  /* Helper: schedule a single SngEvent at (baseTick + tickOffset). */
  auto scheduleOne = [&](const SngEvent& e, uint32_t tickOffset) {
    unsigned int schedTick = baseTick + tickOffset;

    switch (e.type) {
    case SngEvent::NoteOn:
    case SngEvent::NoteOff:
    case SngEvent::CC:
    case SngEvent::Program: {
      /* Route through timer callback */
      PendingSngNoteEvent::Type t;
      uint8_t d1 = e.data1, d2 = e.data2;
      switch (e.type) {
      case SngEvent::NoteOn:  t = PendingSngNoteEvent::NoteOn; break;
      case SngEvent::NoteOff: t = PendingSngNoteEvent::NoteOff; d2 = 0; break;
      case SngEvent::CC:      t = PendingSngNoteEvent::CC; break;
      default:                t = PendingSngNoteEvent::ProgramChange; d2 = 0; break;
      }
      size_t idx = pendingSngEvents.size();
      pendingSngEvents.push_back({t, e.channel, d1, d2});
      intptr_t timerData = -(static_cast<intptr_t>(idx) + 1);

      fluid_event_set_dest(evt.get(), callbackSeqId);
      fluid_event_timer(evt.get(), reinterpret_cast<void*>(timerData));
      fluid_sequencer_send_at(sequencer.get(), evt.get(), schedTick, /*absolute=*/1);
      break;
    }

    case SngEvent::PitchBend:
      fluid_event_set_dest(evt.get(), synthSeqId);
      fluid_event_pitch_bend(evt.get(), e.channel, e.pitchBend14);
      fluid_sequencer_send_at(sequencer.get(), evt.get(), schedTick, /*absolute=*/1);
      break;

    case SngEvent::Tempo:
      fluid_event_set_dest(evt.get(), synthSeqId);
      fluid_event_scale(evt.get(), e.tempoScale);
      fluid_sequencer_send_at(sequencer.get(), evt.get(), schedTick, /*absolute=*/1);
      break;
    }
  };

  uint32_t lastTick = 0;

  /* Schedule all parsed events at their native tick positions */
  for (const auto& e : events) {
    scheduleOne(e, e.absTick);
    if (e.absTick > lastTick)
      lastTick = e.absTick;
  }

  /* ── Loop support ──
   * If the song has a loop and --duration was specified, re-schedule the
   * events from the loop body (loopStartTick..loopEndTick) at successive
   * offsets until maxDurationTicks is reached. */
  if (loopInfo.hasLoop && maxDurationTicks > 0 && lastTick < maxDurationTicks) {
    uint32_t loopLen = loopInfo.loopEndTick - loopInfo.loopStartTick;
    if (loopLen > 0) {
      /* Collect events within the loop body */
      std::vector<const SngEvent*> loopEvents;
      for (const auto& e : events) {
        if (e.absTick >= loopInfo.loopStartTick &&
            e.absTick < loopInfo.loopEndTick)
          loopEvents.push_back(&e);
      }

      /* Keep re-scheduling the loop body until we've filled maxDurationTicks.
       * Each iteration shifts events by one additional loopLen. */
      unsigned int loopIter = 0;
      while (lastTick < maxDurationTicks) {
        uint32_t iterBase = loopInfo.loopEndTick + loopLen * loopIter;
        if (iterBase >= maxDurationTicks)
          break;
        for (const auto* ep : loopEvents) {
          uint32_t relTick = ep->absTick - loopInfo.loopStartTick;
          uint32_t newTick = iterBase + relTick;
          if (newTick >= maxDurationTicks)
            break;
          scheduleOne(*ep, newTick);
          if (newTick > lastTick)
            lastTick = newTick;
        }
        ++loopIter;
      }
      fmt::print("fluidsyX: looped {} iterations to reach {} ticks\n",
             loopIter, lastTick);
    }
  }

  fmt::print("fluidsyX: scheduled {} SNG ticks of song data ({} SNG events, "
         "{} routed through SoundMacro)\n",
         lastTick, events.size(), pendingSngEvents.size());
  return static_cast<double>(lastTick);
}

/* ═══════════════════ Song playback loop ═══════════════════ */

void FluidsyXApp::songLoop(const SongGroupIndex& index) {
  /* Store a reference to the SongGroupIndex so that timer callbacks can
   * resolve note events through the page→keymap/layer→SoundMacro chain. */
  activeSongGroup = &index;

  /* Reset per-channel ADSR mappings from any previous playback */
  for (auto& m : channelAdsrMap)
    m = {};

  /* Apply the MIDI setup for the selected song to FluidSynth channels */
  std::map<SongId, std::array<SongGroupIndex::MIDISetup, 16>> sortEntries(
      index.m_midiSetups.cbegin(), index.m_midiSetups.cend());

  auto setupIt = sortEntries.find(setupId);
  if (setupIt != sortEntries.end()) {
    const auto& midiSetup = setupIt->second;
    for (int ch = 0; ch < 16; ++ch) {
      channelPrograms[ch] = midiSetup[ch].programNo;
      fluid_synth_program_change(synth.get(), ch, midiSetup[ch].programNo);
      fluid_synth_cc(synth.get(), ch, 7, midiSetup[ch].volume);
      fluid_synth_cc(synth.get(), ch, 10, midiSetup[ch].panning);
      fluid_synth_cc(synth.get(), ch, 91, midiSetup[ch].reverb);
      fluid_synth_cc(synth.get(), ch, 93, midiSetup[ch].chorus);

      channelCtrlVals[ch][7]  = static_cast<int8_t>(midiSetup[ch].volume);
      channelCtrlVals[ch][10] = static_cast<int8_t>(midiSetup[ch].panning);
      channelCtrlVals[ch][91] = static_cast<int8_t>(midiSetup[ch].reverb);
      channelCtrlVals[ch][93] = static_cast<int8_t>(midiSetup[ch].chorus);
    }
  }

  /* Schedule all song events on the FluidSynth sequencer */
  if (!selectedSong) {
    fmt::print(stderr, "fluidsyX: no song data to play\n");
    return;
  }

  double totalTicks = scheduleSongEvents(selectedSong->m_data.get(),
                                         selectedSong->m_size);
  if (totalTicks <= 0.0) {
    fmt::print(stderr, "fluidsyX: no events to play\n");
    return;
  }

  /* Wait for playback to finish.
   * The sequencer is already dispatching events in real-time via the
   * audio driver thread.  We just monitor progress until the last
   * scheduled tick has been reached, plus a short tail for release
   * envelopes to ring out. */
  fmt::print("fluidsyX: playing song (setup {}, {} ticks) — press Q or Ctrl-C to stop\n",
         setupId, static_cast<unsigned int>(totalTicks));

  enableRawMode();

  unsigned int startTick = fluid_sequencer_get_tick(sequencer.get());
  unsigned int endTick = startTick + static_cast<unsigned int>(totalTicks);
  /* Add a tail in sequencer ticks (at current time-scale rate).
   * Use the current scale to compute ~2 seconds worth of ticks. */
  double curScale = fluid_sequencer_get_time_scale(sequencer.get());
  unsigned int tailTicks = static_cast<unsigned int>(curScale * 2.0);
  unsigned int tailEnd = endTick + tailTicks;

  while (g_running.load()) {
    unsigned int now = fluid_sequencer_get_tick(sequencer.get());

    /* Check for user input (Q to quit, space for panic) */
    int key = readKey();
    if (key >= 0) {
      int ch = tolower(key);
      if (ch == 'q')
        break;
      if (ch == ' ') {
        for (int c = 0; c < 16; ++c)
          fluid_synth_all_notes_off(synth.get(), c);
      }
    }

    /* Print progress (tick-based) */
    unsigned int elapsed = (now > startTick) ? (now - startTick) : 0;
    fmt::print("\r  tick {} / {}  ", elapsed,
           static_cast<unsigned int>(totalTicks));
    fflush(stdout);

    if (now >= tailEnd)
      break;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  /* All notes off */
  for (int c = 0; c < 16; ++c)
    fluid_synth_all_notes_off(synth.get(), c);
  activeMacros.clear();
  pendingSngEvents.clear();
  activeSongGroup = nullptr;
  fmt::print("\n");
}

/* ═══════════════════ SFX playback loop ═══════════════════ */

void FluidsyXApp::sfxLoop(const SFXGroupIndex& index) {
  fmt::print("<space>: keyon/keyoff, <left/right>: cycle SFX, "
         "<up/down>: volume, <Q>: quit\n");

  std::map<SFXId, SFXGroupIndex::SFXEntry> sortEntries(
      index.m_sfxEntries.cbegin(), index.m_sfxEntries.cend());

  auto sfxIt = sortEntries.cbegin();
  int sfxId = -1;
  bool isPlaying = false;

  if (sfxIt != sortEntries.cend()) {
    sfxId = sfxIt->first.id;
    fmt::print("  SFX {}, VOL {}%\n", sfxId,
           static_cast<int>(std::round(volume * 100)));
  }

  enableRawMode();

  while (g_running.load()) {
    int key = readKey();
    if (key < 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    int ch = tolower(key);
    bool updateDisp = false;

    if (ch == 'q')
      break;

    switch (ch) {
    case ' ': {
      if (isPlaying) {
        /* Key off */
        fluid_synth_all_notes_off(synth.get(), 0);
        activeMacros.clear();
        isPlaying = false;
      } else if (sfxIt != sortEntries.cend()) {
        /* Key on */
        const SFXGroupIndex::SFXEntry& entry = sfxIt->second;
        const SoundMacro* macro =
            activePool ? activePool->soundMacro(entry.objId) : nullptr;
        if (macro) {
          unsigned int now = fluid_sequencer_get_tick(sequencer.get());
          enqueueSoundMacro(macro, 0, 0, entry.defKey, entry.defVel, now);
        } else {
          fluid_synth_noteon(synth.get(), 0, entry.defKey, entry.defVel);
        }
        isPlaying = true;
      }
      updateDisp = true;
      break;
    }
    case 27: {
      int next1 = readKey();
      if (next1 == '[') {
        int next2 = readKey();
        switch (next2) {
        case 'D': /* Left – prev SFX */
          if (sfxIt != sortEntries.cbegin()) {
            --sfxIt;
            sfxId = sfxIt->first.id;
            updateDisp = true;
          }
          break;
        case 'C': { /* Right – next SFX */
          auto nextIt = sfxIt;
          ++nextIt;
          if (nextIt != sortEntries.cend()) {
            sfxIt = nextIt;
            sfxId = sfxIt->first.id;
            updateDisp = true;
          }
          break;
        }
        case 'A': /* Up – volume up */
          volume = std::clamp(volume + 0.05f, 0.f, 1.f);
          fluid_synth_set_gain(synth.get(), volume);
          updateDisp = true;
          break;
        case 'B': /* Down – volume down */
          volume = std::clamp(volume - 0.05f, 0.f, 1.f);
          fluid_synth_set_gain(synth.get(), volume);
          updateDisp = true;
          break;
        default:
          break;
        }
      }
      break;
    }
    default:
      break;
    }

    if (updateDisp) {
      fmt::print("\r                                                                "
             "                \r  {} SFX {}, VOL {}%",
             isPlaying ? '>' : ' ', sfxId,
             static_cast<int>(std::round(volume * 100)));
      fflush(stdout);
    }
  }

  fluid_synth_all_notes_off(synth.get(), 0);
  activeMacros.clear();
  fmt::print("\n");
}

/* ═══════════════════ main ═══════════════════ */

int main(int argc, char** argv) {
  signal(SIGINT, signalHandler);
#ifndef _WIN32
  signal(SIGTERM, signalHandler);
#endif

  FluidsyXApp app;

  if (argc < 2) {
    fmt::print(stderr,
            "Usage: fluidsyX [--verbose] [--duration <ticks>] <musyx-group-path> [<songs-file>] [soundfont.sf2]\n"
            "\n"
            "  Plays MusyX SoundMacro data using FluidSynth.\n"
            "  MusyX samples are decoded and loaded as a virtual SoundFont.\n"
            "  An optional songs file (.son/.sng/.song) can be specified to\n"
            "  play a specific MusyX song sequence.\n"
            "  An optional external .sf2 SoundFont can be specified as a\n"
            "  fallback for programs not covered by the MusyX data.\n"
            "\n"
            "Options:\n"
            "  --verbose           Log every SoundMacro command as it executes\n"
            "                      with all introspection fields.  Unimplemented\n"
            "                      commands are always logged regardless.\n"
            "  --duration <ticks>  Total playback duration in SNG ticks.\n"
            "                      When specified, the song loops until the\n"
            "                      given tick count is reached.\n");
    return 1;
  }

  /* Parse positional arguments.
   * argv[1..] may include --verbose and --duration <ticks>.  Remaining
   * positional args: first is the group path, then classified by extension:
   * .sf2 → external SoundFont, anything else → songs file (consistent with
   * amuserender / amuseplay). */
  const char* groupPath = nullptr;
  const char* songsPath = nullptr;
  const char* sf2Path   = nullptr;

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--verbose") == 0) {
      app.verbose = true;
    } else if (strcmp(argv[i], "--duration") == 0) {
      if (i + 1 < argc) {
        const char* arg = argv[++i];
        char* end = nullptr;
        unsigned long val = strtoul(arg, &end, 10);
        if (end == arg || *end != '\0' || val == 0) {
          fmt::print(stderr, "fluidsyX: --duration requires a positive integer (got '{}')\n", arg);
          return 1;
        }
        app.maxDurationTicks = static_cast<uint32_t>(val);
      } else {
        fmt::print(stderr, "fluidsyX: --duration requires a value\n");
        return 1;
      }
    } else if (!groupPath) {
      groupPath = argv[i];
    } else if (fluid_is_soundfont(argv[i])) {
      sf2Path = argv[i];
    } else {
      songsPath = argv[i];
    }
  }

  if (!groupPath) {
    fmt::print(stderr, "fluidsyX: no group path specified\n");
    return 1;
  }

  /* 1. Initialise FluidSynth */
  if (!app.initFluidSynth())
    return 1;

  /* 2. Optionally load a SoundFont for audible output */
  if (sf2Path) {
    int sfId = fluid_synth_sfload(app.synth.get(), sf2Path, /*reset_presets=*/1);
    if (sfId < 0) {
      fmt::print(stderr,
              "fluidsyX: warning: failed to load SoundFont '{}' – "
              "sequencer events will still be processed but may be inaudible\n",
              sf2Path);
    } else {
      fmt::print("fluidsyX: loaded SoundFont '{}' (id={})\n", sf2Path, sfId);
    }
  } else {
    fmt::print("fluidsyX: no external SoundFont specified; MusyX samples will "
           "be used for playback\n\n");
  }

  /* 3. Load MusyX data */
  if (!app.loadMusyXData(groupPath))
    return 1;

  /* 4. Attempt loading song data.
   *    If a separate songs file was provided use that; otherwise try
   *    loading songs embedded in the group path (same behaviour as
   *    amuserender / amuseplay). */
  std::vector<std::pair<std::string, ContainerRegistry::SongData>> songs;
  if (songsPath)
    songs = ContainerRegistry::LoadSongs(songsPath);
  else
    songs = ContainerRegistry::LoadSongs(groupPath);

  if (!songs.empty()) {
    bool play = true;
    if (songs.size() > 1) {
      fmt::print("Multiple Songs discovered:\n");
      int idx = 0;
      for (const auto& pair : songs) {
        fmt::print("    {} {} (Group {}, Setup {})\n", idx++,
               pair.first, pair.second.m_groupId,
               pair.second.m_setupId);
      }
      int userSel = 0;
      fmt::print("Enter Song Number: ");
      if (scanf("%d", &userSel) <= 0 ||
          userSel < 0 ||
          userSel >= static_cast<int>(songs.size())) {
        fmt::print(stderr, "fluidsyX: invalid song selection\n");
        app.shutdownFluidSynth();
        return 1;
      }
      while (getchar() != '\n')
        ;
      app.groupId = songs[userSel].second.m_groupId;
      app.setupId = songs[userSel].second.m_setupId;
      app.selectedSong = &songs[userSel].second;
    } else {
      /* Ask Y/N for single song */
      fmt::print("Play Song '{}'? (Y/N): ", songs[0].first);
      char yn = 0;
      if (scanf(" %c", &yn) > 0 && tolower(yn) == 'y') {
        app.groupId = songs[0].second.m_groupId;
        app.setupId = songs[0].second.m_setupId;
        app.selectedSong = &songs[0].second;
      } else {
        play = false;
      }
      while (getchar() != '\n')
        ;
    }
    (void)play;
  }

  /* 5. Resolve group via setup if needed */
  if (app.groupId == -1 && app.setupId != -1) {
    for (const auto& pair : app.allSongGroups) {
      for (const auto& setup : pair.second.second->m_midiSetups) {
        if (setup.first == app.setupId) {
          app.groupId = pair.first.id;
          break;
        }
      }
      if (app.groupId != -1)
        break;
    }
  }

  /* 6. Select group interactively if not yet determined */
  if (int err = app.selectGroup()) {
    app.shutdownFluidSynth();
    return err;
  }

  /* 7. Find the pool for the active group */
  app.activePool = app.findPoolForGroup();
  if (!app.activePool) {
    fmt::print(stderr, "fluidsyX: unable to find pool for group {}\n",
            app.groupId);
    app.shutdownFluidSynth();
    return 1;
  }

  fmt::print("fluidsyX: group {} selected ({}), {} SoundMacros in pool\n",
         app.groupId, app.sfxGroup ? "SFX" : "Song",
         app.activePool->soundMacros().size());

  /* 7b. Build the MusyX virtual SoundFont from decoded samples.
   *     This gives FluidSynth access to the original MusyX sounds. */
  if (!app.buildMusyXSoundFont()) {
    fmt::print(stderr, "fluidsyX: warning: could not build MusyX SoundFont; "
            "playback will use external SoundFont if available\n");
  }

  /* 8. Enter playback loop */
  if (app.sfxGroup) {
    auto sfxIt = app.allSFXGroups.find(app.groupId);
    if (sfxIt != app.allSFXGroups.end())
      app.sfxLoop(*sfxIt->second.second);
  } else {
    auto songIt = app.allSongGroups.find(app.groupId);
    if (songIt != app.allSongGroups.end())
      app.songLoop(*songIt->second.second);
  }

  /* 9. Cleanup */
  disableRawMode();
  app.shutdownFluidSynth();
  fmt::print("fluidsyX: goodbye\n");
  return 0;
}
