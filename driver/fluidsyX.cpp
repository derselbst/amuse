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
#include "amuse/Common.hpp"

#include <fluidsynth.h>

#include <algorithm>
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

/* ────────────── Runtime state for a single SoundMacro VM ────────────── */

struct MacroExecContext {
  const SoundMacro* macro = nullptr;
  int pc = 0; /* program counter (command index) */
  uint8_t midiKey = 60;
  uint8_t midiVel = 100;
  int channel = 0;
  double ticksPerSec = 1000.0; /* default: 1 tick = 1 ms */
  int loopCountdown = -1;
  int loopStep = 0;
  bool ended = false;
  bool waitingKeyoff = false;
  bool waitingSampleEnd = false;
  /* variable bank (32 × 32-bit) */
  int32_t vars[32] = {};
};

/* ──────────────────── FluidSynth state ──────────────────── */

struct FluidsyXApp {
  /* FluidSynth objects */
  fluid_settings_t* settings = nullptr;
  fluid_synth_t* synth = nullptr;
  fluid_audio_driver_t* adriver = nullptr;
  fluid_sequencer_t* sequencer = nullptr;
  fluid_seq_id_t synthSeqId = -1;
  fluid_seq_id_t callbackSeqId = -1;

  /* Amuse parsed data */
  std::vector<std::pair<std::string, IntrusiveAudioGroupData>> data;
  std::list<AudioGroupProject> projs;
  std::vector<AudioGroupPool> pools;
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

  /* RNG */
  std::mt19937 rng{std::random_device{}()};

  /* ── lifecycle helpers ── */

  bool initFluidSynth();
  void shutdownFluidSynth();

  bool loadMusyXData(const char* path);
  int selectGroup();
  const AudioGroupPool* findPoolForGroup();

  void songLoop(const SongGroupIndex& index);
  void sfxLoop(const SFXGroupIndex& index);

  /* ── SoundMacro → FluidSynth translation ── */

  /** Enqueue all commands of a SoundMacro, starting at \p step. */
  void enqueueSoundMacro(const SoundMacro* sm, int step, int channel,
                         uint8_t key, uint8_t vel, unsigned int startTick);

  /** Process one command of a MacroExecContext, scheduling FluidSynth events.
   *  Returns the additional tick delay introduced by the command. */
  unsigned int processMacroCmd(MacroExecContext& ctx, unsigned int curTick);

  /** Timer callback trampoline */
  static void timerCallback(unsigned int time, fluid_event_t* event,
                            fluid_sequencer_t* seq, void* data);

  /** Kick off the next pending timer step for all active macros */
  void scheduleNextTimerStep();
};

/* ═══════════════════ FluidSynth init / shutdown ═══════════════════ */

bool FluidsyXApp::initFluidSynth() {
  settings = new_fluid_settings();
  if (!settings) {
    fprintf(stderr, "fluidsyX: failed to create FluidSynth settings\n");
    return false;
  }

  /* Prefer PulseAudio, fall back to ALSA, then whatever is available */
#ifndef _WIN32
  fluid_settings_setstr(settings, "audio.driver", "pulseaudio");
#endif
  fluid_settings_setnum(settings, "synth.gain", 0.5);
  fluid_settings_setint(settings, "synth.polyphony", 256);

  synth = new_fluid_synth(settings);
  if (!synth) {
    fprintf(stderr, "fluidsyX: failed to create FluidSynth synthesizer\n");
    return false;
  }

  adriver = new_fluid_audio_driver(settings, synth);
  if (!adriver) {
    fprintf(stderr, "fluidsyX: failed to create FluidSynth audio driver\n");
    return false;
  }

  /* Create sequencer (use system timer so it advances in real-time) */
  sequencer = new_fluid_sequencer2(/*use_system_timer=*/1);
  if (!sequencer) {
    fprintf(stderr, "fluidsyX: failed to create FluidSynth sequencer\n");
    return false;
  }

  /* Register the synth as a sequencer destination */
  synthSeqId = fluid_sequencer_register_fluidsynth(sequencer, synth);

  /* Register our application callback client for timer events */
  callbackSeqId = fluid_sequencer_register_client(
      sequencer, "fluidsyX", &FluidsyXApp::timerCallback, this);

  return true;
}

