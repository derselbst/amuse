// Manually-generated serialization code replacing target_atdna output for
// include/amuse/AudioGroupProject.hpp

#include "amuse/AudioGroupProject.hpp"

namespace amuse {

// =============================================================================
// GroupHeader<E>
// =============================================================================
template <> void GroupHeader<std::endian::big>::read(std::istream& r) {
    groupEndOff = amuse::io::readUint32Big(r);
    groupId.read(r);
    type = static_cast<GroupType>(amuse::io::readUint16Big(r));
    soundMacroIdsOff = amuse::io::readUint32Big(r);
    samplIdsOff = amuse::io::readUint32Big(r);
    tableIdsOff = amuse::io::readUint32Big(r);
    keymapIdsOff = amuse::io::readUint32Big(r);
    layerIdsOff = amuse::io::readUint32Big(r);
    pageTableOff = amuse::io::readUint32Big(r);
    drumTableOff = amuse::io::readUint32Big(r);
    midiSetupsOff = amuse::io::readUint32Big(r);
}
template <> void GroupHeader<std::endian::big>::write(std::ostream& w) const {
    amuse::io::writeUint32Big(w, groupEndOff);
    groupId.write(w);
    amuse::io::writeUint16Big(w, static_cast<uint16_t>(type));
    amuse::io::writeUint32Big(w, soundMacroIdsOff);
    amuse::io::writeUint32Big(w, samplIdsOff);
    amuse::io::writeUint32Big(w, tableIdsOff);
    amuse::io::writeUint32Big(w, keymapIdsOff);
    amuse::io::writeUint32Big(w, layerIdsOff);
    amuse::io::writeUint32Big(w, pageTableOff);
    amuse::io::writeUint32Big(w, drumTableOff);
    amuse::io::writeUint32Big(w, midiSetupsOff);
}
template <> size_t GroupHeader<std::endian::big>::binarySize(size_t s) const { s += 40; return s; }
template <> void GroupHeader<std::endian::big>::readYaml(amuse::io::YAMLDocReader&) {}
template <> void GroupHeader<std::endian::big>::writeYaml(amuse::io::YAMLDocWriter&) const {}

template <> void GroupHeader<std::endian::little>::read(std::istream& r) {
    groupEndOff = amuse::io::readUint32Little(r);
    groupId.read(r);
    type = static_cast<GroupType>(amuse::io::readUint16Little(r));
    soundMacroIdsOff = amuse::io::readUint32Little(r);
    samplIdsOff = amuse::io::readUint32Little(r);
    tableIdsOff = amuse::io::readUint32Little(r);
    keymapIdsOff = amuse::io::readUint32Little(r);
    layerIdsOff = amuse::io::readUint32Little(r);
    pageTableOff = amuse::io::readUint32Little(r);
    drumTableOff = amuse::io::readUint32Little(r);
    midiSetupsOff = amuse::io::readUint32Little(r);
}
template <> void GroupHeader<std::endian::little>::write(std::ostream& w) const {
    amuse::io::writeUint32Little(w, groupEndOff);
    groupId.write(w);
    amuse::io::writeUint16Little(w, static_cast<uint16_t>(type));
    amuse::io::writeUint32Little(w, soundMacroIdsOff);
    amuse::io::writeUint32Little(w, samplIdsOff);
    amuse::io::writeUint32Little(w, tableIdsOff);
    amuse::io::writeUint32Little(w, keymapIdsOff);
    amuse::io::writeUint32Little(w, layerIdsOff);
    amuse::io::writeUint32Little(w, pageTableOff);
    amuse::io::writeUint32Little(w, drumTableOff);
    amuse::io::writeUint32Little(w, midiSetupsOff);
}
template <> size_t GroupHeader<std::endian::little>::binarySize(size_t s) const { s += 40; return s; }
template <> void GroupHeader<std::endian::little>::readYaml(amuse::io::YAMLDocReader&) {}
template <> void GroupHeader<std::endian::little>::writeYaml(amuse::io::YAMLDocWriter&) const {}

template <std::endian DNAE>
std::string_view GroupHeader<DNAE>::DNAType() { return "amuse::GroupHeader"; }
template struct GroupHeader<std::endian::big>;
template struct GroupHeader<std::endian::little>;

// =============================================================================
// SongGroupIndex::PageEntryDNA<E> (AT_DECL_DNA_YAML)
// =============================================================================
template <> void SongGroupIndex::PageEntryDNA<std::endian::big>::read(std::istream& r) {
    objId.read(r);
    priority = amuse::io::readUByte(r); maxVoices = amuse::io::readUByte(r); programNo = amuse::io::readUByte(r);
    r.seekg(1, std::ios_base::cur);
}
template <> void SongGroupIndex::PageEntryDNA<std::endian::big>::write(std::ostream& w) const {
    objId.write(w);
    amuse::io::writeUByte(w, priority); amuse::io::writeUByte(w, maxVoices); amuse::io::writeUByte(w, programNo);
    uint8_t pad = 0; amuse::io::writeUByte(w, pad);
}
template <> size_t SongGroupIndex::PageEntryDNA<std::endian::big>::binarySize(size_t s) const { s += 6; return s; }
template <> void SongGroupIndex::PageEntryDNA<std::endian::big>::readYaml(amuse::io::YAMLDocReader&) {}
template <> void SongGroupIndex::PageEntryDNA<std::endian::big>::writeYaml(amuse::io::YAMLDocWriter&) const {}

template <> void SongGroupIndex::PageEntryDNA<std::endian::little>::read(std::istream& r) {
    objId.read(r);
    priority = amuse::io::readUByte(r); maxVoices = amuse::io::readUByte(r); programNo = amuse::io::readUByte(r);
    r.seekg(1, std::ios_base::cur);
}
template <> void SongGroupIndex::PageEntryDNA<std::endian::little>::write(std::ostream& w) const {
    objId.write(w);
    amuse::io::writeUByte(w, priority); amuse::io::writeUByte(w, maxVoices); amuse::io::writeUByte(w, programNo);
    uint8_t pad = 0; amuse::io::writeUByte(w, pad);
}
template <> size_t SongGroupIndex::PageEntryDNA<std::endian::little>::binarySize(size_t s) const { s += 6; return s; }
template <> void SongGroupIndex::PageEntryDNA<std::endian::little>::readYaml(amuse::io::YAMLDocReader&) {}
template <> void SongGroupIndex::PageEntryDNA<std::endian::little>::writeYaml(amuse::io::YAMLDocWriter&) const {}

template <std::endian DNAE>
std::string_view SongGroupIndex::PageEntryDNA<DNAE>::DNAType() { return "amuse::SongGroupIndex::PageEntryDNA"; }
template struct SongGroupIndex::PageEntryDNA<std::endian::big>;
template struct SongGroupIndex::PageEntryDNA<std::endian::little>;

// =============================================================================
// SongGroupIndex::MusyX1PageEntryDNA<E> (AT_DECL_DNA)
// =============================================================================
template <> void SongGroupIndex::MusyX1PageEntryDNA<std::endian::big>::read(std::istream& r) {
    objId.read(r);
    priority = amuse::io::readUByte(r); maxVoices = amuse::io::readUByte(r); unk = amuse::io::readUByte(r); programNo = amuse::io::readUByte(r);
    r.seekg(2, std::ios_base::cur);
}
template <> void SongGroupIndex::MusyX1PageEntryDNA<std::endian::big>::write(std::ostream& w) const {
    objId.write(w);
    amuse::io::writeUByte(w, priority); amuse::io::writeUByte(w, maxVoices); amuse::io::writeUByte(w, unk); amuse::io::writeUByte(w, programNo);
    uint8_t pad[2] = {}; amuse::io::writeBytes(w, pad, 2);
}
template <> size_t SongGroupIndex::MusyX1PageEntryDNA<std::endian::big>::binarySize(size_t s) const { s += 8; return s; }
template <> void SongGroupIndex::MusyX1PageEntryDNA<std::endian::big>::readYaml(amuse::io::YAMLDocReader&) {}
template <> void SongGroupIndex::MusyX1PageEntryDNA<std::endian::big>::writeYaml(amuse::io::YAMLDocWriter&) const {}

template <> void SongGroupIndex::MusyX1PageEntryDNA<std::endian::little>::read(std::istream& r) {
    objId.read(r);
    priority = amuse::io::readUByte(r); maxVoices = amuse::io::readUByte(r); unk = amuse::io::readUByte(r); programNo = amuse::io::readUByte(r);
    r.seekg(2, std::ios_base::cur);
}
template <> void SongGroupIndex::MusyX1PageEntryDNA<std::endian::little>::write(std::ostream& w) const {
    objId.write(w);
    amuse::io::writeUByte(w, priority); amuse::io::writeUByte(w, maxVoices); amuse::io::writeUByte(w, unk); amuse::io::writeUByte(w, programNo);
    uint8_t pad[2] = {}; amuse::io::writeBytes(w, pad, 2);
}
template <> size_t SongGroupIndex::MusyX1PageEntryDNA<std::endian::little>::binarySize(size_t s) const { s += 8; return s; }
template <> void SongGroupIndex::MusyX1PageEntryDNA<std::endian::little>::readYaml(amuse::io::YAMLDocReader&) {}
template <> void SongGroupIndex::MusyX1PageEntryDNA<std::endian::little>::writeYaml(amuse::io::YAMLDocWriter&) const {}

template <std::endian DNAE>
std::string_view SongGroupIndex::MusyX1PageEntryDNA<DNAE>::DNAType() { return "amuse::SongGroupIndex::MusyX1PageEntryDNA"; }
template struct SongGroupIndex::MusyX1PageEntryDNA<std::endian::big>;
template struct SongGroupIndex::MusyX1PageEntryDNA<std::endian::little>;

// =============================================================================
// SongGroupIndex::PageEntry (BigDNA, AT_DECL_DNA_YAML)
// =============================================================================
void SongGroupIndex::PageEntry::read(std::istream& r) {
    objId.read(r); priority = amuse::io::readUByte(r); maxVoices = amuse::io::readUByte(r);
}
void SongGroupIndex::PageEntry::write(std::ostream& w) const {
    objId.write(w); amuse::io::writeUByte(w, priority); amuse::io::writeUByte(w, maxVoices);
}
size_t SongGroupIndex::PageEntry::binarySize(size_t s) const { s += 4; return s; }
void SongGroupIndex::PageEntry::readYaml(amuse::io::YAMLDocReader&) {}
void SongGroupIndex::PageEntry::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SongGroupIndex::PageEntry::DNAType() { return "amuse::SongGroupIndex::PageEntry"; }

// =============================================================================
// SongGroupIndex::MusyX1MIDISetup (BigDNA, AT_DECL_DNA_YAML)
// =============================================================================
void SongGroupIndex::MusyX1MIDISetup::read(std::istream& r) {
    programNo = amuse::io::readUByte(r); volume = amuse::io::readUByte(r); panning = amuse::io::readUByte(r);
    reverb = amuse::io::readUByte(r); chorus = amuse::io::readUByte(r);
    r.seekg(3, std::ios_base::cur);
}
void SongGroupIndex::MusyX1MIDISetup::write(std::ostream& w) const {
    amuse::io::writeUByte(w, programNo); amuse::io::writeUByte(w, volume); amuse::io::writeUByte(w, panning);
    amuse::io::writeUByte(w, reverb); amuse::io::writeUByte(w, chorus);
    uint8_t pad[3] = {}; amuse::io::writeBytes(w, pad, 3);
}
size_t SongGroupIndex::MusyX1MIDISetup::binarySize(size_t s) const { s += 8; return s; }
void SongGroupIndex::MusyX1MIDISetup::readYaml(amuse::io::YAMLDocReader&) {}
void SongGroupIndex::MusyX1MIDISetup::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SongGroupIndex::MusyX1MIDISetup::DNAType() { return "amuse::SongGroupIndex::MusyX1MIDISetup"; }

// =============================================================================
// SongGroupIndex::MIDISetup (BigDNA, AT_DECL_DNA_YAML)
// =============================================================================
void SongGroupIndex::MIDISetup::read(std::istream& r) {
    programNo = amuse::io::readUByte(r); volume = amuse::io::readUByte(r); panning = amuse::io::readUByte(r);
    reverb = amuse::io::readUByte(r); chorus = amuse::io::readUByte(r);
}
void SongGroupIndex::MIDISetup::write(std::ostream& w) const {
    amuse::io::writeUByte(w, programNo); amuse::io::writeUByte(w, volume); amuse::io::writeUByte(w, panning);
    amuse::io::writeUByte(w, reverb); amuse::io::writeUByte(w, chorus);
}
size_t SongGroupIndex::MIDISetup::binarySize(size_t s) const { s += 5; return s; }
void SongGroupIndex::MIDISetup::readYaml(amuse::io::YAMLDocReader&) {}
void SongGroupIndex::MIDISetup::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SongGroupIndex::MIDISetup::DNAType() { return "amuse::SongGroupIndex::MIDISetup"; }

// =============================================================================
// SFXGroupIndex::SFXEntryDNA<E> (BigDNA template, AT_DECL_DNA)
// =============================================================================
template <> void SFXGroupIndex::SFXEntryDNA<std::endian::big>::read(std::istream& r) {
    sfxId.read(r); objId.read(r);
    priority = amuse::io::readUByte(r); maxVoices = amuse::io::readUByte(r); defVel = amuse::io::readUByte(r);
    panning = amuse::io::readUByte(r); defKey = amuse::io::readUByte(r);
    r.seekg(1, std::ios_base::cur);
}
template <> void SFXGroupIndex::SFXEntryDNA<std::endian::big>::write(std::ostream& w) const {
    sfxId.write(w); objId.write(w);
    amuse::io::writeUByte(w, priority); amuse::io::writeUByte(w, maxVoices); amuse::io::writeUByte(w, defVel);
    amuse::io::writeUByte(w, panning); amuse::io::writeUByte(w, defKey);
    uint8_t pad = 0; amuse::io::writeUByte(w, pad);
}
template <> size_t SFXGroupIndex::SFXEntryDNA<std::endian::big>::binarySize(size_t s) const { s += 10; return s; }
template <> void SFXGroupIndex::SFXEntryDNA<std::endian::big>::readYaml(amuse::io::YAMLDocReader&) {}
template <> void SFXGroupIndex::SFXEntryDNA<std::endian::big>::writeYaml(amuse::io::YAMLDocWriter&) const {}

template <> void SFXGroupIndex::SFXEntryDNA<std::endian::little>::read(std::istream& r) {
    sfxId.read(r); objId.read(r);
    priority = amuse::io::readUByte(r); maxVoices = amuse::io::readUByte(r); defVel = amuse::io::readUByte(r);
    panning = amuse::io::readUByte(r); defKey = amuse::io::readUByte(r);
    r.seekg(1, std::ios_base::cur);
}
template <> void SFXGroupIndex::SFXEntryDNA<std::endian::little>::write(std::ostream& w) const {
    sfxId.write(w); objId.write(w);
    amuse::io::writeUByte(w, priority); amuse::io::writeUByte(w, maxVoices); amuse::io::writeUByte(w, defVel);
    amuse::io::writeUByte(w, panning); amuse::io::writeUByte(w, defKey);
    uint8_t pad = 0; amuse::io::writeUByte(w, pad);
}
template <> size_t SFXGroupIndex::SFXEntryDNA<std::endian::little>::binarySize(size_t s) const { s += 10; return s; }
template <> void SFXGroupIndex::SFXEntryDNA<std::endian::little>::readYaml(amuse::io::YAMLDocReader&) {}
template <> void SFXGroupIndex::SFXEntryDNA<std::endian::little>::writeYaml(amuse::io::YAMLDocWriter&) const {}

template <std::endian DNAE>
std::string_view SFXGroupIndex::SFXEntryDNA<DNAE>::DNAType() { return "amuse::SFXGroupIndex::SFXEntryDNA"; }
template struct SFXGroupIndex::SFXEntryDNA<std::endian::big>;
template struct SFXGroupIndex::SFXEntryDNA<std::endian::little>;

// =============================================================================
// SFXGroupIndex::SFXEntry (BigDNA, AT_DECL_DNA_YAML)
// =============================================================================
void SFXGroupIndex::SFXEntry::read(std::istream& r) {
    objId.read(r); priority = amuse::io::readUByte(r); maxVoices = amuse::io::readUByte(r);
    defVel = amuse::io::readUByte(r); panning = amuse::io::readUByte(r); defKey = amuse::io::readUByte(r);
}
void SFXGroupIndex::SFXEntry::write(std::ostream& w) const {
    objId.write(w); amuse::io::writeUByte(w, priority); amuse::io::writeUByte(w, maxVoices);
    amuse::io::writeUByte(w, defVel); amuse::io::writeUByte(w, panning); amuse::io::writeUByte(w, defKey);
}
size_t SFXGroupIndex::SFXEntry::binarySize(size_t s) const { s += 7; return s; }
void SFXGroupIndex::SFXEntry::readYaml(amuse::io::YAMLDocReader&) {}
void SFXGroupIndex::SFXEntry::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SFXGroupIndex::SFXEntry::DNAType() { return "amuse::SFXGroupIndex::SFXEntry"; }

} // namespace amuse
