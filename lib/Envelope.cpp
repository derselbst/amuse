#include "amuse/Envelope.hpp"

#include <array>

#include "amuse/AudioGroupPool.hpp"
#include "amuse/Voice.hpp"

namespace amuse {

void Envelope::reset(const ADSR* adsr) {
  m_phase = State::Attack;
  m_curTime = 0.0;
  m_attackTime = adsr->getAttack();
  m_decayTime = adsr->getDecay();
  m_sustainFactor = adsr->getSustain();
  m_releaseTime = adsr->getRelease();
  m_releaseStartFactor = 0.0;
  m_adsrSet = true;
}

void Envelope::reset(const ADSRDLS* adsr, int8_t note, int8_t vel) {
  m_phase = State::Attack;
  m_curTime = 0.0;
  m_attackTime = adsr->getVelToAttack(vel);
  m_decayTime = adsr->getKeyToDecay(note);
  m_sustainFactor = adsr->getSustain();
  m_releaseTime = adsr->getRelease();
  m_releaseStartFactor = 0.0;
  m_adsrSet = true;
}

void Envelope::keyOff(const Voice& vox) {
  double releaseTime = m_releaseTime;
  if (vox.m_state.m_useAdsrControllers) {
    releaseTime = MIDItoTIME[std::clamp(int(vox.getCtrlValue(vox.m_state.m_midiRelease)), 0, 103)] / 1000.0;
  }

  m_phase = (releaseTime != 0.0) ? State::Release : State::Complete;
  m_curTime = 0.0;
}

void Envelope::keyOff() {
  m_phase = (m_releaseTime != 0.0) ? State::Release : State::Complete;
  m_curTime = 0.0;
}

float Envelope::advance(double dt, const Voice& vox) {
  if (!m_adsrSet && !vox.m_state.m_useAdsrControllers)
    return 1.f;

  double thisTime = m_curTime;
  m_curTime += dt;

  switch (m_phase) {
  case State::Attack: {
    double attackTime = m_attackTime;
    if (vox.m_state.m_useAdsrControllers) {
      attackTime = MIDItoTIME[std::clamp(int(vox.getCtrlValue(vox.m_state.m_midiAttack)), 0, 103)] / 1000.0;
    }

    if (attackTime == 0.0) {
      m_phase = State::Decay;
      m_curTime = 0.0;
      m_releaseStartFactor = 1.f;
      return 1.f;
    }
    double attackFac = thisTime / attackTime;
    if (attackFac >= 1.0) {
      m_phase = State::Decay;
      m_curTime = 0.0;
      m_releaseStartFactor = 1.f;
      return 1.f;
    }
    m_releaseStartFactor = attackFac;
    return attackFac;
  }
  case State::Decay: {
    double decayTime = m_decayTime;
    if (vox.m_state.m_useAdsrControllers) {
      decayTime = MIDItoTIME[std::clamp(int(vox.getCtrlValue(vox.m_state.m_midiDecay)), 0, 103)] / 1000.0;
    }

    double sustainFactor = m_sustainFactor;
    if (vox.m_state.m_useAdsrControllers) {
      sustainFactor = std::clamp(int(vox.getCtrlValue(vox.m_state.m_midiSustain)), 0, 127) / 127.0;
    }

    if (decayTime == 0.0) {
      m_phase = State::Sustain;
      m_curTime = 0.0;
      m_releaseStartFactor = sustainFactor;
      return sustainFactor;
    }
    double decayFac = thisTime / decayTime;
    if (decayFac >= 1.0) {
      m_phase = State::Sustain;
      m_curTime = 0.0;
      m_releaseStartFactor = sustainFactor;
      return sustainFactor;
    }
    m_releaseStartFactor = (1.0 - decayFac) + decayFac * sustainFactor;
    return m_releaseStartFactor;
  }
  case State::Sustain: {
    double sustainFactor = m_sustainFactor;
    if (vox.m_state.m_useAdsrControllers) {
      sustainFactor = std::clamp(int(vox.getCtrlValue(vox.m_state.m_midiSustain)), 0, 127) / 127.0;
    }

    return sustainFactor;
  }
  case State::Release: {
    double releaseTime = m_releaseTime;
    if (vox.m_state.m_useAdsrControllers) {
      releaseTime = MIDItoTIME[std::clamp(int(vox.getCtrlValue(vox.m_state.m_midiRelease)), 0, 103)] / 1000.0;
    }

    if (releaseTime == 0.0) {
      m_phase = State::Complete;
      return 0.f;
    }
    double releaseFac = thisTime / releaseTime;
    if (releaseFac >= 1.0) {
      m_phase = State::Complete;
      return 0.f;
    }
    return std::min(m_releaseStartFactor, 1.0 - releaseFac);
  }
  case State::Complete:
  default:
    return 0.f;
  }
}

float Envelope::advance(double dt) {
  if (!m_adsrSet)
    return 1.f;

  double thisTime = m_curTime;
  m_curTime += dt;

  switch (m_phase) {
  case State::Attack: {
    double attackTime = m_attackTime;

    if (attackTime == 0.0) {
      m_phase = State::Decay;
      m_curTime = 0.0;
      m_releaseStartFactor = 1.f;
      return 1.f;
    }
    double attackFac = thisTime / attackTime;
    if (attackFac >= 1.0) {
      m_phase = State::Decay;
      m_curTime = 0.0;
      m_releaseStartFactor = 1.f;
      return 1.f;
    }
    m_releaseStartFactor = attackFac;
    return attackFac;
  }
  case State::Decay: {
    double decayTime = m_decayTime;
    double sustainFactor = m_sustainFactor;

    if (decayTime == 0.0) {
      m_phase = State::Sustain;
      m_curTime = 0.0;
      m_releaseStartFactor = sustainFactor;
      return sustainFactor;
    }
    double decayFac = thisTime / decayTime;
    if (decayFac >= 1.0) {
      m_phase = State::Sustain;
      m_curTime = 0.0;
      m_releaseStartFactor = sustainFactor;
      return sustainFactor;
    }
    m_releaseStartFactor = (1.0 - decayFac) + decayFac * sustainFactor;
    return m_releaseStartFactor;
  }
  case State::Sustain: {
    return m_sustainFactor;
  }
  case State::Release: {
    double releaseTime = m_releaseTime;

    if (releaseTime == 0.0) {
      m_phase = State::Complete;
      return 0.f;
    }
    double releaseFac = thisTime / releaseTime;
    if (releaseFac >= 1.0) {
      m_phase = State::Complete;
      return 0.f;
    }
    return std::min(m_releaseStartFactor, 1.0 - releaseFac);
  }
  case State::Complete:
  default:
    return 0.f;
  }
}

bool Envelope::isComplete(const Voice& vox) const {
  return (m_adsrSet || vox.m_state.m_useAdsrControllers) && m_phase == State::Complete;
}
} // namespace amuse
