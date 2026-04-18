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
#include "amuse/FluidsyXMacroContext.hpp"

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
#include <optional>

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

// Initial MIDI CC values for "cold" start
static constexpr std::array<uint8_t, 134> inpColdMIDIDefaults = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x00, 0x00, 0x40, 0x7F, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x7F, 0x7F, 0x7F, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00,
};

// Initial MIDI CC values for "warm" reset
// 0xFF means unchanged
static constexpr std::array<uint8_t, 134> inpWarmMIDIDefaults = {
    0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x40, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

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
static constexpr float kFixedPoint16_16Divisor = 1.0f / 65536.0f;

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

/* ── Collected SNG event for scheduling ── */

struct SngEvent {
  enum Type { NoteOn, NoteOff, CC, Program, PitchBend, Tempo, WarmReset, AllNotesOff };
  Type     type;
  uint32_t absTick;
  uint8_t  channel;
  uint16_t data1;   /* note / ctrl / program  (uint16 to hold MusyX extended CC 128-133) */
  uint8_t  data2;   /* velocity / value */
  int      pitchBend14; /* 14-bit pitch bend (only for PitchBend type) */
  double   tempoScale;  /* ticks/sec (only for Tempo type) */
  int      trackIdx;    /* originating track index (0..63), -1 for global (e.g. Tempo) */
};

/** Per-track loop information.
 *  In the original amuse, each track independently detects its loop marker
 *  (regionIndex == -2) and loops back to its own loopStartTick.  Tracks with
 *  shorter loops repeat more often than tracks with longer loops. */
struct SngTrackLoop {
  int      trackIdx     = -1;
  uint8_t  channel      = 0;
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
                           std::vector<SngTrackLoop>& outTrackLoops) {
  SngHeader hdr = *reinterpret_cast<const SngHeader*>(sngData);
  if (bigEndian)
    hdr.swapFromBig();

  uint32_t initBpm = hdr.initialTempo & 0x7fffffffu;
  outInitialScale = bpmToScale(initBpm);
  outTrackLoops.clear();

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
      ev.trackIdx = -1; /* global event */
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
                             region->progNum, 0, 0, 0.0, i});
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
          auto delta = DecodeDelta(pdata);
          pitchTick += delta.first;
          pitchVal  += delta.second;
          int bend14 = std::clamp(pitchVal + kPitchBendCenter, 0, kPitchBendMax);
          outEvents.push_back({SngEvent::PitchBend, regStart + pitchTick,
                               midiChan, 0, 0, bend14, 0.0, i});
        }
      }

      /* --- Continuous modulation data --- */
      if (thdr.modOff != 0) {
        const unsigned char* mdata = sngData + thdr.modOff;
        int32_t modVal = 0;
        uint32_t modTick = 0;
        while (mdata[0] != 0x80 || mdata[1] != 0x00) {
          auto delta = DecodeDelta(mdata);
          modTick += delta.first;
          modVal  += delta.second;
          uint8_t ccVal = static_cast<uint8_t>(
              std::clamp(modVal / kSngModToCCDivisor, 0, 127));
          outEvents.push_back({SngEvent::CC, regStart + modTick,
                               midiChan, 1 /*mod wheel*/, ccVal, 0, 0.0, i});
        }
      }

      /* --- Note / CC / Program commands --- */
      if (sngVersion == 1) {
        /* Revision format */
        int32_t waitCountdown = static_cast<int32_t>(DecodeTime(data));
        while (true) {
          if (*reinterpret_cast<const uint16_t*>(data) == 0xffff)
            break;

          uint32_t evTick = regStart + static_cast<uint32_t>(waitCountdown);

          if ((data[0] & 0x80) != 0) {
            /* Special event: key byte has bit 7 set.
             * Match the original MusyX SDK HandleEvent (seq.c) encoding:
             *   velocity == 0:       Program change  (prog = key & 0x7f)
             *   velocity == 1:       CC 0x82 (130)   (val  = key & 0x7f)
             *   velocity & 0x80:     CC              (ctrl = velocity & 0x7f, val = key & 0x7f)
             *   velocity 2..127:     ignored / internal meta-command
             */
            uint8_t key = data[0];
            uint8_t vel = data[1];
            if (vel == 0) {
              /* Program change */
              uint8_t prog = key & 0x7f;
              outEvents.push_back({SngEvent::Program, evTick, midiChan,
                                   prog, 0, 0, 0.0, i});
            } else if (vel == 1) {
              /* MusyX extended CC 0x82 (130) */
              uint8_t val = key & 0x7f;
              outEvents.push_back({SngEvent::CC, evTick, midiChan,
                                   static_cast<uint16_t>(0x82), val, 0, 0.0, i});
            }
            else
            {
              switch (vel & 0x7f) {
              case 0x68:
                // Cross-fade command - ignore
                break;
              case 0x69:
                // priority command - ignore
                // seqMIDIPriority[curSeqId][midi] = key & 0x7f;
                break;
              case 0x6a:
                // priority command - ignore
                // seqMIDIPriority[curSeqId][midi] = (key & 0x7f) + 0x80;
                break;
              case 0x79:
                // calls inpResetMidiCtrl(midi, curSeqId, FALSE) and effectively repopulates all CCs with inpWarmMIDIDefaults
                // finally invalidates the last Note with inpSetMidiLastNote(ch, set, 0xFF);
                outEvents.push_back({SngEvent::WarmReset, evTick, midiChan, 0, 0, 0, 0.0, i});
                break;
              case 0x7b:
                // calls KeyOffNotes(), i.e. effectively all notes off
                outEvents.push_back({SngEvent::AllNotesOff, evTick, midiChan, 0, 0, 0, 0.0, i});
                break;
              default:
              {
                /* Standard CC: ctrl = vel & 0x7f, val = key & 0x7f */
                uint16_t ctrl = vel & 0x7f;
                uint8_t val  = key & 0x7f;
                outEvents.push_back({SngEvent::CC, evTick, midiChan, ctrl, val, 0, 0.0, i});
              }
                break;
              }
            }
            data += 2;
          } else {
            /* Note */
            uint8_t note = data[0] & 0x7f;
            uint8_t vel  = data[1] & 0x7f;
            uint16_t length = bigEndian
                ? SBig(*reinterpret_cast<const uint16_t*>(data + 2))
                : *reinterpret_cast<const uint16_t*>(data + 2);
            outEvents.push_back({SngEvent::NoteOn, evTick, midiChan,
                                 note, vel, 0, 0.0, i});
            if (length == 0) {
              outEvents.push_back({SngEvent::NoteOff, evTick, midiChan,
                                   note, 0, 0, 0.0, i});
            } else {
              outEvents.push_back({SngEvent::NoteOff,
                                   evTick + length, midiChan,
                                   note, 0, 0, 0.0, i});
            }
            data += 4;
          }
          waitCountdown += static_cast<int32_t>(DecodeTime(data));
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

          if ((data[2] & 0x80) == 0) {
            /* Note: key byte has bit 7 clear */
            uint16_t length = bigEndian
                ? SBig(*reinterpret_cast<const uint16_t*>(data))
                : *reinterpret_cast<const uint16_t*>(data);
            uint8_t note = data[2] & 0x7f;
            uint8_t vel  = data[3] & 0x7f;
            outEvents.push_back({SngEvent::NoteOn, evTick, midiChan,
                                 note, vel, 0, 0.0, i});
            if (length == 0) {
              outEvents.push_back({SngEvent::NoteOff, evTick, midiChan,
                                   note, 0, 0, 0.0, i});
            } else {
              outEvents.push_back({SngEvent::NoteOff,
                                   evTick + length, midiChan,
                                   note, 0, 0, 0.0, i});
            }
          } else {
            /* Special event: key byte has bit 7 set.
             * Match the original MusyX SDK HandleEvent (seq.c) encoding:
             *   velocity == 0:       Program change  (prog = key & 0x7f)
             *   velocity == 1:       CC 0x82 (130)   (val  = key & 0x7f)
             *   velocity & 0x80:     CC              (ctrl = velocity & 0x7f, val = key & 0x7f)
             *   velocity 2..127:     ignored / internal meta-command
             */
            uint8_t key = data[2];
            uint8_t vel = data[3];
            if (vel == 0) {
              /* Program change */
              uint8_t prog = key & 0x7f;
              outEvents.push_back({SngEvent::Program, evTick, midiChan,
                                   prog, 0, 0, 0.0, i});
            } else if (vel == 1) {
              /* MusyX extended CC 0x82 (130) */
              uint8_t val = key & 0x7f;
              outEvents.push_back({SngEvent::CC, evTick, midiChan,
                                   static_cast<uint16_t>(0x82), val, 0, 0.0, i});
            } else if ((vel & 0x80) != 0) {
              /* Standard CC: ctrl = vel & 0x7f, val = key & 0x7f */
              uint16_t ctrl = vel & 0x7f;
              uint8_t val  = key & 0x7f;
              outEvents.push_back({SngEvent::CC, evTick, midiChan, ctrl, val, 0, 0.0, i});
            }
            /* else: velocity 2..127 → ignored (internal MusyX meta-commands) */
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
     * Each track independently detects its loop and gets its own loop range,
     * mirroring amuse's per-track SongState::Track::advance() loop handling.
     *
     * IMPORTANT: loopStartTick is taken from the SNG header's loopStartTicks[]
     * (per-channel if bit31 of initialTempo is set, otherwise global slot 0),
     * matching amuse's Track constructor.  The loopTo region index only
     * determines WHERE the data pointer resets to, NOT the tick offset. */
    {
      int16_t termIdx = bigEndian ? SBig(nextRegion->regionIndex) : nextRegion->regionIndex;
      if (termIdx == -2) {
        uint32_t loopEndTick = bigEndian ? SBig(nextRegion->startTick) : nextRegion->startTick;
        /* Read loopStartTick from header, exactly as amuse does in
         * SongState::initialize() → Track(…, loopStart, …) */
        uint32_t loopStartTick;
        if ((hdr.initialTempo & 0x80000000u) != 0u) {
          loopStartTick = hdr.loopStartTicks[midiChan];
        } else {
          loopStartTick = hdr.loopStartTicks[0];
        }
        if (loopEndTick > loopStartTick) {
          outTrackLoops.push_back({i, midiChan, loopStartTick, loopEndTick});
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

  FluidModPtr         modBlueprintADSR{nullptr, &delete_fluid_mod};
  FluidModPtr         modBlueprintVelToAttack{nullptr, &delete_fluid_mod};

  static constexpr auto attackDecayReleaseModCallback = [](const fluid_mod_t* mod, int value, int range, int is_src1, void* data)
  {
      fluid_voice_t* voice = static_cast<fluid_voice_t*>(data);

      // SetAdsrCtrl overrides the attack/decay/release times, but fluidsynth's modulator adds them to the destination generator.
      // Work this around by subtracting the current value of the destination generator from the modulator output, effectively treating the current value as a baseline.
      double offsetToCompensate = fluid_voice_gen_get(voice, fluid_mod_get_dest(mod));
      return secondsToTimecents(MIDItoTIME[std::clamp(value,  0, 103)] / 1000.0) - offsetToCompensate;
  };

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
    enum Type { NoteOn, NoteOff, ProgramChange, CC, WarmReset };
    Type     type;
    uint8_t  channel;
    uint16_t note;     /* also: CC number for Type::CC  (uint16 for MusyX extended CC) */
    uint8_t  velocity; /* also: CC value for Type::CC */
  };
  std::vector<PendingSngNoteEvent> pendingSngEvents;

  /* ── lifecycle helpers ── */

  [[nodiscard]] bool initFluidSynth();
  void initCCDefault(const std::array<uint8_t, 134>& defaults);
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

  /** Execute an event trap: redirect macro execution to the trap target.
   *  If the trap's macroId matches the current macro, just jump to the step.
   *  If it's a different macro, replace the current macro entirely.
   *  Returns true if the trap was executed, false if the macro couldn't be found. */
  bool executeTrap(MacroExecContext& ctx, MacroExecContext::EventTrap& trap,
                   unsigned int curTick);

  void killVoice(fluid_synth_t* synth, MacroExecContext& ctx);
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
  if (ctx.curDetune != 0)
    fluid_voice_gen_set(voice, GEN_FINETUNE, static_cast<float>(ctx.curDetune));
  /* Replay any commands that were executed before the voice was created.
   * Save/restore pc to prevent pending commands from advancing the program counter. */
  if (!ctx.pendingCmds.empty()) {
      int savedPc = ctx.pc;
      for (auto* pendingCmd : ctx.pendingCmds) {
          pendingCmd->DoFluid(ctx, voice);
      }
      ctx.pc = savedPc;
      ctx.pendingCmds.clear();
  }
  if (ctx.adsrTableId.has_value()) {
    /* Convert MusyX linear sustain factor (0x1000 == 100%) to SF2 centibels
     * of attenuation (0 cB = full level, 1440 cB = silence).
     * Uses the same linear mapping as the ADSR CC modulator. */
    auto sustainToCentibels = [](double factor) -> float {
      return static_cast<float>(std::clamp((1.0 - factor) * 1440.0, 0.0, 1440.0));
    };

    // | Field                       | MusyX Format                   | SF2 Expected                       | Conversion                          |
    // |-----------------------------|--------------------------------|------------------------------------|--------------------------------------|
    // | Attack/Decay/Release (ADSR) | uint16_t milliseconds            | timecents                        | `secondsToTimecents(ms / 1000.0)`   |
    // | Attack/Decay (ADSRDLS)      | uint32_t 16.16 fixed-point timecents | timecents                    | `int32_t(val) / 65536.0`            |
    // | Release (ADSRDLS)           | uint16_t milliseconds            | timecents                        | `secondsToTimecents(ms / 1000.0)`   |
    // | Sustain (both)              | uint16_t linear (0x1000=100%)    | centibels (0=full, 1440=silence) | `(1 - factor) * 1440`               |
    // | keyToDecay                  | uint32_t 16.16 fixed-point timecents | plain timecents/key          | `int32_t(val) / 65536.0`            |
    if (std::get<1>(*ctx.adsrTableId)) {
      /* ── DLS mode ── */
      const ADSRDLS* adsr = app->activePool->tableAsAdsrDLS(std::get<0>(*ctx.adsrTableId));
      if (adsr) {
        /* Attack/Decay: stored as 16.16 fixed-point time-cents;
         * FluidSynth expects plain timecents.  0x80000000 = default/disabled. */
        if (adsr->attack != 0x80000000) {
          fluid_voice_gen_set(voice, GEN_VOLENVATTACK,
                              static_cast<float>(static_cast<int32_t>(adsr->attack) * kFixedPoint16_16Divisor));
          fluid_voice_update_param(voice, GEN_VOLENVATTACK);
        }
        if (adsr->decay != 0x80000000) {
          fluid_voice_gen_set(voice, GEN_VOLENVDECAY,
                              static_cast<float>(static_cast<int32_t>(adsr->decay) * kFixedPoint16_16Divisor));
          fluid_voice_update_param(voice, GEN_VOLENVDECAY);
        }
        /* Key-to-decay: 16.16 fixed-point timecents-per-key */
        if (adsr->keyToDecay != 0x80000000) {
          float keynumToVolEnvDecay = static_cast<float>(static_cast<int32_t>(adsr->keyToDecay) * (-1 / (128 * 65536.0)));
          fluid_voice_gen_set(voice, GEN_KEYTOVOLENVDECAY, keynumToVolEnvDecay);
          fluid_voice_update_param(voice, GEN_KEYTOVOLENVDECAY);

          // musyx adds (key * (dscale / 8388608)) to decay time (anchored at key=0).
          // SF2 adds ((60 - key) * keynumToVolEnvDecay) (anchored at key=60).
          // Compensate the SF2 base decay for the anchor-point mismatch.
          // SF2 applies:   total = GEN_VOLENVDECAY + (60 - key) * keynumToVolEnvDecay
          // musyx applies: total = base_dtime_TC + key * (dscale / 8388608)
          // Equating the key-independent parts:
          // GEN_VOLENVDECAY = base_dtime_TC - 60 * keynumToVolEnvDecay
          // Where base_dtime_TC = adsr->decay / 65536
          auto decayTc = fluid_voice_gen_get(voice, GEN_VOLENVDECAY);
          auto compensateDecay = std::round(decayTc - 60 * keynumToVolEnvDecay);
          fluid_voice_gen_set(voice, GEN_VOLENVDECAY, compensateDecay);
          fluid_voice_update_param(voice, GEN_VOLENVDECAY);
        }

        /* Sustain: 0x1000 == 100% linear → centibels */
        fluid_voice_gen_set(voice, GEN_VOLENVSUSTAIN,
                            sustainToCentibels(adsr->getSustain()));
        fluid_voice_update_param(voice, GEN_VOLENVSUSTAIN);
        /* Release: stored in milliseconds → convert to timecents */
        fluid_voice_gen_set(voice, GEN_VOLENVRELEASE,
                            static_cast<float>(secondsToTimecents(adsr->getRelease())));
        fluid_voice_update_param(voice, GEN_VOLENVRELEASE);

        /* Velocity-to-attack: add a modulator so that velocity scales
         * the attack time in timecent space, mirroring the MusyX
         * formula: attack_tc += (vel/128) * velToAttack */
        // musyx adds vel * velToAttack / (128 * 65536) TC, starting from 0 at vel=0.
        // SF2 applies (vel / 128) * modulator_amount TC, also starting from 0 at vel=0.
        // Both are anchored at velocity 0, so the base attackVolEnv generator is unaffected and needs no offset.
        if (adsr->velToAttack != 0x80000000 && app->modBlueprintVelToAttack) {
          fluid_mod_set_amount(app->modBlueprintVelToAttack.get(),
                               static_cast<int32_t>(adsr->velToAttack) * kFixedPoint16_16Divisor);
          fluid_voice_add_mod(voice, app->modBlueprintVelToAttack.get(), FLUID_VOICE_OVERWRITE);
        }
      }
    } else {
      /* ── Standard ADSR mode ── */
      const ADSR* adsr = app->activePool->tableAsAdsr(std::get<0>(*ctx.adsrTableId));
      if (adsr) {
        /* Attack/Decay/Release: stored in milliseconds → convert to SF2 timecents */
        fluid_voice_gen_set(voice, GEN_VOLENVATTACK,
                            static_cast<float>(secondsToTimecents(adsr->getAttack())));
        fluid_voice_update_param(voice, GEN_VOLENVATTACK);
        fluid_voice_gen_set(voice, GEN_VOLENVDECAY,
                            static_cast<float>(secondsToTimecents(adsr->getDecay())));
        fluid_voice_update_param(voice, GEN_VOLENVDECAY);
        /* Sustain: 0x1000 == 100% linear → centibels */
        fluid_voice_gen_set(voice, GEN_VOLENVSUSTAIN,
                            sustainToCentibels(adsr->getSustain()));
        fluid_voice_update_param(voice, GEN_VOLENVSUSTAIN);
        fluid_voice_gen_set(voice, GEN_VOLENVRELEASE,
                            static_cast<float>(secondsToTimecents(adsr->getRelease())));
        fluid_voice_update_param(voice, GEN_VOLENVRELEASE);
      }
    }
    ctx.adsrTableId = std::nullopt; // Clear pending ADSR table since it's now applied
  }

  app->applyAdsrToVoice(voice, ctx, false);

  fluid_synth_start_voice(synth, voice);
  return FLUID_OK;
}

static void dummy_preset_free(fluid_preset_t* preset) {
  delete_fluid_preset(preset);
}

/* ═══════════════════ FluidSynth init / shutdown ═══════════════════ */

bool FluidsyXApp::initFluidSynth() {
    modBlueprintADSR.reset(new_fluid_mod());

    /* VelToAttack modulator: velocity (GC) → GEN_VOLENVATTACK.
     * Amount is set per-voice to velToAttack / 65536.0 (plain timecents). */
    modBlueprintVelToAttack.reset(new_fluid_mod());
    fluid_mod_set_source1(modBlueprintVelToAttack.get(), FLUID_MOD_VELOCITY,
                          FLUID_MOD_GC | FLUID_MOD_LINEAR | FLUID_MOD_UNIPOLAR | FLUID_MOD_POSITIVE);
    fluid_mod_set_source2(modBlueprintVelToAttack.get(), FLUID_MOD_NONE, 0);
    fluid_mod_set_dest(modBlueprintVelToAttack.get(), GEN_VOLENVATTACK);
    fluid_mod_set_amount(modBlueprintVelToAttack.get(), 1); /* overridden per voice */

  settings.reset(new_fluid_settings());
  if (!settings) {
    fmt::print(stderr, "fluidsyX: failed to create FluidSynth settings\n");
    return false;
  }

  // Processing soundMacros via callback can be too expensive for the default period-size of 64 samples
  fluid_settings_setint(settings.get(), "audio.period-size", 256);
  fluid_settings_setint(settings.get(), "synth.verbose", 0);
  fluid_settings_setnum(settings.get(), "synth.gain", 0.9);
  fluid_settings_setnum(settings.get(), "synth.reverb.level", 0.8);
  fluid_settings_setnum(settings.get(), "synth.reverb.room-size", 0.7);
  fluid_settings_setnum(settings.get(), "synth.reverb.width", 1);
  fluid_settings_setnum(settings.get(), "synth.reverb.damp", 0);
  // Use FluidSynth's linear portamento mode via the portamento-time setting.
  fluid_settings_setstr(settings.get(), "synth.portamento-time", "linear");

  synth.reset(new_fluid_synth(settings.get()));
  if (!synth) {
    fmt::print(stderr, "fluidsyX: failed to create FluidSynth synthesizer\n");
    return false;
  }
  fluid_synth_set_channel_type(synth.get(), 9, CHANNEL_TYPE_MELODIC);
  fluid_synth_bank_select(synth.get(), 9, 0); // force drum channel to bank 0 rather than bank 128

  // Remove fluidsynth's non-standard MIDI CC 8 for stereo balance.
  // We don't have stereo samples and CC8 might interfere with SoundMacros that use it for other purposes
  fluid_mod_t* balanceMod = static_cast<fluid_mod_t*>(alloca(fluid_mod_sizeof()));
  fluid_mod_set_source1(balanceMod, 8,
                        FLUID_MOD_CC
                        | FLUID_MOD_CONCAVE
                        | FLUID_MOD_BIPOLAR
                        | FLUID_MOD_POSITIVE
                        );
  fluid_mod_set_source2(balanceMod, 0, 0);
  fluid_mod_set_dest(balanceMod, GEN_CUSTOM_BALANCE);
  fluid_mod_set_amount(balanceMod, 960); // amount doesn't matter for fluid_synth_remove_default_mod()
  fluid_synth_remove_default_mod(synth.get(), balanceMod);

  /* Create sequencer and use the sample-timer */
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
    modBlueprintADSR.reset();
    modBlueprintVelToAttack.reset();
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
  ctx.voiceId = 0;
}

/** Immediately silence a voice (near-instant release).
 *  Sets the volume envelope release to the minimum timecent value before
 *  calling fluid_synth_stop(), making the release effectively instant. */
void FluidsyXApp::killVoice(fluid_synth_t* synth, MacroExecContext& ctx) {
  if (ctx.voiceId != 0) {
    fluid_voice_t* v = findVoiceById(synth, ctx.voiceId);
    if (v) {
      // unset any release-related mods so they don't interfere with the instant release time
      if(ctx.useAdsrControllers)
      {
        fluid_mod_set_source1(modBlueprintADSR.get(), ctx.midiRelease, fluid_mod_get_flags1(modBlueprintADSR.get()));
        fluid_mod_set_dest(modBlueprintADSR.get(), GEN_VOLENVRELEASE);
        fluid_mod_set_amount(modBlueprintADSR.get(), 0);
        fluid_voice_add_mod(v, modBlueprintADSR.get(), FLUID_VOICE_OVERWRITE);
        // restore the amount for the blueprint mod so it can be reused for upcoming release mods
        fluid_mod_set_amount(modBlueprintADSR.get(), 1);
      }
      /* Set release to near-instant */
      fluid_voice_gen_set(v, GEN_VOLENVRELEASE, kInstantReleaseTimecents);
      fluid_voice_update_param(v, GEN_VOLENVRELEASE);
    }
    fluid_synth_stop(synth, ctx.voiceId);
  }
  ctx.voiceId = 0;
}

bool FluidsyXApp::executeTrap(MacroExecContext& ctx,
                              MacroExecContext::EventTrap& trap,
                              unsigned int curTick) {
  if (!trap.isSet())
    return false;

  /* Look up the target SoundMacro by ID in the active pool.
   * In amuse's Voice::keyOff(), if the trap's macroId matches the current
   * macro, it just jumps to the step (setPC).  Otherwise it loads a new
   * macro entirely (loadMacroObject). */
  const SoundMacro* targetMacro = nullptr;

  /* Check if the trap targets the CURRENT macro (same ID → just jump to step).
   * We compare by ID: look up the pool's macro ID → SoundMacro* mapping. */
  if (activePool) {
    SoundMacroId smId;
    smId.id = trap.macroId;
    auto it = activePool->soundMacros().find(smId);
    if (it != activePool->soundMacros().end())
      targetMacro = it->second.get();
  }

  if (!targetMacro) {
    if (verbose)
      fmt::print(stderr, "fluidsyX: trap target macroId {} not found in pool\n",
                 trap.macroId);
    return false;
  }

  if (targetMacro != ctx.macro) {
    /* Different macro — replace the current macro. */
    ctx.macro = targetMacro;
  }
  /* Jump to the target step (same or different macro). */
  int step = static_cast<int>(trap.macroStep);
  if (step >= 0 && step < static_cast<int>(ctx.macro->m_cmds.size()))
    ctx.pc = step;
  else
    ctx.pc = 0;

  /* Clear wait state so macro processing resumes immediately. */
  ctx.ended = false;
  ctx.inIndefiniteWait = false;
  ctx.waitingKeyoff = false;
  ctx.waitingSampleEnd = false;

  if (verbose)
    fmt::print("fluidsyX: [ch{}] trap fired → macro {} step {}\n",
               ctx.channel, trap.macroId, trap.macroStep);

  /* Clear the trap after firing (one-shot, matches musyx ExecuteTrap which
   * sets trapEventAddr[trapType] = NULL on fire). */
  trap.clear();

  return true;
}

void FluidsyXApp::applyAdsrToVoice(fluid_voice_t* v, MacroExecContext& ctx,
                                    bool started) {
  if (!ctx.useAdsrControllers || !v)
    return;

#if FLUID_VERSION_AT_LEAST(2,5,0)
  fluid_mod_set_source1(modBlueprintADSR.get(), FLUID_MOD_NONE,
                        FLUID_MOD_CC | FLUID_MOD_CUSTOM | FLUID_MOD_UNIPOLAR | FLUID_MOD_POSITIVE);
  fluid_mod_set_amount(modBlueprintADSR.get(), 1);
  fluid_mod_set_custom_mapping(modBlueprintADSR.get(), attackDecayReleaseModCallback, v);

  /* Decay */
  fluid_mod_set_source1(modBlueprintADSR.get(), ctx.midiDecay, fluid_mod_get_flags1(modBlueprintADSR.get()));
  // Sustain should influcene decay time like so, but this causes incorrect sound.
  //fluid_mod_set_source2(modBlueprintADSR.get(), ctx.midiSustain, FLUID_MOD_CC | FLUID_MOD_UNIPOLAR | FLUID_MOD_NEGATIVE | FLUID_MOD_LINEAR);
  fluid_mod_set_dest(modBlueprintADSR.get(), GEN_VOLENVDECAY);
  fluid_voice_add_mod(v, modBlueprintADSR.get(), FLUID_VOICE_OVERWRITE);

  /* Attack */
  fluid_mod_set_source1(modBlueprintADSR.get(), ctx.midiAttack, fluid_mod_get_flags1(modBlueprintADSR.get()));
  fluid_mod_set_source2(modBlueprintADSR.get(), FLUID_MOD_NONE, 0);
  fluid_mod_set_dest(modBlueprintADSR.get(), GEN_VOLENVATTACK);
  fluid_voice_add_mod(v, modBlueprintADSR.get(), FLUID_VOICE_OVERWRITE);

  /* Release */
  fluid_mod_set_source1(modBlueprintADSR.get(), ctx.midiRelease, fluid_mod_get_flags1(modBlueprintADSR.get()));
  fluid_mod_set_dest(modBlueprintADSR.get(), GEN_VOLENVRELEASE);
  fluid_voice_add_mod(v, modBlueprintADSR.get(), FLUID_VOICE_OVERWRITE);

  /* Sustain – SF2: 0 cB = max volume, 1440 cB = silence */
  fluid_mod_set_source1(modBlueprintADSR.get(), ctx.midiSustain,
                          FLUID_MOD_CC | FLUID_MOD_UNIPOLAR | FLUID_MOD_NEGATIVE | FLUID_MOD_CONCAVE);
  fluid_mod_set_dest(modBlueprintADSR.get(), GEN_VOLENVSUSTAIN);
  fluid_mod_set_amount(modBlueprintADSR.get(), 1440);
  fluid_voice_add_mod(v, modBlueprintADSR.get(), FLUID_VOICE_OVERWRITE);

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

/* ═══════════════════ SoundMacro DoFluid overrides ═══════════════════
 * Each override extracts its logic from the former processMacroCmd() switch.
 * Methods that need app-level state cast ctx.appData to FluidsyXApp*.
 * Methods that need a voice but don't have one yet push `this` to
 * ctx.pendingCmds so they can be replayed once the voice is created. */

unsigned int SoundMacro::CmdEnd::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  ctx.ended = true;
  return 0;
}

unsigned int SoundMacro::CmdStop::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  ctx.ended = true;
  return 0;
}

// Does the wait time precede the conditions, or can conditions wake up early?
// Conditions can wake up early — time and conditions are ORed. The wakeup paths are fully independent:
//     Timer expiry (macHandle, line 1682): on every audio tick, all voices in the time queue whose wait ≤ macRealTime are moved to the active list. This happens regardless of keyoff/sampleend state.
//     Keyoff (macSetExternalKeyoff, line 1713): when a keyoff arrives, if the "wake on keyoff" flag (cFlags & 4) is set, macMakeActive is called immediately. This calls UnYieldMacro, which removes the voice from the time queue, resets wait = 0, and clears both 0x40000 and 0x4 flags. The voice resumes before its timer fires.
//     Sampleend (macSampleEndNotify, line 1701): exactly the same mechanism via macMakeActive when the DSP signals sample completion.
// So whichever event (timer, keyoff, sampleend) happens first wakes the macro. The other pending conditions are simply cancelled when UnYieldMacro clears the flags.
unsigned int SoundMacro::CmdWaitTicks::DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const {
  auto *app = static_cast<FluidsyXApp*>(ctx.appData);
  unsigned int delay = 0;
  // Step 0 – Zero time → skip entirely (synthmacros.c:75) ms = (u16)(cstep->para[1] >> 0x10)
  if(ticksOrMs != 0)
  {
    // Step 1 – Keyoff pre-check (synthmacros.c:76-86): If the Keyoff flag is set and if a keyoff has already been received and if the sustain pedal is not held: return 0
    if(v)
    {
        if (ctx.waitingKeyoff && ctx.keyoffReceived && !fluid_voice_is_sustained(v) && !fluid_voice_is_sostenuto(v)) {
        ctx.waitingKeyoff = false;
        return 0;
      }
    }
    // If the Keyoff flag is not set: the "wake on keyoff" flag is cleared (cFlags &= ~4).
    ctx.waitingKeyoff = keyOff;

    // Step 2 – Sampleend pre-check (synthmacros.c:88-95)
    if (ctx.waitingSampleEnd && ctx.sampleEndReceived) {
      ctx.waitingKeyoff = false;
      ctx.waitingSampleEnd = false;
      return 0;
    }
    // If the Sampleend flag is not set: the "wake on sampleend" flag is cleared (cFlags &= ~0x40000).
    ctx.waitingSampleEnd = sampleEnd;

    uint16_t ticks = ticksOrMs;
    // Step 3 – Random (synthmacros.c:97–99)
    if(random)
    {
      std::uniform_int_distribution<int> dist(0, ticks-1);
      ticks = dist(app->rng);
    }
    // Step 4 – Endless wait (synthmacros.c:101,126–128)
    else if (ticks == std::numeric_limits<uint16_t>::max()) {
      ctx.inIndefiniteWait = true;
    }
    // Step 5 – Deadline calculation (synthmacros.c:101–120)
    if(absolute)
    {
      if (msSwitch) {
        unsigned int now = fluid_sequencer_get_tick(app->sequencer.get());
        unsigned int scheduledTicks = ctx.macStartTime + ticks;
        int ticksUntil = static_cast<int>(scheduledTicks) - static_cast<int>(now);
        delay = static_cast<unsigned int>(std::max(0, ticksUntil));
      } else {
        // Note: technically, this is not correct. It should be svoice->waitTime + ms
        // i.e. relative to the end of the previous wait
        delay = static_cast<unsigned int>(ticks * 1000.0 / ctx.ticksPerSec);
      }
    }
    else
    {
      if (msSwitch) {
        delay = ticks;
      } else {
        delay = static_cast<unsigned int>(ticks * 1000.0 / ctx.ticksPerSec);
      }
    }

    // Step 6 – Already-elapsed deadline check (lines 122–125) After computing svoice->wait, if the deadline is not in the future (!(svoice->wait > macRealTime)): the wait is cancelled (svoice->wait = 0, svoice->waitTime = svoice->wait). The voice is not suspended; execution continues to the next instruction.

    // Step 7 – Suspension (lines 130–136) If svoice->wait != 0 (a real wait was established):

    // For finite waits: the voice is inserted into the sorted macTimeQueueRoot linked list by deadline.
    // For endless waits (wait == -1): it is skipped from the time queue entirely.
    // macMakeInactive(svoice, MAC_STATE_YIELDED) suspends execution. Returns 1.

  }
  ctx.pc++;
  return delay;
}

unsigned int SoundMacro::CmdWaitMs::DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const {
  SoundMacro::CmdWaitTicks waitCmd;
  waitCmd.keyOff = this->keyOff;
  waitCmd.random = this->random;
  waitCmd.sampleEnd = this->sampleEnd;
  waitCmd.absolute = this->absolute;
  waitCmd.msSwitch = true;
  waitCmd.ticksOrMs = this->ms;
  return waitCmd.DoFluid(ctx, v);
}

unsigned int SoundMacro::CmdGoto::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  auto* app = static_cast<FluidsyXApp*>(ctx.appData);
  if (macro.id.id != 0xffff && macro.id.id != 0) {
    const SoundMacro* target = app->activePool ? app->activePool->soundMacro(macro.id) : nullptr;
    if (target) {
      ctx.macro = target;
      ctx.pc = macroStep.step;
    } else {
      ctx.pc++;
    }
  } else {
    ctx.pc = macroStep.step;
  }
  return 0;
}

unsigned int SoundMacro::CmdLoop::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  if ((keyOff && ctx.keyoffReceived) || (sampleEnd && ctx.sampleEndReceived)) {
    ctx.loopCountdown = -1;
    ctx.pc++;
    return 0;
  }
  if (ctx.loopCountdown < 0) {
    uint16_t useTimes = times;
    if (useTimes == kInfiniteLoopSentinel) {
      ctx.loopCountdown = -2;
    } else {
      ctx.loopCountdown = static_cast<int>(useTimes);
    }
    ctx.loopStep = macroStep.step;
  }
  if (ctx.loopCountdown == -2) {
    ctx.pc = ctx.loopStep;
  } else if (ctx.loopCountdown > 0) {
    ctx.loopCountdown--;
    ctx.pc = ctx.loopStep;
  } else {
    ctx.loopCountdown = -1;
    ctx.pc++;
  }
  return 0;
}

unsigned int SoundMacro::CmdReturn::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  ctx.ended = true;
  return 0;
}

