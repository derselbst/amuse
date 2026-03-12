// Manually-generated serialization code replacing target_atdna output for
// include/amuse/AudioGroupSampleDirectory.hpp

#include "amuse/AudioGroupSampleDirectory.hpp"

namespace amuse {

// ── DSPADPCMHeader ────────────────────────────────────────────────────────────
void DSPADPCMHeader::read(std::istream& r) {
    x0_num_samples  = amuse::io::readUint32Big(r);
    x4_num_nibbles  = amuse::io::readUint32Big(r);
    x8_sample_rate  = amuse::io::readUint32Big(r);
    xc_loop_flag    = amuse::io::readUint16Big(r);
    xe_format       = amuse::io::readUint16Big(r);
    x10_loop_start_nibble = amuse::io::readUint32Big(r);
    x14_loop_end_nibble   = amuse::io::readUint32Big(r);
    x18_ca          = amuse::io::readUint32Big(r);
    for (int i = 0; i < 8; ++i) {
        x1c_coef[i][0] = amuse::io::readInt16Big(r);
        x1c_coef[i][1] = amuse::io::readInt16Big(r);
    }
    x3c_gain        = amuse::io::readInt16Big(r);
    x3e_ps          = amuse::io::readInt16Big(r);
    x40_hist1       = amuse::io::readInt16Big(r);
    x42_hist2       = amuse::io::readInt16Big(r);
    x44_loop_ps     = amuse::io::readInt16Big(r);
    x46_loop_hist1  = amuse::io::readInt16Big(r);
    x48_loop_hist2  = amuse::io::readInt16Big(r);
    m_pitch         = amuse::io::readUByte(r);
    r.seekg(21, std::ios_base::cur);
}
void DSPADPCMHeader::write(std::ostream& w) const {
    amuse::io::writeUint32Big(w, x0_num_samples);
    amuse::io::writeUint32Big(w, x4_num_nibbles);
    amuse::io::writeUint32Big(w, x8_sample_rate);
    amuse::io::writeUint16Big(w, xc_loop_flag);
    amuse::io::writeUint16Big(w, xe_format);
    amuse::io::writeUint32Big(w, x10_loop_start_nibble);
    amuse::io::writeUint32Big(w, x14_loop_end_nibble);
    amuse::io::writeUint32Big(w, x18_ca);
    for (int i = 0; i < 8; ++i) {
        amuse::io::writeInt16Big(w, x1c_coef[i][0]);
        amuse::io::writeInt16Big(w, x1c_coef[i][1]);
    }
    amuse::io::writeInt16Big(w, x3c_gain);
    amuse::io::writeInt16Big(w, x3e_ps);
    amuse::io::writeInt16Big(w, x40_hist1);
    amuse::io::writeInt16Big(w, x42_hist2);
    amuse::io::writeInt16Big(w, x44_loop_ps);
    amuse::io::writeInt16Big(w, x46_loop_hist1);
    amuse::io::writeInt16Big(w, x48_loop_hist2);
    amuse::io::writeUByte(w, m_pitch);
    uint8_t pad[21] = {};
    amuse::io::writeBytes(w, pad, 21);
}
size_t DSPADPCMHeader::binarySize(size_t s) const { s += 96; return s; }
void DSPADPCMHeader::readYaml(amuse::io::YAMLDocReader&) {}
void DSPADPCMHeader::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view DSPADPCMHeader::DNAType() { return "amuse::DSPADPCMHeader"; }

// ── VADPCMHeader ──────────────────────────────────────────────────────────────
void VADPCMHeader::read(std::istream& r) {
    m_pitchSampleRate   = amuse::io::readUint32Big(r);
    m_numSamples        = amuse::io::readUint32Big(r);
    m_loopStartSample   = amuse::io::readUint32Big(r);
    m_loopLengthSamples = amuse::io::readUint32Big(r);
}
void VADPCMHeader::write(std::ostream& w) const {
    amuse::io::writeUint32Big(w, m_pitchSampleRate);
    amuse::io::writeUint32Big(w, m_numSamples);
    amuse::io::writeUint32Big(w, m_loopStartSample);
    amuse::io::writeUint32Big(w, m_loopLengthSamples);
}
size_t VADPCMHeader::binarySize(size_t s) const { s += 16; return s; }
void VADPCMHeader::readYaml(amuse::io::YAMLDocReader&) {}
void VADPCMHeader::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view VADPCMHeader::DNAType() { return "amuse::VADPCMHeader"; }

// ── WAVFormatChunk ────────────────────────────────────────────────────────────
void WAVFormatChunk::read(std::istream& r) {
    sampleFmt    = amuse::io::readUint16Little(r);
    numChannels  = amuse::io::readUint16Little(r);
    sampleRate   = amuse::io::readUint32Little(r);
    byteRate     = amuse::io::readUint32Little(r);
    blockAlign   = amuse::io::readUint16Little(r);
    bitsPerSample = amuse::io::readUint16Little(r);
}
void WAVFormatChunk::write(std::ostream& w) const {
    amuse::io::writeUint16Little(w, sampleFmt);
    amuse::io::writeUint16Little(w, numChannels);
    amuse::io::writeUint32Little(w, sampleRate);
    amuse::io::writeUint32Little(w, byteRate);
    amuse::io::writeUint16Little(w, blockAlign);
    amuse::io::writeUint16Little(w, bitsPerSample);
}
size_t WAVFormatChunk::binarySize(size_t s) const { s += 16; return s; }
void WAVFormatChunk::readYaml(amuse::io::YAMLDocReader&) {}
void WAVFormatChunk::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view WAVFormatChunk::DNAType() { return "amuse::WAVFormatChunk"; }

// ── WAVSampleChunk ────────────────────────────────────────────────────────────
void WAVSampleChunk::read(std::istream& r) {
    smplManufacturer  = amuse::io::readUint32Little(r);
    smplProduct       = amuse::io::readUint32Little(r);
    smplPeriod        = amuse::io::readUint32Little(r);
    midiNote          = amuse::io::readUint32Little(r);
    midiPitchFrac     = amuse::io::readUint32Little(r);
    smpteFormat       = amuse::io::readUint32Little(r);
    smpteOffset       = amuse::io::readUint32Little(r);
    numSampleLoops    = amuse::io::readUint32Little(r);
    additionalDataSize = amuse::io::readUint32Little(r);
}
void WAVSampleChunk::write(std::ostream& w) const {
    amuse::io::writeUint32Little(w, smplManufacturer);
    amuse::io::writeUint32Little(w, smplProduct);
    amuse::io::writeUint32Little(w, smplPeriod);
    amuse::io::writeUint32Little(w, midiNote);
    amuse::io::writeUint32Little(w, midiPitchFrac);
    amuse::io::writeUint32Little(w, smpteFormat);
    amuse::io::writeUint32Little(w, smpteOffset);
    amuse::io::writeUint32Little(w, numSampleLoops);
    amuse::io::writeUint32Little(w, additionalDataSize);
}
size_t WAVSampleChunk::binarySize(size_t s) const { s += 36; return s; }
void WAVSampleChunk::readYaml(amuse::io::YAMLDocReader&) {}
void WAVSampleChunk::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view WAVSampleChunk::DNAType() { return "amuse::WAVSampleChunk"; }

// ── WAVSampleLoop ─────────────────────────────────────────────────────────────
void WAVSampleLoop::read(std::istream& r) {
    cuePointId = amuse::io::readUint32Little(r);
    loopType   = amuse::io::readUint32Little(r);
    start      = amuse::io::readUint32Little(r);
    end        = amuse::io::readUint32Little(r);
    fraction   = amuse::io::readUint32Little(r);
    playCount  = amuse::io::readUint32Little(r);
}
void WAVSampleLoop::write(std::ostream& w) const {
    amuse::io::writeUint32Little(w, cuePointId);
    amuse::io::writeUint32Little(w, loopType);
    amuse::io::writeUint32Little(w, start);
    amuse::io::writeUint32Little(w, end);
    amuse::io::writeUint32Little(w, fraction);
    amuse::io::writeUint32Little(w, playCount);
}
size_t WAVSampleLoop::binarySize(size_t s) const { s += 24; return s; }
void WAVSampleLoop::readYaml(amuse::io::YAMLDocReader&) {}
void WAVSampleLoop::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view WAVSampleLoop::DNAType() { return "amuse::WAVSampleLoop"; }

// ── WAVHeader ─────────────────────────────────────────────────────────────────
void WAVHeader::read(std::istream& r) {
    riffMagic    = amuse::io::readUint32Little(r);
    wavChuckSize = amuse::io::readUint32Little(r);
    wavMagic     = amuse::io::readUint32Little(r);
    fmtMagic     = amuse::io::readUint32Little(r);
    fmtChunkSize = amuse::io::readUint32Little(r);
    fmtChunk.read(r);
    smplMagic    = amuse::io::readUint32Little(r);
    smplChunkSize = amuse::io::readUint32Little(r);
    smplChunk.read(r);
    dataMagic    = amuse::io::readUint32Little(r);
    dataChunkSize = amuse::io::readUint32Little(r);
}
void WAVHeader::write(std::ostream& w) const {
    amuse::io::writeUint32Little(w, riffMagic);
    amuse::io::writeUint32Little(w, wavChuckSize);
    amuse::io::writeUint32Little(w, wavMagic);
    amuse::io::writeUint32Little(w, fmtMagic);
    amuse::io::writeUint32Little(w, fmtChunkSize);
    fmtChunk.write(w);
    amuse::io::writeUint32Little(w, smplMagic);
    amuse::io::writeUint32Little(w, smplChunkSize);
    smplChunk.write(w);
    amuse::io::writeUint32Little(w, dataMagic);
    amuse::io::writeUint32Little(w, dataChunkSize);
}
size_t WAVHeader::binarySize(size_t s) const { s += 12 + 8 + 16 + 8 + 36 + 8; return s; }
void WAVHeader::readYaml(amuse::io::YAMLDocReader&) {}
void WAVHeader::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view WAVHeader::DNAType() { return "amuse::WAVHeader"; }

// ── WAVHeaderLoop ─────────────────────────────────────────────────────────────
void WAVHeaderLoop::read(std::istream& r) {
    riffMagic    = amuse::io::readUint32Little(r);
    wavChuckSize = amuse::io::readUint32Little(r);
    wavMagic     = amuse::io::readUint32Little(r);
    fmtMagic     = amuse::io::readUint32Little(r);
    fmtChunkSize = amuse::io::readUint32Little(r);
    fmtChunk.read(r);
    smplMagic    = amuse::io::readUint32Little(r);
    smplChunkSize = amuse::io::readUint32Little(r);
    smplChunk.read(r);
    sampleLoop.read(r);
    dataMagic    = amuse::io::readUint32Little(r);
    dataChunkSize = amuse::io::readUint32Little(r);
}
void WAVHeaderLoop::write(std::ostream& w) const {
    amuse::io::writeUint32Little(w, riffMagic);
    amuse::io::writeUint32Little(w, wavChuckSize);
    amuse::io::writeUint32Little(w, wavMagic);
    amuse::io::writeUint32Little(w, fmtMagic);
    amuse::io::writeUint32Little(w, fmtChunkSize);
    fmtChunk.write(w);
    amuse::io::writeUint32Little(w, smplMagic);
    amuse::io::writeUint32Little(w, smplChunkSize);
    smplChunk.write(w);
    sampleLoop.write(w);
    amuse::io::writeUint32Little(w, dataMagic);
    amuse::io::writeUint32Little(w, dataChunkSize);
}
size_t WAVHeaderLoop::binarySize(size_t s) const { s += 12 + 8 + 16 + 8 + 36 + 24 + 8; return s; }
void WAVHeaderLoop::readYaml(amuse::io::YAMLDocReader&) {}
void WAVHeaderLoop::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view WAVHeaderLoop::DNAType() { return "amuse::WAVHeaderLoop"; }

// ── EntryDNA<Big> ─────────────────────────────────────────────────────────────
template <> void AudioGroupSampleDirectory::EntryDNA<amuse::Endian::Big>::read(std::istream& r) {
    m_sfxId.read(r);
    r.seekg(2, std::ios_base::cur);
    m_sampleOff    = amuse::io::readUint32Big(r);
    m_unk          = amuse::io::readUint32Big(r);
    m_pitch        = amuse::io::readUByte(r);
    r.seekg(1, std::ios_base::cur);
    m_sampleRate   = amuse::io::readUint16Big(r);
    m_numSamples   = amuse::io::readUint32Big(r);
    m_loopStartSample   = amuse::io::readUint32Big(r);
    m_loopLengthSamples = amuse::io::readUint32Big(r);
    m_adpcmParmOffset   = amuse::io::readUint32Big(r);
}
template <> void AudioGroupSampleDirectory::EntryDNA<amuse::Endian::Big>::write(std::ostream& w) const {
    m_sfxId.write(w);
    uint8_t pad2[2] = {};
    amuse::io::writeBytes(w, pad2, 2);
    amuse::io::writeUint32Big(w, m_sampleOff);
    amuse::io::writeUint32Big(w, m_unk);
    amuse::io::writeUByte(w, m_pitch);
    uint8_t pad1[1] = {};
    amuse::io::writeBytes(w, pad1, 1);
    amuse::io::writeUint16Big(w, m_sampleRate);
    amuse::io::writeUint32Big(w, m_numSamples);
    amuse::io::writeUint32Big(w, m_loopStartSample);
    amuse::io::writeUint32Big(w, m_loopLengthSamples);
    amuse::io::writeUint32Big(w, m_adpcmParmOffset);
}
template <> size_t AudioGroupSampleDirectory::EntryDNA<amuse::Endian::Big>::binarySize(size_t s) const { s += 24; return s; }
template <> void AudioGroupSampleDirectory::EntryDNA<amuse::Endian::Big>::readYaml(amuse::io::YAMLDocReader&) {}
template <> void AudioGroupSampleDirectory::EntryDNA<amuse::Endian::Big>::writeYaml(amuse::io::YAMLDocWriter&) const {}

template <> void AudioGroupSampleDirectory::EntryDNA<amuse::Endian::Little>::read(std::istream& r) {
    m_sfxId.read(r);
    r.seekg(2, std::ios_base::cur);
    m_sampleOff    = amuse::io::readUint32Little(r);
    m_unk          = amuse::io::readUint32Little(r);
    m_pitch        = amuse::io::readUByte(r);
    r.seekg(1, std::ios_base::cur);
    m_sampleRate   = amuse::io::readUint16Little(r);
    m_numSamples   = amuse::io::readUint32Little(r);
    m_loopStartSample   = amuse::io::readUint32Little(r);
    m_loopLengthSamples = amuse::io::readUint32Little(r);
    m_adpcmParmOffset   = amuse::io::readUint32Little(r);
}
template <> void AudioGroupSampleDirectory::EntryDNA<amuse::Endian::Little>::write(std::ostream& w) const {
    m_sfxId.write(w);
    uint8_t pad2[2] = {};
    amuse::io::writeBytes(w, pad2, 2);
    amuse::io::writeUint32Little(w, m_sampleOff);
    amuse::io::writeUint32Little(w, m_unk);
    amuse::io::writeUByte(w, m_pitch);
    uint8_t pad1[1] = {};
    amuse::io::writeBytes(w, pad1, 1);
    amuse::io::writeUint16Little(w, m_sampleRate);
    amuse::io::writeUint32Little(w, m_numSamples);
    amuse::io::writeUint32Little(w, m_loopStartSample);
    amuse::io::writeUint32Little(w, m_loopLengthSamples);
    amuse::io::writeUint32Little(w, m_adpcmParmOffset);
}
template <> size_t AudioGroupSampleDirectory::EntryDNA<amuse::Endian::Little>::binarySize(size_t s) const { s += 24; return s; }
template <> void AudioGroupSampleDirectory::EntryDNA<amuse::Endian::Little>::readYaml(amuse::io::YAMLDocReader&) {}
template <> void AudioGroupSampleDirectory::EntryDNA<amuse::Endian::Little>::writeYaml(amuse::io::YAMLDocWriter&) const {}
template struct AudioGroupSampleDirectory::EntryDNA<amuse::Endian::Big>;
template struct AudioGroupSampleDirectory::EntryDNA<amuse::Endian::Little>;

// ── MusyX1SdirEntry<Big/Little> ───────────────────────────────────────────────
template <> void AudioGroupSampleDirectory::MusyX1SdirEntry<amuse::Endian::Big>::read(std::istream& r) {
    m_sfxId.read(r);
    r.seekg(2, std::ios_base::cur);
    m_sampleOff       = amuse::io::readUint32Big(r);
    m_pitchSampleRate = amuse::io::readUint32Big(r);
    m_numSamples      = amuse::io::readUint32Big(r);
    m_loopStartSample = amuse::io::readUint32Big(r);
    m_loopLengthSamples = amuse::io::readUint32Big(r);
}
template <> void AudioGroupSampleDirectory::MusyX1SdirEntry<amuse::Endian::Big>::write(std::ostream& w) const {
    m_sfxId.write(w);
    uint8_t pad[2] = {};
    amuse::io::writeBytes(w, pad, 2);
    amuse::io::writeUint32Big(w, m_sampleOff);
    amuse::io::writeUint32Big(w, m_pitchSampleRate);
    amuse::io::writeUint32Big(w, m_numSamples);
    amuse::io::writeUint32Big(w, m_loopStartSample);
    amuse::io::writeUint32Big(w, m_loopLengthSamples);
}
template <> size_t AudioGroupSampleDirectory::MusyX1SdirEntry<amuse::Endian::Big>::binarySize(size_t s) const { s += 24; return s; }
template <> void AudioGroupSampleDirectory::MusyX1SdirEntry<amuse::Endian::Big>::readYaml(amuse::io::YAMLDocReader&) {}
template <> void AudioGroupSampleDirectory::MusyX1SdirEntry<amuse::Endian::Big>::writeYaml(amuse::io::YAMLDocWriter&) const {}

template <> void AudioGroupSampleDirectory::MusyX1SdirEntry<amuse::Endian::Little>::read(std::istream& r) {
    m_sfxId.read(r);
    r.seekg(2, std::ios_base::cur);
    m_sampleOff       = amuse::io::readUint32Little(r);
    m_pitchSampleRate = amuse::io::readUint32Little(r);
    m_numSamples      = amuse::io::readUint32Little(r);
    m_loopStartSample = amuse::io::readUint32Little(r);
    m_loopLengthSamples = amuse::io::readUint32Little(r);
}
template <> void AudioGroupSampleDirectory::MusyX1SdirEntry<amuse::Endian::Little>::write(std::ostream& w) const {
    m_sfxId.write(w);
    uint8_t pad[2] = {};
    amuse::io::writeBytes(w, pad, 2);
    amuse::io::writeUint32Little(w, m_sampleOff);
    amuse::io::writeUint32Little(w, m_pitchSampleRate);
    amuse::io::writeUint32Little(w, m_numSamples);
    amuse::io::writeUint32Little(w, m_loopStartSample);
    amuse::io::writeUint32Little(w, m_loopLengthSamples);
}
template <> size_t AudioGroupSampleDirectory::MusyX1SdirEntry<amuse::Endian::Little>::binarySize(size_t s) const { s += 24; return s; }
template <> void AudioGroupSampleDirectory::MusyX1SdirEntry<amuse::Endian::Little>::readYaml(amuse::io::YAMLDocReader&) {}
template <> void AudioGroupSampleDirectory::MusyX1SdirEntry<amuse::Endian::Little>::writeYaml(amuse::io::YAMLDocWriter&) const {}
template struct AudioGroupSampleDirectory::MusyX1SdirEntry<amuse::Endian::Big>;
template struct AudioGroupSampleDirectory::MusyX1SdirEntry<amuse::Endian::Little>;

// ── MusyX1AbsSdirEntry<Big/Little> ────────────────────────────────────────────
template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<amuse::Endian::Big>::read(std::istream& r) {
    m_sfxId.read(r);
    r.seekg(2, std::ios_base::cur);
    m_sampleOff       = amuse::io::readUint32Big(r);
    m_unk             = amuse::io::readUint32Big(r);
    m_pitchSampleRate = amuse::io::readUint32Big(r);
    m_numSamples      = amuse::io::readUint32Big(r);
    m_loopStartSample = amuse::io::readUint32Big(r);
    m_loopLengthSamples = amuse::io::readUint32Big(r);
}
template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<amuse::Endian::Big>::write(std::ostream& w) const {
    m_sfxId.write(w);
    uint8_t pad[2] = {};
    amuse::io::writeBytes(w, pad, 2);
    amuse::io::writeUint32Big(w, m_sampleOff);
    amuse::io::writeUint32Big(w, m_unk);
    amuse::io::writeUint32Big(w, m_pitchSampleRate);
    amuse::io::writeUint32Big(w, m_numSamples);
    amuse::io::writeUint32Big(w, m_loopStartSample);
    amuse::io::writeUint32Big(w, m_loopLengthSamples);
}
template <> size_t AudioGroupSampleDirectory::MusyX1AbsSdirEntry<amuse::Endian::Big>::binarySize(size_t s) const { s += 28; return s; }
template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<amuse::Endian::Big>::readYaml(amuse::io::YAMLDocReader&) {}
template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<amuse::Endian::Big>::writeYaml(amuse::io::YAMLDocWriter&) const {}

template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<amuse::Endian::Little>::read(std::istream& r) {
    m_sfxId.read(r);
    r.seekg(2, std::ios_base::cur);
    m_sampleOff       = amuse::io::readUint32Little(r);
    m_unk             = amuse::io::readUint32Little(r);
    m_pitchSampleRate = amuse::io::readUint32Little(r);
    m_numSamples      = amuse::io::readUint32Little(r);
    m_loopStartSample = amuse::io::readUint32Little(r);
    m_loopLengthSamples = amuse::io::readUint32Little(r);
}
template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<amuse::Endian::Little>::write(std::ostream& w) const {
    m_sfxId.write(w);
    uint8_t pad[2] = {};
    amuse::io::writeBytes(w, pad, 2);
    amuse::io::writeUint32Little(w, m_sampleOff);
    amuse::io::writeUint32Little(w, m_unk);
    amuse::io::writeUint32Little(w, m_pitchSampleRate);
    amuse::io::writeUint32Little(w, m_numSamples);
    amuse::io::writeUint32Little(w, m_loopStartSample);
    amuse::io::writeUint32Little(w, m_loopLengthSamples);
}
template <> size_t AudioGroupSampleDirectory::MusyX1AbsSdirEntry<amuse::Endian::Little>::binarySize(size_t s) const { s += 28; return s; }
template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<amuse::Endian::Little>::readYaml(amuse::io::YAMLDocReader&) {}
template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<amuse::Endian::Little>::writeYaml(amuse::io::YAMLDocWriter&) const {}
template struct AudioGroupSampleDirectory::MusyX1AbsSdirEntry<amuse::Endian::Big>;
template struct AudioGroupSampleDirectory::MusyX1AbsSdirEntry<amuse::Endian::Little>;

// DNAType for template struct instantiations
template <amuse::Endian DNAE>
std::string_view AudioGroupSampleDirectory::EntryDNA<DNAE>::DNAType() {
    return "amuse::AudioGroupSampleDirectory::EntryDNA";
}

template <amuse::Endian DNAE>
std::string_view AudioGroupSampleDirectory::MusyX1SdirEntry<DNAE>::DNAType() {
    return "amuse::AudioGroupSampleDirectory::MusyX1SdirEntry";
}

template <amuse::Endian DNAE>
std::string_view AudioGroupSampleDirectory::MusyX1AbsSdirEntry<DNAE>::DNAType() {
    return "amuse::AudioGroupSampleDirectory::MusyX1AbsSdirEntry";
}

} // namespace amuse