void FluidsyXApp::shutdownFluidSynth() {
  if (sequencer) {
    if (callbackSeqId >= 0)
      fluid_sequencer_unregister_client(sequencer, callbackSeqId);
    /* synthSeqId is cleaned up by delete_fluid_sequencer */
    delete_fluid_sequencer(sequencer);
    sequencer = nullptr;
  }
  if (adriver) {
    delete_fluid_audio_driver(adriver);
    adriver = nullptr;
  }
  if (synth) {
    delete_fluid_synth(synth);
    synth = nullptr;
  }
  if (settings) {
    delete_fluid_settings(settings);
    settings = nullptr;
  }
}

/* ═══════════════════ MusyX data loading ═══════════════════ */

bool FluidsyXApp::loadMusyXData(const char* path) {
  ContainerRegistry::Type cType = ContainerRegistry::DetectContainerType(path);
  if (cType == ContainerRegistry::Type::Invalid) {
    fprintf(stderr, "fluidsyX: invalid or missing data at '%s'\n", path);
    return false;
  }
  printf("fluidsyX: found '%s' Audio Group data\n",
         ContainerRegistry::TypeToName(cType));

  data = ContainerRegistry::LoadContainer(path);
  if (data.empty()) {
    fprintf(stderr, "fluidsyX: no groups loaded from '%s'\n", path);
    return false;
  }

  for (auto& grp : data) {
    projs.push_back(AudioGroupProject::CreateAudioGroupProject(grp.second));
    AudioGroupProject& proj = projs.back();
    totalGroups += proj.sfxGroups().size() + proj.songGroups().size();

    for (auto it = proj.songGroups().begin(); it != proj.songGroups().end();
         ++it)
      allSongGroups[it->first] = std::make_pair(&grp, it->second);

    for (auto it = proj.sfxGroups().begin(); it != proj.sfxGroups().end(); ++it)
      allSFXGroups[it->first] = std::make_pair(&grp, it->second);

    /* Also parse the pool so we can access SoundMacro commands */
    pools.push_back(AudioGroupPool::CreateAudioGroupPool(grp.second));
  }

  return true;
}

/* ═══════════════════ Group selection (interactive) ═══════════════════ */

