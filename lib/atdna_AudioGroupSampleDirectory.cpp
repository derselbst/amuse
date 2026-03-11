// Manually-generated serialization code replacing target_atdna output for
// include/amuse/AudioGroupSampleDirectory.hpp

#include "amuse/AudioGroupSampleDirectory.hpp"

using namespace athena::io;

namespace amuse {

// ── DSPADPCMHeader ────────────────────────────────────────────────────────────
template <> void DSPADPCMHeader::Enumerate<DNAOpRead>(IStreamReader& r) {
    x0_num_samples  = r.readUint32Big();
    x4_num_nibbles  = r.readUint32Big();
    x8_sample_rate  = r.readUint32Big();
    xc_loop_flag    = r.readUint16Big();
    xe_format       = r.readUint16Big();
    x10_loop_start_nibble = r.readUint32Big();
    x14_loop_end_nibble   = r.readUint32Big();
    x18_ca          = r.readUint32Big();
    for (int i = 0; i < 8; ++i) {
        x1c_coef[i][0] = r.readInt16Big();
        x1c_coef[i][1] = r.readInt16Big();
    }
    x3c_gain        = r.readInt16Big();
    x3e_ps          = r.readInt16Big();
    x40_hist1       = r.readInt16Big();
    x42_hist2       = r.readInt16Big();
    x44_loop_ps     = r.readInt16Big();
    x46_loop_hist1  = r.readInt16Big();
    x48_loop_hist2  = r.readInt16Big();
    m_pitch         = r.readUByte();
    r.seek(21, athena::SeekOrigin::Current);
}
template <> void DSPADPCMHeader::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUint32Big(x0_num_samples);
    w.writeUint32Big(x4_num_nibbles);
    w.writeUint32Big(x8_sample_rate);
    w.writeUint16Big(xc_loop_flag);
    w.writeUint16Big(xe_format);
    w.writeUint32Big(x10_loop_start_nibble);
    w.writeUint32Big(x14_loop_end_nibble);
    w.writeUint32Big(x18_ca);
    for (int i = 0; i < 8; ++i) {
        w.writeInt16Big(x1c_coef[i][0]);
        w.writeInt16Big(x1c_coef[i][1]);
    }
    w.writeInt16Big(x3c_gain);
    w.writeInt16Big(x3e_ps);
    w.writeInt16Big(x40_hist1);
    w.writeInt16Big(x42_hist2);
    w.writeInt16Big(x44_loop_ps);
    w.writeInt16Big(x46_loop_hist1);
    w.writeInt16Big(x48_loop_hist2);
    w.writeUByte(m_pitch);
    uint8_t pad[21] = {};
    w.writeBytes(pad, 21);
}
template <> void DSPADPCMHeader::Enumerate<DNAOpBinarySize>(size_t& s) { s += 96; }
template <> void DSPADPCMHeader::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void DSPADPCMHeader::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
std::string_view DSPADPCMHeader::DNAType() { return "amuse::DSPADPCMHeader"; }

// ── VADPCMHeader ──────────────────────────────────────────────────────────────
template <> void VADPCMHeader::Enumerate<DNAOpRead>(IStreamReader& r) {
    m_pitchSampleRate   = r.readUint32Big();
    m_numSamples        = r.readUint32Big();
    m_loopStartSample   = r.readUint32Big();
    m_loopLengthSamples = r.readUint32Big();
}
template <> void VADPCMHeader::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUint32Big(m_pitchSampleRate);
    w.writeUint32Big(m_numSamples);
    w.writeUint32Big(m_loopStartSample);
    w.writeUint32Big(m_loopLengthSamples);
}
template <> void VADPCMHeader::Enumerate<DNAOpBinarySize>(size_t& s) { s += 16; }
template <> void VADPCMHeader::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void VADPCMHeader::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
std::string_view VADPCMHeader::DNAType() { return "amuse::VADPCMHeader"; }

