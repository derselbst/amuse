// Manually-generated serialization code replacing target_atdna output for
// include/amuse/AudioGroupSampleDirectory.hpp

#include "amuse/AudioGroupSampleDirectory.hpp"

using namespace athena::io;

namespace amuse {

// ── DSPADPCMHeader ────────────────────────────────────────────────────────────
template <> void DSPADPCMHeader::Enumerate<DNAOpRead>(IStreamReader& r) {
    x0_num_samples.v  = r.readUint32Big();
    x4_num_nibbles.v  = r.readUint32Big();
    x8_sample_rate.v  = r.readUint32Big();
    xc_loop_flag.v    = r.readUint16Big();
    xe_format.v       = r.readUint16Big();
    x10_loop_start_nibble.v = r.readUint32Big();
    x14_loop_end_nibble.v   = r.readUint32Big();
    x18_ca.v          = r.readUint32Big();
    for (int i = 0; i < 8; ++i) {
        x1c_coef[i][0].v = r.readInt16Big();
        x1c_coef[i][1].v = r.readInt16Big();
    }
    x3c_gain.v        = r.readInt16Big();
    x3e_ps.v          = r.readInt16Big();
    x40_hist1.v       = r.readInt16Big();
    x42_hist2.v       = r.readInt16Big();
    x44_loop_ps.v     = r.readInt16Big();
    x46_loop_hist1.v  = r.readInt16Big();
    x48_loop_hist2.v  = r.readInt16Big();
    m_pitch.v         = r.readUByte();
    r.seek(21, athena::SeekOrigin::Current);
}
template <> void DSPADPCMHeader::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUint32Big(x0_num_samples.v);
    w.writeUint32Big(x4_num_nibbles.v);
    w.writeUint32Big(x8_sample_rate.v);
    w.writeUint16Big(xc_loop_flag.v);
    w.writeUint16Big(xe_format.v);
    w.writeUint32Big(x10_loop_start_nibble.v);
    w.writeUint32Big(x14_loop_end_nibble.v);
    w.writeUint32Big(x18_ca.v);
    for (int i = 0; i < 8; ++i) {
        w.writeInt16Big(x1c_coef[i][0].v);
        w.writeInt16Big(x1c_coef[i][1].v);
    }
    w.writeInt16Big(x3c_gain.v);
    w.writeInt16Big(x3e_ps.v);
    w.writeInt16Big(x40_hist1.v);
    w.writeInt16Big(x42_hist2.v);
    w.writeInt16Big(x44_loop_ps.v);
    w.writeInt16Big(x46_loop_hist1.v);
    w.writeInt16Big(x48_loop_hist2.v);
    w.writeUByte(m_pitch.v);
    uint8_t pad[21] = {};
    w.writeBytes(pad, 21);
}
template <> void DSPADPCMHeader::Enumerate<DNAOpBinarySize>(size_t& s) { s += 96; }
template <> void DSPADPCMHeader::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void DSPADPCMHeader::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
std::string_view DSPADPCMHeader::DNAType() { return "amuse::DSPADPCMHeader"; }

// ── VADPCMHeader ──────────────────────────────────────────────────────────────
template <> void VADPCMHeader::Enumerate<DNAOpRead>(IStreamReader& r) {
    m_pitchSampleRate.v   = r.readUint32Big();
    m_numSamples.v        = r.readUint32Big();
    m_loopStartSample.v   = r.readUint32Big();
    m_loopLengthSamples.v = r.readUint32Big();
}
template <> void VADPCMHeader::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUint32Big(m_pitchSampleRate.v);
    w.writeUint32Big(m_numSamples.v);
    w.writeUint32Big(m_loopStartSample.v);
    w.writeUint32Big(m_loopLengthSamples.v);
}
template <> void VADPCMHeader::Enumerate<DNAOpBinarySize>(size_t& s) { s += 16; }
template <> void VADPCMHeader::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void VADPCMHeader::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
std::string_view VADPCMHeader::DNAType() { return "amuse::VADPCMHeader"; }

// ── WAVFormatChunk ────────────────────────────────────────────────────────────
template <> void WAVFormatChunk::Enumerate<DNAOpRead>(IStreamReader& r) {
    sampleFmt.v    = r.readUint16Little();
    numChannels.v  = r.readUint16Little();
    sampleRate.v   = r.readUint32Little();
    byteRate.v     = r.readUint32Little();
    blockAlign.v   = r.readUint16Little();
    bitsPerSample.v = r.readUint16Little();
}
template <> void WAVFormatChunk::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUint16Little(sampleFmt.v);
    w.writeUint16Little(numChannels.v);
    w.writeUint32Little(sampleRate.v);
    w.writeUint32Little(byteRate.v);
    w.writeUint16Little(blockAlign.v);
    w.writeUint16Little(bitsPerSample.v);
}
template <> void WAVFormatChunk::Enumerate<DNAOpBinarySize>(size_t& s) { s += 16; }
template <> void WAVFormatChunk::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void WAVFormatChunk::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
std::string_view WAVFormatChunk::DNAType() { return "amuse::WAVFormatChunk"; }