unsigned int SoundMacro::CmdGoSub::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  auto* app = static_cast<FluidsyXApp*>(ctx.appData);
  if (macro.id.id != 0xffff && macro.id.id != 0) {
    const SoundMacro* target = app->activePool ? app->activePool->soundMacro(macro.id) : nullptr;
    if (target) {
      ctx.macro = target;
      ctx.pc = macroStep.step;
    } else {
      ctx.pc++;
    }
  } else {
    ctx.pc = macroStep.step;
  }
  return 0;
}

unsigned int SoundMacro::CmdSetNote::DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const {
  auto* app = static_cast<FluidsyXApp*>(ctx.appData);
  unsigned int delay = 0;
  if(v)
  {
    // Technically not correct to set it directly. MusyX "glides" to the new pitch.
    fluid_voice_gen_set(v, GEN_COARSETUNE, key - ctx.orgNote);
    fluid_voice_gen_set(v, GEN_FINETUNE,   detune);
    fluid_voice_update_param(v, GEN_COARSETUNE);
    fluid_voice_update_param(v, GEN_FINETUNE);
  }
  else
  {
    // remember for startSample
    ctx.curNote = static_cast<uint8_t>(std::clamp(static_cast<int>(key), 0, 127));
    ctx.curDetune = detune;
  }
  if (msSwitch)
    delay = ticksOrMs;
  else
    delay = static_cast<unsigned int>(ticksOrMs * 1000.0 / ctx.ticksPerSec);
  ctx.pc++;
  return delay;
}

