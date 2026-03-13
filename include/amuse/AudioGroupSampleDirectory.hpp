#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "amuse/Common.hpp"

namespace amuse {
class AudioGroupData;
class AudioGroupDatabase;

struct DSPADPCMHeader : BigDNA {
  AT_DECL_DNA
  Value<uint32_t> x0_num_samples;
  Value<uint32_t> x4_num_nibbles;
  Value<uint32_t> x8_sample_rate;
  Value<uint16_t> xc_loop_flag = 0;
  Value<uint16_t> xe_format = 0; /* 0 for ADPCM */
  Value<uint32_t> x10_loop_start_nibble = 0;
  Value<uint32_t> x14_loop_end_nibble = 0;
  Value<uint32_t> x18_ca = 0;
  int16_t x1c_coef[8][2]{};
  Value<int16_t> x3c_gain = 0;
  Value<int16_t> x3e_ps = 0;
  Value<int16_t> x40_hist1 = 0;
  Value<int16_t> x42_hist2 = 0;
  Value<int16_t> x44_loop_ps = 0;
  Value<int16_t> x46_loop_hist1 = 0;
  Value<int16_t> x48_loop_hist2 = 0;
  Value<uint8_t> m_pitch = 0; // Stash this in the padding
  Seek<21, amuse::SeekOrigin::Current> pad;
};

struct VADPCMHeader : BigDNA {
  AT_DECL_DNA
  Value<uint32_t> m_pitchSampleRate;
  Value<uint32_t> m_numSamples;
  Value<uint32_t> m_loopStartSample;
  Value<uint32_t> m_loopLengthSamples;
};

struct WAVFormatChunk : LittleDNA {
  AT_DECL_DNA
  Value<uint16_t> sampleFmt = 1;
  Value<uint16_t> numChannels = 1;
  Value<uint32_t> sampleRate;
  Value<uint32_t> byteRate;       // sampleRate * numChannels * bitsPerSample/8
  Value<uint16_t> blockAlign = 2; // numChannels * bitsPerSample/8
  Value<uint16_t> bitsPerSample = 16;
};

struct WAVSampleChunk : LittleDNA {
  AT_DECL_DNA
  Value<uint32_t> smplManufacturer = 0;
  Value<uint32_t> smplProduct = 0;
  Value<uint32_t> smplPeriod; // 1 / sampleRate in nanoseconds
  Value<uint32_t> midiNote;   // native MIDI note of sample
  Value<uint32_t> midiPitchFrac = 0;
  Value<uint32_t> smpteFormat = 0;
  Value<uint32_t> smpteOffset = 0;
  Value<uint32_t> numSampleLoops = 0;
  Value<uint32_t> additionalDataSize = 0;
};

struct WAVSampleLoop : LittleDNA {
  AT_DECL_DNA
  Value<uint32_t> cuePointId = 0;
  Value<uint32_t> loopType = 0; // 0: forward loop
  Value<uint32_t> start;
  Value<uint32_t> end;
  Value<uint32_t> fraction = 0;
  Value<uint32_t> playCount = 0;
};

struct WAVHeader : LittleDNA {
  AT_DECL_DNA
  Value<uint32_t> riffMagic = SBIG('RIFF');
  Value<uint32_t> wavChuckSize; // everything - 8
  Value<uint32_t> wavMagic = SBIG('WAVE');

  Value<uint32_t> fmtMagic = SBIG('fmt ');
  Value<uint32_t> fmtChunkSize = 16;
  WAVFormatChunk fmtChunk;

  Value<uint32_t> smplMagic = SBIG('smpl');
  Value<uint32_t> smplChunkSize = 36; // 36 + numSampleLoops*24
  WAVSampleChunk smplChunk;

  Value<uint32_t> dataMagic = SBIG('data');
  Value<uint32_t> dataChunkSize; // numSamples * numChannels * bitsPerSample/8
};

struct WAVHeaderLoop : LittleDNA {
  AT_DECL_DNA
  Value<uint32_t> riffMagic = SBIG('RIFF');
  Value<uint32_t> wavChuckSize; // everything - 8
  Value<uint32_t> wavMagic = SBIG('WAVE');