// ── WAVSampleChunk ────────────────────────────────────────────────────────────
template <> void WAVSampleChunk::Enumerate<DNAOpRead>(IStreamReader& r) {
    smplManufacturer.v  = r.readUint32Little();
    smplProduct.v       = r.readUint32Little();
    smplPeriod.v        = r.readUint32Little();
    midiNote.v          = r.readUint32Little();
    midiPitchFrac.v     = r.readUint32Little();
    smpteFormat.v       = r.readUint32Little();
    smpteOffset.v       = r.readUint32Little();
    numSampleLoops.v    = r.readUint32Little();
    additionalDataSize.v = r.readUint32Little();
}
template <> void WAVSampleChunk::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUint32Little(smplManufacturer.v);
    w.writeUint32Little(smplProduct.v);
    w.writeUint32Little(smplPeriod.v);
    w.writeUint32Little(midiNote.v);
    w.writeUint32Little(midiPitchFrac.v);
    w.writeUint32Little(smpteFormat.v);
    w.writeUint32Little(smpteOffset.v);
    w.writeUint32Little(numSampleLoops.v);
    w.writeUint32Little(additionalDataSize.v);
}
template <> void WAVSampleChunk::Enumerate<DNAOpBinarySize>(size_t& s) { s += 36; }
template <> void WAVSampleChunk::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void WAVSampleChunk::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
std::string_view WAVSampleChunk::DNAType() { return "amuse::WAVSampleChunk"; }

// ── WAVSampleLoop ─────────────────────────────────────────────────────────────
template <> void WAVSampleLoop::Enumerate<DNAOpRead>(IStreamReader& r) {
    cuePointId.v = r.readUint32Little();
    loopType.v   = r.readUint32Little();
    start.v      = r.readUint32Little();
    end.v        = r.readUint32Little();
    fraction.v   = r.readUint32Little();
    playCount.v  = r.readUint32Little();
}
template <> void WAVSampleLoop::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUint32Little(cuePointId.v);
    w.writeUint32Little(loopType.v);
    w.writeUint32Little(start.v);
    w.writeUint32Little(end.v);
    w.writeUint32Little(fraction.v);
    w.writeUint32Little(playCount.v);
}
template <> void WAVSampleLoop::Enumerate<DNAOpBinarySize>(size_t& s) { s += 24; }
template <> void WAVSampleLoop::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void WAVSampleLoop::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
std::string_view WAVSampleLoop::DNAType() { return "amuse::WAVSampleLoop"; }

// ── WAVHeader ─────────────────────────────────────────────────────────────────
template <> void WAVHeader::Enumerate<DNAOpRead>(IStreamReader& r) {
    riffMagic.v    = r.readUint32Little();
    wavChuckSize.v = r.readUint32Little();
    wavMagic.v     = r.readUint32Little();
    fmtMagic.v     = r.readUint32Little();
    fmtChunkSize.v = r.readUint32Little();
    fmtChunk.read(r);
    smplMagic.v    = r.readUint32Little();
    smplChunkSize.v = r.readUint32Little();
    smplChunk.read(r);
    dataMagic.v    = r.readUint32Little();
    dataChunkSize.v = r.readUint32Little();
}
template <> void WAVHeader::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUint32Little(riffMagic.v);
    w.writeUint32Little(wavChuckSize.v);
    w.writeUint32Little(wavMagic.v);
    w.writeUint32Little(fmtMagic.v);
    w.writeUint32Little(fmtChunkSize.v);
    fmtChunk.write(w);
    w.writeUint32Little(smplMagic.v);
    w.writeUint32Little(smplChunkSize.v);
    smplChunk.write(w);
    w.writeUint32Little(dataMagic.v);
    w.writeUint32Little(dataChunkSize.v);
}
template <> void WAVHeader::Enumerate<DNAOpBinarySize>(size_t& s) { s += 12 + 8 + 16 + 8 + 36 + 8; }
template <> void WAVHeader::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void WAVHeader::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
std::string_view WAVHeader::DNAType() { return "amuse::WAVHeader"; }