unsigned int SoundMacro::CmdAddNote::DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const {
  auto* app = static_cast<FluidsyXApp*>(ctx.appData);
  unsigned int delay = 0;
  if(v)
  {
    if (originalKey) {
      // orgNote should serve as reference
      fluid_voice_gen_set(v, GEN_COARSETUNE, add);
    } else {
      // Current note pitch serves as reference
      // ctx.curNote + add;
      fluid_voice_gen_set(v, GEN_COARSETUNE, fluid_voice_gen_get(v, GEN_COARSETUNE) + add);
    }
    fluid_voice_gen_set(v, GEN_FINETUNE,   detune);
    fluid_voice_update_param(v, GEN_FINETUNE);
    fluid_voice_update_param(v, GEN_COARSETUNE);
  }
  else
  {
    ctx.curDetune = detune;
    ctx.pendingCmds.push_back(this);
  }

  if (msSwitch)
    delay = ticksOrMs;
  else
    delay = static_cast<unsigned int>(ticksOrMs * 1000.0 / ctx.ticksPerSec);
  ctx.pc++;
  return delay;
}

unsigned int SoundMacro::CmdRndNote::DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const {
  auto* app = static_cast<FluidsyXApp*>(ctx.appData);
  int lo = noteLo;
  int hi = noteHi;
  if (absRel) {
    lo = static_cast<int>(ctx.curNote) - noteLo;
    hi = static_cast<int>(ctx.curNote) + noteHi;
  }
  if (lo > hi)
    std::swap(lo, hi);
  std::uniform_int_distribution<int> dist(lo, hi);
  int note = dist(app->rng);
  ctx.curNote = static_cast<uint8_t>(std::clamp(note, 0, 127));
  if (fixedFree) {
    std::uniform_int_distribution<int> detuneDist(-100, 100);
    ctx.curDetune = detuneDist(app->rng);
  } else {
    ctx.curDetune = detune;
  }
  if(v)
  {
    // Technically not correct to set it directly. MusyX "glides" to the new pitch.
    fluid_voice_gen_set(v, GEN_COARSETUNE, ctx.curNote - ctx.orgNote);
    fluid_voice_gen_set(v, GEN_FINETUNE,   detune);
    fluid_voice_update_param(v, GEN_COARSETUNE);
    fluid_voice_update_param(v, GEN_FINETUNE);
  }
  else
  {
    // curDetune and curNote assignment done, nothing do to
  }
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdSetPitch::DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const {
  auto* app = static_cast<FluidsyXApp*>(ctx.appData);
  double freq = static_cast<double>(hz) + fine / 65536.0;
  if (freq > 0.0) {
    double midiNote = 69.0 + 12.0 * std::log2(freq / 440.0);
    int noteInt = static_cast<int>(std::round(midiNote));
    noteInt = std::clamp(noteInt, 0, 127);
    double centFrac = (midiNote - noteInt) * 100.0;
    ctx.curNote = static_cast<uint8_t>(noteInt);
    ctx.curDetune = static_cast<int>(std::round(centFrac));
    if(v)
    {
      // Technically not correct to set it directly. MusyX "glides" to the new pitch.
      fluid_voice_gen_set(v, GEN_COARSETUNE, ctx.curNote - ctx.orgNote);
      fluid_voice_gen_set(v, GEN_FINETUNE,   ctx.curDetune);
      fluid_voice_update_param(v, GEN_COARSETUNE);
      fluid_voice_update_param(v, GEN_FINETUNE);
    }
    else
    {
      // curNote and curDetune assignment done, nothing do to
    }
  }
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdPitchWheelR::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  auto* app = static_cast<FluidsyXApp*>(ctx.appData);
  if (rangeUp != rangeDown) {
    fmt::print(stderr,
            "fluidsyX: warning: PitchWheelR has asymmetric range "
            "(up={}, down={}); FluidSynth only supports symmetric "
            "pitch bend range, using rangeUp={}\n",
            static_cast<int>(rangeUp), static_cast<int>(rangeDown),
            static_cast<int>(rangeUp));
  }
  FluidEventPtr evt(new_fluid_event(), &delete_fluid_event);
  fluid_event_set_source(evt.get(), app->callbackSeqId);
  fluid_event_set_dest(evt.get(), app->synthSeqId);
  fluid_event_pitch_wheelsens(evt.get(), ctx.channel, rangeUp);
  fluid_sequencer_send_now(app->sequencer.get(), evt.get());
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdScaleVolume::DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const {
  int vol = (ctx.midiVel * (scale + 128)) / 256 + add;
  vol = std::clamp(vol, 0, 127);
  float attn = (vol > 0)
      ? static_cast<float>(-200.0 * std::log10(vol / 127.0))
      : 1440.0f;
  attn = std::clamp(attn, 0.0f, 1440.0f);
  if (v) {
    fluid_voice_gen_set(v, GEN_ATTENUATION, attn);
    fluid_voice_update_param(v, GEN_ATTENUATION);
  } else {
    ctx.pendingCmds.push_back(this);
  }
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdScaleVolumeDLS::DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const {
  int vol = (ctx.midiVel * (scale + 32768)) / 65536;
  vol = std::clamp(vol, 0, 127);
  float attn = (vol > 0)
      ? static_cast<float>(-200.0 * std::log10(vol / 127.0))
      : 1440.0f;
  attn = std::clamp(attn, 0.0f, 1440.0f);
  if (v) {
    fluid_voice_gen_set(v, GEN_ATTENUATION, attn);
    fluid_voice_update_param(v, GEN_ATTENUATION);
  } else {
    ctx.pendingCmds.push_back(this);
  }
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdSetAdsrCtrl::DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const {
  auto* app = static_cast<FluidsyXApp*>(ctx.appData);
  ctx.useAdsrControllers = true;
  ctx.midiAttack  = attack;
  ctx.midiDecay   = decay;
  ctx.midiSustain = sustain;
  ctx.midiRelease = release;
  if (ctx.channel < 16) {
    auto& m = app->channelAdsrMap[ctx.channel];
    m.active    = true;
    m.attackCC  = attack;
    m.decayCC   = decay;
    m.sustainCC = sustain;
    m.releaseCC = release;
  }
  if (v) {
    app->applyAdsrToVoice(v, ctx, /*started=*/true);
  }
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdPanning::DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const {
  if (panPosition == -128) {
    /* -128 = surround-channel only; no-op in FluidSynth */
  } else {
    float panGen = (panPosition / 127.0f) * 500.0f;
    if (v) {
      fluid_voice_gen_set(v, GEN_PAN, panGen);
      fluid_voice_update_param(v, GEN_PAN);
    } else {
      ctx.pendingCmds.push_back(this);
    }
  }
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdPianoPan::DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const {
  int diff = ctx.orgNote - centerKey;
  int panPos = centerPan + diff * scale / 127;
  panPos = std::clamp(panPos, -127, 127);
  float panGen = (panPos / 127.0f) * 500.0f;
  if (v) {
    fluid_voice_gen_set(v, GEN_PAN, panGen);
    fluid_voice_update_param(v, GEN_PAN);
  } else {
    ctx.pendingCmds.push_back(this);
  }
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdStartSample::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  auto* app = static_cast<FluidsyXApp*>(ctx.appData);
  fluid_sample_t* flSamp = nullptr;
  bool looped = false;
  if (app->musyxSfData) {
    auto sIt = app->musyxSfData->sampleIndex.find(sample.id.id);
    if (sIt != app->musyxSfData->sampleIndex.end()) {
      auto& ds = app->musyxSfData->samples[sIt->second];
      flSamp = ds.flSample;
      looped = ds.looped;
    }
  }
  if (flSamp && app->dummyPreset) {
    app->pendingVoiceStart.flSamp = flSamp;
    app->pendingVoiceStart.looped = looped;
    app->pendingVoiceStart.macroContext = &ctx;
    unsigned int id = app->nextVoiceId++;
    if (id == 0) id = app->nextVoiceId++;
    if (fluid_synth_start(app->synth.get(), id, app->dummyPreset, 0, ctx.channel,
                           ctx.curNote, ctx.midiVel) == FLUID_OK) {
      ctx.voiceId  = id;
    } else {
      fmt::print(stderr, "fluidsyX: warning: voice start failed for "
                      "sample {} on ch {} key {}\n",
              sample.id.id, ctx.channel, ctx.curNote);
    }
    app->pendingVoiceStart.flSamp = nullptr;
  } else {
    FluidEventPtr evt(new_fluid_event(), &delete_fluid_event);
    fluid_event_set_source(evt.get(), app->callbackSeqId);
    fluid_event_set_dest(evt.get(), app->synthSeqId);
    fmt::print(stderr, "fluidsyX: note on event for sample {} on ch {} key {} (sample not found, using preset fallback)\n",
            sample.id.id, ctx.channel, ctx.curNote);
    fluid_event_noteon(evt.get(), ctx.channel, ctx.curNote, ctx.midiVel);
    fluid_sequencer_send_now(app->sequencer.get(), evt.get());
  }
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdStopSample::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  auto* app = static_cast<FluidsyXApp*>(ctx.appData);
  fmt::print(stderr, "fluidsyX: OFFing voice ID {} on ch {}\n", ctx.voiceId, ctx.channel);
  app->killVoice(app->synth.get(), ctx);
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdKeyOff::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  auto* app = static_cast<FluidsyXApp*>(ctx.appData);
  releaseVoice(app->synth.get(), ctx);
  ctx.keyoffReceived = true;
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdSendKeyOff::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  auto* app = static_cast<FluidsyXApp*>(ctx.appData);
  int targetMacroId = -1;
  if (lastStarted) {
    targetMacroId = ctx.lastPlayMacroId;
  } else {
    targetMacroId = ctx.vars[variable & 0x1f];
  }
  if (targetMacroId >= 0) {
    auto childIt = app->activeMacros.find(targetMacroId);
    if (childIt != app->activeMacros.end() && !childIt->second.ended) {
      fmt::print(stderr, "fluidsyX: sending key off to foreign macro ID {} on ch {} (lastStarted={}, var={})\n",
              targetMacroId, ctx.channel, lastStarted, variable & 0x1f);
      releaseVoice(app->synth.get(), childIt->second);
      childIt->second.keyoffReceived = true;
    } else {
      fmt::print(stderr, "fluidsyX: warning: SendKeyOff target macro ID {} not found or already ended (lastStarted={}, var={})\n",
              targetMacroId, lastStarted, variable & 0x1f);
    }
  } else {
    fmt::print(stderr, "fluidsyX: warning: SendKeyOff with no valid target (lastStarted={}, var={})\n",
            lastStarted, variable & 0x1f);
  }
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdPlayMacro::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  auto* app = static_cast<FluidsyXApp*>(ctx.appData);
  if (app->activePool && macro.id.id != 0xffff) {
    const SoundMacro* child = app->activePool->soundMacro(macro.id);
    if (child) {
      int childKey = std::clamp(static_cast<int>(ctx.orgNote) + addNote, 0, 127);
      unsigned int curTick = fluid_sequencer_get_tick(app->sequencer.get());
      int childId = app->enqueueSoundMacro(child, macroStep.step, ctx.channel,
                        static_cast<uint8_t>(childKey), ctx.midiVel,
                        curTick);
      ctx.lastPlayMacroId = childId;
      fmt::print(stderr, "fluidsyX: started child macro ID {} on ch {} with key {}\n", childId, ctx.channel, childKey);
    } else {
      fmt::print(stderr, "fluidsyX: warning: PlayMacro target macro ID {} not found\n", macro.id.id);
    }
  } else {
    fmt::print(stderr, "fluidsyX: warning: PlayMacro with no valid target (macro ID {})\n", macro.id.id);
  }
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdSplitKey::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  auto* app = static_cast<FluidsyXApp*>(ctx.appData);
  if (ctx.orgNote >= static_cast<uint8_t>(key)) {
    if (macro.id.id != 0xffff && macro.id.id != 0 && app->activePool) {
      const SoundMacro* target = app->activePool->soundMacro(macro.id);
      if (target) {
        ctx.macro = target;
        ctx.pc = macroStep.step;
        return 0;
      }
    }
    ctx.pc = macroStep.step;
  } else {
    ctx.pc++;
  }
  return 0;
}

unsigned int SoundMacro::CmdSplitVel::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  auto* app = static_cast<FluidsyXApp*>(ctx.appData);
  if (ctx.midiVel >= static_cast<uint8_t>(velocity)) {
    if (macro.id.id != 0xffff && macro.id.id != 0 && app->activePool) {
      const SoundMacro* target = app->activePool->soundMacro(macro.id);
      if (target) {
        ctx.macro = target;
        ctx.pc = macroStep.step;
        return 0;
      }
    }
    ctx.pc = macroStep.step;
  } else {
    ctx.pc++;
  }
  return 0;
}

unsigned int SoundMacro::CmdSplitRnd::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  auto* app = static_cast<FluidsyXApp*>(ctx.appData);
  std::uniform_int_distribution<int> dist(0, 255);
  if (dist(app->rng) >= rnd) {
    if (macro.id.id != 0xffff && macro.id.id != 0 && app->activePool) {
      const SoundMacro* target = app->activePool->soundMacro(macro.id);
      if (target) {
        ctx.macro = target;
        ctx.pc = macroStep.step;
        return 0;
      }
    }
    ctx.pc = macroStep.step;
  } else {
    ctx.pc++;
  }
  return 0;
}

unsigned int SoundMacro::CmdMod2Vibrange::DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const {
  if (v) {
    fluid_mod_t* mod2vibrange_mod = static_cast<fluid_mod_t*>(alloca(fluid_mod_sizeof()));
    fluid_mod_set_source1(mod2vibrange_mod, 1, FLUID_MOD_CC | FLUID_MOD_LINEAR | FLUID_MOD_UNIPOLAR | FLUID_MOD_POSITIVE);
    fluid_mod_set_source2(mod2vibrange_mod, 0, 0);
    fluid_mod_set_dest   (mod2vibrange_mod, GEN_VIBLFOTOPITCH);
    fluid_mod_set_amount (mod2vibrange_mod, keys * 100 + cents);
    fluid_voice_add_mod  (v, mod2vibrange_mod, FLUID_VOICE_ADD);
  } else {
    ctx.pendingCmds.push_back(this);
  }
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdVibrato::DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const {
  if (v) {
    fluid_mod_t* vib_mod = static_cast<fluid_mod_t*>(alloca(fluid_mod_sizeof()));
    uint8_t modSrc;
    uint8_t modFlag = FLUID_MOD_LINEAR | FLUID_MOD_UNIPOLAR | FLUID_MOD_POSITIVE;
    double vibDepthCents = levelNote * 100.0 + levelFine;
    uint8_t mwFlag = static_cast<uint8_t>(modwheelFlag);
    switch (mwFlag) {
      default:
      case 0:
        modSrc = FLUID_MOD_NONE;
        modFlag |= FLUID_MOD_GC;
        break;
      case 1:
        modSrc = 1;
        modFlag |= FLUID_MOD_CC;
        break;
      case 2:
        modSrc = FLUID_MOD_CHANNELPRESSURE;
        modFlag |= FLUID_MOD_GC;
        break;
    }
    fluid_mod_set_source2(vib_mod, FLUID_MOD_NONE, 0);
    fluid_mod_set_dest   (vib_mod, GEN_VIBLFOTOPITCH);
    if (mwFlag == 1 || mwFlag == 2) {
      fluid_mod_set_source1(vib_mod, modSrc, modFlag);
      fluid_mod_set_amount (vib_mod, vibDepthCents);
      fluid_voice_add_mod  (v, vib_mod, FLUID_VOICE_OVERWRITE);
    } else if (mwFlag == 0) {
      fluid_voice_gen_set(v, GEN_VIBLFOTOPITCH, vibDepthCents);
      fluid_voice_update_param(v, GEN_VIBLFOTOPITCH);
    } else {
      fmt::print(stderr, "fluidsyX: warning: unknown modwheelFlag value {} in Vibrato command; treating as modwheelFlag==0\n", modwheelFlag);
      ctx.pc++;
      return 0;
    }
    if (mwFlag == 0 || mwFlag == 2) {
      modFlag &= ~FLUID_MOD_GC;
      modFlag |= FLUID_MOD_CC;
      fluid_mod_set_source1(vib_mod, 1, modFlag);
      fluid_mod_set_amount(vib_mod, 0);
      fluid_voice_add_mod(v, vib_mod, FLUID_VOICE_OVERWRITE);
    }
    if (ticksOrMs > 0) {
      float q = msSwitch ? 1000.0f : static_cast<float>(ctx.ticksPerSec);
      float periodSec = ticksOrMs / q;
      if (periodSec > 0.0f) {
        float freqHz = 1.0f / periodSec;
        float freqCents = 1200.0f * std::log2(freqHz / 8.176f);
        fluid_voice_gen_set(v, GEN_VIBLFOFREQ, freqCents);
        fluid_voice_update_param(v, GEN_VIBLFOFREQ);
      }
    }
  } else {
    ctx.pendingCmds.push_back(this);
  }
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdPortamento::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  auto* app = static_cast<FluidsyXApp*>(ctx.appData);
  bool enable = (portState != SoundMacro::CmdPortamento::PortState::Disable);
  FluidEventPtr evt(new_fluid_event(), &delete_fluid_event);
  fluid_event_set_source(evt.get(), app->callbackSeqId);
  fluid_event_set_dest(evt.get(), app->synthSeqId);
  fluid_event_control_change(evt.get(), ctx.channel, 65, enable ? 127 : 0);
  fluid_sequencer_send_now(app->sequencer.get(), evt.get());
  if (enable) {
    uint16_t timeVal = ticksOrMs;
    double timeMs;
    if (msSwitch) {
      timeMs = static_cast<double>(timeVal);
    } else {
      timeMs = timeVal * 1000.0 / ctx.ticksPerSec;
    }
    fluid_event_control_change(evt.get(), ctx.channel, 5, static_cast<int>(timeMs) / 128);
    fluid_sequencer_send_now(app->sequencer.get(), evt.get());
    fluid_event_control_change(evt.get(), ctx.channel, 37, static_cast<int>(timeMs) % 128);
    fluid_sequencer_send_now(app->sequencer.get(), evt.get());
    if (portType == SoundMacro::CmdPortamento::PortType::LastPressed)
      fluid_synth_set_portamento_mode(app->synth.get(), ctx.channel,
                                      FLUID_CHANNEL_PORTAMENTO_MODE_LEGATO_ONLY);
    else
      fluid_synth_set_portamento_mode(app->synth.get(), ctx.channel,
                                      FLUID_CHANNEL_PORTAMENTO_MODE_EACH_NOTE);
    fmt::print(stderr, "fluidsyX: PORTAMENTO ON!!! for ch {} with time {} ms (mode {})\n",
            ctx.channel, timeMs, (portType == SoundMacro::CmdPortamento::PortType::LastPressed) ? "legato-only" : "each-note");
  }
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdSetVar::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  int idx = static_cast<int>(a);
  if (idx >= 0 && idx < 32)
    ctx.vars[idx] = imm;
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdAddVars::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  int ia = static_cast<int>(a);
  int ib = static_cast<int>(b);
  if (ia >= 0 && ia < 32 && ib >= 0 && ib < 32)
    ctx.vars[ia] += ctx.vars[ib];
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdSubVars::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  int ia = static_cast<int>(a);
  int ib = static_cast<int>(b);
  if (ia >= 0 && ia < 32 && ib >= 0 && ib < 32)
    ctx.vars[ia] -= ctx.vars[ib];
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdMulVars::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  int ia = static_cast<int>(a);
  int ib = static_cast<int>(b);
  if (ia >= 0 && ia < 32 && ib >= 0 && ib < 32)
    ctx.vars[ia] *= ctx.vars[ib];
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdDivVars::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  int ia = static_cast<int>(a);
  int ib = static_cast<int>(b);
  if (ia >= 0 && ia < 32 && ib >= 0 && ib < 32 && ctx.vars[ib] != 0)
    ctx.vars[ia] /= ctx.vars[ib];
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdAddIVars::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  int ia = static_cast<int>(a);
  if (ia >= 0 && ia < 32)
    ctx.vars[ia] += imm;
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdIfEqual::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  int ia = static_cast<int>(a);
  int ib = static_cast<int>(b);
  bool cond = false;
  if (ia >= 0 && ia < 32 && ib >= 0 && ib < 32)
    cond = (ctx.vars[ia] == ctx.vars[ib]);
  if (cond) {
    if (!notEq)
      ctx.pc = macroStep.step;
    else
      ctx.pc++;
  } else {
    if (notEq)
      ctx.pc = macroStep.step;
    else
      ctx.pc++;
  }
  return 0;
}

unsigned int SoundMacro::CmdIfLess::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  int ia = static_cast<int>(a);
  int ib = static_cast<int>(b);
  bool cond = false;
  if (ia >= 0 && ia < 32 && ib >= 0 && ib < 32)
    cond = (ctx.vars[ia] < ctx.vars[ib]);
  if (cond) {
    if (!notLt)
      ctx.pc = macroStep.step;
    else
      ctx.pc++;
  } else {
    if (notLt)
      ctx.pc = macroStep.step;
    else
      ctx.pc++;
  }
  return 0;
}

unsigned int SoundMacro::CmdTrapEvent::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  MacroExecContext::EventTrap trap;
  trap.macroId = static_cast<uint16_t>(macro.id.id);
  trap.macroStep = static_cast<uint16_t>(macroStep.step);
  switch (event) {
  case SoundMacro::CmdTrapEvent::EventType::KeyOff:
    ctx.keyoffTrap = trap;
    break;
  case SoundMacro::CmdTrapEvent::EventType::SampleEnd:
    ctx.sampleEndTrap = trap;
    break;
  case SoundMacro::CmdTrapEvent::EventType::MessageRecv:
    ctx.messageTrap = trap;
    break;
  default: break;
  }
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdUntrapEvent::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  switch (event) {
  case SoundMacro::CmdTrapEvent::EventType::KeyOff:
    ctx.keyoffTrap.clear();
    break;
  case SoundMacro::CmdTrapEvent::EventType::SampleEnd:
    ctx.sampleEndTrap.clear();
    break;
  case SoundMacro::CmdTrapEvent::EventType::MessageRecv:
    ctx.messageTrap.clear();
    break;
  default: break;
  }
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdSetAdsr::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  ctx.adsrTableId = std::make_optional(std::make_tuple(uint16_t(table.id.id), dlsMode));
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdModeSelect::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  if (dlsVol == true && itd == false) {
    /* Matches fluidsynth's default curve; ignore */
  }
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdSRCmodeSelect::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const {
  if (type0SrcFilter != 0 || srcType != 1) {
    fmt::print(stderr, "fluidsyX: ignoring SRCmodeSelect with srcType={} type0SrcFilter={}\n", srcType, type0SrcFilter);
  }
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdSetKeygroup::DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const {
  if (v) {
    fluid_voice_gen_set(v, GEN_EXCLUSIVECLASS, group);
  } else {
    ctx.pendingCmds.push_back(this);
  }
  ctx.pc++;
  return 0;
}

unsigned int SoundMacro::CmdSetupLFO::DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const {
  if (v) {
    switch(this->lfoNumber)
    {
      case 0: {
      fluid_voice_gen_set(v, GEN_MODLFODELAY, -12000);
      float freqHz = 1000.0f / this->periodInMs;
      float freqCents = 1200.0f * std::log2(freqHz / 8.176f);
      fluid_voice_gen_set(v, GEN_MODLFOFREQ, freqCents);
      }
      break;
      case 1: {
      fluid_voice_gen_set(v, GEN_VIBLFODELAY, -12000);
      float freqHz = 1000.0f / this->periodInMs;
      float freqCents = 1200.0f * std::log2(freqHz / 8.176f);
      fluid_voice_gen_set(v, GEN_VIBLFOFREQ, freqCents);
      }
      break;
      default:
      break;
    }
  } else {
    ctx.pendingCmds.push_back(this);
  }
  ctx.pc++;
  return 0;
}

/* ── CmdSetupTremolo ──
 * MusyX: treScale / treModAddScale configure tremolo depth and mod‑wheel scaling.
 * SF2 mapping: GEN_MODLFOTOVOL controls the mod‑LFO → volume depth (centibels).
 * The mod‑LFO frequency is set separately by CmdSetupLFO(0).
 *
 * modwAddScale controls how much the mod wheel (CC 1) adds to the tremolo depth.
 * MusyX formula:
 *   mscale = 1 − Modulation × (4096 − treModAddScale) / 8192²
 *   effectiveScale = treScale × mscale / 4096
 *   voiceVol *= 1 − lfo × (1 − effectiveScale)
 *
 * treModAddScale = 4096 → mod wheel has no effect (mscale always 1).
 * treModAddScale = 0    → mod wheel maximally deepens the tremolo. */
unsigned int SoundMacro::CmdSetupTremolo::DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const {
  ctx.treScale = scale;
  ctx.treModAddScale = modwAddScale;

  if (v) {
    /* Convert MusyX linear depth [0…1] to centibels.
     * depth = (4096 − |scale|) / 4096   (0 = no tremolo, 1 = max)
     * In MusyX the LFO swings volume by ±depth/2; the attenuation side is
     * deeper in dB so we match that: cB = −200·log10(1 − depth/2). */
    float depth = (4096.0f - std::abs(static_cast<float>(scale))) / 4096.0f;
    float depthCb = 0.0f;
    if (depth > 0.001f) {
      float minLin = std::max(1.0f - depth * 0.5f, 0.01f);
      depthCb = -200.0f * std::log10(minLin);
    }
    fluid_voice_gen_set(v, GEN_MODLFOTOVOL, depthCb);
    fluid_voice_update_param(v, GEN_MODLFOTOVOL);

    /* Add CC 1 (mod wheel) modulator for the modwAddScale contribution.
     * When modwAddScale < 4096 the mod wheel deepens the tremolo. */
    if (modwAddScale < 4096) {
      fluid_mod_t* mod = static_cast<fluid_mod_t*>(alloca(fluid_mod_sizeof()));
      fluid_mod_set_source1(mod, 1 /* CC 1 */,
                            FLUID_MOD_CC | FLUID_MOD_LINEAR |
                            FLUID_MOD_UNIPOLAR | FLUID_MOD_POSITIVE);
      fluid_mod_set_source2(mod, FLUID_MOD_NONE, 0);
      fluid_mod_set_dest(mod, GEN_MODLFOTOVOL);
      float extraDepth = (4096.0f - modwAddScale) / 4096.0f;
      float extraMinLin = std::max(1.0f - extraDepth * 0.5f, 0.01f);
      float extraCb = -200.0f * std::log10(extraMinLin);
      fluid_mod_set_amount(mod, extraCb);
      fluid_voice_add_mod(v, mod, FLUID_VOICE_ADD);
    }
  } else {
    ctx.pendingCmds.push_back(this);
  }
  ctx.pc++;
  return 0;
}

/* ── CmdTremoloSelect ──
 * MusyX: SelectSource(svoice, &svoice->inpTremolo, cstep, …)
 * Configures which MIDI source feeds the tremolo evaluator.
 *
 * CC 130 = LFO 1 output.  FluidSynth has no CC 130 — the mod‑LFO is an
 * internal oscillator.  When midiControl == 130 we map it directly to
 * GEN_MODLFOTOVOL (the SF2 mod‑LFO → volume path), which is the exact
 * semantic equivalent of MusyX's "use LFO1 for tremolo".
 *
 * For standard MIDI CCs (0‑119) the CC value IS the oscillating tremolo
 * signal (bipolar around center, like an external LFO).  We create a
 * bipolar modulator CC → GEN_ATTENUATION whose depth is derived from
 * ctx.treScale (set by CmdSetupTremolo).
 *
 * MusyX tremolo depth formula:
 *   depth = 1 − treScale/4096
 *   volume swings ±(depth × 0.5) in linear space
 *   At max depth (treScale=0): min volume = 0.5 → −6 dB ≈ 60 cB */
unsigned int SoundMacro::CmdTremoloSelect::DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const {
  float scaleFactor = (scalingPercentage + fineScaling / 100.0f) / 100.0f;

  if (v) {
    /* Compute tremolo depth in centibels from ctx.treScale.
     * Match the attenuation side of the MusyX linear swing:
     *   depthCb = −200·log10(1 − depth/2)
     * where depth = (4096 − |treScale|) / 4096.  At max depth ≈ 60 cB. */
    float depth = (4096.0f - std::abs(static_cast<float>(ctx.treScale))) / 4096.0f;
    float depthCb = 0.0f;
    if (depth > 0.001f) {
      float minLin = std::max(1.0f - depth * 0.5f, 0.01f);
      depthCb = -200.0f * std::log10(minLin);
    }

    if (midiControl == 130) {
      /* CC 130 = LFO 1: the mod‑LFO is the source.  GEN_MODLFOTOVOL is the
       * SF2 mod‑LFO → volume path; scale by scaleFactor. */
      fluid_voice_gen_set(v, GEN_MODLFOTOVOL, depthCb * scaleFactor);
      fluid_voice_update_param(v, GEN_MODLFOTOVOL);
    } else if (midiControl < 120) {
      /* Standard MIDI CC: the CC value IS the bipolar tremolo signal
       * (like an external LFO oscillating around CC 64 = center).
       * Route CC → GEN_ATTENUATION with BIPOLAR mapping so that:
       *   CC  0 → −depthCb cB (louder)
       *   CC 64 →   0      cB (no change)
       *   CC 127 → +depthCb cB (quieter) */
      fluid_mod_t* mod = static_cast<fluid_mod_t*>(alloca(fluid_mod_sizeof()));
      fluid_mod_set_source1(mod, midiControl,
                            FLUID_MOD_CC | FLUID_MOD_LINEAR |
                            FLUID_MOD_BIPOLAR | FLUID_MOD_POSITIVE);
      fluid_mod_set_source2(mod, FLUID_MOD_NONE, 0);
      fluid_mod_set_dest(mod, GEN_ATTENUATION);
      fluid_mod_set_amount(mod, depthCb * scaleFactor);
      int mode = (combine == Combine::Set) ? FLUID_VOICE_OVERWRITE : FLUID_VOICE_ADD;
      fluid_voice_add_mod(v, mod, mode);
    } else {
      /* CC 128 (pitchbend), 129 (aftertouch), 131 (LFO2), 132+ (surround etc.)
       * — not directly mappable to SF2 modulators; log a warning. */
      fmt::print(stderr, "fluidsyX: TremoloSelect: unsupported midiControl {} — ignoring\n",
                 static_cast<int>(midiControl));
    }
  } else {
    ctx.pendingCmds.push_back(this);
  }
  ctx.pc++;
  return 0;
}

/* ── CmdWiiUnknown (opCode 0x5E) = FilterParameterSelect ──
 * MusyX: SelectSource(svoice, &svoice->inpFilterParameter, cstep, 0x800, 0x4000)
 * Configures a CC → filter cutoff frequency modulation.
 * SF2 mapping: modulator CC → GEN_FILTERFC (filter cutoff in cents). */
unsigned int SoundMacro::CmdWiiUnknown::DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const {
  float scaleFactor = (scalingPercentage + fineScaling / 100.0f) / 100.0f;

  if (v) {
    if (midiControl < 120) {
      fluid_mod_t* mod = static_cast<fluid_mod_t*>(alloca(fluid_mod_sizeof()));
      fluid_mod_set_source1(mod, midiControl,
                            FLUID_MOD_CC | FLUID_MOD_LINEAR |
                            FLUID_MOD_UNIPOLAR | FLUID_MOD_POSITIVE);
      fluid_mod_set_source2(mod, FLUID_MOD_NONE, 0);
      fluid_mod_set_dest(mod, GEN_FILTERFC);
      /* Scale to a reasonable filter cutoff range in cents.
       * SF2 filter cutoff is in absolute cents (8.176 Hz = 0 cents).
       * A full CC sweep of ~6000 cents ≈ 4 octaves is reasonable. */
      fluid_mod_set_amount(mod, 6000.0f * scaleFactor);
      int mode = (combine == Combine::Set) ? FLUID_VOICE_OVERWRITE : FLUID_VOICE_ADD;
      fluid_voice_add_mod(v, mod, mode);
    } else if (midiControl == 130 || midiControl == 131) {
      /* CC 130/131 = LFO: route mod‑LFO or vib‑LFO to filter cutoff */
      int gen = (midiControl == 130) ? GEN_MODLFOTOFILTERFC : GEN_MODENVTOFILTERFC;
      fluid_voice_gen_set(v, gen, 6000.0f * scaleFactor);
      fluid_voice_update_param(v, gen);
    } else {
      fmt::print(stderr, "fluidsyX: FilterParameterSelect: unsupported midiControl {} — ignoring\n",
                 static_cast<int>(midiControl));
    }
  } else {
    ctx.pendingCmds.push_back(this);
  }
  ctx.pc++;
  return 0;
}

/* ── CmdWiiUnknown2 (opCode 0x5F) = FilterSwitchSelect ──
 * MusyX: SelectSource(svoice, &svoice->inpFilterSwitch, cstep, 0x40, 0x2000)
 * Controls whether the filter is active.  In SF2 there is no discrete
 * filter on/off; instead we use a CC modulator that pushes the cutoff
 * to maximum (effectively bypassing the filter) when the CC is low. */
unsigned int SoundMacro::CmdWiiUnknown2::DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const {
  float scaleFactor = (scalingPercentage + fineScaling / 100.0f) / 100.0f;

  if (v) {
    if (midiControl < 120) {
      /* Use a NEGATIVE modulator: when CC is 0 the filter cutoff is pushed
       * high (filter open / bypassed); when CC is 127 the cutoff moves down
       * (filter engaged). */
      fluid_mod_t* mod = static_cast<fluid_mod_t*>(alloca(fluid_mod_sizeof()));
      fluid_mod_set_source1(mod, midiControl,
                            FLUID_MOD_CC | FLUID_MOD_LINEAR |
                            FLUID_MOD_UNIPOLAR | FLUID_MOD_NEGATIVE);
      fluid_mod_set_source2(mod, FLUID_MOD_NONE, 0);
      fluid_mod_set_dest(mod, GEN_FILTERFC);
      /* Positive amount: when source is at max (CC 127 → after NEGATIVE → 0)
       * the offset is 0; when source is at min (CC 0 → after NEGATIVE → 1)
       * the offset opens the filter fully. */
      fluid_mod_set_amount(mod, 13500.0f * scaleFactor);
      int mode = (combine == Combine::Set) ? FLUID_VOICE_OVERWRITE : FLUID_VOICE_ADD;
      fluid_voice_add_mod(v, mod, mode);
    } else {
      fmt::print(stderr, "fluidsyX: FilterSwitchSelect: unsupported midiControl {} — ignoring\n",
                 static_cast<int>(midiControl));
    }
  } else {
    ctx.pendingCmds.push_back(this);
  }
  ctx.pc++;
  return 0;
}

/* ═══════════════════ SoundMacro → FluidSynth translation ═══════════════════ */

unsigned int FluidsyXApp::processMacroCmd(MacroExecContext& ctx,
                                          unsigned int curTick) {
  if (ctx.ended || !ctx.macro)
    return 0;
  if (ctx.pc < 0 || ctx.pc >= static_cast<int>(ctx.macro->m_cmds.size())) {
    ctx.ended = true;
    return 0;
  }
  bool timingMismatch = curTick != fluid_sequencer_get_tick(sequencer.get());
  if(timingMismatch)
    fmt::print(stderr, "Warning: processMacroCmd called with curTick {}, but sequencer tick is {}. Timing may be inaccurate.\n", curTick, fluid_sequencer_get_tick(sequencer.get()));

  const SoundMacro::ICmd& cmd = *ctx.macro->m_cmds[ctx.pc];

  if (verbose || timingMismatch)
    fmt::print("fluidsyX: [ch{} pc{}] {}\n", ctx.channel, ctx.pc,
               cmd.formatMacroCmd(curTick));

  fluid_voice_t* v = getActiveVoice(synth.get(), ctx);
  return cmd.DoFluid(ctx, v);
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
  ctx.curNote = key;
  ctx.orgNote = key;
  ctx.midiVel = vel;
  /* triggerNote = the original SNG note (before keymap/layer transpose).
   * Used to match note-off events.  If not supplied, defaults to key. */
  ctx.triggerNote = (triggerNote == 0xff) ? key : triggerNote;
  ctx.appData = this;
  ctx.macStartTime = startTick;

  /* Walk through commands that execute instantly (no delay).
   * When a command introduces a delay, schedule a timer event and stop. */
  unsigned int tick = startTick;
  int safetyCounter = 0;
  while (!ctx.ended && safetyCounter < kMaxMacroCmdsPerBurst) {
    unsigned int d = processMacroCmd(ctx, tick);
    if (ctx.inIndefiniteWait) {
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
      /* Trigger keyoff on all active macro voices triggered by this
       * (channel, note).  We match on triggerNote (the original SNG note
       * before keymap/layer transpose).
       *
       * Original amuse: Voice::keyOff() first checks m_keyoffTrap.
       * If set, the trap redirects macro execution to the trap target
       * WITHOUT triggering ADSR release.  The new macro code can issue
       * its own KeyOff if desired.
       * If no trap: _macroKeyOff() → _doKeyOff() → ADSR release +
       * keyoffNotify (sets m_keyoff flag so waiting macros can resume). */
      uint8_t ch = sngEvt.channel;
      uint8_t note = sngEvt.note;
      for (auto& [id, mctx] : app->activeMacros) {
        if (mctx.channel == ch && mctx.triggerNote == note && !mctx.ended && !mctx.keyoffReceived) {
          if (mctx.keyoffTrap.isSet()) {
            /* musyx macSetExternalKeyoff sets cFlags|=8 (keyoff received) BEFORE
             * calling ExecuteTrap, so the trap macro has full awareness. */
            mctx.keyoffReceived = true;
            /* Trap is registered — redirect execution instead of releasing. */
            if (app->executeTrap(mctx, mctx.keyoffTrap, time)) {
              /* Schedule timer to continue macro execution from trap target */
              FluidEventPtr resumeEvt(new_fluid_event(), &delete_fluid_event);
              fluid_event_set_source(resumeEvt.get(), app->callbackSeqId);
              fluid_event_set_dest(resumeEvt.get(), app->callbackSeqId);
              fluid_event_timer(resumeEvt.get(), reinterpret_cast<void*>(
                  static_cast<intptr_t>(id)));
              fluid_sequencer_send_now(app->sequencer.get(), resumeEvt.get());
            }
          } else {
            /* No trap — normal keyoff behavior. */
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
              fluid_sequencer_send_now(app->sequencer.get(), resumeEvt.get());
            }
          }
          // key off processed for this macro, stop processing here
          break;
        }
      }
      break;
    }
    case PendingSngNoteEvent::ProgramChange:
      app->channelPrograms[sngEvt.channel] = sngEvt.note; /* note field = program */
      fluid_synth_program_change(app->synth.get(), sngEvt.channel, sngEvt.note);
      break;

    case PendingSngNoteEvent::CC: {
      uint8_t  ch  = sngEvt.channel;
      uint16_t cc  = sngEvt.note;     /* note field = CC number (may be > 127 for MusyX extended CCs) */
      uint8_t  val = sngEvt.velocity; /* velocity field = CC value */

      /* Forward the raw CC to FluidSynth (for non-ADSR uses).
       * MusyX extended CCs (>= 128) will be rejected by FluidSynth since it only
       * supports the standard MIDI CC range 0-127. */
      if(fluid_synth_cc(app->synth.get(), ch, cc, val) != FLUID_OK) {
        fmt::print(stderr, "fluidsyX: failed to send CC event to FluidSynth (ch {}, cc {}, val {})\n", ch, cc, val);
      }
      break;
    }

    case PendingSngNoteEvent::WarmReset:
      app->initCCDefault(inpWarmMIDIDefaults);
      break;
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
    if (ctx.inIndefiniteWait) {
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
  std::vector<SngTrackLoop> trackLoops;
  if (!parseSngEvents(sngData, bigEndian, sngVersion, events, initialScale,
                      trackLoops)) {
    fmt::print(stderr, "fluidsyX: failed to parse SNG events\n");
    return 0.0;
  }

  fmt::print("fluidsyX: parsed {} song events, initial tempo scale {:.1f} ticks/s\n",
         events.size(), initialScale);
  for (const auto& tl : trackLoops)
    fmt::print("fluidsyX: track {} (ch {}) loop: ticks {} → {} (body = {} ticks)\n",
           tl.trackIdx, tl.channel,
           tl.loopStartTick, tl.loopEndTick,
           tl.loopEndTick - tl.loopStartTick);
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
    case SngEvent::WarmReset:
    case SngEvent::Program: {
      /* Route through timer callback */
      PendingSngNoteEvent::Type t;
      uint16_t d1 = e.data1;
      uint8_t  d2 = e.data2;
      switch (e.type) {
      case SngEvent::NoteOn:  t = PendingSngNoteEvent::NoteOn; break;
      case SngEvent::NoteOff: t = PendingSngNoteEvent::NoteOff; d2 = 0; break;
      case SngEvent::CC:      t = PendingSngNoteEvent::CC; break;
      case SngEvent::WarmReset: t = PendingSngNoteEvent::WarmReset; d1 = d2 = 0; break;
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
    case SngEvent::AllNotesOff:
      fluid_event_set_dest(evt.get(), synthSeqId);
      fluid_event_all_notes_off(evt.get(), e.channel);
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

  /* ── Per-track loop support ──
   * Each track independently loops back to its own loopStartTick when it
   * reaches its loopEndTick.  Tracks with shorter loops repeat more often.
   *
   * In amuse, Track::advance() calls resetTempo() when a track loops,
   * causing the tempo table to be re-traversed on each loop iteration.
   * We mirror this by also rescheduling tempo events (trackIdx == -1)
   * within each loop iteration — using the longest loop period so that
   * tempo changes are replayed consistently.
   *
   * This mirrors amuse's SongState::Track::advance() where each track
   * checks its own loop marker independently. */
  if (!trackLoops.empty() && maxDurationTicks > 0 && lastTick < maxDurationTicks) {
    /* Collect global tempo events for rescheduling with loop iterations. */
    uint32_t maxLoopEndTick = 0;
    uint32_t maxLoopStartTick = 0;
    for (const auto& tl : trackLoops) {
      if (tl.loopEndTick > maxLoopEndTick) {
        maxLoopEndTick = tl.loopEndTick;
        maxLoopStartTick = tl.loopStartTick;
      }
    }
    uint32_t maxLoopLen = maxLoopEndTick - maxLoopStartTick;

    std::vector<const SngEvent*> tempoEvents;
    if (maxLoopLen > 0) {
      for (const auto& e : events) {
        if (e.trackIdx == -1 && e.type == SngEvent::Tempo &&
            e.absTick >= maxLoopStartTick && e.absTick < maxLoopEndTick)
          tempoEvents.push_back(&e);
      }
    }

    /* For each track with a loop, collect its events and re-schedule */
    for (const auto& tl : trackLoops) {
      uint32_t loopLen = tl.loopEndTick - tl.loopStartTick;
      if (loopLen == 0)
        continue;

      /* Collect events belonging to this track within the loop body */
      std::vector<const SngEvent*> loopEvents;
      for (const auto& e : events) {
        if (e.trackIdx == tl.trackIdx &&
            e.absTick >= tl.loopStartTick &&
            e.absTick < tl.loopEndTick)
          loopEvents.push_back(&e);
      }

      /* Re-schedule this track's loop body independently */
      unsigned int loopIter = 0;
      uint32_t trackLastTick = tl.loopEndTick;
      while (trackLastTick < maxDurationTicks) {
        uint32_t iterBase = tl.loopEndTick + loopLen * loopIter;
        if (iterBase >= maxDurationTicks)
          break;
        for (const auto* ep : loopEvents) {
          uint32_t relTick = ep->absTick - tl.loopStartTick;
          uint32_t newTick = iterBase + relTick;
          if (newTick >= maxDurationTicks)
            break;
          scheduleOne(*ep, newTick);
          if (newTick > trackLastTick)
            trackLastTick = newTick;
        }
        ++loopIter;
      }
      if (trackLastTick > lastTick)
        lastTick = trackLastTick;
      fmt::print("fluidsyX: track {} (ch {}): looped {} iterations to {} ticks "
                 "(body {} ticks)\n",
             tl.trackIdx, tl.channel, loopIter, trackLastTick, loopLen);
    }

    /* Reschedule tempo events at the longest loop period.  In amuse,
     * resetTempo() resets the tempo pointer so changes replay on each
     * loop iteration.  We reschedule them at maxLoopLen intervals. */
    if (maxLoopLen > 0 && !tempoEvents.empty()) {
      unsigned int tempoIter = 0;
      uint32_t tempoLastTick = maxLoopEndTick;
      while (tempoLastTick < maxDurationTicks) {
        uint32_t iterBase = maxLoopEndTick + maxLoopLen * tempoIter;
        if (iterBase >= maxDurationTicks)
          break;
        for (const auto* ep : tempoEvents) {
          uint32_t relTick = ep->absTick - maxLoopStartTick;
          uint32_t newTick = iterBase + relTick;
          if (newTick >= maxDurationTicks)
            break;
          scheduleOne(*ep, newTick);
        }
        /* Advance trackLastTick by one full loop body regardless of how many
         * events were actually scheduled, ensuring the while-loop terminates
         * even if no tempo events fall below maxDurationTicks. */
        tempoLastTick = iterBase + maxLoopLen;
        ++tempoIter;
      }
      if (tempoLastTick > lastTick)
        lastTick = std::min(tempoLastTick, maxDurationTicks);
    }
  }

  fmt::print("fluidsyX: scheduled {} SNG ticks of song data ({} SNG events, "
         "{} routed through SoundMacro)\n",
         lastTick, events.size(), pendingSngEvents.size());
  return static_cast<double>(lastTick);
}

/* ═══════════════════ Song playback loop ═══════════════════ */

void FluidsyXApp::initCCDefault(const std::array<uint8_t, 134>& defaults)
{
  for (int cc = 0; cc < 128; ++cc)
    {
      // skip CCs that fluidsynth is alergic to, or that are not allowed to modulate either per SF2 spec
      switch(cc)
      {
            case 0x7C: // OMNI_OFF
            case 0x7D: // OMNI_ON
            case 0x7E: // POLY_OFF
            case 0x7F: // POLY_ON
            case 0x60: // DATA_ENTRY_INC
            case 0x61: // DATA_ENTRY_DEC
            case 0x62: // NRPN_LSB
            case 0x63: // NRPN_MSB
            case 0x64: // RPN_LSB
            case 0x65: // RPN_MSB
            case 0x26: // DATA_ENTRY_LSB
            case 0x06: // DATA_ENTRY_MSB
              continue;
      }
      if(defaults[cc] == 0xFF)
      {
        // leave the CC unchanged per MusyX behavior
        continue;
      }
        for (int ch = 0; ch < 16; ++ch) {
        {
          fluid_synth_cc(synth.get(), ch, cc, defaults[cc]);
        }
      }
    }
}

/* ═══════════════════ Console UI ═══════════════════ */

/**
 * Shared console UI for interactive Song and SFX playback.
 *
 * Handles keyboard input, keeps display state, and delegates synth
 * operations back to FluidsyXApp.  The caller provides a sorted list of
 * entry IDs and a label so the same class works for both Song and SFX modes.
 */
class ConsoleUI {
public:
  /**
   * @param app       The owning FluidsyXApp.
   * @param ids       Sorted list of entry IDs (SongIDs or SFXIds).
   * @param currentId Initial entry ID to select.
   * @param label     Display label ("Song" or "SFX").
   */
  ConsoleUI(FluidsyXApp& app, std::vector<int> ids, int currentId,
            const char* label)
      : app_(app)
      , ids_(std::move(ids))
      , label_(label)
  {
    for (size_t i = 0; i < ids_.size(); ++i) {
      if (ids_[i] == currentId) {
        idx_ = static_cast<int>(i);
        break;
      }
    }
  }

  /** Result of a single UI poll iteration. */
  enum class Action {
    Continue,     /**< keep going */
    Quit,         /**< user pressed Q or Ctrl-C */
    ChangeEntry,  /**< user switched to a different entry via LEFT/RIGHT */
    Space         /**< user pressed SPACE (caller decides semantics) */
  };

  /** Non-blocking poll: reads keyboard, updates display, returns action. */
  Action poll(unsigned int elapsed, unsigned int totalTicks) {
    int key = readKey();
    if (key < 0) {
      updateDisplay(elapsed, totalTicks);
      return Action::Continue;
    }

    if (key == 27)
      return handleEscapeSequence(elapsed, totalTicks);

    if (key == 3)
      return Action::Quit;

    int lower = tolower(key);
    if (lower == 'q')
      return Action::Quit;

    if (key == ' ') {
      updateDisplay(elapsed, totalTicks);
      return Action::Space;
    }

    if (lower == 'c') {
      promptChannelSelect();
      updateDisplay(elapsed, totalTicks);
      return Action::Continue;
    }

    if (lower == 's') {
      promptCC19();
      updateDisplay(elapsed, totalTicks);
      return Action::Continue;
    }

    updateDisplay(elapsed, totalTicks);
    return Action::Continue;
  }

  /** The currently selected entry ID. */
  int currentId() const {
    if (idx_ >= 0 && idx_ < static_cast<int>(ids_.size()))
      return ids_[idx_];
    return -1;
  }

  /** The active MIDI channel (-1 = all). */
  int activeChannel() const { return activeChannel_; }

  /** Force a display refresh. */
  void refresh(unsigned int elapsed, unsigned int totalTicks) {
    updateDisplay(elapsed, totalTicks);
  }

private:
  FluidsyXApp& app_;
  std::vector<int> ids_;        /**< sorted entry IDs */
  int idx_ = 0;                 /**< index into ids_ */
  int activeChannel_ = -1;     /**< -1 = all channels */
  const char* label_;           /**< "Song" or "SFX" */

  /* ── helpers ── */

  /** Handle an escape sequence (arrow keys). */
  Action handleEscapeSequence(unsigned int elapsed, unsigned int totalTicks) {
    int next1 = readKey();
    if (next1 != '[') {
      updateDisplay(elapsed, totalTicks);
      return Action::Continue;
    }
    int next2 = readKey();
    switch (next2) {
    case 'A': /* Up – volume up */
      app_.volume = std::clamp(app_.volume + 0.1f, 0.f, 2.f);
      fluid_synth_set_gain(app_.synth.get(), app_.volume);
      break;
    case 'B': /* Down – volume down */
      app_.volume = std::clamp(app_.volume - 0.1f, 0.f, 2.f);
      fluid_synth_set_gain(app_.synth.get(), app_.volume);
      break;
    case 'D': /* Left – previous entry */
      if (idx_ > 0) {
        --idx_;
        updateDisplay(elapsed, totalTicks);
        return Action::ChangeEntry;
      }
      break;
    case 'C': /* Right – next entry */
      if (idx_ + 1 < static_cast<int>(ids_.size())) {
        ++idx_;
        updateDisplay(elapsed, totalTicks);
        return Action::ChangeEntry;
      }
      break;
    default:
      break;
    }
    updateDisplay(elapsed, totalTicks);
    return Action::Continue;
  }

  /** Read a line of text in raw mode (echoing chars, supporting backspace).
   *  Returns the entered string, or std::nullopt if ESC was pressed. */
  std::optional<std::string> readLineRaw() {
    std::string buf;
    for (;;) {
      if (!g_running.load())
        return std::nullopt;
      int k = readKey();
      if (k < 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        continue;
      }
      if (k == 27)            /* ESC – cancel */
        return std::nullopt;
      if (k == '\r' || k == '\n')  /* Enter – confirm */
        return buf;
      if (k == 127 || k == 8) { /* Backspace */
        if (!buf.empty()) {
          buf.pop_back();
          fmt::print("\b \b");
          fflush(stdout);
        }
        continue;
      }
      if (k >= 32 && k < 127) {
        buf.push_back(static_cast<char>(k));
        fmt::print("{}", static_cast<char>(k));
        fflush(stdout);
      }
    }
  }

  /** Prompt user to select a MIDI channel (-1 = all, 0-15). */
  void promptChannelSelect() {
    fmt::print("\r                                                                                \r");
    fmt::print("  Channel (-1=all, 0-15): ");
    fflush(stdout);

    auto input = readLineRaw();
    if (!input) return;

    char* end = nullptr;
    long val = strtol(input->c_str(), &end, 10);
    if (end == input->c_str() || *end != '\0' || val < -1 || val > 15) {
      fmt::print("  [invalid]\n");
      return;
    }
    activeChannel_ = static_cast<int>(val);
  }

  /** Prompt user to enter a CC19 value (0-127) and send it. */
  void promptCC19() {
    fmt::print("\r                                                                                \r");
    fmt::print("  CC19 value (0-127): ");
    fflush(stdout);

    auto input = readLineRaw();
    if (!input) return;

    char* end = nullptr;
    long val = strtol(input->c_str(), &end, 10);
    if (end == input->c_str() || *end != '\0' || val < 0 || val > 127) {
      fmt::print("  [invalid]\n");
      return;
    }

    int ccVal = static_cast<int>(val);
    if (activeChannel_ < 0) {
      for (int c = 0; c < 16; ++c)
        fluid_synth_cc(app_.synth.get(), c, 19, ccVal);
    } else {
      fluid_synth_cc(app_.synth.get(), activeChannel_, 19, ccVal);
    }
  }

  /** Redraw the status line (non-verbose mode only). */
  void updateDisplay(unsigned int elapsed, unsigned int totalTicks) {
    if (app_.verbose) return;

    std::string chStr = (activeChannel_ < 0) ? "all" : std::to_string(activeChannel_);
    int volumePct = static_cast<int>(std::round(app_.volume * 100));

    fmt::print("\r                                                                                \r"
               "  tick {}/{} | {} {} | VOL {}% | CH {}",
               elapsed, totalTicks,
               label_, currentId(), volumePct, chStr);
    fflush(stdout);
  }
};

/* ═══════════════════ Song playback loop ═══════════════════ */

void FluidsyXApp::songLoop(const SongGroupIndex& index) {
  /* Store a reference to the SongGroupIndex so that timer callbacks can
   * resolve note events through the page→keymap/layer→SoundMacro chain. */
  activeSongGroup = &index;

  enableRawMode();

  /* Build sorted SongID list for the ConsoleUI */
  std::vector<int> songIds;
  for (const auto& [sid, setup] : index.m_midiSetups)
    songIds.push_back(sid.id);
  std::sort(songIds.begin(), songIds.end());

  ConsoleUI ui(*this, std::move(songIds), setupId, "Song");
  bool firstRun = true;

  while (g_running.load()) {
    /* (Re)initialise per-channel state for each song */
    for (auto& m : channelAdsrMap)
      m = {};

    /* Apply the MIDI setup for the selected song to FluidSynth channels */
    setupId = ui.currentId();
    std::map<SongId, std::array<SongGroupIndex::MIDISetup, 16>> sortEntries(
        index.m_midiSetups.cbegin(), index.m_midiSetups.cend());

    auto setupIt = sortEntries.find(setupId);
    if (setupIt != sortEntries.end()) {
      const auto& midiSetup = setupIt->second;

      // Init all CCs to their default "cold" values
      initCCDefault(inpColdMIDIDefaults);

      for (int ch = 0; ch < 16; ++ch) {
        channelPrograms[ch] = midiSetup[ch].programNo;
        fluid_synth_program_change(synth.get(), ch, midiSetup[ch].programNo);
        fluid_synth_cc(synth.get(), ch, 7, midiSetup[ch].volume);
        fluid_synth_cc(synth.get(), ch, 10, midiSetup[ch].panning);
        fluid_synth_cc(synth.get(), ch, 91, midiSetup[ch].reverb);
        fluid_synth_cc(synth.get(), ch, 93, midiSetup[ch].chorus);
      }
    }

    /* Schedule all song events on the FluidSynth sequencer */
    if (!selectedSong) {
      fmt::print(stderr, "fluidsyX: no song data to play\n");
      break;
    }

    double totalTicks = scheduleSongEvents(selectedSong->m_data.get(),
                                           selectedSong->m_size);
    if (totalTicks <= 0.0) {
      fmt::print(stderr, "fluidsyX: no events to play\n");
      break;
    }

    if (firstRun) {
      fmt::print("fluidsyX: playing song (setup {}, {} ticks)\n"
                 "  SPACE=panic  LEFT/RIGHT=song  UP/DOWN=vol  C=channel  S=CC19  Q=quit\n",
                 setupId, static_cast<unsigned int>(totalTicks));
      firstRun = false;
    } else {
      fmt::print("\nfluidsyX: switched to song (setup {}, {} ticks)\n",
                 setupId, static_cast<unsigned int>(totalTicks));
    }

    unsigned int startTick = fluid_sequencer_get_tick(sequencer.get());
    unsigned int endTick = startTick + static_cast<unsigned int>(totalTicks);
    double curScale = fluid_sequencer_get_time_scale(sequencer.get());
    unsigned int tailTicks = static_cast<unsigned int>(curScale * 2.0);
    unsigned int tailEnd = endTick + tailTicks;

    bool songFinished = true;
    while (g_running.load()) {
      unsigned int now = fluid_sequencer_get_tick(sequencer.get());
      unsigned int elapsed = (now > startTick) ? (now - startTick) : 0;

      auto action = ui.poll(elapsed, static_cast<unsigned int>(totalTicks));
      if (action == ConsoleUI::Action::Quit) {
        songFinished = true;
        g_running.store(false);
        break;
      }
      if (action == ConsoleUI::Action::Space) {
        /* Panic: all notes off on all channels */
        for (int c = 0; c < 16; ++c)
          fluid_synth_all_notes_off(synth.get(), c);
      }
      if (action == ConsoleUI::Action::ChangeEntry) {
        /* Stop current playback and restart with new song */
        fluid_sequencer_remove_events(sequencer.get(), -1, -1, -1);
        adriver.reset(new_fluid_audio_driver(settings.get(), synth.get()));
        for (int c = 0; c < 16; ++c)
          fluid_synth_all_notes_off(synth.get(), c);
        activeMacros.clear();
        pendingSngEvents.clear();
        setupId = ui.currentId();
        songFinished = false;
        break;
      }

      if (now >= tailEnd)
        break;

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (songFinished)
      break;
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
  fmt::print("  SPACE=keyon/keyoff  LEFT/RIGHT=SFX  UP/DOWN=vol  C=channel  S=CC19  Q=quit\n");

  /* Build sorted SFX ID list for the ConsoleUI */
  std::map<SFXId, SFXGroupIndex::SFXEntry> sortEntries(
      index.m_sfxEntries.cbegin(), index.m_sfxEntries.cend());

  std::vector<int> sfxIds;
  for (const auto& [sid, entry] : sortEntries)
    sfxIds.push_back(sid.id);

  int initialId = sfxIds.empty() ? -1 : sfxIds.front();
  bool isPlaying = false;

  enableRawMode();

  ConsoleUI ui(*this, std::move(sfxIds), initialId, "SFX");
  ui.refresh(0, 0);

  while (g_running.load()) {
    auto action = ui.poll(0, 0);

    switch (action) {
    case ConsoleUI::Action::Quit:
      goto done;

    case ConsoleUI::Action::Space: {
      int curId = ui.currentId();
      if (isPlaying) {
        /* Key off */
        fluid_synth_all_notes_off(synth.get(), 0);
        activeMacros.clear();
        isPlaying = false;
      } else if (curId >= 0) {
        /* Key on */
        auto sfxIt = sortEntries.find(curId);
        if (sfxIt != sortEntries.end()) {
          const SFXGroupIndex::SFXEntry& entry = sfxIt->second;
          const SoundMacro* macro =
              activePool ? activePool->soundMacro(entry.objId) : nullptr;
          if (macro) {
            unsigned int now = fluid_sequencer_get_tick(sequencer.get());
            enqueueSoundMacro(macro, 0, 0, entry.defKey, entry.defVel, now);
          } else {
            fmt::print(stderr, "fluidsyX: failed to resolve SoundMacro for SFX {}\n", curId);
          }
          isPlaying = true;
        }
      }
      ui.refresh(0, 0);
      break;
    }

    case ConsoleUI::Action::ChangeEntry:
      /* Stop current sound when switching SFX */
      if (isPlaying) {
        fluid_synth_all_notes_off(synth.get(), 0);
        activeMacros.clear();
        isPlaying = false;
      }
      break;

    case ConsoleUI::Action::Continue:
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      break;
    }
  }

done:
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

  /* 7. Initialise FluidSynth */
  if (!app.initFluidSynth())
    return 1;

  /* 7a. Optionally load a SoundFont for audible output */
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
