// Manually-generated serialization code replacing target_atdna output for
// include/amuse/AudioGroupProject.hpp

#include "amuse/AudioGroupProject.hpp"

namespace amuse {
using namespace athena::io;

// =============================================================================
// GroupHeader<E>
// =============================================================================
template <> template <> void GroupHeader<athena::Endian::Big>::Enumerate<DNAOpRead>(IStreamReader& r) {
    groupEndOff = r.readUint32Big();
    groupId.read(r);
    type = static_cast<GroupType>(r.readUint16Big());
    soundMacroIdsOff = r.readUint32Big();
    samplIdsOff = r.readUint32Big();
    tableIdsOff = r.readUint32Big();
    keymapIdsOff = r.readUint32Big();
    layerIdsOff = r.readUint32Big();
    pageTableOff = r.readUint32Big();
    drumTableOff = r.readUint32Big();
    midiSetupsOff = r.readUint32Big();
}
template <> template <> void GroupHeader<athena::Endian::Big>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUint32Big(groupEndOff);
    groupId.write(w);
    w.writeUint16Big(static_cast<uint16_t>(static_cast<GroupType>(type)));
    w.writeUint32Big(soundMacroIdsOff);
    w.writeUint32Big(samplIdsOff);
    w.writeUint32Big(tableIdsOff);
    w.writeUint32Big(keymapIdsOff);
    w.writeUint32Big(layerIdsOff);
    w.writeUint32Big(pageTableOff);
    w.writeUint32Big(drumTableOff);
    w.writeUint32Big(midiSetupsOff);
}
template <> template <> void GroupHeader<athena::Endian::Big>::Enumerate<DNAOpBinarySize>(size_t& s) { s += 40; }
template <> template <> void GroupHeader<athena::Endian::Big>::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> template <> void GroupHeader<athena::Endian::Big>::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}

template <> template <> void GroupHeader<athena::Endian::Little>::Enumerate<DNAOpRead>(IStreamReader& r) {
    groupEndOff = r.readUint32Little();
    groupId.read(r);
    type = static_cast<GroupType>(r.readUint16Little());
    soundMacroIdsOff = r.readUint32Little();
    samplIdsOff = r.readUint32Little();
    tableIdsOff = r.readUint32Little();
    keymapIdsOff = r.readUint32Little();
    layerIdsOff = r.readUint32Little();
    pageTableOff = r.readUint32Little();
    drumTableOff = r.readUint32Little();
    midiSetupsOff = r.readUint32Little();
}
template <> template <> void GroupHeader<athena::Endian::Little>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUint32Little(groupEndOff);
    groupId.write(w);
    w.writeUint16Little(static_cast<uint16_t>(static_cast<GroupType>(type)));
    w.writeUint32Little(soundMacroIdsOff);
    w.writeUint32Little(samplIdsOff);
    w.writeUint32Little(tableIdsOff);
    w.writeUint32Little(keymapIdsOff);
    w.writeUint32Little(layerIdsOff);
    w.writeUint32Little(pageTableOff);
    w.writeUint32Little(drumTableOff);
    w.writeUint32Little(midiSetupsOff);
}
template <> template <> void GroupHeader<athena::Endian::Little>::Enumerate<DNAOpBinarySize>(size_t& s) { s += 40; }
template <> template <> void GroupHeader<athena::Endian::Little>::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> template <> void GroupHeader<athena::Endian::Little>::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}

template <athena::Endian DNAE>
std::string_view GroupHeader<DNAE>::DNAType() { return "amuse::GroupHeader"; }
template struct GroupHeader<athena::Endian::Big>;
template struct GroupHeader<athena::Endian::Little>;