// ── WAVHeaderLoop ─────────────────────────────────────────────────────────────
template <> void WAVHeaderLoop::Enumerate<DNAOpRead>(IStreamReader& r) {
    riffMagic.v    = r.readUint32Little();
    wavChuckSize.v = r.readUint32Little();
    wavMagic.v     = r.readUint32Little();
    fmtMagic.v     = r.readUint32Little();
    fmtChunkSize.v = r.readUint32Little();
    fmtChunk.read(r);
    smplMagic.v    = r.readUint32Little();
    smplChunkSize.v = r.readUint32Little();
    smplChunk.read(r);
    sampleLoop.read(r);
    dataMagic.v    = r.readUint32Little();
    dataChunkSize.v = r.readUint32Little();
}
template <> void WAVHeaderLoop::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUint32Little(riffMagic.v);
    w.writeUint32Little(wavChuckSize.v);
    w.writeUint32Little(wavMagic.v);
    w.writeUint32Little(fmtMagic.v);
    w.writeUint32Little(fmtChunkSize.v);
    fmtChunk.write(w);
    w.writeUint32Little(smplMagic.v);
    w.writeUint32Little(smplChunkSize.v);
    smplChunk.write(w);
    sampleLoop.write(w);
    w.writeUint32Little(dataMagic.v);
    w.writeUint32Little(dataChunkSize.v);
}
template <> void WAVHeaderLoop::Enumerate<DNAOpBinarySize>(size_t& s) { s += 12 + 8 + 16 + 8 + 36 + 24 + 8; }
template <> void WAVHeaderLoop::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void WAVHeaderLoop::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
std::string_view WAVHeaderLoop::DNAType() { return "amuse::WAVHeaderLoop"; }

// ── EntryDNA<Big> ─────────────────────────────────────────────────────────────
template <> template <> void AudioGroupSampleDirectory::EntryDNA<athena::Endian::Big>::Enumerate<DNAOpRead>(IStreamReader& r) {
    m_sfxId.read(r);
    r.seek(2, athena::SeekOrigin::Current);
    m_sampleOff.v    = r.readUint32Big();
    m_unk.v          = r.readUint32Big();
    m_pitch.v        = r.readUByte();
    r.seek(1, athena::SeekOrigin::Current);
    m_sampleRate.v   = r.readUint16Big();
    m_numSamples.v   = r.readUint32Big();
    m_loopStartSample.v   = r.readUint32Big();
    m_loopLengthSamples.v = r.readUint32Big();
    m_adpcmParmOffset.v   = r.readUint32Big();
}
template <> template <> void AudioGroupSampleDirectory::EntryDNA<athena::Endian::Big>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    m_sfxId.write(w);
    uint8_t pad2[2] = {};
    w.writeBytes(pad2, 2);
    w.writeUint32Big(m_sampleOff.v);
    w.writeUint32Big(m_unk.v);
    w.writeUByte(m_pitch.v);
    uint8_t pad1[1] = {};
    w.writeBytes(pad1, 1);
    w.writeUint16Big(m_sampleRate.v);
    w.writeUint32Big(m_numSamples.v);
    w.writeUint32Big(m_loopStartSample.v);
    w.writeUint32Big(m_loopLengthSamples.v);
    w.writeUint32Big(m_adpcmParmOffset.v);
}
template <> template <> void AudioGroupSampleDirectory::EntryDNA<athena::Endian::Big>::Enumerate<DNAOpBinarySize>(size_t& s) { s += 24; }
template <> template <> void AudioGroupSampleDirectory::EntryDNA<athena::Endian::Big>::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> template <> void AudioGroupSampleDirectory::EntryDNA<athena::Endian::Big>::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}

template <> template <> void AudioGroupSampleDirectory::EntryDNA<athena::Endian::Little>::Enumerate<DNAOpRead>(IStreamReader& r) {
    m_sfxId.read(r);
    r.seek(2, athena::SeekOrigin::Current);
    m_sampleOff.v    = r.readUint32Little();
    m_unk.v          = r.readUint32Little();
    m_pitch.v        = r.readUByte();
    r.seek(1, athena::SeekOrigin::Current);
    m_sampleRate.v   = r.readUint16Little();
    m_numSamples.v   = r.readUint32Little();
    m_loopStartSample.v   = r.readUint32Little();
    m_loopLengthSamples.v = r.readUint32Little();
    m_adpcmParmOffset.v   = r.readUint32Little();
}
template <> template <> void AudioGroupSampleDirectory::EntryDNA<athena::Endian::Little>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    m_sfxId.write(w);
    uint8_t pad2[2] = {};
    w.writeBytes(pad2, 2);
    w.writeUint32Little(m_sampleOff.v);
    w.writeUint32Little(m_unk.v);
    w.writeUByte(m_pitch.v);
    uint8_t pad1[1] = {};
    w.writeBytes(pad1, 1);
    w.writeUint16Little(m_sampleRate.v);
    w.writeUint32Little(m_numSamples.v);
    w.writeUint32Little(m_loopStartSample.v);
    w.writeUint32Little(m_loopLengthSamples.v);
    w.writeUint32Little(m_adpcmParmOffset.v);
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
    m_sampleOff.v       = r.readUint32Big();
    m_pitchSampleRate.v = r.readUint32Big();
    m_numSamples.v      = r.readUint32Big();
    m_loopStartSample.v = r.readUint32Big();
    m_loopLengthSamples.v = r.readUint32Big();
}
template <> template <> void AudioGroupSampleDirectory::MusyX1SdirEntry<athena::Endian::Big>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    m_sfxId.write(w);
    uint8_t pad[2] = {};
    w.writeBytes(pad, 2);
    w.writeUint32Big(m_sampleOff.v);
    w.writeUint32Big(m_pitchSampleRate.v);
    w.writeUint32Big(m_numSamples.v);
    w.writeUint32Big(m_loopStartSample.v);
    w.writeUint32Big(m_loopLengthSamples.v);
}
template <> template <> void AudioGroupSampleDirectory::MusyX1SdirEntry<athena::Endian::Big>::Enumerate<DNAOpBinarySize>(size_t& s) { s += 24; }
template <> template <> void AudioGroupSampleDirectory::MusyX1SdirEntry<athena::Endian::Big>::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> template <> void AudioGroupSampleDirectory::MusyX1SdirEntry<athena::Endian::Big>::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}

