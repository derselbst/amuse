#pragma once

#include <array>
#include <cstdint>

#include "amuse/Entity.hpp"

namespace amuse {
class Sequencer;

enum class SongPlayState { Stopped, Playing };

/** Real-time state of Song execution */
class SongState {
  friend class SongConverter;
  friend class Voice;

  /** Song header */
  struct Header {
    uint32_t m_trackIdxOff;
    uint32_t m_regionIdxOff;
    uint32_t m_chanMapOff;
    uint32_t m_tempoTableOff;
    uint32_t m_initialTempo; /* Top bit indicates per-channel looping */
    uint32_t m_loopStartTicks[16];
    uint32_t m_chanMapOff2;
    void swapToBig();
    void swapFromBig();
    Header& operator=(const Header& other);
    Header(const Header& other) { *this = other; }
    Header() = default;
  } m_header;

  /** Track region ('clip' in an NLA representation) */
  struct TrackRegion {
    uint32_t m_startTick;
    uint8_t m_progNum;
    uint8_t m_unk1;
    uint16_t m_unk2;
    int16_t m_regionIndex; /* -1 to terminate song, -2 to loop to previous region */
    int16_t m_loopToRegion;
    bool indexDone(bool bigEndian, bool loop) const;
    bool indexValid(bool bigEndian) const;
    int indexLoop(bool bigEndian) const;
  };

  /** Tempo change entry */
  struct TempoChange {
    uint32_t m_tick;  /**< Relative song ticks from previous tempo change */
    uint32_t m_tempo; /**< Tempo value in beats-per-minute (at 384 ticks per quarter-note) */
    void swapBig();
  };

  const unsigned char* m_songData = nullptr; /**< Base pointer to active song */
  int m_sngVersion;                          /**< Detected song revision, 1 has RLE-compressed delta-times */
  bool m_bigEndian;                          /**< True if loaded song is big-endian data */

  /** State of a single track within arrangement */
  struct Track {
    struct Header {
      uint32_t m_type;
      uint32_t m_pitchOff;
      uint32_t m_modOff;
      void swapBig();
    };

    SongState* m_parent = nullptr;
    uint8_t m_midiChan = 0xff;                 /**< MIDI channel number of song channel */
    const TrackRegion* m_initRegion = nullptr; /**< Pointer to first track region */
    const TrackRegion* m_curRegion = nullptr;  /**< Pointer to currently-playing track region */
    const TrackRegion* m_nextRegion = nullptr; /**< Pointer to next-queued track region */

    double m_remDt = 0.0;         /**< Remaining dt for keeping remainder between cycles */
    uint32_t m_curTick = 0;       /**< Current playback position for this track */
    uint32_t m_loopStartTick = 0; /**< Tick to loop back to */
    /** Current pointer to tempo control, iterated over playback */
    const TempoChange* m_tempoPtr = nullptr;
    uint32_t m_tempo = 0; /**< Current tempo (beats per minute) */

    const unsigned char* m_data = nullptr;           /**< Pointer to upcoming command data */
    const unsigned char* m_pitchWheelData = nullptr; /**< Pointer to upcoming pitch data */
    const unsigned char* m_modWheelData = nullptr;   /**< Pointer to upcoming modulation data */
    int32_t m_pitchVal = 0;                          /**< Accumulated value of pitch */
    uint32_t m_nextPitchTick = 0;                    /**< Upcoming position of pitch wheel change */
    int32_t m_nextPitchDelta = 0;                    /**< Upcoming delta value of pitch */
    int32_t m_modVal = 0;                            /**< Accumulated value of mod */
    uint32_t m_nextModTick = 0;                      /**< Upcoming position of mod wheel change */
    int32_t m_nextModDelta = 0;                      /**< Upcoming delta value of mod */
    std::array<int, 128> m_remNoteLengths = {};      /**< Remaining ticks per note */

    int32_t m_eventWaitCountdown = 0; /**< Current wait in ticks */
    int32_t m_lastN64EventTick =
        0; /**< Last command time on this channel (for computing delta times from absolute times in N64 songs) */

