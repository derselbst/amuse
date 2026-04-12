/**
 * @file SoundMacroFluidStubs.cpp
 * @brief Weak default DoFluid implementations for all SoundMacro command subclasses.
 *
 * The real implementations live in driver/fluidsyX.cpp and are compiled only when
 * building the fluidsyX target.  These weak stubs allow other targets (e.g. amuseconv)
 * to link against libamuse.a without pulling in fluidsyX.cpp.
 *
 * When fluidsyX.cpp is linked, its strong definitions override these weak ones.
 */

#include "amuse/AudioGroupPool.hpp"
#include "amuse/FluidsyXMacroContext.hpp"

#ifdef __GNUC__
#define WEAK_DOFLUID __attribute__((weak))
#else
#define WEAK_DOFLUID
#endif

namespace amuse {

#define STUB(Cls) \
  WEAK_DOFLUID unsigned int SoundMacro::Cls::DoFluid(MacroExecContext& ctx, fluid_voice_t*) const { \
    ctx.pc++; return 0; \
  }

STUB(CmdEnd)
STUB(CmdStop)
STUB(CmdWaitTicks)
STUB(CmdWaitMs)
STUB(CmdGoto)
STUB(CmdLoop)
STUB(CmdReturn)
STUB(CmdGoSub)
STUB(CmdSetNote)
STUB(CmdAddNote)
STUB(CmdLastNote)
STUB(CmdRndNote)
STUB(CmdSetPitch)
STUB(CmdPitchWheelR)
STUB(CmdScaleVolume)
STUB(CmdScaleVolumeDLS)
STUB(CmdSetAdsrCtrl)
STUB(CmdPanning)
STUB(CmdPianoPan)
STUB(CmdStartSample)
STUB(CmdStopSample)
STUB(CmdKeyOff)
STUB(CmdSendKeyOff)
STUB(CmdPlayMacro)
STUB(CmdSplitKey)
STUB(CmdSplitVel)
STUB(CmdSplitRnd)
STUB(CmdMod2Vibrange)
STUB(CmdVibrato)
STUB(CmdPortamento)
STUB(CmdSetVar)
STUB(CmdAddVars)
STUB(CmdSubVars)
STUB(CmdMulVars)
STUB(CmdDivVars)
STUB(CmdAddIVars)
STUB(CmdIfEqual)
STUB(CmdIfLess)
STUB(CmdTrapEvent)
STUB(CmdUntrapEvent)
STUB(CmdSetAdsr)
STUB(CmdModeSelect)
STUB(CmdSRCmodeSelect)
STUB(CmdSetKeygroup)
STUB(CmdSetupLFO)
STUB(CmdSetupTremolo)
STUB(CmdTremoloSelect)
STUB(CmdWiiUnknown)
STUB(CmdWiiUnknown2)

#undef STUB

} // namespace amuse