// ── WAVFormatChunk ────────────────────────────────────────────────────────────
template <> void WAVFormatChunk::Enumerate<DNAOpRead>(IStreamReader& r) {
    sampleFmt    = r.readUint16Little();
    numChannels  = r.readUint16Little();
    sampleRate   = r.readUint32Little();
    byteRate     = r.readUint32Little();
    blockAlign   = r.readUint16Little();
    bitsPerSample = r.readUint16Little();
}
template <> void WAVFormatChunk::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUint16Little(sampleFmt);
    w.writeUint16Little(numChannels);
    w.writeUint32Little(sampleRate);
    w.writeUint32Little(byteRate);
    w.writeUint16Little(blockAlign);
    w.writeUint16Little(bitsPerSample);
}
template <> void WAVFormatChunk::Enumerate<DNAOpBinarySize>(size_t& s) { s += 16; }
template <> void WAVFormatChunk::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void WAVFormatChunk::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
std::string_view WAVFormatChunk::DNAType() { return "amuse::WAVFormatChunk"; }

// ── WAVSampleChunk ────────────────────────────────────────────────────────────
template <> void WAVSampleChunk::Enumerate<DNAOpRead>(IStreamReader& r) {
    smplManufacturer  = r.readUint32Little();
    smplProduct       = r.readUint32Little();
    smplPeriod        = r.readUint32Little();
    midiNote          = r.readUint32Little();
    midiPitchFrac     = r.readUint32Little();
    smpteFormat       = r.readUint32Little();
    smpteOffset       = r.readUint32Little();
    numSampleLoops    = r.readUint32Little();
    additionalDataSize = r.readUint32Little();
}
template <> void WAVSampleChunk::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUint32Little(smplManufacturer);
    w.writeUint32Little(smplProduct);
    w.writeUint32Little(smplPeriod);
    w.writeUint32Little(midiNote);
    w.writeUint32Little(midiPitchFrac);
    w.writeUint32Little(smpteFormat);
    w.writeUint32Little(smpteOffset);
    w.writeUint32Little(numSampleLoops);
    w.writeUint32Little(additionalDataSize);
}
template <> void WAVSampleChunk::Enumerate<DNAOpBinarySize>(size_t& s) { s += 36; }
template <> void WAVSampleChunk::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void WAVSampleChunk::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
std::string_view WAVSampleChunk::DNAType() { return "amuse::WAVSampleChunk"; }

// ── WAVSampleLoop ─────────────────────────────────────────────────────────────
template <> void WAVSampleLoop::Enumerate<DNAOpRead>(IStreamReader& r) {
    cuePointId = r.readUint32Little();
    loopType   = r.readUint32Little();
    start      = r.readUint32Little();
    end        = r.readUint32Little();
    fraction   = r.readUint32Little();
    playCount  = r.readUint32Little();
}
template <> void WAVSampleLoop::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUint32Little(cuePointId);
    w.writeUint32Little(loopType);
    w.writeUint32Little(start);
    w.writeUint32Little(end);
    w.writeUint32Little(fraction);
    w.writeUint32Little(playCount);
}
template <> void WAVSampleLoop::Enumerate<DNAOpBinarySize>(size_t& s) { s += 24; }
template <> void WAVSampleLoop::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void WAVSampleLoop::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
std::string_view WAVSampleLoop::DNAType() { return "amuse::WAVSampleLoop"; }

// ── WAVHeader ─────────────────────────────────────────────────────────────────
template <> void WAVHeader::Enumerate<DNAOpRead>(IStreamReader& r) {
    riffMagic    = r.readUint32Little();
    wavChuckSize = r.readUint32Little();
    wavMagic     = r.readUint32Little();
    fmtMagic     = r.readUint32Little();
    fmtChunkSize = r.readUint32Little();
    fmtChunk.read(r);
    smplMagic    = r.readUint32Little();
    smplChunkSize = r.readUint32Little();
    smplChunk.read(r);
    dataMagic    = r.readUint32Little();
    dataChunkSize = r.readUint32Little();
}
template <> void WAVHeader::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUint32Little(riffMagic);
    w.writeUint32Little(wavChuckSize);
    w.writeUint32Little(wavMagic);
    w.writeUint32Little(fmtMagic);
    w.writeUint32Little(fmtChunkSize);
    fmtChunk.write(w);
    w.writeUint32Little(smplMagic);
    w.writeUint32Little(smplChunkSize);
    smplChunk.write(w);
    w.writeUint32Little(dataMagic);
    w.writeUint32Little(dataChunkSize);
}
template <> void WAVHeader::Enumerate<DNAOpBinarySize>(size_t& s) { s += 12 + 8 + 16 + 8 + 36 + 8; }
template <> void WAVHeader::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void WAVHeader::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
std::string_view WAVHeader::DNAType() { return "amuse::WAVHeader"; }

