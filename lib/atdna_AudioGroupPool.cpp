// Manually-generated serialization code replacing target_atdna output for
// include/amuse/AudioGroupPool.hpp
//
// This file provides direct read/write/binarySize/readYaml/writeYaml method
// implementations for all DNA-tagged types in AudioGroupPool.hpp.

#include "amuse/AudioGroupPool.hpp"

namespace amuse {

// =============================================================================
// Helper macros
// =============================================================================

// VolSelect / PanSelect / ... all have the same 6-byte layout.
#define SELECT_CMD_IMPL(T)                                                               \
    void SoundMacro::T::read(std::istream& r) {                                         \
        midiControl = amuse::io::readUByte(r);                                           \
        scalingPercentage = amuse::io::readInt16Little(r);                               \
        combine = static_cast<Combine>(amuse::io::readByte(r));                          \
        isVar = amuse::io::readBool(r);                                                  \
        fineScaling = amuse::io::readByte(r);                                            \
    }                                                                                    \
    void SoundMacro::T::write(std::ostream& w) const {                                  \
        amuse::io::writeUByte(w, midiControl);                                           \
        amuse::io::writeInt16Little(w, scalingPercentage);                               \
        amuse::io::writeByte(w, static_cast<int8_t>(combine));                           \
        amuse::io::writeBool(w, isVar);                                                  \
        amuse::io::writeByte(w, fineScaling);                                            \
    }                                                                                    \
    size_t SoundMacro::T::binarySize(size_t s) const { s += 6; return s; }              \
    void SoundMacro::T::readYaml(amuse::io::YAMLDocReader&) {}                          \
    void SoundMacro::T::writeYaml(amuse::io::YAMLDocWriter&) const {}                   \
    std::string_view SoundMacro::T::DNAType() { return "amuse::SoundMacro::" #T; }

// =============================================================================
// PoolHeader<E>
// =============================================================================
template <> void PoolHeader<amuse::Endian::Big>::read(std::istream& r) {
    soundMacrosOffset = amuse::io::readUint32Big(r);
    tablesOffset = amuse::io::readUint32Big(r);
    keymapsOffset = amuse::io::readUint32Big(r);
    layersOffset = amuse::io::readUint32Big(r);
}
template <> void PoolHeader<amuse::Endian::Big>::write(std::ostream& w) const {
    amuse::io::writeUint32Big(w, soundMacrosOffset);
    amuse::io::writeUint32Big(w, tablesOffset);
    amuse::io::writeUint32Big(w, keymapsOffset);
    amuse::io::writeUint32Big(w, layersOffset);
}
template <> size_t PoolHeader<amuse::Endian::Big>::binarySize(size_t s) const { s += 16; return s; }
template <> void PoolHeader<amuse::Endian::Big>::readYaml(amuse::io::YAMLDocReader&) {}
template <> void PoolHeader<amuse::Endian::Big>::writeYaml(amuse::io::YAMLDocWriter&) const {}

template <> void PoolHeader<amuse::Endian::Little>::read(std::istream& r) {
    soundMacrosOffset = amuse::io::readUint32Little(r);
    tablesOffset = amuse::io::readUint32Little(r);
    keymapsOffset = amuse::io::readUint32Little(r);
    layersOffset = amuse::io::readUint32Little(r);
}
template <> void PoolHeader<amuse::Endian::Little>::write(std::ostream& w) const {
    amuse::io::writeUint32Little(w, soundMacrosOffset);
    amuse::io::writeUint32Little(w, tablesOffset);
    amuse::io::writeUint32Little(w, keymapsOffset);
    amuse::io::writeUint32Little(w, layersOffset);
}
template <> size_t PoolHeader<amuse::Endian::Little>::binarySize(size_t s) const { s += 16; return s; }
template <> void PoolHeader<amuse::Endian::Little>::readYaml(amuse::io::YAMLDocReader&) {}
template <> void PoolHeader<amuse::Endian::Little>::writeYaml(amuse::io::YAMLDocWriter&) const {}

template <amuse::Endian DNAE>
std::string_view PoolHeader<DNAE>::DNAType() { return "amuse::PoolHeader"; }
template struct PoolHeader<amuse::Endian::Big>;
template struct PoolHeader<amuse::Endian::Little>;

// =============================================================================
// ObjectHeader<E>
// =============================================================================
template <> void ObjectHeader<amuse::Endian::Big>::read(std::istream& r) {
    size = amuse::io::readUint32Big(r);
    objectId.read(r);
    r.seekg(2, std::ios_base::cur);
}
template <> void ObjectHeader<amuse::Endian::Big>::write(std::ostream& w) const {
    amuse::io::writeUint32Big(w, size);
    objectId.write(w);
    uint8_t pad[2] = {};
    amuse::io::writeBytes(w, pad, 2);
}
template <> size_t ObjectHeader<amuse::Endian::Big>::binarySize(size_t s) const { s += 8; return s; }
template <> void ObjectHeader<amuse::Endian::Big>::readYaml(amuse::io::YAMLDocReader&) {}
template <> void ObjectHeader<amuse::Endian::Big>::writeYaml(amuse::io::YAMLDocWriter&) const {}

template <> void ObjectHeader<amuse::Endian::Little>::read(std::istream& r) {
    size = amuse::io::readUint32Little(r);
    objectId.read(r);
    r.seekg(2, std::ios_base::cur);
}
template <> void ObjectHeader<amuse::Endian::Little>::write(std::ostream& w) const {
    amuse::io::writeUint32Little(w, size);
    objectId.write(w);
    uint8_t pad[2] = {};
    amuse::io::writeBytes(w, pad, 2);
}
template <> size_t ObjectHeader<amuse::Endian::Little>::binarySize(size_t s) const { s += 8; return s; }
template <> void ObjectHeader<amuse::Endian::Little>::readYaml(amuse::io::YAMLDocReader&) {}
template <> void ObjectHeader<amuse::Endian::Little>::writeYaml(amuse::io::YAMLDocWriter&) const {}

template <amuse::Endian DNAE>
std::string_view ObjectHeader<DNAE>::DNAType() { return "amuse::ObjectHeader"; }
template struct ObjectHeader<amuse::Endian::Big>;
template struct ObjectHeader<amuse::Endian::Little>;

// =============================================================================
// SoundMacro::ICmd – base class, no data fields
// =============================================================================
void SoundMacro::ICmd::read(std::istream&) {}
void SoundMacro::ICmd::write(std::ostream&) const {}
size_t SoundMacro::ICmd::binarySize(size_t s) const { return s; }
void SoundMacro::ICmd::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::ICmd::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::ICmd::DNAType() { return "amuse::SoundMacro::ICmd"; }

// =============================================================================
// No-field commands
// =============================================================================
void SoundMacro::CmdEnd::read(std::istream&) {}
void SoundMacro::CmdEnd::write(std::ostream&) const {}
size_t SoundMacro::CmdEnd::binarySize(size_t s) const { return s; }
void SoundMacro::CmdEnd::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdEnd::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdEnd::DNAType() { return "amuse::SoundMacro::CmdEnd"; }

void SoundMacro::CmdStop::read(std::istream&) {}
void SoundMacro::CmdStop::write(std::ostream&) const {}
size_t SoundMacro::CmdStop::binarySize(size_t s) const { return s; }
void SoundMacro::CmdStop::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdStop::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdStop::DNAType() { return "amuse::SoundMacro::CmdStop"; }

void SoundMacro::CmdStopSample::read(std::istream&) {}
void SoundMacro::CmdStopSample::write(std::ostream&) const {}
size_t SoundMacro::CmdStopSample::binarySize(size_t s) const { return s; }
void SoundMacro::CmdStopSample::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdStopSample::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdStopSample::DNAType() { return "amuse::SoundMacro::CmdStopSample"; }

void SoundMacro::CmdKeyOff::read(std::istream&) {}
void SoundMacro::CmdKeyOff::write(std::ostream&) const {}
size_t SoundMacro::CmdKeyOff::binarySize(size_t s) const { return s; }
void SoundMacro::CmdKeyOff::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdKeyOff::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdKeyOff::DNAType() { return "amuse::SoundMacro::CmdKeyOff"; }

void SoundMacro::CmdReturn::read(std::istream&) {}
void SoundMacro::CmdReturn::write(std::ostream&) const {}
size_t SoundMacro::CmdReturn::binarySize(size_t s) const { return s; }
void SoundMacro::CmdReturn::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdReturn::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdReturn::DNAType() { return "amuse::SoundMacro::CmdReturn"; }

// =============================================================================
// SplitKey: key(1) macro(2) macroStep(2)
// =============================================================================
void SoundMacro::CmdSplitKey::read(std::istream& r) {
    key = amuse::io::readByte(r); macro.read(r); macroStep.read(r);
}
void SoundMacro::CmdSplitKey::write(std::ostream& w) const {
    amuse::io::writeByte(w, key); macro.write(w); macroStep.write(w);
}
size_t SoundMacro::CmdSplitKey::binarySize(size_t s) const { s += 5; return s; }
void SoundMacro::CmdSplitKey::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdSplitKey::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdSplitKey::DNAType() { return "amuse::SoundMacro::CmdSplitKey"; }

// SplitVel: velocity(1) macro(2) macroStep(2)
void SoundMacro::CmdSplitVel::read(std::istream& r) {
    velocity = amuse::io::readByte(r); macro.read(r); macroStep.read(r);
}
void SoundMacro::CmdSplitVel::write(std::ostream& w) const {
    amuse::io::writeByte(w, velocity); macro.write(w); macroStep.write(w);
}
size_t SoundMacro::CmdSplitVel::binarySize(size_t s) const { s += 5; return s; }
void SoundMacro::CmdSplitVel::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdSplitVel::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdSplitVel::DNAType() { return "amuse::SoundMacro::CmdSplitVel"; }

// WaitTicks: keyOff(1) random(1) sampleEnd(1) absolute(1) msSwitch(1) ticksOrMs(2)
void SoundMacro::CmdWaitTicks::read(std::istream& r) {
    keyOff = amuse::io::readBool(r); random = amuse::io::readBool(r); sampleEnd = amuse::io::readBool(r);
    absolute = amuse::io::readBool(r); msSwitch = amuse::io::readBool(r); ticksOrMs = amuse::io::readUint16Little(r);
}
void SoundMacro::CmdWaitTicks::write(std::ostream& w) const {
    amuse::io::writeBool(w, keyOff); amuse::io::writeBool(w, random); amuse::io::writeBool(w, sampleEnd);
    amuse::io::writeBool(w, absolute); amuse::io::writeBool(w, msSwitch); amuse::io::writeUint16Little(w, ticksOrMs);
}
size_t SoundMacro::CmdWaitTicks::binarySize(size_t s) const { s += 7; return s; }
void SoundMacro::CmdWaitTicks::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdWaitTicks::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdWaitTicks::DNAType() { return "amuse::SoundMacro::CmdWaitTicks"; }

// Loop: keyOff(1) random(1) sampleEnd(1) macroStep(2) times(2)
void SoundMacro::CmdLoop::read(std::istream& r) {
    keyOff = amuse::io::readBool(r); random = amuse::io::readBool(r); sampleEnd = amuse::io::readBool(r);
    macroStep.read(r); times = amuse::io::readUint16Little(r);
}
void SoundMacro::CmdLoop::write(std::ostream& w) const {
    amuse::io::writeBool(w, keyOff); amuse::io::writeBool(w, random); amuse::io::writeBool(w, sampleEnd);
    macroStep.write(w); amuse::io::writeUint16Little(w, times);
}
size_t SoundMacro::CmdLoop::binarySize(size_t s) const { s += 7; return s; }
void SoundMacro::CmdLoop::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdLoop::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdLoop::DNAType() { return "amuse::SoundMacro::CmdLoop"; }

// Goto: seek(1) macro(2) macroStep(2)
void SoundMacro::CmdGoto::read(std::istream& r) {
    r.seekg(1, std::ios_base::cur); macro.read(r); macroStep.read(r);
}
void SoundMacro::CmdGoto::write(std::ostream& w) const {
    uint8_t pad = 0; amuse::io::writeUByte(w, pad); macro.write(w); macroStep.write(w);
}
size_t SoundMacro::CmdGoto::binarySize(size_t s) const { s += 5; return s; }
void SoundMacro::CmdGoto::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdGoto::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdGoto::DNAType() { return "amuse::SoundMacro::CmdGoto"; }

// WaitMs: keyOff(1) random(1) sampleEnd(1) absolute(1) seek(1) ms(2)
void SoundMacro::CmdWaitMs::read(std::istream& r) {
    keyOff = amuse::io::readBool(r); random = amuse::io::readBool(r); sampleEnd = amuse::io::readBool(r); absolute = amuse::io::readBool(r);
    r.seekg(1, std::ios_base::cur); ms = amuse::io::readUint16Little(r);
}
void SoundMacro::CmdWaitMs::write(std::ostream& w) const {
    amuse::io::writeBool(w, keyOff); amuse::io::writeBool(w, random); amuse::io::writeBool(w, sampleEnd); amuse::io::writeBool(w, absolute);
    uint8_t pad = 0; amuse::io::writeUByte(w, pad); amuse::io::writeUint16Little(w, ms);
}
size_t SoundMacro::CmdWaitMs::binarySize(size_t s) const { s += 7; return s; }
void SoundMacro::CmdWaitMs::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdWaitMs::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdWaitMs::DNAType() { return "amuse::SoundMacro::CmdWaitMs"; }

// PlayMacro: addNote(1) macro(2) macroStep(2) priority(1) maxVoices(1)
void SoundMacro::CmdPlayMacro::read(std::istream& r) {
    addNote = amuse::io::readByte(r); macro.read(r); macroStep.read(r);
    priority = amuse::io::readUByte(r); maxVoices = amuse::io::readUByte(r);
}
void SoundMacro::CmdPlayMacro::write(std::ostream& w) const {
    amuse::io::writeByte(w, addNote); macro.write(w); macroStep.write(w);
    amuse::io::writeUByte(w, priority); amuse::io::writeUByte(w, maxVoices);
}
size_t SoundMacro::CmdPlayMacro::binarySize(size_t s) const { s += 7; return s; }
void SoundMacro::CmdPlayMacro::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdPlayMacro::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdPlayMacro::DNAType() { return "amuse::SoundMacro::CmdPlayMacro"; }

// SendKeyOff: variable(1) lastStarted(1)
void SoundMacro::CmdSendKeyOff::read(std::istream& r) {
    variable = amuse::io::readUByte(r); lastStarted = amuse::io::readBool(r);
}
void SoundMacro::CmdSendKeyOff::write(std::ostream& w) const {
    amuse::io::writeUByte(w, variable); amuse::io::writeBool(w, lastStarted);
}
size_t SoundMacro::CmdSendKeyOff::binarySize(size_t s) const { s += 2; return s; }
void SoundMacro::CmdSendKeyOff::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdSendKeyOff::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdSendKeyOff::DNAType() { return "amuse::SoundMacro::CmdSendKeyOff"; }

// SplitMod: modValue(1) macro(2) macroStep(2)
void SoundMacro::CmdSplitMod::read(std::istream& r) {
    modValue = amuse::io::readByte(r); macro.read(r); macroStep.read(r);
}
void SoundMacro::CmdSplitMod::write(std::ostream& w) const {
    amuse::io::writeByte(w, modValue); macro.write(w); macroStep.write(w);
}
size_t SoundMacro::CmdSplitMod::binarySize(size_t s) const { s += 5; return s; }
void SoundMacro::CmdSplitMod::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdSplitMod::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdSplitMod::DNAType() { return "amuse::SoundMacro::CmdSplitMod"; }

// PianoPan: scale(1) centerKey(1) centerPan(1)
void SoundMacro::CmdPianoPan::read(std::istream& r) {
    scale = amuse::io::readByte(r); centerKey = amuse::io::readByte(r); centerPan = amuse::io::readByte(r);
}
void SoundMacro::CmdPianoPan::write(std::ostream& w) const {
    amuse::io::writeByte(w, scale); amuse::io::writeByte(w, centerKey); amuse::io::writeByte(w, centerPan);
}
size_t SoundMacro::CmdPianoPan::binarySize(size_t s) const { s += 3; return s; }
void SoundMacro::CmdPianoPan::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdPianoPan::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdPianoPan::DNAType() { return "amuse::SoundMacro::CmdPianoPan"; }

// SetAdsr: table(2) dlsMode(1)
void SoundMacro::CmdSetAdsr::read(std::istream& r) {
    table.read(r); dlsMode = amuse::io::readBool(r);
}
void SoundMacro::CmdSetAdsr::write(std::ostream& w) const {
    table.write(w); amuse::io::writeBool(w, dlsMode);
}
size_t SoundMacro::CmdSetAdsr::binarySize(size_t s) const { s += 3; return s; }
void SoundMacro::CmdSetAdsr::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdSetAdsr::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdSetAdsr::DNAType() { return "amuse::SoundMacro::CmdSetAdsr"; }

// ScaleVolume: scale(1) add(1) table(2) originalVol(1)
void SoundMacro::CmdScaleVolume::read(std::istream& r) {
    scale = amuse::io::readByte(r); add = amuse::io::readByte(r); table.read(r); originalVol = amuse::io::readBool(r);
}
void SoundMacro::CmdScaleVolume::write(std::ostream& w) const {
    amuse::io::writeByte(w, scale); amuse::io::writeByte(w, add); table.write(w); amuse::io::writeBool(w, originalVol);
}
size_t SoundMacro::CmdScaleVolume::binarySize(size_t s) const { s += 5; return s; }
void SoundMacro::CmdScaleVolume::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdScaleVolume::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdScaleVolume::DNAType() { return "amuse::SoundMacro::CmdScaleVolume"; }

// Panning: panPosition(1) timeMs(2) width(1)
void SoundMacro::CmdPanning::read(std::istream& r) {
    panPosition = amuse::io::readByte(r); timeMs = amuse::io::readUint16Little(r); width = amuse::io::readByte(r);
}
void SoundMacro::CmdPanning::write(std::ostream& w) const {
    amuse::io::writeByte(w, panPosition); amuse::io::writeUint16Little(w, timeMs); amuse::io::writeByte(w, width);
}
size_t SoundMacro::CmdPanning::binarySize(size_t s) const { s += 4; return s; }
void SoundMacro::CmdPanning::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdPanning::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdPanning::DNAType() { return "amuse::SoundMacro::CmdPanning"; }

// Envelope: scale(1) add(1) table(2) msSwitch(1) ticksOrMs(2)
void SoundMacro::CmdEnvelope::read(std::istream& r) {
    scale = amuse::io::readByte(r); add = amuse::io::readByte(r); table.read(r);
    msSwitch = amuse::io::readBool(r); ticksOrMs = amuse::io::readUint16Little(r);
}
void SoundMacro::CmdEnvelope::write(std::ostream& w) const {
    amuse::io::writeByte(w, scale); amuse::io::writeByte(w, add); table.write(w);
    amuse::io::writeBool(w, msSwitch); amuse::io::writeUint16Little(w, ticksOrMs);
}
size_t SoundMacro::CmdEnvelope::binarySize(size_t s) const { s += 7; return s; }
void SoundMacro::CmdEnvelope::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdEnvelope::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdEnvelope::DNAType() { return "amuse::SoundMacro::CmdEnvelope"; }

// StartSample: sample(2) mode(1) offset(4)
void SoundMacro::CmdStartSample::read(std::istream& r) {
    sample.read(r); mode = static_cast<Mode>(amuse::io::readByte(r)); offset = amuse::io::readUint32Little(r);
}
void SoundMacro::CmdStartSample::write(std::ostream& w) const {
    sample.write(w); amuse::io::writeByte(w, static_cast<int8_t>(mode));
    amuse::io::writeUint32Little(w, offset);
}
size_t SoundMacro::CmdStartSample::binarySize(size_t s) const { s += 7; return s; }
void SoundMacro::CmdStartSample::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdStartSample::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdStartSample::DNAType() { return "amuse::SoundMacro::CmdStartSample"; }

// SplitRnd: rnd(1) macro(2) macroStep(2)
void SoundMacro::CmdSplitRnd::read(std::istream& r) {
    rnd = amuse::io::readUByte(r); macro.read(r); macroStep.read(r);
}
void SoundMacro::CmdSplitRnd::write(std::ostream& w) const {
    amuse::io::writeUByte(w, rnd); macro.write(w); macroStep.write(w);
}
size_t SoundMacro::CmdSplitRnd::binarySize(size_t s) const { s += 5; return s; }
void SoundMacro::CmdSplitRnd::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdSplitRnd::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdSplitRnd::DNAType() { return "amuse::SoundMacro::CmdSplitRnd"; }

// FadeIn: scale(1) add(1) table(2) msSwitch(1) ticksOrMs(2)
void SoundMacro::CmdFadeIn::read(std::istream& r) {
    scale = amuse::io::readByte(r); add = amuse::io::readByte(r); table.read(r);
    msSwitch = amuse::io::readBool(r); ticksOrMs = amuse::io::readUint16Little(r);
}
void SoundMacro::CmdFadeIn::write(std::ostream& w) const {
    amuse::io::writeByte(w, scale); amuse::io::writeByte(w, add); table.write(w);
    amuse::io::writeBool(w, msSwitch); amuse::io::writeUint16Little(w, ticksOrMs);
}
size_t SoundMacro::CmdFadeIn::binarySize(size_t s) const { s += 7; return s; }
void SoundMacro::CmdFadeIn::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdFadeIn::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdFadeIn::DNAType() { return "amuse::SoundMacro::CmdFadeIn"; }

// Spanning: spanPosition(1) timeMs(2) width(1)
void SoundMacro::CmdSpanning::read(std::istream& r) {
    spanPosition = amuse::io::readByte(r); timeMs = amuse::io::readUint16Little(r); width = amuse::io::readByte(r);
}
void SoundMacro::CmdSpanning::write(std::ostream& w) const {
    amuse::io::writeByte(w, spanPosition); amuse::io::writeUint16Little(w, timeMs); amuse::io::writeByte(w, width);
}
size_t SoundMacro::CmdSpanning::binarySize(size_t s) const { s += 4; return s; }
void SoundMacro::CmdSpanning::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdSpanning::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdSpanning::DNAType() { return "amuse::SoundMacro::CmdSpanning"; }

// SetAdsrCtrl: attack(1) decay(1) sustain(1) release(1)
void SoundMacro::CmdSetAdsrCtrl::read(std::istream& r) {
    attack = amuse::io::readUByte(r); decay = amuse::io::readUByte(r); sustain = amuse::io::readUByte(r); release = amuse::io::readUByte(r);
}
void SoundMacro::CmdSetAdsrCtrl::write(std::ostream& w) const {
    amuse::io::writeUByte(w, attack); amuse::io::writeUByte(w, decay); amuse::io::writeUByte(w, sustain); amuse::io::writeUByte(w, release);
}
size_t SoundMacro::CmdSetAdsrCtrl::binarySize(size_t s) const { s += 4; return s; }
void SoundMacro::CmdSetAdsrCtrl::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdSetAdsrCtrl::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdSetAdsrCtrl::DNAType() { return "amuse::SoundMacro::CmdSetAdsrCtrl"; }

// RndNote: noteLo(1) detune(1) noteHi(1) fixedFree(1) absRel(1)
void SoundMacro::CmdRndNote::read(std::istream& r) {
    noteLo = amuse::io::readByte(r); detune = amuse::io::readByte(r); noteHi = amuse::io::readByte(r);
    fixedFree = amuse::io::readBool(r); absRel = amuse::io::readBool(r);
}
void SoundMacro::CmdRndNote::write(std::ostream& w) const {
    amuse::io::writeByte(w, noteLo); amuse::io::writeByte(w, detune); amuse::io::writeByte(w, noteHi);
    amuse::io::writeBool(w, fixedFree); amuse::io::writeBool(w, absRel);
}
size_t SoundMacro::CmdRndNote::binarySize(size_t s) const { s += 5; return s; }
void SoundMacro::CmdRndNote::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdRndNote::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdRndNote::DNAType() { return "amuse::SoundMacro::CmdRndNote"; }

// AddNote: add(1) detune(1) originalKey(1) seek(1) msSwitch(1) ticksOrMs(2)
void SoundMacro::CmdAddNote::read(std::istream& r) {
    add = amuse::io::readByte(r); detune = amuse::io::readByte(r); originalKey = amuse::io::readBool(r);
    r.seekg(1, std::ios_base::cur); msSwitch = amuse::io::readBool(r); ticksOrMs = amuse::io::readUint16Little(r);
}
void SoundMacro::CmdAddNote::write(std::ostream& w) const {
    amuse::io::writeByte(w, add); amuse::io::writeByte(w, detune); amuse::io::writeBool(w, originalKey);
    uint8_t pad = 0; amuse::io::writeUByte(w, pad); amuse::io::writeBool(w, msSwitch); amuse::io::writeUint16Little(w, ticksOrMs);
}
size_t SoundMacro::CmdAddNote::binarySize(size_t s) const { s += 7; return s; }
void SoundMacro::CmdAddNote::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdAddNote::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdAddNote::DNAType() { return "amuse::SoundMacro::CmdAddNote"; }

// SetNote: key(1) detune(1) seek(2) msSwitch(1) ticksOrMs(2)
void SoundMacro::CmdSetNote::read(std::istream& r) {
    key = amuse::io::readByte(r); detune = amuse::io::readByte(r);
    r.seekg(2, std::ios_base::cur); msSwitch = amuse::io::readBool(r); ticksOrMs = amuse::io::readUint16Little(r);
}
void SoundMacro::CmdSetNote::write(std::ostream& w) const {
    amuse::io::writeByte(w, key); amuse::io::writeByte(w, detune);
    uint8_t pad[2] = {}; amuse::io::writeBytes(w, pad, 2); amuse::io::writeBool(w, msSwitch); amuse::io::writeUint16Little(w, ticksOrMs);
}
size_t SoundMacro::CmdSetNote::binarySize(size_t s) const { s += 7; return s; }
void SoundMacro::CmdSetNote::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdSetNote::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdSetNote::DNAType() { return "amuse::SoundMacro::CmdSetNote"; }

// LastNote: add(1) detune(1) seek(2) msSwitch(1) ticksOrMs(2)
void SoundMacro::CmdLastNote::read(std::istream& r) {
    add = amuse::io::readByte(r); detune = amuse::io::readByte(r);
    r.seekg(2, std::ios_base::cur); msSwitch = amuse::io::readBool(r); ticksOrMs = amuse::io::readUint16Little(r);
}
void SoundMacro::CmdLastNote::write(std::ostream& w) const {
    amuse::io::writeByte(w, add); amuse::io::writeByte(w, detune);
    uint8_t pad[2] = {}; amuse::io::writeBytes(w, pad, 2); amuse::io::writeBool(w, msSwitch); amuse::io::writeUint16Little(w, ticksOrMs);
}
size_t SoundMacro::CmdLastNote::binarySize(size_t s) const { s += 7; return s; }
void SoundMacro::CmdLastNote::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdLastNote::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdLastNote::DNAType() { return "amuse::SoundMacro::CmdLastNote"; }

// Portamento: portState(1) portType(1) seek(2) msSwitch(1) ticksOrMs(2)
void SoundMacro::CmdPortamento::read(std::istream& r) {
    portState = static_cast<PortState>(amuse::io::readByte(r)); portType = static_cast<PortType>(amuse::io::readByte(r));
    r.seekg(2, std::ios_base::cur); msSwitch = amuse::io::readBool(r); ticksOrMs = amuse::io::readUint16Little(r);
}
void SoundMacro::CmdPortamento::write(std::ostream& w) const {
    amuse::io::writeByte(w, static_cast<int8_t>(portState));
    amuse::io::writeByte(w, static_cast<int8_t>(portType));
    uint8_t pad[2] = {}; amuse::io::writeBytes(w, pad, 2); amuse::io::writeBool(w, msSwitch); amuse::io::writeUint16Little(w, ticksOrMs);
}
size_t SoundMacro::CmdPortamento::binarySize(size_t s) const { s += 7; return s; }
void SoundMacro::CmdPortamento::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdPortamento::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdPortamento::DNAType() { return "amuse::SoundMacro::CmdPortamento"; }

// Vibrato: levelNote(1) levelFine(1) modwheelFlag(1) seek(1) msSwitch(1) ticksOrMs(2)
void SoundMacro::CmdVibrato::read(std::istream& r) {
    levelNote = amuse::io::readByte(r); levelFine = amuse::io::readByte(r); modwheelFlag = amuse::io::readBool(r);
    r.seekg(1, std::ios_base::cur); msSwitch = amuse::io::readBool(r); ticksOrMs = amuse::io::readUint16Little(r);
}
void SoundMacro::CmdVibrato::write(std::ostream& w) const {
    amuse::io::writeByte(w, levelNote); amuse::io::writeByte(w, levelFine); amuse::io::writeBool(w, modwheelFlag);
    uint8_t pad = 0; amuse::io::writeUByte(w, pad); amuse::io::writeBool(w, msSwitch); amuse::io::writeUint16Little(w, ticksOrMs);
}
size_t SoundMacro::CmdVibrato::binarySize(size_t s) const { s += 7; return s; }
void SoundMacro::CmdVibrato::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdVibrato::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdVibrato::DNAType() { return "amuse::SoundMacro::CmdVibrato"; }

// PitchSweep1/2: times(1) add(2) seek(1) msSwitch(1) ticksOrMs(2)
void SoundMacro::CmdPitchSweep1::read(std::istream& r) {
    times = amuse::io::readByte(r); add = amuse::io::readInt16Little(r);
    r.seekg(1, std::ios_base::cur); msSwitch = amuse::io::readBool(r); ticksOrMs = amuse::io::readUint16Little(r);
}
void SoundMacro::CmdPitchSweep1::write(std::ostream& w) const {
    amuse::io::writeByte(w, times); amuse::io::writeInt16Little(w, add);
    uint8_t pad = 0; amuse::io::writeUByte(w, pad); amuse::io::writeBool(w, msSwitch); amuse::io::writeUint16Little(w, ticksOrMs);
}
size_t SoundMacro::CmdPitchSweep1::binarySize(size_t s) const { s += 7; return s; }
void SoundMacro::CmdPitchSweep1::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdPitchSweep1::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdPitchSweep1::DNAType() { return "amuse::SoundMacro::CmdPitchSweep1"; }

void SoundMacro::CmdPitchSweep2::read(std::istream& r) {
    times = amuse::io::readByte(r); add = amuse::io::readInt16Little(r);
    r.seekg(1, std::ios_base::cur); msSwitch = amuse::io::readBool(r); ticksOrMs = amuse::io::readUint16Little(r);
}
void SoundMacro::CmdPitchSweep2::write(std::ostream& w) const {
    amuse::io::writeByte(w, times); amuse::io::writeInt16Little(w, add);
    uint8_t pad = 0; amuse::io::writeUByte(w, pad); amuse::io::writeBool(w, msSwitch); amuse::io::writeUint16Little(w, ticksOrMs);
}
size_t SoundMacro::CmdPitchSweep2::binarySize(size_t s) const { s += 7; return s; }
void SoundMacro::CmdPitchSweep2::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdPitchSweep2::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdPitchSweep2::DNAType() { return "amuse::SoundMacro::CmdPitchSweep2"; }

// SetPitch: hz(3 via LittleUInt24) fine(2)
void SoundMacro::CmdSetPitch::read(std::istream& r) {
    hz.read(r); fine = amuse::io::readUint16Little(r);
}
void SoundMacro::CmdSetPitch::write(std::ostream& w) const {
    hz.write(w); amuse::io::writeUint16Little(w, fine);
}
size_t SoundMacro::CmdSetPitch::binarySize(size_t s) const { s += 5; return s; }
void SoundMacro::CmdSetPitch::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdSetPitch::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdSetPitch::DNAType() { return "amuse::SoundMacro::CmdSetPitch"; }

// SetPitchAdsr: table(2) seek(1) keys(1) cents(1)
void SoundMacro::CmdSetPitchAdsr::read(std::istream& r) {
    table.read(r); r.seekg(1, std::ios_base::cur); keys = amuse::io::readByte(r); cents = amuse::io::readByte(r);
}
void SoundMacro::CmdSetPitchAdsr::write(std::ostream& w) const {
    table.write(w); uint8_t pad = 0; amuse::io::writeUByte(w, pad); amuse::io::writeByte(w, keys); amuse::io::writeByte(w, cents);
}
size_t SoundMacro::CmdSetPitchAdsr::binarySize(size_t s) const { s += 5; return s; }
void SoundMacro::CmdSetPitchAdsr::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdSetPitchAdsr::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdSetPitchAdsr::DNAType() { return "amuse::SoundMacro::CmdSetPitchAdsr"; }

// ScaleVolumeDLS: scale(2) originalVol(1)
void SoundMacro::CmdScaleVolumeDLS::read(std::istream& r) {
    scale = amuse::io::readInt16Little(r); originalVol = amuse::io::readBool(r);
}
void SoundMacro::CmdScaleVolumeDLS::write(std::ostream& w) const {
    amuse::io::writeInt16Little(w, scale); amuse::io::writeBool(w, originalVol);
}
size_t SoundMacro::CmdScaleVolumeDLS::binarySize(size_t s) const { s += 3; return s; }
void SoundMacro::CmdScaleVolumeDLS::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdScaleVolumeDLS::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdScaleVolumeDLS::DNAType() { return "amuse::SoundMacro::CmdScaleVolumeDLS"; }

// Mod2Vibrange: keys(1) cents(1)
void SoundMacro::CmdMod2Vibrange::read(std::istream& r) {
    keys = amuse::io::readByte(r); cents = amuse::io::readByte(r);
}
void SoundMacro::CmdMod2Vibrange::write(std::ostream& w) const {
    amuse::io::writeByte(w, keys); amuse::io::writeByte(w, cents);
}
size_t SoundMacro::CmdMod2Vibrange::binarySize(size_t s) const { s += 2; return s; }
void SoundMacro::CmdMod2Vibrange::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdMod2Vibrange::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdMod2Vibrange::DNAType() { return "amuse::SoundMacro::CmdMod2Vibrange"; }

// SetupTremolo: scale(2) seek(1) modwAddScale(2)
void SoundMacro::CmdSetupTremolo::read(std::istream& r) {
    scale = amuse::io::readInt16Little(r); r.seekg(1, std::ios_base::cur); modwAddScale = amuse::io::readInt16Little(r);
}
void SoundMacro::CmdSetupTremolo::write(std::ostream& w) const {
    amuse::io::writeInt16Little(w, scale); uint8_t pad = 0; amuse::io::writeUByte(w, pad); amuse::io::writeInt16Little(w, modwAddScale);
}
size_t SoundMacro::CmdSetupTremolo::binarySize(size_t s) const { s += 5; return s; }
void SoundMacro::CmdSetupTremolo::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdSetupTremolo::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdSetupTremolo::DNAType() { return "amuse::SoundMacro::CmdSetupTremolo"; }

// GoSub: seek(1) macro(2) macroStep(2)
void SoundMacro::CmdGoSub::read(std::istream& r) {
    r.seekg(1, std::ios_base::cur); macro.read(r); macroStep.read(r);
}
void SoundMacro::CmdGoSub::write(std::ostream& w) const {
    uint8_t pad = 0; amuse::io::writeUByte(w, pad); macro.write(w); macroStep.write(w);
}
size_t SoundMacro::CmdGoSub::binarySize(size_t s) const { s += 5; return s; }
void SoundMacro::CmdGoSub::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdGoSub::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdGoSub::DNAType() { return "amuse::SoundMacro::CmdGoSub"; }

// TrapEvent: event(1) macro(2) macroStep(2)
void SoundMacro::CmdTrapEvent::read(std::istream& r) {
    event = static_cast<EventType>(amuse::io::readByte(r)); macro.read(r); macroStep.read(r);
}
void SoundMacro::CmdTrapEvent::write(std::ostream& w) const {
    amuse::io::writeByte(w, static_cast<int8_t>(event)); macro.write(w); macroStep.write(w);
}
size_t SoundMacro::CmdTrapEvent::binarySize(size_t s) const { s += 5; return s; }
void SoundMacro::CmdTrapEvent::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdTrapEvent::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdTrapEvent::DNAType() { return "amuse::SoundMacro::CmdTrapEvent"; }

// UntrapEvent: event(1)
void SoundMacro::CmdUntrapEvent::read(std::istream& r) {
    event = static_cast<CmdTrapEvent::EventType>(amuse::io::readByte(r));
}
void SoundMacro::CmdUntrapEvent::write(std::ostream& w) const {
    amuse::io::writeByte(w, static_cast<int8_t>(event));
}
size_t SoundMacro::CmdUntrapEvent::binarySize(size_t s) const { s += 1; return s; }
void SoundMacro::CmdUntrapEvent::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdUntrapEvent::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdUntrapEvent::DNAType() { return "amuse::SoundMacro::CmdUntrapEvent"; }

// SendMessage: isVar(1) macro(2) voiceVar(1) valueVar(1)
void SoundMacro::CmdSendMessage::read(std::istream& r) {
    isVar = amuse::io::readBool(r); macro.read(r); voiceVar = amuse::io::readUByte(r); valueVar = amuse::io::readUByte(r);
}
void SoundMacro::CmdSendMessage::write(std::ostream& w) const {
    amuse::io::writeBool(w, isVar); macro.write(w); amuse::io::writeUByte(w, voiceVar); amuse::io::writeUByte(w, valueVar);
}
size_t SoundMacro::CmdSendMessage::binarySize(size_t s) const { s += 5; return s; }
void SoundMacro::CmdSendMessage::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdSendMessage::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdSendMessage::DNAType() { return "amuse::SoundMacro::CmdSendMessage"; }

// GetMessage: variable(1)
void SoundMacro::CmdGetMessage::read(std::istream& r) {
    variable = amuse::io::readUByte(r);
}
void SoundMacro::CmdGetMessage::write(std::ostream& w) const {
    amuse::io::writeUByte(w, variable);
}
size_t SoundMacro::CmdGetMessage::binarySize(size_t s) const { s += 1; return s; }
void SoundMacro::CmdGetMessage::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdGetMessage::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdGetMessage::DNAType() { return "amuse::SoundMacro::CmdGetMessage"; }

// GetVid: variable(1) playMacro(1)
void SoundMacro::CmdGetVid::read(std::istream& r) {
    variable = amuse::io::readUByte(r); playMacro = amuse::io::readBool(r);
}
void SoundMacro::CmdGetVid::write(std::ostream& w) const {
    amuse::io::writeUByte(w, variable); amuse::io::writeBool(w, playMacro);
}
size_t SoundMacro::CmdGetVid::binarySize(size_t s) const { s += 2; return s; }
void SoundMacro::CmdGetVid::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdGetVid::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdGetVid::DNAType() { return "amuse::SoundMacro::CmdGetVid"; }

// AddAgeCount: seek(1) add(2)
void SoundMacro::CmdAddAgeCount::read(std::istream& r) {
    r.seekg(1, std::ios_base::cur); add = amuse::io::readInt16Little(r);
}
void SoundMacro::CmdAddAgeCount::write(std::ostream& w) const {
    uint8_t pad = 0; amuse::io::writeUByte(w, pad); amuse::io::writeInt16Little(w, add);
}
size_t SoundMacro::CmdAddAgeCount::binarySize(size_t s) const { s += 3; return s; }
void SoundMacro::CmdAddAgeCount::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdAddAgeCount::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdAddAgeCount::DNAType() { return "amuse::SoundMacro::CmdAddAgeCount"; }

// SetAgeCount: seek(1) counter(2)
void SoundMacro::CmdSetAgeCount::read(std::istream& r) {
    r.seekg(1, std::ios_base::cur); counter = amuse::io::readUint16Little(r);
}
void SoundMacro::CmdSetAgeCount::write(std::ostream& w) const {
    uint8_t pad = 0; amuse::io::writeUByte(w, pad); amuse::io::writeUint16Little(w, counter);
}
size_t SoundMacro::CmdSetAgeCount::binarySize(size_t s) const { s += 3; return s; }
void SoundMacro::CmdSetAgeCount::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdSetAgeCount::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdSetAgeCount::DNAType() { return "amuse::SoundMacro::CmdSetAgeCount"; }

// SendFlag: flagId(1) value(1)
void SoundMacro::CmdSendFlag::read(std::istream& r) {
    flagId = amuse::io::readUByte(r); value = amuse::io::readUByte(r);
}
void SoundMacro::CmdSendFlag::write(std::ostream& w) const {
    amuse::io::writeUByte(w, flagId); amuse::io::writeUByte(w, value);
}
size_t SoundMacro::CmdSendFlag::binarySize(size_t s) const { s += 2; return s; }
void SoundMacro::CmdSendFlag::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdSendFlag::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdSendFlag::DNAType() { return "amuse::SoundMacro::CmdSendFlag"; }

// PitchWheelR: rangeUp(1) rangeDown(1)
void SoundMacro::CmdPitchWheelR::read(std::istream& r) {
    rangeUp = amuse::io::readByte(r); rangeDown = amuse::io::readByte(r);
}
void SoundMacro::CmdPitchWheelR::write(std::ostream& w) const {
    amuse::io::writeByte(w, rangeUp); amuse::io::writeByte(w, rangeDown);
}
size_t SoundMacro::CmdPitchWheelR::binarySize(size_t s) const { s += 2; return s; }
void SoundMacro::CmdPitchWheelR::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdPitchWheelR::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdPitchWheelR::DNAType() { return "amuse::SoundMacro::CmdPitchWheelR"; }

// SetPriority: prio(1)
void SoundMacro::CmdSetPriority::read(std::istream& r) {
    prio = amuse::io::readUByte(r);
}
void SoundMacro::CmdSetPriority::write(std::ostream& w) const {
    amuse::io::writeUByte(w, prio);
}
size_t SoundMacro::CmdSetPriority::binarySize(size_t s) const { s += 1; return s; }
void SoundMacro::CmdSetPriority::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdSetPriority::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdSetPriority::DNAType() { return "amuse::SoundMacro::CmdSetPriority"; }

// AddPriority: seek(1) prio(2)
void SoundMacro::CmdAddPriority::read(std::istream& r) {
    r.seekg(1, std::ios_base::cur); prio = amuse::io::readInt16Little(r);
}
void SoundMacro::CmdAddPriority::write(std::ostream& w) const {
    uint8_t pad = 0; amuse::io::writeUByte(w, pad); amuse::io::writeInt16Little(w, prio);
}
size_t SoundMacro::CmdAddPriority::binarySize(size_t s) const { s += 3; return s; }
void SoundMacro::CmdAddPriority::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdAddPriority::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdAddPriority::DNAType() { return "amuse::SoundMacro::CmdAddPriority"; }

// AgeCntSpeed: seek(3) time(4)
void SoundMacro::CmdAgeCntSpeed::read(std::istream& r) {
    r.seekg(3, std::ios_base::cur); time = amuse::io::readUint32Little(r);
}
void SoundMacro::CmdAgeCntSpeed::write(std::ostream& w) const {
    uint8_t pad[3] = {}; amuse::io::writeBytes(w, pad, 3); amuse::io::writeUint32Little(w, time);
}
size_t SoundMacro::CmdAgeCntSpeed::binarySize(size_t s) const { s += 7; return s; }
void SoundMacro::CmdAgeCntSpeed::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdAgeCntSpeed::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdAgeCntSpeed::DNAType() { return "amuse::SoundMacro::CmdAgeCntSpeed"; }

// AgeCntVel: seek(1) ageBase(2) ageScale(2)
void SoundMacro::CmdAgeCntVel::read(std::istream& r) {
    r.seekg(1, std::ios_base::cur); ageBase = amuse::io::readUint16Little(r); ageScale = amuse::io::readUint16Little(r);
}
void SoundMacro::CmdAgeCntVel::write(std::ostream& w) const {
    uint8_t pad = 0; amuse::io::writeUByte(w, pad); amuse::io::writeUint16Little(w, ageBase); amuse::io::writeUint16Little(w, ageScale);
}
size_t SoundMacro::CmdAgeCntVel::binarySize(size_t s) const { s += 5; return s; }
void SoundMacro::CmdAgeCntVel::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdAgeCntVel::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdAgeCntVel::DNAType() { return "amuse::SoundMacro::CmdAgeCntVel"; }

// Select commands (VolSelect, PanSelect, ... all 6-byte, same layout)
SELECT_CMD_IMPL(CmdVolSelect)
SELECT_CMD_IMPL(CmdPanSelect)
SELECT_CMD_IMPL(CmdPitchWheelSelect)
SELECT_CMD_IMPL(CmdModWheelSelect)
SELECT_CMD_IMPL(CmdPedalSelect)
SELECT_CMD_IMPL(CmdPortamentoSelect)
SELECT_CMD_IMPL(CmdReverbSelect)
SELECT_CMD_IMPL(CmdSpanSelect)
SELECT_CMD_IMPL(CmdDopplerSelect)
SELECT_CMD_IMPL(CmdTremoloSelect)
SELECT_CMD_IMPL(CmdPreASelect)
SELECT_CMD_IMPL(CmdPreBSelect)
SELECT_CMD_IMPL(CmdPostBSelect)

// AuxAFXSelect/AuxBFXSelect: same as Select + paramIndex(1) = 7 bytes
void SoundMacro::CmdAuxAFXSelect::read(std::istream& r) {
    midiControl = amuse::io::readUByte(r); scalingPercentage = amuse::io::readInt16Little(r);
    combine = static_cast<Combine>(amuse::io::readByte(r)); isVar = amuse::io::readBool(r);
    fineScaling = amuse::io::readByte(r); paramIndex = amuse::io::readUByte(r);
}
void SoundMacro::CmdAuxAFXSelect::write(std::ostream& w) const {
    amuse::io::writeUByte(w, midiControl); amuse::io::writeInt16Little(w, scalingPercentage);
    amuse::io::writeByte(w, static_cast<int8_t>(combine)); amuse::io::writeBool(w, isVar);
    amuse::io::writeByte(w, fineScaling); amuse::io::writeUByte(w, paramIndex);
}
size_t SoundMacro::CmdAuxAFXSelect::binarySize(size_t s) const { s += 7; return s; }
void SoundMacro::CmdAuxAFXSelect::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdAuxAFXSelect::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdAuxAFXSelect::DNAType() { return "amuse::SoundMacro::CmdAuxAFXSelect"; }

void SoundMacro::CmdAuxBFXSelect::read(std::istream& r) {
    midiControl = amuse::io::readUByte(r); scalingPercentage = amuse::io::readInt16Little(r);
    combine = static_cast<Combine>(amuse::io::readByte(r)); isVar = amuse::io::readBool(r);
    fineScaling = amuse::io::readByte(r); paramIndex = amuse::io::readUByte(r);
}
void SoundMacro::CmdAuxBFXSelect::write(std::ostream& w) const {
    amuse::io::writeUByte(w, midiControl); amuse::io::writeInt16Little(w, scalingPercentage);
    amuse::io::writeByte(w, static_cast<int8_t>(combine)); amuse::io::writeBool(w, isVar);
    amuse::io::writeByte(w, fineScaling); amuse::io::writeUByte(w, paramIndex);
}
size_t SoundMacro::CmdAuxBFXSelect::binarySize(size_t s) const { s += 7; return s; }
void SoundMacro::CmdAuxBFXSelect::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdAuxBFXSelect::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdAuxBFXSelect::DNAType() { return "amuse::SoundMacro::CmdAuxBFXSelect"; }

// SetupLFO: lfoNumber(1) periodInMs(2)
void SoundMacro::CmdSetupLFO::read(std::istream& r) {
    lfoNumber = amuse::io::readUByte(r); periodInMs = amuse::io::readInt16Little(r);
}
void SoundMacro::CmdSetupLFO::write(std::ostream& w) const {
    amuse::io::writeUByte(w, lfoNumber); amuse::io::writeInt16Little(w, periodInMs);
}
size_t SoundMacro::CmdSetupLFO::binarySize(size_t s) const { s += 3; return s; }
void SoundMacro::CmdSetupLFO::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdSetupLFO::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdSetupLFO::DNAType() { return "amuse::SoundMacro::CmdSetupLFO"; }

// ModeSelect: dlsVol(1) itd(1)
void SoundMacro::CmdModeSelect::read(std::istream& r) {
    dlsVol = amuse::io::readBool(r); itd = amuse::io::readBool(r);
}
void SoundMacro::CmdModeSelect::write(std::ostream& w) const {
    amuse::io::writeBool(w, dlsVol); amuse::io::writeBool(w, itd);
}
size_t SoundMacro::CmdModeSelect::binarySize(size_t s) const { s += 2; return s; }
void SoundMacro::CmdModeSelect::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdModeSelect::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdModeSelect::DNAType() { return "amuse::SoundMacro::CmdModeSelect"; }

// SetKeygroup: group(1) killNow(1)
void SoundMacro::CmdSetKeygroup::read(std::istream& r) {
    group = amuse::io::readUByte(r); killNow = amuse::io::readBool(r);
}
void SoundMacro::CmdSetKeygroup::write(std::ostream& w) const {
    amuse::io::writeUByte(w, group); amuse::io::writeBool(w, killNow);
}
size_t SoundMacro::CmdSetKeygroup::binarySize(size_t s) const { s += 2; return s; }
void SoundMacro::CmdSetKeygroup::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdSetKeygroup::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdSetKeygroup::DNAType() { return "amuse::SoundMacro::CmdSetKeygroup"; }

// SRCmodeSelect: srcType(1) type0SrcFilter(1)
void SoundMacro::CmdSRCmodeSelect::read(std::istream& r) {
    srcType = amuse::io::readUByte(r); type0SrcFilter = amuse::io::readUByte(r);
}
void SoundMacro::CmdSRCmodeSelect::write(std::ostream& w) const {
    amuse::io::writeUByte(w, srcType); amuse::io::writeUByte(w, type0SrcFilter);
}
size_t SoundMacro::CmdSRCmodeSelect::binarySize(size_t s) const { s += 2; return s; }
void SoundMacro::CmdSRCmodeSelect::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdSRCmodeSelect::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdSRCmodeSelect::DNAType() { return "amuse::SoundMacro::CmdSRCmodeSelect"; }

// WiiUnknown: flag(1)
void SoundMacro::CmdWiiUnknown::read(std::istream& r) {
    flag = amuse::io::readBool(r);
}
void SoundMacro::CmdWiiUnknown::write(std::ostream& w) const {
    amuse::io::writeBool(w, flag);
}
size_t SoundMacro::CmdWiiUnknown::binarySize(size_t s) const { s += 1; return s; }
void SoundMacro::CmdWiiUnknown::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdWiiUnknown::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdWiiUnknown::DNAType() { return "amuse::SoundMacro::CmdWiiUnknown"; }

void SoundMacro::CmdWiiUnknown2::read(std::istream& r) {
    flag = amuse::io::readBool(r);
}
void SoundMacro::CmdWiiUnknown2::write(std::ostream& w) const {
    amuse::io::writeBool(w, flag);
}
size_t SoundMacro::CmdWiiUnknown2::binarySize(size_t s) const { s += 1; return s; }
void SoundMacro::CmdWiiUnknown2::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdWiiUnknown2::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdWiiUnknown2::DNAType() { return "amuse::SoundMacro::CmdWiiUnknown2"; }

// AddVars: varCtrlA(1) a(1) varCtrlB(1) b(1) varCtrlC(1) c(1)
void SoundMacro::CmdAddVars::read(std::istream& r) {
    varCtrlA = amuse::io::readBool(r); a = amuse::io::readUByte(r); varCtrlB = amuse::io::readBool(r);
    b = amuse::io::readUByte(r); varCtrlC = amuse::io::readBool(r); c = amuse::io::readUByte(r);
}
void SoundMacro::CmdAddVars::write(std::ostream& w) const {
    amuse::io::writeBool(w, varCtrlA); amuse::io::writeUByte(w, a); amuse::io::writeBool(w, varCtrlB);
    amuse::io::writeUByte(w, b); amuse::io::writeBool(w, varCtrlC); amuse::io::writeUByte(w, c);
}
size_t SoundMacro::CmdAddVars::binarySize(size_t s) const { s += 6; return s; }
void SoundMacro::CmdAddVars::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdAddVars::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdAddVars::DNAType() { return "amuse::SoundMacro::CmdAddVars"; }

// SubVars/MulVars/DivVars: same as AddVars but signed a/b/c
void SoundMacro::CmdSubVars::read(std::istream& r) {
    varCtrlA = amuse::io::readBool(r); a = amuse::io::readByte(r); varCtrlB = amuse::io::readBool(r);
    b = amuse::io::readByte(r); varCtrlC = amuse::io::readBool(r); c = amuse::io::readByte(r);
}
void SoundMacro::CmdSubVars::write(std::ostream& w) const {
    amuse::io::writeBool(w, varCtrlA); amuse::io::writeByte(w, a); amuse::io::writeBool(w, varCtrlB);
    amuse::io::writeByte(w, b); amuse::io::writeBool(w, varCtrlC); amuse::io::writeByte(w, c);
}
size_t SoundMacro::CmdSubVars::binarySize(size_t s) const { s += 6; return s; }
void SoundMacro::CmdSubVars::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdSubVars::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdSubVars::DNAType() { return "amuse::SoundMacro::CmdSubVars"; }

void SoundMacro::CmdMulVars::read(std::istream& r) {
    varCtrlA = amuse::io::readBool(r); a = amuse::io::readByte(r); varCtrlB = amuse::io::readBool(r);
    b = amuse::io::readByte(r); varCtrlC = amuse::io::readBool(r); c = amuse::io::readByte(r);
}
void SoundMacro::CmdMulVars::write(std::ostream& w) const {
    amuse::io::writeBool(w, varCtrlA); amuse::io::writeByte(w, a); amuse::io::writeBool(w, varCtrlB);
    amuse::io::writeByte(w, b); amuse::io::writeBool(w, varCtrlC); amuse::io::writeByte(w, c);
}
size_t SoundMacro::CmdMulVars::binarySize(size_t s) const { s += 6; return s; }
void SoundMacro::CmdMulVars::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdMulVars::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdMulVars::DNAType() { return "amuse::SoundMacro::CmdMulVars"; }

void SoundMacro::CmdDivVars::read(std::istream& r) {
    varCtrlA = amuse::io::readBool(r); a = amuse::io::readByte(r); varCtrlB = amuse::io::readBool(r);
    b = amuse::io::readByte(r); varCtrlC = amuse::io::readBool(r); c = amuse::io::readByte(r);
}
void SoundMacro::CmdDivVars::write(std::ostream& w) const {
    amuse::io::writeBool(w, varCtrlA); amuse::io::writeByte(w, a); amuse::io::writeBool(w, varCtrlB);
    amuse::io::writeByte(w, b); amuse::io::writeBool(w, varCtrlC); amuse::io::writeByte(w, c);
}
size_t SoundMacro::CmdDivVars::binarySize(size_t s) const { s += 6; return s; }
void SoundMacro::CmdDivVars::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdDivVars::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdDivVars::DNAType() { return "amuse::SoundMacro::CmdDivVars"; }

// AddIVars: varCtrlA(1) a(1) varCtrlB(1) b(1) imm(2)
void SoundMacro::CmdAddIVars::read(std::istream& r) {
    varCtrlA = amuse::io::readBool(r); a = amuse::io::readByte(r); varCtrlB = amuse::io::readBool(r);
    b = amuse::io::readByte(r); imm = amuse::io::readInt16Little(r);
}
void SoundMacro::CmdAddIVars::write(std::ostream& w) const {
    amuse::io::writeBool(w, varCtrlA); amuse::io::writeByte(w, a); amuse::io::writeBool(w, varCtrlB);
    amuse::io::writeByte(w, b); amuse::io::writeInt16Little(w, imm);
}
size_t SoundMacro::CmdAddIVars::binarySize(size_t s) const { s += 6; return s; }
void SoundMacro::CmdAddIVars::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdAddIVars::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdAddIVars::DNAType() { return "amuse::SoundMacro::CmdAddIVars"; }

// SetVar: varCtrlA(1) a(1) seek(1) imm(2)
void SoundMacro::CmdSetVar::read(std::istream& r) {
    varCtrlA = amuse::io::readBool(r); a = amuse::io::readByte(r);
    r.seekg(1, std::ios_base::cur); imm = amuse::io::readInt16Little(r);
}
void SoundMacro::CmdSetVar::write(std::ostream& w) const {
    amuse::io::writeBool(w, varCtrlA); amuse::io::writeByte(w, a);
    uint8_t pad = 0; amuse::io::writeUByte(w, pad); amuse::io::writeInt16Little(w, imm);
}
size_t SoundMacro::CmdSetVar::binarySize(size_t s) const { s += 5; return s; }
void SoundMacro::CmdSetVar::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdSetVar::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdSetVar::DNAType() { return "amuse::SoundMacro::CmdSetVar"; }

// IfEqual: varCtrlA(1) a(1) varCtrlB(1) b(1) notEq(1) macroStep(2)
void SoundMacro::CmdIfEqual::read(std::istream& r) {
    varCtrlA = amuse::io::readBool(r); a = amuse::io::readByte(r); varCtrlB = amuse::io::readBool(r);
    b = amuse::io::readByte(r); notEq = amuse::io::readBool(r); macroStep.read(r);
}
void SoundMacro::CmdIfEqual::write(std::ostream& w) const {
    amuse::io::writeBool(w, varCtrlA); amuse::io::writeByte(w, a); amuse::io::writeBool(w, varCtrlB);
    amuse::io::writeByte(w, b); amuse::io::writeBool(w, notEq); macroStep.write(w);
}
size_t SoundMacro::CmdIfEqual::binarySize(size_t s) const { s += 7; return s; }
void SoundMacro::CmdIfEqual::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdIfEqual::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdIfEqual::DNAType() { return "amuse::SoundMacro::CmdIfEqual"; }

// IfLess: varCtrlA(1) a(1) varCtrlB(1) b(1) notLt(1) macroStep(2)
void SoundMacro::CmdIfLess::read(std::istream& r) {
    varCtrlA = amuse::io::readBool(r); a = amuse::io::readByte(r); varCtrlB = amuse::io::readBool(r);
    b = amuse::io::readByte(r); notLt = amuse::io::readBool(r); macroStep.read(r);
}
void SoundMacro::CmdIfLess::write(std::ostream& w) const {
    amuse::io::writeBool(w, varCtrlA); amuse::io::writeByte(w, a); amuse::io::writeBool(w, varCtrlB);
    amuse::io::writeByte(w, b); amuse::io::writeBool(w, notLt); macroStep.write(w);
}
size_t SoundMacro::CmdIfLess::binarySize(size_t s) const { s += 7; return s; }
void SoundMacro::CmdIfLess::readYaml(amuse::io::YAMLDocReader&) {}
void SoundMacro::CmdIfLess::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view SoundMacro::CmdIfLess::DNAType() { return "amuse::SoundMacro::CmdIfLess"; }

// =============================================================================
// ITable – base class, no fields
// =============================================================================
void ITable::read(std::istream&) {}
void ITable::write(std::ostream&) const {}
size_t ITable::binarySize(size_t s) const { return s; }
void ITable::readYaml(amuse::io::YAMLDocReader&) {}
void ITable::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view ITable::DNAType() { return "amuse::ITable"; }

// ADSR: attack(2) decay(2) sustain(2) release(2) – little-endian
void ADSR::read(std::istream& r) {
    attack = amuse::io::readUint16Little(r); decay = amuse::io::readUint16Little(r);
    sustain = amuse::io::readUint16Little(r); release = amuse::io::readUint16Little(r);
}
void ADSR::write(std::ostream& w) const {
    amuse::io::writeUint16Little(w, attack); amuse::io::writeUint16Little(w, decay);
    amuse::io::writeUint16Little(w, sustain); amuse::io::writeUint16Little(w, release);
}
size_t ADSR::binarySize(size_t s) const { s += 8; return s; }
void ADSR::readYaml(amuse::io::YAMLDocReader&) {}
void ADSR::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view ADSR::DNAType() { return "amuse::ADSR"; }

// ADSRDLS: attack(4) decay(4) sustain(2) release(2) velToAttack(4) keyToDecay(4)
void ADSRDLS::read(std::istream& r) {
    attack = amuse::io::readUint32Little(r); decay = amuse::io::readUint32Little(r);
    sustain = amuse::io::readUint16Little(r); release = amuse::io::readUint16Little(r);
    velToAttack = amuse::io::readUint32Little(r); keyToDecay = amuse::io::readUint32Little(r);
}
void ADSRDLS::write(std::ostream& w) const {
    amuse::io::writeUint32Little(w, attack); amuse::io::writeUint32Little(w, decay);
    amuse::io::writeUint16Little(w, sustain); amuse::io::writeUint16Little(w, release);
    amuse::io::writeUint32Little(w, velToAttack); amuse::io::writeUint32Little(w, keyToDecay);
}
size_t ADSRDLS::binarySize(size_t s) const { s += 20; return s; }
void ADSRDLS::readYaml(amuse::io::YAMLDocReader&) {}
void ADSRDLS::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view ADSRDLS::DNAType() { return "amuse::ADSRDLS"; }

// Curve: read/write/binarySize/readYaml/writeYaml defined in AudioGroupPool.cpp

// =============================================================================
// KeymapDNA<E>
// =============================================================================
template <> void KeymapDNA<amuse::Endian::Big>::read(std::istream& r) {
    macro.read(r); transpose = amuse::io::readByte(r); pan = amuse::io::readByte(r); prioOffset = amuse::io::readByte(r);
    r.seekg(3, std::ios_base::cur);
}
template <> void KeymapDNA<amuse::Endian::Big>::write(std::ostream& w) const {
    macro.write(w); amuse::io::writeByte(w, transpose); amuse::io::writeByte(w, pan); amuse::io::writeByte(w, prioOffset);
    uint8_t pad[3] = {}; amuse::io::writeBytes(w, pad, 3);
}
template <> size_t KeymapDNA<amuse::Endian::Big>::binarySize(size_t s) const { s += 8; return s; }
template <> void KeymapDNA<amuse::Endian::Big>::readYaml(amuse::io::YAMLDocReader&) {}
template <> void KeymapDNA<amuse::Endian::Big>::writeYaml(amuse::io::YAMLDocWriter&) const {}

template <> void KeymapDNA<amuse::Endian::Little>::read(std::istream& r) {
    macro.read(r); transpose = amuse::io::readByte(r); pan = amuse::io::readByte(r); prioOffset = amuse::io::readByte(r);
    r.seekg(3, std::ios_base::cur);
}
template <> void KeymapDNA<amuse::Endian::Little>::write(std::ostream& w) const {
    macro.write(w); amuse::io::writeByte(w, transpose); amuse::io::writeByte(w, pan); amuse::io::writeByte(w, prioOffset);
    uint8_t pad[3] = {}; amuse::io::writeBytes(w, pad, 3);
}
template <> size_t KeymapDNA<amuse::Endian::Little>::binarySize(size_t s) const { s += 8; return s; }
template <> void KeymapDNA<amuse::Endian::Little>::readYaml(amuse::io::YAMLDocReader&) {}
template <> void KeymapDNA<amuse::Endian::Little>::writeYaml(amuse::io::YAMLDocWriter&) const {}

template <amuse::Endian DNAE>
std::string_view KeymapDNA<DNAE>::DNAType() { return "amuse::KeymapDNA"; }
template struct KeymapDNA<amuse::Endian::Big>;
template struct KeymapDNA<amuse::Endian::Little>;

// =============================================================================
// LayerMappingDNA<E>
// =============================================================================
template <> void LayerMappingDNA<amuse::Endian::Big>::read(std::istream& r) {
    macro.read(r); keyLo = amuse::io::readByte(r); keyHi = amuse::io::readByte(r); transpose = amuse::io::readByte(r);
    volume = amuse::io::readByte(r); prioOffset = amuse::io::readByte(r); span = amuse::io::readByte(r); pan = amuse::io::readByte(r);
    r.seekg(3, std::ios_base::cur);
}
template <> void LayerMappingDNA<amuse::Endian::Big>::write(std::ostream& w) const {
    macro.write(w); amuse::io::writeByte(w, keyLo); amuse::io::writeByte(w, keyHi); amuse::io::writeByte(w, transpose);
    amuse::io::writeByte(w, volume); amuse::io::writeByte(w, prioOffset); amuse::io::writeByte(w, span); amuse::io::writeByte(w, pan);
    uint8_t pad[3] = {}; amuse::io::writeBytes(w, pad, 3);
}
template <> size_t LayerMappingDNA<amuse::Endian::Big>::binarySize(size_t s) const { s += 12; return s; }
template <> void LayerMappingDNA<amuse::Endian::Big>::readYaml(amuse::io::YAMLDocReader&) {}
template <> void LayerMappingDNA<amuse::Endian::Big>::writeYaml(amuse::io::YAMLDocWriter&) const {}

template <> void LayerMappingDNA<amuse::Endian::Little>::read(std::istream& r) {
    macro.read(r); keyLo = amuse::io::readByte(r); keyHi = amuse::io::readByte(r); transpose = amuse::io::readByte(r);
    volume = amuse::io::readByte(r); prioOffset = amuse::io::readByte(r); span = amuse::io::readByte(r); pan = amuse::io::readByte(r);
    r.seekg(3, std::ios_base::cur);
}
template <> void LayerMappingDNA<amuse::Endian::Little>::write(std::ostream& w) const {
    macro.write(w); amuse::io::writeByte(w, keyLo); amuse::io::writeByte(w, keyHi); amuse::io::writeByte(w, transpose);
    amuse::io::writeByte(w, volume); amuse::io::writeByte(w, prioOffset); amuse::io::writeByte(w, span); amuse::io::writeByte(w, pan);
    uint8_t pad[3] = {}; amuse::io::writeBytes(w, pad, 3);
}
template <> size_t LayerMappingDNA<amuse::Endian::Little>::binarySize(size_t s) const { s += 12; return s; }
template <> void LayerMappingDNA<amuse::Endian::Little>::readYaml(amuse::io::YAMLDocReader&) {}
template <> void LayerMappingDNA<amuse::Endian::Little>::writeYaml(amuse::io::YAMLDocWriter&) const {}

template <amuse::Endian DNAE>
std::string_view LayerMappingDNA<DNAE>::DNAType() { return "amuse::LayerMappingDNA"; }
template struct LayerMappingDNA<amuse::Endian::Big>;
template struct LayerMappingDNA<amuse::Endian::Little>;

// =============================================================================
// Keymap (BigDNA, AT_DECL_DNA_YAML)
// =============================================================================
void Keymap::read(std::istream& r) {
    macro.read(r); transpose = amuse::io::readByte(r); pan = amuse::io::readByte(r); prioOffset = amuse::io::readByte(r);
}
void Keymap::write(std::ostream& w) const {
    macro.write(w); amuse::io::writeByte(w, transpose); amuse::io::writeByte(w, pan); amuse::io::writeByte(w, prioOffset);
}
size_t Keymap::binarySize(size_t s) const { s += 5; return s; }
void Keymap::readYaml(amuse::io::YAMLDocReader&) {}
void Keymap::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view Keymap::DNAType() { return "amuse::Keymap"; }

// =============================================================================
// LayerMapping (BigDNA, AT_DECL_DNA_YAML)
// =============================================================================
void LayerMapping::read(std::istream& r) {
    macro.read(r); keyLo = amuse::io::readByte(r); keyHi = amuse::io::readByte(r); transpose = amuse::io::readByte(r);
    volume = amuse::io::readByte(r); prioOffset = amuse::io::readByte(r); span = amuse::io::readByte(r); pan = amuse::io::readByte(r);
}
void LayerMapping::write(std::ostream& w) const {
    macro.write(w); amuse::io::writeByte(w, keyLo); amuse::io::writeByte(w, keyHi); amuse::io::writeByte(w, transpose);
    amuse::io::writeByte(w, volume); amuse::io::writeByte(w, prioOffset); amuse::io::writeByte(w, span); amuse::io::writeByte(w, pan);
}
size_t LayerMapping::binarySize(size_t s) const { s += 9; return s; }
void LayerMapping::readYaml(amuse::io::YAMLDocReader&) {}
void LayerMapping::writeYaml(amuse::io::YAMLDocWriter&) const {}
std::string_view LayerMapping::DNAType() { return "amuse::LayerMapping"; }

} // namespace amuse