  Value<uint32_t> fmtMagic = SBIG('fmt ');
  Value<uint32_t> fmtChunkSize = 16;
  WAVFormatChunk fmtChunk;

  Value<uint32_t> smplMagic = SBIG('smpl');
  Value<uint32_t> smplChunkSize = 60; // 36 + numSampleLoops*24
  WAVSampleChunk smplChunk;
  WAVSampleLoop sampleLoop;

  Value<uint32_t> dataMagic = SBIG('data');
  Value<uint32_t> dataChunkSize; // numSamples * numChannels * bitsPerSample/8
};

enum class SampleFormat : uint8_t {
  DSP,      /**< GCN DSP-ucode ADPCM (very common for GameCube games) */
  DSP_DRUM, /**< GCN DSP-ucode ADPCM (seems to be set into drum samples for expanding their amplitude appropriately)
             */
  PCM,      /**< Big-endian PCM found in MusyX2 demo GM instruments */
  N64,      /**< 2-stage VADPCM coding with SAMP-embedded codebooks */
  PCM_PC    /**< Little-endian PCM found in PC Rogue Squadron (actually enum 0 which conflicts with DSP-ADPCM) */
};

enum class SampleFileState {
  NoData,
  MemoryOnlyWAV,
  MemoryOnlyCompressed,
  WAVRecent,
  CompressedRecent,
  CompressedNoWAV,
  WAVNoCompressed
};

/** Indexes individual samples in SAMP chunk */
class AudioGroupSampleDirectory {
  friend class AudioGroup;

public:
  union ADPCMParms {
    struct DSPParms {
      uint16_t m_bytesPerFrame;
      uint8_t m_ps;
      uint8_t m_lps;
      int16_t m_hist2;
      int16_t m_hist1;
      int16_t m_coefs[8][2];
    } dsp;
    struct VADPCMParms {
      int16_t m_coefs[8][2][8];
    } vadpcm;
    void swapBigDSP();
    void swapBigVADPCM();
  };

  template <std::endian DNAEn>
  struct AT_SPECIALIZE_PARMS(std::endian::big, std::endian::little) EntryDNA : BigDNA {
    AT_DECL_DNA
    SampleIdDNA<DNAEn> m_sfxId;
    Seek<2, amuse::SeekOrigin::Current> pad;
    Value<uint32_t, DNAEn> m_sampleOff;
    Value<uint32_t, DNAEn> m_unk;
    Value<uint8_t, DNAEn> m_pitch;
    Seek<1, amuse::SeekOrigin::Current> pad2;
    Value<uint16_t, DNAEn> m_sampleRate;
    Value<uint32_t, DNAEn> m_numSamples; // Top 8 bits is SampleFormat
    Value<uint32_t, DNAEn> m_loopStartSample;
    Value<uint32_t, DNAEn> m_loopLengthSamples;
    Value<uint32_t, DNAEn> m_adpcmParmOffset;

    void _setLoopStartSample(uint32_t sample) {
      m_loopLengthSamples += m_loopStartSample - sample;
      m_loopStartSample = sample;
    }
    void setLoopEndSample(uint32_t sample) { m_loopLengthSamples = sample + 1 - m_loopStartSample; }
  };
  template <std::endian DNAEn>
  struct AT_SPECIALIZE_PARMS(std::endian::big, std::endian::little) MusyX1SdirEntry : BigDNA {
    AT_DECL_DNA
    SampleIdDNA<DNAEn> m_sfxId;
    Seek<2, amuse::SeekOrigin::Current> pad;
    Value<uint32_t, DNAEn> m_sampleOff;
    Value<uint32_t, DNAEn> m_pitchSampleRate;
    Value<uint32_t, DNAEn> m_numSamples;
    Value<uint32_t, DNAEn> m_loopStartSample;
    Value<uint32_t, DNAEn> m_loopLengthSamples;
  };
  template <std::endian DNAEn>
  struct AT_SPECIALIZE_PARMS(std::endian::big, std::endian::little) MusyX1AbsSdirEntry : BigDNA {
    AT_DECL_DNA
    SampleIdDNA<DNAEn> m_sfxId;
    Seek<2, amuse::SeekOrigin::Current> pad;
    Value<uint32_t, DNAEn> m_sampleOff;
    Value<uint32_t, DNAEn> m_unk;
    Value<uint32_t, DNAEn> m_pitchSampleRate;
    Value<uint32_t, DNAEn> m_numSamples;
    Value<uint32_t, DNAEn> m_loopStartSample;
    Value<uint32_t, DNAEn> m_loopLengthSamples;
  };
  struct EntryData {
    uint32_t m_sampleOff = 0;
    uint32_t m_unk = 0;
    uint8_t m_pitch = 0;
    uint16_t m_sampleRate = 0;
    uint32_t m_numSamples = 0; // Top 8 bits is SampleFormat
    uint32_t m_loopStartSample = 0;
    uint32_t m_loopLengthSamples = 0;
    uint32_t m_adpcmParmOffset = 0;