// ── WAVHeaderLoop ─────────────────────────────────────────────────────────────
template <> void WAVHeaderLoop::Enumerate<DNAOpRead>(IStreamReader& r) {
    riffMagic    = r.readUint32Little();
    wavChuckSize = r.readUint32Little();
    wavMagic     = r.readUint32Little();
    fmtMagic     = r.readUint32Little();
    fmtChunkSize = r.readUint32Little();
    fmtChunk.read(r);
    smplMagic    = r.readUint32Little();
    smplChunkSize = r.readUint32Little();
    smplChunk.read(r);
    sampleLoop.read(r);
    dataMagic    = r.readUint32Little();
    dataChunkSize = r.readUint32Little();
}
template <> void WAVHeaderLoop::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUint32Little(riffMagic);
    w.writeUint32Little(wavChuckSize);
    w.writeUint32Little(wavMagic);
    w.writeUint32Little(fmtMagic);
    w.writeUint32Little(fmtChunkSize);
    fmtChunk.write(w);
    w.writeUint32Little(smplMagic);
    w.writeUint32Little(smplChunkSize);
    smplChunk.write(w);
    sampleLoop.write(w);
    w.writeUint32Little(dataMagic);
    w.writeUint32Little(dataChunkSize);
}
template <> void WAVHeaderLoop::Enumerate<DNAOpBinarySize>(size_t& s) { s += 12 + 8 + 16 + 8 + 36 + 24 + 8; }
template <> void WAVHeaderLoop::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void WAVHeaderLoop::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
std::string_view WAVHeaderLoop::DNAType() { return "amuse::WAVHeaderLoop"; }

// ── EntryDNA<Big> ─────────────────────────────────────────────────────────────
template <> template <> void AudioGroupSampleDirectory::EntryDNA<athena::Endian::Big>::Enumerate<DNAOpRead>(IStreamReader& r) {
    m_sfxId.read(r);
    r.seek(2, athena::SeekOrigin::Current);
    m_sampleOff    = r.readUint32Big();
    m_unk          = r.readUint32Big();
    m_pitch        = r.readUByte();
    r.seek(1, athena::SeekOrigin::Current);
    m_sampleRate   = r.readUint16Big();
    m_numSamples   = r.readUint32Big();
    m_loopStartSample   = r.readUint32Big();
    m_loopLengthSamples = r.readUint32Big();
    m_adpcmParmOffset   = r.readUint32Big();
}
template <> template <> void AudioGroupSampleDirectory::EntryDNA<athena::Endian::Big>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    m_sfxId.write(w);
    uint8_t pad2[2] = {};
    w.writeBytes(pad2, 2);
    w.writeUint32Big(m_sampleOff);
    w.writeUint32Big(m_unk);
    w.writeUByte(m_pitch);
    uint8_t pad1[1] = {};
    w.writeBytes(pad1, 1);
    w.writeUint16Big(m_sampleRate);
    w.writeUint32Big(m_numSamples);
    w.writeUint32Big(m_loopStartSample);
    w.writeUint32Big(m_loopLengthSamples);
    w.writeUint32Big(m_adpcmParmOffset);
}
template <> template <> void AudioGroupSampleDirectory::EntryDNA<athena::Endian::Big>::Enumerate<DNAOpBinarySize>(size_t& s) { s += 24; }
template <> template <> void AudioGroupSampleDirectory::EntryDNA<athena::Endian::Big>::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> template <> void AudioGroupSampleDirectory::EntryDNA<athena::Endian::Big>::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}

