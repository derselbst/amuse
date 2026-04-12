#pragma once

#include "amuse/AudioGroupPool.hpp"
#include <optional>
#include <array>
#include <cstdint>
#include <vector>

/* ────────────── Runtime state for a single SoundMacro VM ────────────── */

namespace amuse {

struct MacroExecContext {
  const SoundMacro* macro = nullptr;
  int pc = 0; /* program counter (command index) */
  uint8_t curNote = 60;  /**< Current note (MusyX: SYNTH_VOICE::curNote). Modified by SetNote/AddNote/etc. */
  uint8_t midiVel = 100;
  uint8_t orgNote = 60;  /**< Original trigger note (MusyX: SYNTH_VOICE::orgNote). Set once at voice start, never modified. */
  int channel = 0;
  unsigned int macStartTime = 0; // time in ticks when the macro started, used for relative timed commands/events
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

  /* ── Pending SoundMacro commands ──
   * Commands executed before CmdStartSample creates the voice are stored
   * here. We cannot allocate the voice earlier (e.g. in FluidsyXApp::enqueueSoundMacro()), because fluidsynth wants to know
   * about the sample we're going to use for that voice, which we will only know once CmdStartSample arrived.
   * Once the voice is allocated in dummy_preset_noteon(), all pending
   * commands are replayed against the new voice. */
  std::vector<const SoundMacro::ICmd*> pendingCmds;

  /* ── Opaque pointer to the FluidsyXApp instance ──
   * DoFluid() implementations that need access to app-level state
   * (activePool, sequencer, etc.) cast this to FluidsyXApp*. */
  void* appData = nullptr;

  /* ADSR controller mapping (CmdSetAdsrCtrl).
   * NOTE: In MusyX, ADSR is per-voice (bound to the SoundMacro).
   * With voice-level generators this is now per-voice in FluidSynth too. */
  bool useAdsrControllers = false;
  uint8_t midiAttack  = 0;
  uint8_t midiDecay   = 0;
  uint8_t midiSustain = 0;
  uint8_t midiRelease = 0;

  std::optional<std::tuple<uint16_t, bool>> adsrTableId{}; // Id of the ADSR table to use for this voice, if any (from CmdSetAdsr)

  /* Accumulated fine-tune in cents, from SoundMacro detune commands.
   * MusyX samples do not carry per-sample sub-semitone fine-tuning;
   * fine-tuning is applied entirely at the command level via SetNote,
   * AddNote, LastNote, RndNote (all have a ±99-cent 'detune' field)
   * and SetPitch (absolute Hz + 1/65536 Hz fine). */
  int curDetune = 0;

  /* ── Event traps (TRAP_EVENT / UNTRAP_EVENT) ──
   * A trap registers a "when event X fires, redirect execution to macro M
   * step S" entry.  In amuse, traps are stored on the Voice struct and
   * checked when keyOff / sampleEnd / message events arrive.
   * The trap is cleared after firing (one-shot, matching musyx ExecuteTrap
   * which sets trapEventAddr[trapType] = NULL on fire). */
  struct EventTrap {
    uint16_t macroId = 0xffff;  /**< 0xffff = no trap registered */
    uint16_t macroStep = 0;
    bool isSet() const { return macroId != 0xffff; }
    void clear() { macroId = 0xffff; macroStep = 0; }
  };
  EventTrap keyoffTrap;
  EventTrap sampleEndTrap;
  EventTrap messageTrap;
};

} // namespace amuse