int FluidsyXApp::selectGroup() {
  if (totalGroups == 0) {
    fprintf(stderr, "fluidsyX: empty project\n");
    return 1;
  }

  if (groupId != -1) {
    /* Already set (e.g. from song data) */
    if (allSongGroups.find(groupId) != allSongGroups.end())
      sfxGroup = false;
    else if (allSFXGroups.find(groupId) != allSFXGroups.end())
      sfxGroup = true;
    else {
      fprintf(stderr, "fluidsyX: unable to find Group %d\n", groupId);
      return 1;
    }
  } else if (totalGroups > 1) {
    printf("Multiple Audio Groups discovered:\n");
    for (const auto& pair : allSFXGroups) {
      printf("    %d %s (SFXGroup)  %zu sfx-entries\n", pair.first.id,
             pair.second.first->first.c_str(),
             pair.second.second->m_sfxEntries.size());
    }
    for (const auto& pair : allSongGroups) {
      printf("    %d %s (SongGroup)  %zu normal-pages, %zu drum-pages, "
             "%zu MIDI-setups\n",
             pair.first.id, pair.second.first->first.c_str(),
             pair.second.second->m_normPages.size(),
             pair.second.second->m_drumPages.size(),
             pair.second.second->m_midiSetups.size());
    }
    int userSel = 0;
    printf("Enter Group Number: ");
    if (scanf("%d", &userSel) <= 0) {
      fprintf(stderr, "fluidsyX: unable to parse prompt\n");
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
      fprintf(stderr, "fluidsyX: unable to find Group %d\n", userSel);
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

/* ═══════════════════ SoundMacro → FluidSynth translation ═══════════════════ */

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
  fluid_event_t* evt = new_fluid_event();

  fluid_event_set_source(evt, callbackSeqId);
  fluid_event_set_dest(evt, synthSeqId);

  switch (op) {
  /* ── Termination ── */
  case SoundMacro::CmdOp::End:
  case SoundMacro::CmdOp::Stop: {
    /* Send note-off for the voice */
    fluid_event_noteoff(evt, ctx.channel, ctx.midiKey);
    fluid_sequencer_send_at(sequencer, evt, curTick, /*absolute=*/1);
    ctx.ended = true;
    break;
  }

  /* ── Wait / Timing ── */
  case SoundMacro::CmdOp::WaitTicks: {
    auto& c = static_cast<const SoundMacro::CmdWaitTicks&>(cmd);
    if (c.keyOff) {
      ctx.waitingKeyoff = true;
    }
    if (c.sampleEnd) {
      ctx.waitingSampleEnd = true;
    }
    uint16_t ticks = c.ticksOrMs;
    if (c.msSwitch) {
      /* value is already in ms */
      delay = ticks;
    } else {
      delay = static_cast<unsigned int>(ticks * 1000.0 / ctx.ticksPerSec);
    }
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::WaitMs: {
    auto& c = static_cast<const SoundMacro::CmdWaitMs&>(cmd);
    if (c.keyOff)
      ctx.waitingKeyoff = true;
    if (c.sampleEnd)
      ctx.waitingSampleEnd = true;
    delay = c.ms;
    ctx.pc++;
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
    if (ctx.loopCountdown < 0) {
      /* First encounter: initialize loop */
      ctx.loopCountdown = (c.times == 0) ? -2 : static_cast<int>(c.times);
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
    /* Schedule a pitch bend to account for detune */
    if (c.detune != 0) {
      int bend = 8192 + static_cast<int>(c.detune) * 64;
      bend = std::clamp(bend, 0, 16383);
      fluid_event_pitch_bend(evt, ctx.channel, bend);
      fluid_sequencer_send_at(sequencer, evt, curTick, 1);
    }
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
    if (c.detune != 0) {
      int bend = 8192 + static_cast<int>(c.detune) * 64;
      bend = std::clamp(bend, 0, 16383);
      fluid_event_pitch_bend(evt, ctx.channel, bend);
      fluid_sequencer_send_at(sequencer, evt, curTick, 1);
    }
    if (c.msSwitch)
      delay = c.ticksOrMs;
    else
      delay = static_cast<unsigned int>(c.ticksOrMs * 1000.0 / ctx.ticksPerSec);
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::LastNote: {
    auto& c = static_cast<const SoundMacro::CmdLastNote&>(cmd);
    int newKey = ctx.midiKey + c.add;
    ctx.midiKey = static_cast<uint8_t>(std::clamp(newKey, 0, 127));
    if (c.detune != 0) {
      int bend = 8192 + static_cast<int>(c.detune) * 64;
      bend = std::clamp(bend, 0, 16383);
      fluid_event_pitch_bend(evt, ctx.channel, bend);
      fluid_sequencer_send_at(sequencer, evt, curTick, 1);
    }
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
    ctx.pc++;
    break;
  }

  /* ── Pitch control ── */
  case SoundMacro::CmdOp::SetPitch: {
    /* SetPitch sets an absolute frequency; convert to pitch bend */
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
    fluid_event_pitch_wheelsens(evt, ctx.channel, c.rangeUp);
    fluid_sequencer_send_at(sequencer, evt, curTick, 1);
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
    fluid_event_volume(evt, ctx.channel, vol);
    fluid_sequencer_send_at(sequencer, evt, curTick, 1);
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::ScaleVolumeDLS: {
    auto& c = static_cast<const SoundMacro::CmdScaleVolumeDLS&>(cmd);
    int vol = (ctx.midiVel * (c.scale + 32768)) / 65536;
    vol = std::clamp(vol, 0, 127);
    fluid_event_volume(evt, ctx.channel, vol);
    fluid_sequencer_send_at(sequencer, evt, curTick, 1);
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
    /* Map MIDI CC to ADSR params – advance for now */
    ctx.pc++;
    break;
  }

  /* ── Panning ── */
  case SoundMacro::CmdOp::Panning: {
    auto& c = static_cast<const SoundMacro::CmdPanning&>(cmd);
    /* panPosition is -127..127; map to MIDI 0..127 */
    int pan = (static_cast<int>(c.panPosition) + 127) / 2;
    pan = std::clamp(pan, 0, 127);
    fluid_event_pan(evt, ctx.channel, pan);
    fluid_sequencer_send_at(sequencer, evt, curTick, 1);
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::PianoPan: {
    auto& c = static_cast<const SoundMacro::CmdPianoPan&>(cmd);
    /* Compute panning based on key relative to center */
    int diff = ctx.midiKey - c.centerKey;
    int pan = c.centerPan + diff * c.scale / 127;
    pan = std::clamp((pan + 127) / 2, 0, 127);
    fluid_event_pan(evt, ctx.channel, pan);
    fluid_sequencer_send_at(sequencer, evt, curTick, 1);
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::Spanning: {
    auto& c = static_cast<const SoundMacro::CmdSpanning&>(cmd);
    int pan = (static_cast<int>(c.spanPosition) + 127) / 2;
    pan = std::clamp(pan, 0, 127);
    fluid_event_pan(evt, ctx.channel, pan);
    fluid_sequencer_send_at(sequencer, evt, curTick, 1);
    ctx.pc++;
    break;
  }

  /* ── Sample commands (no-ops without sample loading) ── */
  case SoundMacro::CmdOp::StartSample: {
    /* We cannot play the actual MusyX sample; send a note-on instead
     * so fluidsynth plays whatever SoundFont preset is loaded */
    fluid_event_noteon(evt, ctx.channel, ctx.midiKey, ctx.midiVel);
    fluid_sequencer_send_at(sequencer, evt, curTick, 1);
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::StopSample: {
    fluid_event_noteoff(evt, ctx.channel, ctx.midiKey);
    fluid_sequencer_send_at(sequencer, evt, curTick, 1);
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::KeyOff: {
    fluid_event_noteoff(evt, ctx.channel, ctx.midiKey);
    fluid_sequencer_send_at(sequencer, evt, curTick, 1);
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::SendKeyOff: {
    /* Send key-off to another voice – simplified to same channel */
    fluid_event_noteoff(evt, ctx.channel, ctx.midiKey);
    fluid_sequencer_send_at(sequencer, evt, curTick, 1);
    ctx.pc++;
    break;
  }

  /* ── PlayMacro (spawn child macro) ── */
  case SoundMacro::CmdOp::PlayMacro: {
    auto& c = static_cast<const SoundMacro::CmdPlayMacro&>(cmd);
    if (activePool && c.macro.id.id != 0xffff) {
      const SoundMacro* child = activePool->soundMacro(c.macro.id);
      if (child) {
        int childKey = std::clamp(ctx.midiKey + c.addNote, 0, 127);
        enqueueSoundMacro(child, c.macroStep.step, ctx.channel,
                          static_cast<uint8_t>(childKey), ctx.midiVel,
                          curTick);
      }
    }
    ctx.pc++;
    break;
  }

  /* ── Split / Branch ── */
  case SoundMacro::CmdOp::SplitKey: {
    auto& c = static_cast<const SoundMacro::CmdSplitKey&>(cmd);
    if (ctx.midiKey >= static_cast<uint8_t>(c.key)) {
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
    /* Translate to modulation CC */
    auto& c = static_cast<const SoundMacro::CmdVibrato&>(cmd);
    int modVal = std::clamp(static_cast<int>(c.levelNote) * 8, 0, 127);
    fluid_event_modulation(evt, ctx.channel, modVal);
    fluid_sequencer_send_at(sequencer, evt, curTick, 1);
    ctx.pc++;
    break;
  }
  case SoundMacro::CmdOp::Portamento: {
    auto& c = static_cast<const SoundMacro::CmdPortamento&>(cmd);
    /* CC 65 = portamento on/off, CC 5 = portamento time */
    bool enable = (c.portState != SoundMacro::CmdPortamento::PortState::Disable);
    fluid_event_control_change(evt, ctx.channel, 65, enable ? 127 : 0);
    fluid_sequencer_send_at(sequencer, evt, curTick, 1);
    if (enable) {
      /* Reuse event for portamento time */
      fluid_event_t* evt2 = new_fluid_event();
      fluid_event_set_source(evt2, callbackSeqId);
      fluid_event_set_dest(evt2, synthSeqId);
      uint16_t timeVal = c.ticksOrMs;
      int ccVal = std::clamp(static_cast<int>(timeVal) / 8, 0, 127);
      fluid_event_control_change(evt2, ctx.channel, 5, ccVal);
      fluid_sequencer_send_at(sequencer, evt2, curTick, 1);
      delete_fluid_event(evt2);
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

  /* ── Selector evaluators (advanced controller mapping) ── */
  case SoundMacro::CmdOp::VolSelect:
  case SoundMacro::CmdOp::PanSelect:
  case SoundMacro::CmdOp::PitchWheelSelect:
  case SoundMacro::CmdOp::ModWheelSelect:
  case SoundMacro::CmdOp::PedalSelect:
  case SoundMacro::CmdOp::PortamentoSelect:
  case SoundMacro::CmdOp::ReverbSelect:
  case SoundMacro::CmdOp::SpanSelect:
  case SoundMacro::CmdOp::DopplerSelect:
  case SoundMacro::CmdOp::TremoloSelect:
  case SoundMacro::CmdOp::PreASelect:
  case SoundMacro::CmdOp::PreBSelect:
  case SoundMacro::CmdOp::PostBSelect:
  case SoundMacro::CmdOp::AuxAFXSelect:
  case SoundMacro::CmdOp::AuxBFXSelect: {
    /* Advanced controller routing – skip for now */
    ctx.pc++;
    break;
  }

  /* ── Miscellaneous ── */
  case SoundMacro::CmdOp::SendFlag:
  case SoundMacro::CmdOp::SetPriority:
  case SoundMacro::CmdOp::AddPriority:
  case SoundMacro::CmdOp::AgeCntSpeed:
  case SoundMacro::CmdOp::AgeCntVel:
  case SoundMacro::CmdOp::AddAgeCount:
  case SoundMacro::CmdOp::SetAgeCount:
  case SoundMacro::CmdOp::SetKeygroup:
  case SoundMacro::CmdOp::SRCmodeSelect:
  case SoundMacro::CmdOp::WiiUnknown:
  case SoundMacro::CmdOp::WiiUnknown2: {
    ctx.pc++;
    break;
  }

  default: {
    /* Unknown op – skip */
    ctx.pc++;
    break;
  }
  }

  delete_fluid_event(evt);
  return delay;
}

void FluidsyXApp::enqueueSoundMacro(const SoundMacro* sm, int step,
                                    int channel, uint8_t key, uint8_t vel,
                                    unsigned int startTick) {
  MacroExecContext ctx;
  ctx.macro = sm;
  ctx.pc = step;
  ctx.channel = channel;
  ctx.midiKey = key;
  ctx.midiVel = vel;

  /* Walk through commands that execute instantly (no delay).
   * When a command introduces a delay, schedule a timer event and stop. */
  unsigned int tick = startTick;
  int safetyCounter = 0;
  while (!ctx.ended && safetyCounter < kMaxMacroCmdsPerBurst) {
    unsigned int d = processMacroCmd(ctx, tick);
    tick += d;
    safetyCounter++;
    if (d > 0 && !ctx.ended) {
      /* Store context in the stable map and schedule a timer callback */
      int macroId = nextMacroId++;
      activeMacros[macroId] = ctx;

      fluid_event_t* tevt = new_fluid_event();
      fluid_event_set_source(tevt, callbackSeqId);
      fluid_event_set_dest(tevt, callbackSeqId);
      fluid_event_timer(tevt, reinterpret_cast<void*>(static_cast<intptr_t>(macroId)));
      fluid_sequencer_send_at(sequencer, tevt, tick, /*absolute=*/1);
      delete_fluid_event(tevt);
      return;
    }
  }
}

/* Timer callback – resume a SoundMacro that was waiting */
void FluidsyXApp::timerCallback(unsigned int time, fluid_event_t* event,
                                fluid_sequencer_t* /*seq*/, void* data) {
  auto* app = static_cast<FluidsyXApp*>(data);
  if (!app || fluid_event_get_type(event) != FLUID_SEQ_TIMER)
    return;

  int macroId = static_cast<int>(
      reinterpret_cast<intptr_t>(fluid_event_get_data(event)));
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
    tick += d;
    safetyCounter++;
    if (d > 0 && !ctx.ended) {
      /* Re-schedule the timer */
      fluid_event_t* tevt = new_fluid_event();
      fluid_event_set_source(tevt, app->callbackSeqId);
      fluid_event_set_dest(tevt, app->callbackSeqId);
      fluid_event_timer(tevt, reinterpret_cast<void*>(static_cast<intptr_t>(macroId)));
      fluid_sequencer_send_at(app->sequencer, tevt, tick, /*absolute=*/1);
      delete_fluid_event(tevt);
      return;
    }
  }

  /* If the macro ended, clean it up */
  if (ctx.ended)
    app->activeMacros.erase(it);
}

/* ═══════════════════ Song playback loop ═══════════════════ */

void FluidsyXApp::songLoop(const SongGroupIndex& index) {
  printf(
      "═══════════════════════════════════════════════════════════\n"
      "  ████ ████  ┃  ████ ████ ████   ┃   ████ ████\n"
      "  ████ ████  ┃  ████ ████ ████   ┃   ████ ████\n"
      "  ▌W▐█ ▌E▐█  ┃  ▌T▐█ ▌Y▐█ ▌U▐█   ┃   ▌O▐█ ▌P▐█\n"
      "   │    │    ┃    │    │    │    ┃    │    │\n"
      "A  │ S  │ D  ┃ F  │ G  │ H  │ J  ┃ K  │ L  │ ;\n"
      "═══════════════════════════════════════════════════════════\n"
      "<left/right>: cycle MIDI setup\n"
      "<up/down>: volume\n"
      "<Z/X>: octave, <C/V>: velocity, <B/N>: channel\n"
      "<space>: PANIC (all notes off), <Q>: quit\n\n");

  std::map<SongId, std::array<SongGroupIndex::MIDISetup, 16>> sortEntries(
      index.m_midiSetups.cbegin(), index.m_midiSetups.cend());

  auto setupIt = sortEntries.cbegin();
  if (setupIt != sortEntries.cend()) {
    setupId = setupIt->first.id;

    /* Apply the MIDI setup to FluidSynth channels */
    const auto& midiSetup = setupIt->second;
    for (int ch = 0; ch < 16; ++ch) {
      fluid_synth_program_change(synth, ch, midiSetup[ch].programNo);
      fluid_synth_cc(synth, ch, 7, midiSetup[ch].volume);  /* volume */
      fluid_synth_cc(synth, ch, 10, midiSetup[ch].panning); /* pan */
      fluid_synth_cc(synth, ch, 91, midiSetup[ch].reverb);  /* reverb */
      fluid_synth_cc(synth, ch, 93, midiSetup[ch].chorus);  /* chorus */
    }
  }

  printf("  Setup %d, Chan %d, Octave %d, Vel %d, VOL %d%%\n", setupId,
         chanId, octave, velocity, static_cast<int>(std::round(volume * 100)));

  enableRawMode();

  while (g_running.load()) {
    int key = readKey();
    if (key < 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    int ch = tolower(key);
    bool updateDisp = false;

    if (ch == 'q') {
      break;
    }

    /* Keyboard → MIDI note mapping (same layout as amuseplay) */
    int noteOff = -1;
    switch (ch) {
    case 'a': noteOff = 0; break;
    case 'w': noteOff = 1; break;
    case 's': noteOff = 2; break;
    case 'e': noteOff = 3; break;
    case 'd': noteOff = 4; break;
    case 'f': noteOff = 5; break;
    case 't': noteOff = 6; break;
    case 'g': noteOff = 7; break;
    case 'y': noteOff = 8; break;
    case 'h': noteOff = 9; break;
    case 'u': noteOff = 10; break;
    case 'j': noteOff = 11; break;
    case 'k': noteOff = 12; break;
    case 'o': noteOff = 13; break;
    case 'l': noteOff = 14; break;
    case 'p': noteOff = 15; break;
    case ';':
    case ':': noteOff = 16; break;
    default: break;
    }

    if (noteOff >= 0) {
      int midiNote = (octave + 1) * 12 + noteOff;
      midiNote = std::clamp(midiNote, 0, 127);

      /* Look up the SoundMacro for this program/note via the PageEntry */
      uint8_t prog = 0;
      auto setupSearch = sortEntries.find(setupId);
      if (setupSearch != sortEntries.end())
        prog = setupSearch->second[chanId].programNo;

      /* Find page entry for this program number */
      const SoundMacro* macro = nullptr;
      auto pageIt = index.m_normPages.find(prog);
      if (pageIt != index.m_normPages.end() && activePool) {
        macro = activePool->soundMacro(pageIt->second.objId);
      }

      if (macro) {
        /* Enqueue macro through our translator */
        unsigned int now = fluid_sequencer_get_tick(sequencer);
        enqueueSoundMacro(macro, 0, chanId, static_cast<uint8_t>(midiNote),
                          static_cast<uint8_t>(velocity), now);
      } else {
        /* Fall back to direct FluidSynth note-on */
        fluid_synth_noteon(synth, chanId, midiNote, velocity);
      }
      continue;
    }

    /* Control keys */
    switch (ch) {
    case ' ':
      /* PANIC: all notes off on all channels */
      for (int c = 0; c < 16; ++c)
        fluid_synth_all_notes_off(synth, c);
      activeMacros.clear();
      break;
    case 'z':
      octave = static_cast<int8_t>(std::clamp(static_cast<int>(octave) - 1, -1, 8));
      updateDisp = true;
      break;
    case 'x':
      octave = static_cast<int8_t>(std::clamp(static_cast<int>(octave) + 1, -1, 8));
      updateDisp = true;
      break;
    case 'c':
      velocity = static_cast<int8_t>(std::clamp(static_cast<int>(velocity) - 1, 0, 127));
      updateDisp = true;
      break;
    case 'v':
      velocity = static_cast<int8_t>(std::clamp(static_cast<int>(velocity) + 1, 0, 127));
      updateDisp = true;
      break;
    case 'b':
      chanId = std::clamp(chanId - 1, 0, 15);
      updateDisp = true;
      break;
    case 'n':
      chanId = std::clamp(chanId + 1, 0, 15);
      updateDisp = true;
      break;
    case 27: {
      /* Escape sequence for arrow keys */
      int next1 = readKey();
      if (next1 == '[') {
        int next2 = readKey();
        switch (next2) {
        case 'D': /* Left arrow – previous setup */
          if (setupIt != sortEntries.cbegin()) {
            --setupIt;
            setupId = setupIt->first.id;
            const auto& ms = setupIt->second;
            for (int c = 0; c < 16; ++c) {
              fluid_synth_program_change(synth, c, ms[c].programNo);
              fluid_synth_cc(synth, c, 7, ms[c].volume);
              fluid_synth_cc(synth, c, 10, ms[c].panning);
              fluid_synth_cc(synth, c, 91, ms[c].reverb);
              fluid_synth_cc(synth, c, 93, ms[c].chorus);
            }
            updateDisp = true;
          }
          break;
        case 'C': { /* Right arrow – next setup */
          auto nextIt = setupIt;
          ++nextIt;
          if (nextIt != sortEntries.cend()) {
            setupIt = nextIt;
            setupId = setupIt->first.id;
            const auto& ms = setupIt->second;
            for (int c = 0; c < 16; ++c) {
              fluid_synth_program_change(synth, c, ms[c].programNo);
              fluid_synth_cc(synth, c, 7, ms[c].volume);
              fluid_synth_cc(synth, c, 10, ms[c].panning);
              fluid_synth_cc(synth, c, 91, ms[c].reverb);
              fluid_synth_cc(synth, c, 93, ms[c].chorus);
            }
            updateDisp = true;
          }
          break;
        }
        case 'A': /* Up arrow – volume up */
          volume = std::clamp(volume + 0.05f, 0.f, 1.f);
          fluid_synth_set_gain(synth, volume);
          updateDisp = true;
          break;
        case 'B': /* Down arrow – volume down */
          volume = std::clamp(volume - 0.05f, 0.f, 1.f);
          fluid_synth_set_gain(synth, volume);
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
      printf("\r                                                                "
             "                \r  Setup %d, Chan %d, Octave %d, Vel %d, VOL "
             "%d%%",
             setupId, chanId, octave, velocity,
             static_cast<int>(std::round(volume * 100)));
      fflush(stdout);
    }
  }

  /* Clean up active macros */
  for (int c = 0; c < 16; ++c)
    fluid_synth_all_notes_off(synth, c);
  activeMacros.clear();
  printf("\n");
}

/* ═══════════════════ SFX playback loop ═══════════════════ */

void FluidsyXApp::sfxLoop(const SFXGroupIndex& index) {
  printf("<space>: keyon/keyoff, <left/right>: cycle SFX, "
         "<up/down>: volume, <Q>: quit\n");

  std::map<SFXId, SFXGroupIndex::SFXEntry> sortEntries(
      index.m_sfxEntries.cbegin(), index.m_sfxEntries.cend());

  auto sfxIt = sortEntries.cbegin();
  int sfxId = -1;
  bool isPlaying = false;

  if (sfxIt != sortEntries.cend()) {
    sfxId = sfxIt->first.id;
    printf("  SFX %d, VOL %d%%\n", sfxId,
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
        fluid_synth_all_notes_off(synth, 0);
        activeMacros.clear();
        isPlaying = false;
      } else if (sfxIt != sortEntries.cend()) {
        /* Key on */
        const SFXGroupIndex::SFXEntry& entry = sfxIt->second;
        const SoundMacro* macro =
            activePool ? activePool->soundMacro(entry.objId) : nullptr;
        if (macro) {
          unsigned int now = fluid_sequencer_get_tick(sequencer);
          enqueueSoundMacro(macro, 0, 0, entry.defKey, entry.defVel, now);
        } else {
          fluid_synth_noteon(synth, 0, entry.defKey, entry.defVel);
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
          fluid_synth_set_gain(synth, volume);
          updateDisp = true;
          break;
        case 'B': /* Down – volume down */
          volume = std::clamp(volume - 0.05f, 0.f, 1.f);
          fluid_synth_set_gain(synth, volume);
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
      printf("\r                                                                "
             "                \r  %c SFX %d, VOL %d%%",
             isPlaying ? '>' : ' ', sfxId,
             static_cast<int>(std::round(volume * 100)));
      fflush(stdout);
    }
  }

  fluid_synth_all_notes_off(synth, 0);
  activeMacros.clear();
  printf("\n");
}

/* ═══════════════════ main ═══════════════════ */

int main(int argc, char** argv) {
  signal(SIGINT, signalHandler);
#ifndef _WIN32
  signal(SIGTERM, signalHandler);
#endif

  FluidsyXApp app;

  if (argc < 2) {
    fprintf(stderr,
            "Usage: fluidsyX <musyx-group-path> [soundfont.sf2]\n"
            "\n"
            "  Plays MusyX SoundMacro data using FluidSynth.\n"
            "  An optional SoundFont can be specified for audible output.\n");
    return 1;
  }

  /* 1. Initialise FluidSynth */
  if (!app.initFluidSynth())
    return 1;

  /* 2. Optionally load a SoundFont for audible output */
  if (argc >= 3) {
    int sfId = fluid_synth_sfload(app.synth, argv[2], /*reset_presets=*/1);
    if (sfId < 0) {
      fprintf(stderr,
              "fluidsyX: warning: failed to load SoundFont '%s' – "
              "sequencer events will still be processed but may be inaudible\n",
              argv[2]);
    } else {
      printf("fluidsyX: loaded SoundFont '%s' (id=%d)\n", argv[2], sfId);
    }
  } else {
    printf("fluidsyX: no SoundFont specified – sequencer events will be "
           "processed but may be inaudible\n"
           "          (pass a .sf2 file as second argument for audio)\n\n");
  }

  /* 3. Load MusyX data */
  if (!app.loadMusyXData(argv[1]))
    return 1;

  /* 4. Attempt loading song data */
  std::vector<std::pair<std::string, ContainerRegistry::SongData>> songs =
      ContainerRegistry::LoadSongs(argv[1]);

  if (!songs.empty()) {
    bool play = true;
    if (songs.size() > 1) {
      printf("Multiple Songs discovered:\n");
      int idx = 0;
      for (const auto& pair : songs) {
        printf("    %d %s (Group %d, Setup %d)\n", idx++,
               pair.first.c_str(), pair.second.m_groupId,
               pair.second.m_setupId);
      }
      int userSel = 0;
      printf("Enter Song Number: ");
      if (scanf("%d", &userSel) <= 0 ||
          userSel < 0 ||
          userSel >= static_cast<int>(songs.size())) {
        fprintf(stderr, "fluidsyX: invalid song selection\n");
        app.shutdownFluidSynth();
        return 1;
      }
      while (getchar() != '\n')
        ;
      app.groupId = songs[userSel].second.m_groupId;
      app.setupId = songs[userSel].second.m_setupId;
    } else {
      /* Ask Y/N for single song */
      printf("Play Song '%s'? (Y/N): ", songs[0].first.c_str());
      char yn = 0;
      if (scanf(" %c", &yn) > 0 && tolower(yn) == 'y') {
        app.groupId = songs[0].second.m_groupId;
        app.setupId = songs[0].second.m_setupId;
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
    fprintf(stderr, "fluidsyX: unable to find pool for group %d\n",
            app.groupId);
    app.shutdownFluidSynth();
    return 1;
  }

  printf("fluidsyX: group %d selected (%s), %zu SoundMacros in pool\n",
         app.groupId, app.sfxGroup ? "SFX" : "Song",
         app.activePool->soundMacros().size());

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
  printf("fluidsyX: goodbye\n");
  return 0;
}