template <> template <> void AudioGroupSampleDirectory::EntryDNA<athena::Endian::Little>::Enumerate<DNAOpRead>(IStreamReader& r) {
    m_sfxId.read(r);
    r.seek(2, athena::SeekOrigin::Current);
    m_sampleOff    = r.readUint32Little();
    m_unk          = r.readUint32Little();
    m_pitch        = r.readUByte();
    r.seek(1, athena::SeekOrigin::Current);
    m_sampleRate   = r.readUint16Little();
    m_numSamples   = r.readUint32Little();
    m_loopStartSample   = r.readUint32Little();
    m_loopLengthSamples = r.readUint32Little();
    m_adpcmParmOffset   = r.readUint32Little();
}
template <> template <> void AudioGroupSampleDirectory::EntryDNA<athena::Endian::Little>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    m_sfxId.write(w);
    uint8_t pad2[2] = {};
    w.writeBytes(pad2, 2);
    w.writeUint32Little(m_sampleOff);
    w.writeUint32Little(m_unk);
    w.writeUByte(m_pitch);
    uint8_t pad1[1] = {};
    w.writeBytes(pad1, 1);
    w.writeUint16Little(m_sampleRate);
    w.writeUint32Little(m_numSamples);
    w.writeUint32Little(m_loopStartSample);
    w.writeUint32Little(m_loopLengthSamples);
    w.writeUint32Little(m_adpcmParmOffset);
}
template <> template <> void AudioGroupSampleDirectory::EntryDNA<athena::Endian::Little>::Enumerate<DNAOpBinarySize>(size_t& s) { s += 24; }
template <> template <> void AudioGroupSampleDirectory::EntryDNA<athena::Endian::Little>::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> template <> void AudioGroupSampleDirectory::EntryDNA<athena::Endian::Little>::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
template struct AudioGroupSampleDirectory::EntryDNA<athena::Endian::Big>;
template struct AudioGroupSampleDirectory::EntryDNA<athena::Endian::Little>;

// ── MusyX1SdirEntry<Big/Little> ───────────────────────────────────────────────
template <> template <> void AudioGroupSampleDirectory::MusyX1SdirEntry<athena::Endian::Big>::Enumerate<DNAOpRead>(IStreamReader& r) {
    m_sfxId.read(r);
    r.seek(2, athena::SeekOrigin::Current);
    m_sampleOff       = r.readUint32Big();
    m_pitchSampleRate = r.readUint32Big();
    m_numSamples      = r.readUint32Big();
    m_loopStartSample = r.readUint32Big();
    m_loopLengthSamples = r.readUint32Big();
}
template <> template <> void AudioGroupSampleDirectory::MusyX1SdirEntry<athena::Endian::Big>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    m_sfxId.write(w);
    uint8_t pad[2] = {};
    w.writeBytes(pad, 2);
    w.writeUint32Big(m_sampleOff);
    w.writeUint32Big(m_pitchSampleRate);
    w.writeUint32Big(m_numSamples);
    w.writeUint32Big(m_loopStartSample);
    w.writeUint32Big(m_loopLengthSamples);
}
template <> template <> void AudioGroupSampleDirectory::MusyX1SdirEntry<athena::Endian::Big>::Enumerate<DNAOpBinarySize>(size_t& s) { s += 24; }
template <> template <> void AudioGroupSampleDirectory::MusyX1SdirEntry<athena::Endian::Big>::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> template <> void AudioGroupSampleDirectory::MusyX1SdirEntry<athena::Endian::Big>::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}

template <> template <> void AudioGroupSampleDirectory::MusyX1SdirEntry<athena::Endian::Little>::Enumerate<DNAOpRead>(IStreamReader& r) {
    m_sfxId.read(r);
    r.seek(2, athena::SeekOrigin::Current);
    m_sampleOff       = r.readUint32Little();
    m_pitchSampleRate = r.readUint32Little();
    m_numSamples      = r.readUint32Little();
    m_loopStartSample = r.readUint32Little();
    m_loopLengthSamples = r.readUint32Little();
}
template <> template <> void AudioGroupSampleDirectory::MusyX1SdirEntry<athena::Endian::Little>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    m_sfxId.write(w);
    uint8_t pad[2] = {};
    w.writeBytes(pad, 2);
    w.writeUint32Little(m_sampleOff);
    w.writeUint32Little(m_pitchSampleRate);
    w.writeUint32Little(m_numSamples);
    w.writeUint32Little(m_loopStartSample);
    w.writeUint32Little(m_loopLengthSamples);
}
template <> template <> void AudioGroupSampleDirectory::MusyX1SdirEntry<athena::Endian::Little>::Enumerate<DNAOpBinarySize>(size_t& s) { s += 24; }
template <> template <> void AudioGroupSampleDirectory::MusyX1SdirEntry<athena::Endian::Little>::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> template <> void AudioGroupSampleDirectory::MusyX1SdirEntry<athena::Endian::Little>::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
template struct AudioGroupSampleDirectory::MusyX1SdirEntry<athena::Endian::Big>;
template struct AudioGroupSampleDirectory::MusyX1SdirEntry<athena::Endian::Little>;