// =============================================================================
// SongGroupIndex::PageEntryDNA<E> (AT_DECL_DNA_YAML)
// =============================================================================
template <> template <> void SongGroupIndex::PageEntryDNA<athena::Endian::Big>::Enumerate<DNAOpRead>(IStreamReader& r) {
    objId.read(r);
    priority = r.readUByte(); maxVoices = r.readUByte(); programNo = r.readUByte();
    r.seek(1, athena::SeekOrigin::Current);
}
template <> template <> void SongGroupIndex::PageEntryDNA<athena::Endian::Big>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    objId.write(w);
    w.writeUByte(priority); w.writeUByte(maxVoices); w.writeUByte(programNo);
    uint8_t pad = 0; w.writeUByte(pad);
}
template <> template <> void SongGroupIndex::PageEntryDNA<athena::Endian::Big>::Enumerate<DNAOpBinarySize>(size_t& s) { s += 6; }
template <> template <> void SongGroupIndex::PageEntryDNA<athena::Endian::Big>::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> template <> void SongGroupIndex::PageEntryDNA<athena::Endian::Big>::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}

template <> template <> void SongGroupIndex::PageEntryDNA<athena::Endian::Little>::Enumerate<DNAOpRead>(IStreamReader& r) {
    objId.read(r);
    priority = r.readUByte(); maxVoices = r.readUByte(); programNo = r.readUByte();
    r.seek(1, athena::SeekOrigin::Current);
}
template <> template <> void SongGroupIndex::PageEntryDNA<athena::Endian::Little>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    objId.write(w);
    w.writeUByte(priority); w.writeUByte(maxVoices); w.writeUByte(programNo);
    uint8_t pad = 0; w.writeUByte(pad);
}
template <> template <> void SongGroupIndex::PageEntryDNA<athena::Endian::Little>::Enumerate<DNAOpBinarySize>(size_t& s) { s += 6; }
template <> template <> void SongGroupIndex::PageEntryDNA<athena::Endian::Little>::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> template <> void SongGroupIndex::PageEntryDNA<athena::Endian::Little>::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}

template <athena::Endian DNAE>
std::string_view SongGroupIndex::PageEntryDNA<DNAE>::DNAType() { return "amuse::SongGroupIndex::PageEntryDNA"; }
template struct SongGroupIndex::PageEntryDNA<athena::Endian::Big>;
template struct SongGroupIndex::PageEntryDNA<athena::Endian::Little>;

// =============================================================================
// SongGroupIndex::MusyX1PageEntryDNA<E> (AT_DECL_DNA)
// =============================================================================
template <> template <> void SongGroupIndex::MusyX1PageEntryDNA<athena::Endian::Big>::Enumerate<DNAOpRead>(IStreamReader& r) {
    objId.read(r);
    priority = r.readUByte(); maxVoices = r.readUByte(); unk = r.readUByte(); programNo = r.readUByte();
    r.seek(2, athena::SeekOrigin::Current);
}
template <> template <> void SongGroupIndex::MusyX1PageEntryDNA<athena::Endian::Big>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    objId.write(w);
    w.writeUByte(priority); w.writeUByte(maxVoices); w.writeUByte(unk); w.writeUByte(programNo);
    uint8_t pad[2] = {}; w.writeBytes(pad, 2);
}
template <> template <> void SongGroupIndex::MusyX1PageEntryDNA<athena::Endian::Big>::Enumerate<DNAOpBinarySize>(size_t& s) { s += 8; }
template <> template <> void SongGroupIndex::MusyX1PageEntryDNA<athena::Endian::Big>::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> template <> void SongGroupIndex::MusyX1PageEntryDNA<athena::Endian::Big>::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}

template <> template <> void SongGroupIndex::MusyX1PageEntryDNA<athena::Endian::Little>::Enumerate<DNAOpRead>(IStreamReader& r) {
    objId.read(r);
    priority = r.readUByte(); maxVoices = r.readUByte(); unk = r.readUByte(); programNo = r.readUByte();
    r.seek(2, athena::SeekOrigin::Current);
}
template <> template <> void SongGroupIndex::MusyX1PageEntryDNA<athena::Endian::Little>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    objId.write(w);
    w.writeUByte(priority); w.writeUByte(maxVoices); w.writeUByte(unk); w.writeUByte(programNo);
    uint8_t pad[2] = {}; w.writeBytes(pad, 2);
}
template <> template <> void SongGroupIndex::MusyX1PageEntryDNA<athena::Endian::Little>::Enumerate<DNAOpBinarySize>(size_t& s) { s += 8; }
template <> template <> void SongGroupIndex::MusyX1PageEntryDNA<athena::Endian::Little>::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> template <> void SongGroupIndex::MusyX1PageEntryDNA<athena::Endian::Little>::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}

