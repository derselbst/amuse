#pragma once

#include "amuse/AudioGroupPool.hpp"
#include <optional>
#include <array>
#include <cstdint>

/* ────────────── Runtime state for a single SoundMacro VM ────────────── */

namespace amuse {

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

  std::optional<SoundMacro::CmdAddNote> pendingAddNote{}; /**< AddNote command to apply at voice start */
  std::optional<SoundMacro::CmdSetKeygroup> pendingExclusiveClass{}; /**< Exclusive class to apply at voice start */
  std::optional<SoundMacro::CmdSetupLFO> pendingSetupLFO{}; /**< LFO settings to apply at voice start */

  /* ADSR controller mapping (CmdSetAdsrCtrl).
   * NOTE: In MusyX, ADSR is per-voice (bound to the SoundMacro).
   * With voice-level generators this is now per-voice in FluidSynth too. */
  bool useAdsrControllers = false;
  bool adsrBootstrapped = false;
  uint8_t midiAttack  = 0;
  uint8_t midiDecay   = 0;
  uint8_t midiSustain = 0;
  uint8_t midiRelease = 0;

  std::optional<std::tuple<uint16_t, bool>> adsrTableId{}; // Id of the ADSR table to use for this voice, if any (from CmdSetAdsr)
  /* Controller value storage (0-127 for standard MIDI CCs) */
  std::array<int8_t, 128> ctrlVals = {};

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