template <> template <> void AudioGroupSampleDirectory::MusyX1SdirEntry<athena::Endian::Little>::Enumerate<DNAOpRead>(IStreamReader& r) {
    m_sfxId.read(r);
    r.seek(2, athena::SeekOrigin::Current);
    m_sampleOff.v       = r.readUint32Little();
    m_pitchSampleRate.v = r.readUint32Little();
    m_numSamples.v      = r.readUint32Little();
    m_loopStartSample.v = r.readUint32Little();
    m_loopLengthSamples.v = r.readUint32Little();
}
template <> template <> void AudioGroupSampleDirectory::MusyX1SdirEntry<athena::Endian::Little>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    m_sfxId.write(w);
    uint8_t pad[2] = {};
    w.writeBytes(pad, 2);
    w.writeUint32Little(m_sampleOff.v);
    w.writeUint32Little(m_pitchSampleRate.v);
    w.writeUint32Little(m_numSamples.v);
    w.writeUint32Little(m_loopStartSample.v);
    w.writeUint32Little(m_loopLengthSamples.v);
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
    m_sampleOff.v       = r.readUint32Big();
    m_unk.v             = r.readUint32Big();
    m_pitchSampleRate.v = r.readUint32Big();
    m_numSamples.v      = r.readUint32Big();
    m_loopStartSample.v = r.readUint32Big();
    m_loopLengthSamples.v = r.readUint32Big();
}
template <> template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<athena::Endian::Big>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    m_sfxId.write(w);
    uint8_t pad[2] = {};
    w.writeBytes(pad, 2);
    w.writeUint32Big(m_sampleOff.v);
    w.writeUint32Big(m_unk.v);
    w.writeUint32Big(m_pitchSampleRate.v);
    w.writeUint32Big(m_numSamples.v);
    w.writeUint32Big(m_loopStartSample.v);
    w.writeUint32Big(m_loopLengthSamples.v);
}
template <> template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<athena::Endian::Big>::Enumerate<DNAOpBinarySize>(size_t& s) { s += 28; }
template <> template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<athena::Endian::Big>::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<athena::Endian::Big>::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}

template <> template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<athena::Endian::Little>::Enumerate<DNAOpRead>(IStreamReader& r) {
    m_sfxId.read(r);
    r.seek(2, athena::SeekOrigin::Current);
    m_sampleOff.v       = r.readUint32Little();
    m_unk.v             = r.readUint32Little();
    m_pitchSampleRate.v = r.readUint32Little();
    m_numSamples.v      = r.readUint32Little();
    m_loopStartSample.v = r.readUint32Little();
    m_loopLengthSamples.v = r.readUint32Little();
}
template <> template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<athena::Endian::Little>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    m_sfxId.write(w);
    uint8_t pad[2] = {};
    w.writeBytes(pad, 2);
    w.writeUint32Little(m_sampleOff.v);
    w.writeUint32Little(m_unk.v);
    w.writeUint32Little(m_pitchSampleRate.v);
    w.writeUint32Little(m_numSamples.v);
    w.writeUint32Little(m_loopStartSample.v);
    w.writeUint32Little(m_loopLengthSamples.v);
}
template <> template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<athena::Endian::Little>::Enumerate<DNAOpBinarySize>(size_t& s) { s += 28; }
template <> template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<athena::Endian::Little>::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> template <> void AudioGroupSampleDirectory::MusyX1AbsSdirEntry<athena::Endian::Little>::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
template struct AudioGroupSampleDirectory::MusyX1AbsSdirEntry<athena::Endian::Big>;
template struct AudioGroupSampleDirectory::MusyX1AbsSdirEntry<athena::Endian::Little>;

} // namespace amuse