    /* Stored out-of-band in a platform-dependent way */
    ADPCMParms m_ADPCMParms;

    /* In-memory storage of an individual sample. Editors use this structure
     * to override the loaded sample with a file-backed version without repacking
     * the sample data into a SAMP block. */
    time_t m_looseModTime = 0;
    std::unique_ptr<uint8_t[]> m_looseData;

    /* Use middle C when pitch is (impossibly low) default */
    uint8_t getPitch() const { return m_pitch == 0 ? uint8_t(60) : m_pitch; }
    uint32_t getNumSamples() const { return m_numSamples & 0xffffff; }
    SampleFormat getSampleFormat() const { return SampleFormat(m_numSamples >> 24); }
    bool isFormatDSP() const {
      SampleFormat fmt = getSampleFormat();
      return fmt == SampleFormat::DSP || fmt == SampleFormat::DSP_DRUM;
    }

    bool isLooped() const { return m_loopLengthSamples != 0 && m_loopStartSample != 0xffffffff; }

    void _setLoopStartSample(uint32_t sample) {
      m_loopLengthSamples += m_loopStartSample - sample;
      m_loopStartSample = sample;
    }
    void setLoopStartSample(uint32_t sample);
    uint32_t getLoopStartSample() const { return m_loopStartSample; }
    void setLoopEndSample(uint32_t sample) { m_loopLengthSamples = sample + 1 - m_loopStartSample; }
    uint32_t getLoopEndSample() const { return m_loopStartSample + m_loopLengthSamples - 1; }

    EntryData() = default;

    template <std::endian DNAE>
    EntryData(const EntryDNA<DNAE>& in)
    : m_sampleOff(in.m_sampleOff)
    , m_unk(in.m_unk)
    , m_pitch(in.m_pitch)
    , m_sampleRate(in.m_sampleRate)
    , m_numSamples(in.m_numSamples)
    , m_loopStartSample(in.m_loopStartSample)
    , m_loopLengthSamples(in.m_loopLengthSamples)
    , m_adpcmParmOffset(in.m_adpcmParmOffset) {}

    template <std::endian DNAE>
    EntryData(const MusyX1SdirEntry<DNAE>& in)
    : m_sampleOff(in.m_sampleOff)
    , m_unk(0)
    , m_pitch(in.m_pitchSampleRate >> 24)
    , m_sampleRate(in.m_pitchSampleRate & 0xffff)
    , m_numSamples(in.m_numSamples)
    , m_loopStartSample(in.m_loopStartSample)
    , m_loopLengthSamples(in.m_loopLengthSamples)
    , m_adpcmParmOffset(0) {}

    template <std::endian DNAE>
    EntryData(const MusyX1AbsSdirEntry<DNAE>& in)
    : m_sampleOff(in.m_sampleOff)
    , m_unk(in.m_unk)
    , m_pitch(in.m_pitchSampleRate >> 24)
    , m_sampleRate(in.m_pitchSampleRate & 0xffff)
    , m_numSamples(in.m_numSamples)
    , m_loopStartSample(in.m_loopStartSample)
    , m_loopLengthSamples(in.m_loopLengthSamples)
    , m_adpcmParmOffset(0) {}