// ── MusyX1AbsSdirEntry<Big/Little> ────────────────────────────────────────────
template <> template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<athena::Endian::Big>::Enumerate<DNAOpRead>(IStreamReader& r) {
    m_sfxId.read(r);
    r.seek(2, athena::SeekOrigin::Current);
    m_sampleOff       = r.readUint32Big();
    m_unk             = r.readUint32Big();
    m_pitchSampleRate = r.readUint32Big();
    m_numSamples      = r.readUint32Big();
    m_loopStartSample = r.readUint32Big();
    m_loopLengthSamples = r.readUint32Big();
}
template <> template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<athena::Endian::Big>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    m_sfxId.write(w);
    uint8_t pad[2] = {};
    w.writeBytes(pad, 2);
    w.writeUint32Big(m_sampleOff);
    w.writeUint32Big(m_unk);
    w.writeUint32Big(m_pitchSampleRate);
    w.writeUint32Big(m_numSamples);
    w.writeUint32Big(m_loopStartSample);
    w.writeUint32Big(m_loopLengthSamples);
}
template <> template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<athena::Endian::Big>::Enumerate<DNAOpBinarySize>(size_t& s) { s += 28; }
template <> template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<athena::Endian::Big>::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<athena::Endian::Big>::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}

template <> template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<athena::Endian::Little>::Enumerate<DNAOpRead>(IStreamReader& r) {
    m_sfxId.read(r);
    r.seek(2, athena::SeekOrigin::Current);
    m_sampleOff       = r.readUint32Little();
    m_unk             = r.readUint32Little();
    m_pitchSampleRate = r.readUint32Little();
    m_numSamples      = r.readUint32Little();
    m_loopStartSample = r.readUint32Little();
    m_loopLengthSamples = r.readUint32Little();
}
template <> template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<athena::Endian::Little>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    m_sfxId.write(w);
    uint8_t pad[2] = {};
    w.writeBytes(pad, 2);
    w.writeUint32Little(m_sampleOff);
    w.writeUint32Little(m_unk);
    w.writeUint32Little(m_pitchSampleRate);
    w.writeUint32Little(m_numSamples);
    w.writeUint32Little(m_loopStartSample);
    w.writeUint32Little(m_loopLengthSamples);
}
template <> template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<athena::Endian::Little>::Enumerate<DNAOpBinarySize>(size_t& s) { s += 28; }
template <> template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<athena::Endian::Little>::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<athena::Endian::Little>::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
template struct AudioGroupSampleDirectory::MusyX1AbsSdirEntry<athena::Endian::Big>;
template struct AudioGroupSampleDirectory::MusyX1AbsSdirEntry<athena::Endian::Little>;

// DNAType for template struct instantiations
template <athena::Endian DNAE>
std::string_view AudioGroupSampleDirectory::EntryDNA<DNAE>::DNAType() {
    return "amuse::AudioGroupSampleDirectory::EntryDNA";
}

template <athena::Endian DNAE>
std::string_view AudioGroupSampleDirectory::MusyX1SdirEntry<DNAE>::DNAType() {
    return "amuse::AudioGroupSampleDirectory::MusyX1SdirEntry";
}

template <athena::Endian DNAE>
std::string_view AudioGroupSampleDirectory::MusyX1AbsSdirEntry<DNAE>::DNAType() {
    return "amuse::AudioGroupSampleDirectory::MusyX1AbsSdirEntry";
}

} // namespace amuse