template <athena::Endian DNAE>
std::string_view SongGroupIndex::MusyX1PageEntryDNA<DNAE>::DNAType() { return "amuse::SongGroupIndex::MusyX1PageEntryDNA"; }
template struct SongGroupIndex::MusyX1PageEntryDNA<athena::Endian::Big>;
template struct SongGroupIndex::MusyX1PageEntryDNA<athena::Endian::Little>;

// =============================================================================
// SongGroupIndex::PageEntry (BigDNA, AT_DECL_DNA_YAML)
// =============================================================================
template <> void SongGroupIndex::PageEntry::Enumerate<DNAOpRead>(IStreamReader& r) {
    objId.read(r); priority = r.readUByte(); maxVoices = r.readUByte();
}
template <> void SongGroupIndex::PageEntry::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    objId.write(w); w.writeUByte(priority); w.writeUByte(maxVoices);
}
template <> void SongGroupIndex::PageEntry::Enumerate<DNAOpBinarySize>(size_t& s) { s += 4; }
template <> void SongGroupIndex::PageEntry::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SongGroupIndex::PageEntry::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
std::string_view SongGroupIndex::PageEntry::DNAType() { return "amuse::SongGroupIndex::PageEntry"; }

// =============================================================================
// SongGroupIndex::MusyX1MIDISetup (BigDNA, AT_DECL_DNA_YAML)
// =============================================================================
template <> void SongGroupIndex::MusyX1MIDISetup::Enumerate<DNAOpRead>(IStreamReader& r) {
    programNo = r.readUByte(); volume = r.readUByte(); panning = r.readUByte();
    reverb = r.readUByte(); chorus = r.readUByte();
    r.seek(3, athena::SeekOrigin::Current);
}
template <> void SongGroupIndex::MusyX1MIDISetup::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUByte(programNo); w.writeUByte(volume); w.writeUByte(panning);
    w.writeUByte(reverb); w.writeUByte(chorus);
    uint8_t pad[3] = {}; w.writeBytes(pad, 3);
}
template <> void SongGroupIndex::MusyX1MIDISetup::Enumerate<DNAOpBinarySize>(size_t& s) { s += 8; }
template <> void SongGroupIndex::MusyX1MIDISetup::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SongGroupIndex::MusyX1MIDISetup::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
std::string_view SongGroupIndex::MusyX1MIDISetup::DNAType() { return "amuse::SongGroupIndex::MusyX1MIDISetup"; }

// =============================================================================
// SongGroupIndex::MIDISetup (BigDNA, AT_DECL_DNA_YAML)
// =============================================================================
template <> void SongGroupIndex::MIDISetup::Enumerate<DNAOpRead>(IStreamReader& r) {
    programNo = r.readUByte(); volume = r.readUByte(); panning = r.readUByte();
    reverb = r.readUByte(); chorus = r.readUByte();
}
template <> void SongGroupIndex::MIDISetup::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUByte(programNo); w.writeUByte(volume); w.writeUByte(panning);
    w.writeUByte(reverb); w.writeUByte(chorus);
}
template <> void SongGroupIndex::MIDISetup::Enumerate<DNAOpBinarySize>(size_t& s) { s += 5; }
template <> void SongGroupIndex::MIDISetup::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SongGroupIndex::MIDISetup::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
std::string_view SongGroupIndex::MIDISetup::DNAType() { return "amuse::SongGroupIndex::MIDISetup"; }