    template <std::endian DNAEn>
    EntryDNA<DNAEn> toDNA(SFXId id) const {
      EntryDNA<DNAEn> ret;
      ret.m_sfxId.id = id;
      ret.m_sampleOff = m_sampleOff;
      ret.m_unk = m_unk;
      ret.m_pitch = m_pitch;
      ret.m_sampleRate = m_sampleRate;
      ret.m_numSamples = m_numSamples;
      ret.m_loopStartSample = m_loopStartSample;
      ret.m_loopLengthSamples = m_loopLengthSamples;
      ret.m_adpcmParmOffset = m_adpcmParmOffset;
      return ret;
    }

    void loadLooseDSP(std::string_view dspPath);
    void loadLooseVADPCM(std::string_view vadpcmPath);
    void loadLooseWAV(std::string_view wavPath);

    void patchMetadataDSP(std::string_view dspPath);
    void patchMetadataVADPCM(std::string_view vadpcmPath);
    void patchMetadataWAV(std::string_view wavPath);
  };
  /* This double-wrapper allows Voices to keep a strong reference on
   * a single instance of loaded loose data without being unexpectedly
   * clobbered */
  struct Entry {
    ObjToken<EntryData> m_data;

    Entry() : m_data(MakeObj<EntryData>()) {}

    template <std::endian DNAE>
    Entry(const EntryDNA<DNAE>& in) : m_data(MakeObj<EntryData>(in)) {}

    template <std::endian DNAE>
    Entry(const MusyX1SdirEntry<DNAE>& in) : m_data(MakeObj<EntryData>(in)) {}

    template <std::endian DNAE>
    Entry(const MusyX1AbsSdirEntry<DNAE>& in) : m_data(MakeObj<EntryData>(in)) {}

    template <std::endian DNAEn>
    EntryDNA<DNAEn> toDNA(SFXId id) const {
      return m_data->toDNA<DNAEn>(id);
    }

    void loadLooseData(std::string_view basePath);
    SampleFileState getFileState(std::string_view basePath, std::string* pathOut = nullptr) const;
    void patchSampleMetadata(std::string_view basePath) const;
  };

private:
  std::unordered_map<SampleId, ObjToken<Entry>> m_entries;
  static void _extractWAV(SampleId id, const EntryData& ent, std::string_view destDir,
                          const unsigned char* samp);
  static void _extractCompressed(SampleId id, const EntryData& ent, std::string_view destDir,
                                 const unsigned char* samp, bool compressWAV = false);

public:
  AudioGroupSampleDirectory() = default;
  AudioGroupSampleDirectory(std::istream& r, GCNDataTag);
  AudioGroupSampleDirectory(std::istream& r, const unsigned char* sampData, bool absOffs, N64DataTag);
  AudioGroupSampleDirectory(std::istream& r, bool absOffs, PCDataTag);
  static AudioGroupSampleDirectory CreateAudioGroupSampleDirectory(const AudioGroupData& data);
  static AudioGroupSampleDirectory CreateAudioGroupSampleDirectory(std::string_view groupPath);

  const std::unordered_map<SampleId, ObjToken<Entry>>& sampleEntries() const { return m_entries; }
  std::unordered_map<SampleId, ObjToken<Entry>>& sampleEntries() { return m_entries; }

  void extractWAV(SampleId id, std::string_view destDir, const unsigned char* samp) const;
  void extractAllWAV(std::string_view destDir, const unsigned char* samp) const;
  void extractCompressed(SampleId id, std::string_view destDir, const unsigned char* samp) const;
  void extractAllCompressed(std::string_view destDir, const unsigned char* samp) const;

  void reloadSampleData(std::string_view groupPath);

  std::pair<std::vector<uint8_t>, std::vector<uint8_t>> toGCNData(const AudioGroupDatabase& group) const;

  AudioGroupSampleDirectory(const AudioGroupSampleDirectory&) = delete;
  AudioGroupSampleDirectory& operator=(const AudioGroupSampleDirectory&) = delete;
  AudioGroupSampleDirectory(AudioGroupSampleDirectory&&) = default;
  AudioGroupSampleDirectory& operator=(AudioGroupSampleDirectory&&) = default;
};

using SampleEntry = AudioGroupSampleDirectory::Entry;
using SampleEntryData = AudioGroupSampleDirectory::EntryData;
} // namespace amuse