    Track() = default;
    Track(SongState& parent, uint8_t midiChan, uint32_t loopStart, const TrackRegion* regions, uint32_t tempo);
    explicit operator bool() const { return m_parent != nullptr; }
    void setRegion(const TrackRegion* region);
    void advanceRegion();
    bool advance(Sequencer& seq, double dt);
    void resetTempo();
  };
  std::array<Track, 64> m_tracks;
  const uint32_t* m_regionIdx; /**< Table of offsets to song-region data */

  SongPlayState m_songState = SongPlayState::Playing; /**< High-level state of Song playback */
  bool m_loop = true;                                 /**< Enable looping */

public:
  /** Determine SNG version
   *  @param isBig returns true if big-endian SNG
   *  @return 0 for initial version, 1 for delta-time revision, -1 for non-SNG */
  static int DetectVersion(const unsigned char* ptr, bool& isBig);

  /** initialize state for Song data at `ptr` */
  bool initialize(const unsigned char* ptr, bool loop);

  uint32_t getInitialTempo() const { return m_header.m_initialTempo & 0x7fffffff; }

  /** advances `dt` seconds worth of commands in the Song
   *  @return `true` if END reached
   */
  bool advance(Sequencer& seq, double dt);
};


/* ── SNG variable-length decode helpers ── */

static inline uint16_t DecodeUnsignedValue(const unsigned char*& data) {
  uint16_t ret;
  if (data[0] & 0x80) {
    ret = data[1] | ((data[0] & 0x7f) << 8);
    data += 2;
  } else {
    ret = data[0];
    data += 1;
  }
  return ret;
}

static inline void EncodeUnsignedValue(std::vector<uint8_t>& vecOut, uint16_t val) {
  if (val >= 128) {
    vecOut.push_back(0x80 | ((val >> 8) & 0x7f));
    vecOut.push_back(val & 0xff);
  } else {
    vecOut.push_back(val & 0x7f);
  }
}

static inline int16_t DecodeSignedValue(const unsigned char*& data) {
  int16_t ret;
  if (data[0] & 0x80) {
    ret = data[1] | ((data[0] & 0x7f) << 8);
    ret |= (ret & 0x4000) << 1;
    data += 2;
  } else {
    // Must cast to int8_t first to get sign-extension from bit 7
    ret = int8_t(data[0] | ((data[0] << 1) & 0x80));
    data += 1;
  }
  return ret;
}

static inline void EncodeSignedValue(std::vector<uint8_t>& vecOut, int16_t val) {
  if (val >= 64 || val < -64) {
    vecOut.push_back(0x80 | ((val >> 8) & 0x7f));
    vecOut.push_back(val & 0xff);
  } else {
    vecOut.push_back(val & 0x7f);
  }
}

static inline std::pair<uint32_t, int32_t> DecodeDelta(const unsigned char*& data) {
  std::pair<uint32_t, int32_t> ret = {};
  do {
    if (data[0] == 0x80 && data[1] == 0x00)
      break;
    ret.first += DecodeUnsignedValue(data);
    ret.second = DecodeSignedValue(data);
  } while (ret.second == 0);
  return ret;
}

static inline void EncodeDelta(std::vector<uint8_t>& vecOut, uint32_t deltaTime, int32_t val) {
  while (deltaTime > 32767) {
    EncodeUnsignedValue(vecOut, 32767);
    EncodeSignedValue(vecOut, 0);
    deltaTime -= 32767;
  }
  EncodeUnsignedValue(vecOut, deltaTime);
  EncodeSignedValue(vecOut, val);
}

static inline uint32_t DecodeTime(const unsigned char*& data) {
  uint32_t ret = 0;

  while (true) {
    uint16_t thisPart = SBig(*reinterpret_cast<const uint16_t*>(data));
    uint16_t nextPart = *reinterpret_cast<const uint16_t*>(data + 2);
    if (nextPart == 0) {
      // Automatically consume no-op command as continued time
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
} // namespace amuse