// =============================================================================
// SFXGroupIndex::SFXEntryDNA<E> (BigDNA template, AT_DECL_DNA)
// =============================================================================
template <> template <> void SFXGroupIndex::SFXEntryDNA<athena::Endian::Big>::Enumerate<DNAOpRead>(IStreamReader& r) {
    sfxId.read(r); objId.read(r);
    priority = r.readUByte(); maxVoices = r.readUByte(); defVel = r.readUByte();
    panning = r.readUByte(); defKey = r.readUByte();
    r.seek(1, athena::SeekOrigin::Current);
}
template <> template <> void SFXGroupIndex::SFXEntryDNA<athena::Endian::Big>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    sfxId.write(w); objId.write(w);
    w.writeUByte(priority); w.writeUByte(maxVoices); w.writeUByte(defVel);
    w.writeUByte(panning); w.writeUByte(defKey);
    uint8_t pad = 0; w.writeUByte(pad);
}
template <> template <> void SFXGroupIndex::SFXEntryDNA<athena::Endian::Big>::Enumerate<DNAOpBinarySize>(size_t& s) { s += 10; }
template <> template <> void SFXGroupIndex::SFXEntryDNA<athena::Endian::Big>::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> template <> void SFXGroupIndex::SFXEntryDNA<athena::Endian::Big>::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}

template <> template <> void SFXGroupIndex::SFXEntryDNA<athena::Endian::Little>::Enumerate<DNAOpRead>(IStreamReader& r) {
    sfxId.read(r); objId.read(r);
    priority = r.readUByte(); maxVoices = r.readUByte(); defVel = r.readUByte();
    panning = r.readUByte(); defKey = r.readUByte();
    r.seek(1, athena::SeekOrigin::Current);
}
template <> template <> void SFXGroupIndex::SFXEntryDNA<athena::Endian::Little>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    sfxId.write(w); objId.write(w);
    w.writeUByte(priority); w.writeUByte(maxVoices); w.writeUByte(defVel);
    w.writeUByte(panning); w.writeUByte(defKey);
    uint8_t pad = 0; w.writeUByte(pad);
}
template <> template <> void SFXGroupIndex::SFXEntryDNA<athena::Endian::Little>::Enumerate<DNAOpBinarySize>(size_t& s) { s += 10; }
template <> template <> void SFXGroupIndex::SFXEntryDNA<athena::Endian::Little>::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> template <> void SFXGroupIndex::SFXEntryDNA<athena::Endian::Little>::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}

template <athena::Endian DNAE>
std::string_view SFXGroupIndex::SFXEntryDNA<DNAE>::DNAType() { return "amuse::SFXGroupIndex::SFXEntryDNA"; }
template struct SFXGroupIndex::SFXEntryDNA<athena::Endian::Big>;
template struct SFXGroupIndex::SFXEntryDNA<athena::Endian::Little>;

// =============================================================================
// SFXGroupIndex::SFXEntry (BigDNA, AT_DECL_DNA_YAML)
// =============================================================================
template <> void SFXGroupIndex::SFXEntry::Enumerate<DNAOpRead>(IStreamReader& r) {
    objId.read(r); priority = r.readUByte(); maxVoices = r.readUByte();
    defVel = r.readUByte(); panning = r.readUByte(); defKey = r.readUByte();
}
template <> void SFXGroupIndex::SFXEntry::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    objId.write(w); w.writeUByte(priority); w.writeUByte(maxVoices);
    w.writeUByte(defVel); w.writeUByte(panning); w.writeUByte(defKey);
}
template <> void SFXGroupIndex::SFXEntry::Enumerate<DNAOpBinarySize>(size_t& s) { s += 7; }
template <> void SFXGroupIndex::SFXEntry::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SFXGroupIndex::SFXEntry::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
std::string_view SFXGroupIndex::SFXEntry::DNAType() { return "amuse::SFXGroupIndex::SFXEntry"; }

} // namespace amuse
