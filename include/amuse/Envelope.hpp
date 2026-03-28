#pragma once

#include <cstdint>

namespace amuse {
class Voice;

struct ADSR;
struct ADSRDLS;

// [0..103] -> milliseconds
constexpr std::array<int32_t, 104> MIDItoTIME{
    0,     10,    20,    30,    40,    50,    60,    70,    80,    90,    100,   110,   110,   120,   130,
    140,   150,   160,   170,   190,   200,   220,   230,   250,   270,   290,   310,   330,   350,   380,
    410,   440,   470,   500,   540,   580,   620,   660,   710,   760,   820,   880,   940,   1000,  1000,
    1100,  1200,  1300,  1400,  1500,  1600,  1700,  1800,  2000,  2100,  2300,  2400,  2600,  2800,  3000,
    3200,  3500,  3700,  4000,  4300,  4600,  4900,  5300,  5700,  6100,  6500,  7000,  7500,  8100,  8600,
    9300,  9900,  10000, 11000, 12000, 13000, 14000, 15000, 16000, 17000, 18000, 19000, 21000, 22000, 24000,
    26000, 28000, 30000, 32000, 34000, 37000, 39000, 42000, 45000, 49000, 50000, 55000, 60000, 65000,
};

/** Per-sample state tracker for ADSR envelope data */
class Envelope {
public:
  enum class State { Attack, Decay, Sustain, Release, Complete };

private:
  State m_phase = State::Attack;     /**< Current envelope state */
  double m_attackTime = 0.01;        /**< Time of attack in seconds */
  double m_decayTime = 0.0;          /**< Time of decay in seconds */
  double m_sustainFactor = 1.0;      /**< Evaluated sustain percentage */
  double m_releaseTime = 0.01;       /**< Time of release in seconds */
  double m_releaseStartFactor = 0.0; /**< Level at whenever release event occurs */
  double m_curTime = 0.0;            /**< Current time of envelope stage in seconds */
  bool m_adsrSet = false;

public:
  void reset(const ADSR* adsr);
  void reset(const ADSRDLS* adsr, int8_t note, int8_t vel);
  void keyOff(const Voice& vox);
  void keyOff();
  float advance(double dt, const Voice& vox);
  float advance(double dt);
  bool isComplete(const Voice& vox) const;
  bool isAdsrSet() const { return m_adsrSet; }
};
} // namespace amuse
