// Manually-generated serialization code replacing target_atdna output for
// include/amuse/AudioGroupPool.hpp
//
// This file provides Enumerate<Op> explicit specializations for all DNA-tagged
// types in AudioGroupPool.hpp, plus the virtual read/write/binarySize bodies
// required by the AT_DECL_DNA_YAMLV macro.

#include "amuse/AudioGroupPool.hpp"

namespace amuse {
using namespace athena::io;

// =============================================================================
// Helper macros
// =============================================================================

// Implement the 5 virtual methods for a YAMLV type (AT_DECL_DNA_YAMLV).
#define IMPL_YAMLV(T)                                                                    \
    void T::read(IStreamReader& _r) { Enumerate<DNAOpRead>(_r); }                        \
    void T::write(IStreamWriter& _w) const                                               \
        { const_cast<T*>(this)->Enumerate<DNAOpWrite>(_w); }                             \
    size_t T::binarySize(size_t _s) const                                                \
        { const_cast<T*>(this)->Enumerate<DNAOpBinarySize>(_s); return _s; }             \
    void T::read(YAMLDocReader& _r) { Enumerate<DNAOpReadYaml>(_r); }                    \
    void T::write(YAMLDocWriter& _w) const                                               \
        { const_cast<T*>(this)->Enumerate<DNAOpWriteYaml>(_w); }

// VolSelect / PanSelect / ... all have the same 6-byte layout.
#define SELECT_CMD_IMPL(T)                                                               \
    template <> void SoundMacro::T::Enumerate<DNAOpRead>(IStreamReader& r) {             \
        midiControl = r.readUByte();                                                     \
        scalingPercentage = r.readInt16Little();                                         \
        combine = static_cast<Combine>(r.readByte());                                   \
        isVar = r.readBool();                                                            \
        fineScaling = r.readByte();                                                     \
    }                                                                                    \
    template <> void SoundMacro::T::Enumerate<DNAOpWrite>(IStreamWriter& w) {            \
        w.writeUByte(midiControl);                                                      \
        w.writeInt16Little(scalingPercentage);                                          \
        w.writeByte(static_cast<int8_t>(combine));                \
        w.writeBool(isVar);                                                             \
        w.writeByte(fineScaling);                                                       \
    }                                                                                    \
    template <> void SoundMacro::T::Enumerate<DNAOpBinarySize>(size_t& s) { s += 6; }   \
    template <> void SoundMacro::T::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}          \
    template <> void SoundMacro::T::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}         \
    IMPL_YAMLV(SoundMacro::T)                                                            \
    std::string_view SoundMacro::T::DNAType() { return "amuse::SoundMacro::" #T; }

// =============================================================================
// PoolHeader<E>
// =============================================================================
template <> template <> void PoolHeader<athena::Endian::Big>::Enumerate<DNAOpRead>(IStreamReader& r) {
    soundMacrosOffset = r.readUint32Big();
    tablesOffset = r.readUint32Big();
    keymapsOffset = r.readUint32Big();
    layersOffset = r.readUint32Big();
}
template <> template <> void PoolHeader<athena::Endian::Big>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUint32Big(soundMacrosOffset);
    w.writeUint32Big(tablesOffset);
    w.writeUint32Big(keymapsOffset);
    w.writeUint32Big(layersOffset);
}
template <> template <> void PoolHeader<athena::Endian::Big>::Enumerate<DNAOpBinarySize>(size_t& s) { s += 16; }
template <> template <> void PoolHeader<athena::Endian::Big>::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> template <> void PoolHeader<athena::Endian::Big>::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}

template <> template <> void PoolHeader<athena::Endian::Little>::Enumerate<DNAOpRead>(IStreamReader& r) {
    soundMacrosOffset = r.readUint32Little();
    tablesOffset = r.readUint32Little();
    keymapsOffset = r.readUint32Little();
    layersOffset = r.readUint32Little();
}
template <> template <> void PoolHeader<athena::Endian::Little>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUint32Little(soundMacrosOffset);
    w.writeUint32Little(tablesOffset);
    w.writeUint32Little(keymapsOffset);
    w.writeUint32Little(layersOffset);
}
template <> template <> void PoolHeader<athena::Endian::Little>::Enumerate<DNAOpBinarySize>(size_t& s) { s += 16; }
template <> template <> void PoolHeader<athena::Endian::Little>::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> template <> void PoolHeader<athena::Endian::Little>::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}

template <athena::Endian DNAE>
std::string_view PoolHeader<DNAE>::DNAType() { return "amuse::PoolHeader"; }
template struct PoolHeader<athena::Endian::Big>;
template struct PoolHeader<athena::Endian::Little>;

// =============================================================================
// ObjectHeader<E>
// =============================================================================
template <> template <> void ObjectHeader<athena::Endian::Big>::Enumerate<DNAOpRead>(IStreamReader& r) {
    size = r.readUint32Big();
    objectId.read(r);
    r.seek(2, athena::SeekOrigin::Current);
}
template <> template <> void ObjectHeader<athena::Endian::Big>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUint32Big(size);
    objectId.write(w);
    uint8_t pad[2] = {};
    w.writeBytes(pad, 2);
}
template <> template <> void ObjectHeader<athena::Endian::Big>::Enumerate<DNAOpBinarySize>(size_t& s) { s += 8; }
template <> template <> void ObjectHeader<athena::Endian::Big>::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> template <> void ObjectHeader<athena::Endian::Big>::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}

template <> template <> void ObjectHeader<athena::Endian::Little>::Enumerate<DNAOpRead>(IStreamReader& r) {
    size = r.readUint32Little();
    objectId.read(r);
    r.seek(2, athena::SeekOrigin::Current);
}
template <> template <> void ObjectHeader<athena::Endian::Little>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUint32Little(size);
    objectId.write(w);
    uint8_t pad[2] = {};
    w.writeBytes(pad, 2);
}
template <> template <> void ObjectHeader<athena::Endian::Little>::Enumerate<DNAOpBinarySize>(size_t& s) { s += 8; }
template <> template <> void ObjectHeader<athena::Endian::Little>::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> template <> void ObjectHeader<athena::Endian::Little>::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}

template <athena::Endian DNAE>
std::string_view ObjectHeader<DNAE>::DNAType() { return "amuse::ObjectHeader"; }
template struct ObjectHeader<athena::Endian::Big>;
template struct ObjectHeader<athena::Endian::Little>;

// =============================================================================
// SoundMacro::ICmd – base class, no data fields
// =============================================================================
template <> void SoundMacro::ICmd::Enumerate<DNAOpRead>(IStreamReader&) {}
template <> void SoundMacro::ICmd::Enumerate<DNAOpWrite>(IStreamWriter&) {}
template <> void SoundMacro::ICmd::Enumerate<DNAOpBinarySize>(size_t&) {}
template <> void SoundMacro::ICmd::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::ICmd::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::ICmd)
std::string_view SoundMacro::ICmd::DNAType() { return "amuse::SoundMacro::ICmd"; }

// =============================================================================
// No-field commands
// =============================================================================
template <> void SoundMacro::CmdEnd::Enumerate<DNAOpRead>(IStreamReader&) {}
template <> void SoundMacro::CmdEnd::Enumerate<DNAOpWrite>(IStreamWriter&) {}
template <> void SoundMacro::CmdEnd::Enumerate<DNAOpBinarySize>(size_t&) {}
template <> void SoundMacro::CmdEnd::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdEnd::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdEnd)
std::string_view SoundMacro::CmdEnd::DNAType() { return "amuse::SoundMacro::CmdEnd"; }

template <> void SoundMacro::CmdStop::Enumerate<DNAOpRead>(IStreamReader&) {}
template <> void SoundMacro::CmdStop::Enumerate<DNAOpWrite>(IStreamWriter&) {}
template <> void SoundMacro::CmdStop::Enumerate<DNAOpBinarySize>(size_t&) {}
template <> void SoundMacro::CmdStop::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdStop::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdStop)
std::string_view SoundMacro::CmdStop::DNAType() { return "amuse::SoundMacro::CmdStop"; }

template <> void SoundMacro::CmdStopSample::Enumerate<DNAOpRead>(IStreamReader&) {}
template <> void SoundMacro::CmdStopSample::Enumerate<DNAOpWrite>(IStreamWriter&) {}
template <> void SoundMacro::CmdStopSample::Enumerate<DNAOpBinarySize>(size_t&) {}
template <> void SoundMacro::CmdStopSample::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdStopSample::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdStopSample)
std::string_view SoundMacro::CmdStopSample::DNAType() { return "amuse::SoundMacro::CmdStopSample"; }

template <> void SoundMacro::CmdKeyOff::Enumerate<DNAOpRead>(IStreamReader&) {}
template <> void SoundMacro::CmdKeyOff::Enumerate<DNAOpWrite>(IStreamWriter&) {}
template <> void SoundMacro::CmdKeyOff::Enumerate<DNAOpBinarySize>(size_t&) {}
template <> void SoundMacro::CmdKeyOff::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdKeyOff::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdKeyOff)
std::string_view SoundMacro::CmdKeyOff::DNAType() { return "amuse::SoundMacro::CmdKeyOff"; }

template <> void SoundMacro::CmdReturn::Enumerate<DNAOpRead>(IStreamReader&) {}
template <> void SoundMacro::CmdReturn::Enumerate<DNAOpWrite>(IStreamWriter&) {}
template <> void SoundMacro::CmdReturn::Enumerate<DNAOpBinarySize>(size_t&) {}
template <> void SoundMacro::CmdReturn::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdReturn::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdReturn)
std::string_view SoundMacro::CmdReturn::DNAType() { return "amuse::SoundMacro::CmdReturn"; }

// =============================================================================
// SplitKey: key(1) macro(2) macroStep(2)
// =============================================================================
template <> void SoundMacro::CmdSplitKey::Enumerate<DNAOpRead>(IStreamReader& r) {
    key = r.readByte(); macro.read(r); macroStep.read(r);
}
template <> void SoundMacro::CmdSplitKey::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeByte(key); macro.write(w); macroStep.write(w);
}
template <> void SoundMacro::CmdSplitKey::Enumerate<DNAOpBinarySize>(size_t& s) { s += 5; }
template <> void SoundMacro::CmdSplitKey::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdSplitKey::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdSplitKey)
std::string_view SoundMacro::CmdSplitKey::DNAType() { return "amuse::SoundMacro::CmdSplitKey"; }

// SplitVel: velocity(1) macro(2) macroStep(2)
template <> void SoundMacro::CmdSplitVel::Enumerate<DNAOpRead>(IStreamReader& r) {
    velocity = r.readByte(); macro.read(r); macroStep.read(r);
}
template <> void SoundMacro::CmdSplitVel::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeByte(velocity); macro.write(w); macroStep.write(w);
}
template <> void SoundMacro::CmdSplitVel::Enumerate<DNAOpBinarySize>(size_t& s) { s += 5; }
template <> void SoundMacro::CmdSplitVel::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdSplitVel::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdSplitVel)
std::string_view SoundMacro::CmdSplitVel::DNAType() { return "amuse::SoundMacro::CmdSplitVel"; }

// WaitTicks: keyOff(1) random(1) sampleEnd(1) absolute(1) msSwitch(1) ticksOrMs(2)
template <> void SoundMacro::CmdWaitTicks::Enumerate<DNAOpRead>(IStreamReader& r) {
    keyOff = r.readBool(); random = r.readBool(); sampleEnd = r.readBool();
    absolute = r.readBool(); msSwitch = r.readBool(); ticksOrMs = r.readUint16Little();
}
template <> void SoundMacro::CmdWaitTicks::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeBool(keyOff); w.writeBool(random); w.writeBool(sampleEnd);
    w.writeBool(absolute); w.writeBool(msSwitch); w.writeUint16Little(ticksOrMs);
}
template <> void SoundMacro::CmdWaitTicks::Enumerate<DNAOpBinarySize>(size_t& s) { s += 7; }
template <> void SoundMacro::CmdWaitTicks::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdWaitTicks::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdWaitTicks)
std::string_view SoundMacro::CmdWaitTicks::DNAType() { return "amuse::SoundMacro::CmdWaitTicks"; }

// Loop: keyOff(1) random(1) sampleEnd(1) macroStep(2) times(2)
template <> void SoundMacro::CmdLoop::Enumerate<DNAOpRead>(IStreamReader& r) {
    keyOff = r.readBool(); random = r.readBool(); sampleEnd = r.readBool();
    macroStep.read(r); times = r.readUint16Little();
}
template <> void SoundMacro::CmdLoop::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeBool(keyOff); w.writeBool(random); w.writeBool(sampleEnd);
    macroStep.write(w); w.writeUint16Little(times);
}
template <> void SoundMacro::CmdLoop::Enumerate<DNAOpBinarySize>(size_t& s) { s += 7; }
template <> void SoundMacro::CmdLoop::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdLoop::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdLoop)
std::string_view SoundMacro::CmdLoop::DNAType() { return "amuse::SoundMacro::CmdLoop"; }

// Goto: seek(1) macro(2) macroStep(2)
template <> void SoundMacro::CmdGoto::Enumerate<DNAOpRead>(IStreamReader& r) {
    r.seek(1, athena::SeekOrigin::Current); macro.read(r); macroStep.read(r);
}
template <> void SoundMacro::CmdGoto::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    uint8_t pad = 0; w.writeUByte(pad); macro.write(w); macroStep.write(w);
}
template <> void SoundMacro::CmdGoto::Enumerate<DNAOpBinarySize>(size_t& s) { s += 5; }
template <> void SoundMacro::CmdGoto::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdGoto::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdGoto)
std::string_view SoundMacro::CmdGoto::DNAType() { return "amuse::SoundMacro::CmdGoto"; }

// WaitMs: keyOff(1) random(1) sampleEnd(1) absolute(1) seek(1) ms(2)
template <> void SoundMacro::CmdWaitMs::Enumerate<DNAOpRead>(IStreamReader& r) {
    keyOff = r.readBool(); random = r.readBool(); sampleEnd = r.readBool(); absolute = r.readBool();
    r.seek(1, athena::SeekOrigin::Current); ms = r.readUint16Little();
}
template <> void SoundMacro::CmdWaitMs::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeBool(keyOff); w.writeBool(random); w.writeBool(sampleEnd); w.writeBool(absolute);
    uint8_t pad = 0; w.writeUByte(pad); w.writeUint16Little(ms);
}
template <> void SoundMacro::CmdWaitMs::Enumerate<DNAOpBinarySize>(size_t& s) { s += 7; }
template <> void SoundMacro::CmdWaitMs::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdWaitMs::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdWaitMs)
std::string_view SoundMacro::CmdWaitMs::DNAType() { return "amuse::SoundMacro::CmdWaitMs"; }

// PlayMacro: addNote(1) macro(2) macroStep(2) priority(1) maxVoices(1)
template <> void SoundMacro::CmdPlayMacro::Enumerate<DNAOpRead>(IStreamReader& r) {
    addNote = r.readByte(); macro.read(r); macroStep.read(r);
    priority = r.readUByte(); maxVoices = r.readUByte();
}
template <> void SoundMacro::CmdPlayMacro::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeByte(addNote); macro.write(w); macroStep.write(w);
    w.writeUByte(priority); w.writeUByte(maxVoices);
}
template <> void SoundMacro::CmdPlayMacro::Enumerate<DNAOpBinarySize>(size_t& s) { s += 7; }
template <> void SoundMacro::CmdPlayMacro::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdPlayMacro::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdPlayMacro)
std::string_view SoundMacro::CmdPlayMacro::DNAType() { return "amuse::SoundMacro::CmdPlayMacro"; }

// SendKeyOff: variable(1) lastStarted(1)
template <> void SoundMacro::CmdSendKeyOff::Enumerate<DNAOpRead>(IStreamReader& r) {
    variable = r.readUByte(); lastStarted = r.readBool();
}
template <> void SoundMacro::CmdSendKeyOff::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUByte(variable); w.writeBool(lastStarted);
}
template <> void SoundMacro::CmdSendKeyOff::Enumerate<DNAOpBinarySize>(size_t& s) { s += 2; }
template <> void SoundMacro::CmdSendKeyOff::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdSendKeyOff::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdSendKeyOff)
std::string_view SoundMacro::CmdSendKeyOff::DNAType() { return "amuse::SoundMacro::CmdSendKeyOff"; }

// SplitMod: modValue(1) macro(2) macroStep(2)
template <> void SoundMacro::CmdSplitMod::Enumerate<DNAOpRead>(IStreamReader& r) {
    modValue = r.readByte(); macro.read(r); macroStep.read(r);
}
template <> void SoundMacro::CmdSplitMod::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeByte(modValue); macro.write(w); macroStep.write(w);
}
template <> void SoundMacro::CmdSplitMod::Enumerate<DNAOpBinarySize>(size_t& s) { s += 5; }
template <> void SoundMacro::CmdSplitMod::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdSplitMod::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdSplitMod)
std::string_view SoundMacro::CmdSplitMod::DNAType() { return "amuse::SoundMacro::CmdSplitMod"; }

// PianoPan: scale(1) centerKey(1) centerPan(1)
template <> void SoundMacro::CmdPianoPan::Enumerate<DNAOpRead>(IStreamReader& r) {
    scale = r.readByte(); centerKey = r.readByte(); centerPan = r.readByte();
}
template <> void SoundMacro::CmdPianoPan::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeByte(scale); w.writeByte(centerKey); w.writeByte(centerPan);
}
template <> void SoundMacro::CmdPianoPan::Enumerate<DNAOpBinarySize>(size_t& s) { s += 3; }
template <> void SoundMacro::CmdPianoPan::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdPianoPan::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdPianoPan)
std::string_view SoundMacro::CmdPianoPan::DNAType() { return "amuse::SoundMacro::CmdPianoPan"; }

// SetAdsr: table(2) dlsMode(1)
template <> void SoundMacro::CmdSetAdsr::Enumerate<DNAOpRead>(IStreamReader& r) {
    table.read(r); dlsMode = r.readBool();
}
template <> void SoundMacro::CmdSetAdsr::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    table.write(w); w.writeBool(dlsMode);
}
template <> void SoundMacro::CmdSetAdsr::Enumerate<DNAOpBinarySize>(size_t& s) { s += 3; }
template <> void SoundMacro::CmdSetAdsr::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdSetAdsr::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdSetAdsr)
std::string_view SoundMacro::CmdSetAdsr::DNAType() { return "amuse::SoundMacro::CmdSetAdsr"; }

// ScaleVolume: scale(1) add(1) table(2) originalVol(1)
template <> void SoundMacro::CmdScaleVolume::Enumerate<DNAOpRead>(IStreamReader& r) {
    scale = r.readByte(); add = r.readByte(); table.read(r); originalVol = r.readBool();
}
template <> void SoundMacro::CmdScaleVolume::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeByte(scale); w.writeByte(add); table.write(w); w.writeBool(originalVol);
}
template <> void SoundMacro::CmdScaleVolume::Enumerate<DNAOpBinarySize>(size_t& s) { s += 5; }
template <> void SoundMacro::CmdScaleVolume::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdScaleVolume::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdScaleVolume)
std::string_view SoundMacro::CmdScaleVolume::DNAType() { return "amuse::SoundMacro::CmdScaleVolume"; }

// Panning: panPosition(1) timeMs(2) width(1)
template <> void SoundMacro::CmdPanning::Enumerate<DNAOpRead>(IStreamReader& r) {
    panPosition = r.readByte(); timeMs = r.readUint16Little(); width = r.readByte();
}
template <> void SoundMacro::CmdPanning::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeByte(panPosition); w.writeUint16Little(timeMs); w.writeByte(width);
}
template <> void SoundMacro::CmdPanning::Enumerate<DNAOpBinarySize>(size_t& s) { s += 4; }
template <> void SoundMacro::CmdPanning::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdPanning::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdPanning)
std::string_view SoundMacro::CmdPanning::DNAType() { return "amuse::SoundMacro::CmdPanning"; }

// Envelope: scale(1) add(1) table(2) msSwitch(1) ticksOrMs(2)
template <> void SoundMacro::CmdEnvelope::Enumerate<DNAOpRead>(IStreamReader& r) {
    scale = r.readByte(); add = r.readByte(); table.read(r);
    msSwitch = r.readBool(); ticksOrMs = r.readUint16Little();
}
template <> void SoundMacro::CmdEnvelope::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeByte(scale); w.writeByte(add); table.write(w);
    w.writeBool(msSwitch); w.writeUint16Little(ticksOrMs);
}
template <> void SoundMacro::CmdEnvelope::Enumerate<DNAOpBinarySize>(size_t& s) { s += 7; }
template <> void SoundMacro::CmdEnvelope::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdEnvelope::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdEnvelope)
std::string_view SoundMacro::CmdEnvelope::DNAType() { return "amuse::SoundMacro::CmdEnvelope"; }

// StartSample: sample(2) mode(1) offset(4)
template <> void SoundMacro::CmdStartSample::Enumerate<DNAOpRead>(IStreamReader& r) {
    sample.read(r); mode = static_cast<Mode>(r.readByte()); offset = r.readUint32Little();
}
template <> void SoundMacro::CmdStartSample::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    sample.write(w); w.writeByte(static_cast<int8_t>(mode));
    w.writeUint32Little(offset);
}
template <> void SoundMacro::CmdStartSample::Enumerate<DNAOpBinarySize>(size_t& s) { s += 7; }
template <> void SoundMacro::CmdStartSample::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdStartSample::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdStartSample)
std::string_view SoundMacro::CmdStartSample::DNAType() { return "amuse::SoundMacro::CmdStartSample"; }

// SplitRnd: rnd(1) macro(2) macroStep(2)
template <> void SoundMacro::CmdSplitRnd::Enumerate<DNAOpRead>(IStreamReader& r) {
    rnd = r.readUByte(); macro.read(r); macroStep.read(r);
}
template <> void SoundMacro::CmdSplitRnd::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUByte(rnd); macro.write(w); macroStep.write(w);
}
template <> void SoundMacro::CmdSplitRnd::Enumerate<DNAOpBinarySize>(size_t& s) { s += 5; }
template <> void SoundMacro::CmdSplitRnd::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdSplitRnd::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdSplitRnd)
std::string_view SoundMacro::CmdSplitRnd::DNAType() { return "amuse::SoundMacro::CmdSplitRnd"; }

// FadeIn: scale(1) add(1) table(2) msSwitch(1) ticksOrMs(2)
template <> void SoundMacro::CmdFadeIn::Enumerate<DNAOpRead>(IStreamReader& r) {
    scale = r.readByte(); add = r.readByte(); table.read(r);
    msSwitch = r.readBool(); ticksOrMs = r.readUint16Little();
}
template <> void SoundMacro::CmdFadeIn::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeByte(scale); w.writeByte(add); table.write(w);
    w.writeBool(msSwitch); w.writeUint16Little(ticksOrMs);
}
template <> void SoundMacro::CmdFadeIn::Enumerate<DNAOpBinarySize>(size_t& s) { s += 7; }
template <> void SoundMacro::CmdFadeIn::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdFadeIn::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdFadeIn)
std::string_view SoundMacro::CmdFadeIn::DNAType() { return "amuse::SoundMacro::CmdFadeIn"; }

// Spanning: spanPosition(1) timeMs(2) width(1)
template <> void SoundMacro::CmdSpanning::Enumerate<DNAOpRead>(IStreamReader& r) {
    spanPosition = r.readByte(); timeMs = r.readUint16Little(); width = r.readByte();
}
template <> void SoundMacro::CmdSpanning::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeByte(spanPosition); w.writeUint16Little(timeMs); w.writeByte(width);
}
template <> void SoundMacro::CmdSpanning::Enumerate<DNAOpBinarySize>(size_t& s) { s += 4; }
template <> void SoundMacro::CmdSpanning::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdSpanning::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdSpanning)
std::string_view SoundMacro::CmdSpanning::DNAType() { return "amuse::SoundMacro::CmdSpanning"; }

// SetAdsrCtrl: attack(1) decay(1) sustain(1) release(1)
template <> void SoundMacro::CmdSetAdsrCtrl::Enumerate<DNAOpRead>(IStreamReader& r) {
    attack = r.readUByte(); decay = r.readUByte(); sustain = r.readUByte(); release = r.readUByte();
}
template <> void SoundMacro::CmdSetAdsrCtrl::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUByte(attack); w.writeUByte(decay); w.writeUByte(sustain); w.writeUByte(release);
}
template <> void SoundMacro::CmdSetAdsrCtrl::Enumerate<DNAOpBinarySize>(size_t& s) { s += 4; }
template <> void SoundMacro::CmdSetAdsrCtrl::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdSetAdsrCtrl::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdSetAdsrCtrl)
std::string_view SoundMacro::CmdSetAdsrCtrl::DNAType() { return "amuse::SoundMacro::CmdSetAdsrCtrl"; }

// RndNote: noteLo(1) detune(1) noteHi(1) fixedFree(1) absRel(1)
template <> void SoundMacro::CmdRndNote::Enumerate<DNAOpRead>(IStreamReader& r) {
    noteLo = r.readByte(); detune = r.readByte(); noteHi = r.readByte();
    fixedFree = r.readBool(); absRel = r.readBool();
}
template <> void SoundMacro::CmdRndNote::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeByte(noteLo); w.writeByte(detune); w.writeByte(noteHi);
    w.writeBool(fixedFree); w.writeBool(absRel);
}
template <> void SoundMacro::CmdRndNote::Enumerate<DNAOpBinarySize>(size_t& s) { s += 5; }
template <> void SoundMacro::CmdRndNote::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdRndNote::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdRndNote)
std::string_view SoundMacro::CmdRndNote::DNAType() { return "amuse::SoundMacro::CmdRndNote"; }

// AddNote: add(1) detune(1) originalKey(1) seek(1) msSwitch(1) ticksOrMs(2)
template <> void SoundMacro::CmdAddNote::Enumerate<DNAOpRead>(IStreamReader& r) {
    add = r.readByte(); detune = r.readByte(); originalKey = r.readBool();
    r.seek(1, athena::SeekOrigin::Current); msSwitch = r.readBool(); ticksOrMs = r.readUint16Little();
}
template <> void SoundMacro::CmdAddNote::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeByte(add); w.writeByte(detune); w.writeBool(originalKey);
    uint8_t pad = 0; w.writeUByte(pad); w.writeBool(msSwitch); w.writeUint16Little(ticksOrMs);
}
template <> void SoundMacro::CmdAddNote::Enumerate<DNAOpBinarySize>(size_t& s) { s += 7; }
template <> void SoundMacro::CmdAddNote::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdAddNote::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdAddNote)
std::string_view SoundMacro::CmdAddNote::DNAType() { return "amuse::SoundMacro::CmdAddNote"; }

// SetNote: key(1) detune(1) seek(2) msSwitch(1) ticksOrMs(2)
template <> void SoundMacro::CmdSetNote::Enumerate<DNAOpRead>(IStreamReader& r) {
    key = r.readByte(); detune = r.readByte();
    r.seek(2, athena::SeekOrigin::Current); msSwitch = r.readBool(); ticksOrMs = r.readUint16Little();
}
template <> void SoundMacro::CmdSetNote::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeByte(key); w.writeByte(detune);
    uint8_t pad[2] = {}; w.writeBytes(pad, 2); w.writeBool(msSwitch); w.writeUint16Little(ticksOrMs);
}
template <> void SoundMacro::CmdSetNote::Enumerate<DNAOpBinarySize>(size_t& s) { s += 7; }
template <> void SoundMacro::CmdSetNote::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdSetNote::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdSetNote)
std::string_view SoundMacro::CmdSetNote::DNAType() { return "amuse::SoundMacro::CmdSetNote"; }

// LastNote: add(1) detune(1) seek(2) msSwitch(1) ticksOrMs(2)
template <> void SoundMacro::CmdLastNote::Enumerate<DNAOpRead>(IStreamReader& r) {
    add = r.readByte(); detune = r.readByte();
    r.seek(2, athena::SeekOrigin::Current); msSwitch = r.readBool(); ticksOrMs = r.readUint16Little();
}
template <> void SoundMacro::CmdLastNote::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeByte(add); w.writeByte(detune);
    uint8_t pad[2] = {}; w.writeBytes(pad, 2); w.writeBool(msSwitch); w.writeUint16Little(ticksOrMs);
}
template <> void SoundMacro::CmdLastNote::Enumerate<DNAOpBinarySize>(size_t& s) { s += 7; }
template <> void SoundMacro::CmdLastNote::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdLastNote::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdLastNote)
std::string_view SoundMacro::CmdLastNote::DNAType() { return "amuse::SoundMacro::CmdLastNote"; }

// Portamento: portState(1) portType(1) seek(2) msSwitch(1) ticksOrMs(2)
template <> void SoundMacro::CmdPortamento::Enumerate<DNAOpRead>(IStreamReader& r) {
    portState = static_cast<PortState>(r.readByte()); portType = static_cast<PortType>(r.readByte());
    r.seek(2, athena::SeekOrigin::Current); msSwitch = r.readBool(); ticksOrMs = r.readUint16Little();
}
template <> void SoundMacro::CmdPortamento::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeByte(static_cast<int8_t>(portState));
    w.writeByte(static_cast<int8_t>(portType));
    uint8_t pad[2] = {}; w.writeBytes(pad, 2); w.writeBool(msSwitch); w.writeUint16Little(ticksOrMs);
}
template <> void SoundMacro::CmdPortamento::Enumerate<DNAOpBinarySize>(size_t& s) { s += 7; }
template <> void SoundMacro::CmdPortamento::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdPortamento::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdPortamento)
std::string_view SoundMacro::CmdPortamento::DNAType() { return "amuse::SoundMacro::CmdPortamento"; }

// Vibrato: levelNote(1) levelFine(1) modwheelFlag(1) seek(1) msSwitch(1) ticksOrMs(2)
template <> void SoundMacro::CmdVibrato::Enumerate<DNAOpRead>(IStreamReader& r) {
    levelNote = r.readByte(); levelFine = r.readByte(); modwheelFlag = r.readBool();
    r.seek(1, athena::SeekOrigin::Current); msSwitch = r.readBool(); ticksOrMs = r.readUint16Little();
}
template <> void SoundMacro::CmdVibrato::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeByte(levelNote); w.writeByte(levelFine); w.writeBool(modwheelFlag);
    uint8_t pad = 0; w.writeUByte(pad); w.writeBool(msSwitch); w.writeUint16Little(ticksOrMs);
}
template <> void SoundMacro::CmdVibrato::Enumerate<DNAOpBinarySize>(size_t& s) { s += 7; }
template <> void SoundMacro::CmdVibrato::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdVibrato::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdVibrato)
std::string_view SoundMacro::CmdVibrato::DNAType() { return "amuse::SoundMacro::CmdVibrato"; }

// PitchSweep1/2: times(1) add(2) seek(1) msSwitch(1) ticksOrMs(2)
template <> void SoundMacro::CmdPitchSweep1::Enumerate<DNAOpRead>(IStreamReader& r) {
    times = r.readByte(); add = r.readInt16Little();
    r.seek(1, athena::SeekOrigin::Current); msSwitch = r.readBool(); ticksOrMs = r.readUint16Little();
}
template <> void SoundMacro::CmdPitchSweep1::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeByte(times); w.writeInt16Little(add);
    uint8_t pad = 0; w.writeUByte(pad); w.writeBool(msSwitch); w.writeUint16Little(ticksOrMs);
}
template <> void SoundMacro::CmdPitchSweep1::Enumerate<DNAOpBinarySize>(size_t& s) { s += 7; }
template <> void SoundMacro::CmdPitchSweep1::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdPitchSweep1::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdPitchSweep1)
std::string_view SoundMacro::CmdPitchSweep1::DNAType() { return "amuse::SoundMacro::CmdPitchSweep1"; }

template <> void SoundMacro::CmdPitchSweep2::Enumerate<DNAOpRead>(IStreamReader& r) {
    times = r.readByte(); add = r.readInt16Little();
    r.seek(1, athena::SeekOrigin::Current); msSwitch = r.readBool(); ticksOrMs = r.readUint16Little();
}
template <> void SoundMacro::CmdPitchSweep2::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeByte(times); w.writeInt16Little(add);
    uint8_t pad = 0; w.writeUByte(pad); w.writeBool(msSwitch); w.writeUint16Little(ticksOrMs);
}
template <> void SoundMacro::CmdPitchSweep2::Enumerate<DNAOpBinarySize>(size_t& s) { s += 7; }
template <> void SoundMacro::CmdPitchSweep2::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdPitchSweep2::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdPitchSweep2)
std::string_view SoundMacro::CmdPitchSweep2::DNAType() { return "amuse::SoundMacro::CmdPitchSweep2"; }

// SetPitch: hz(3 via LittleUInt24) fine(2)
template <> void SoundMacro::CmdSetPitch::Enumerate<DNAOpRead>(IStreamReader& r) {
    hz.read(r); fine = r.readUint16Little();
}
template <> void SoundMacro::CmdSetPitch::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    hz.write(w); w.writeUint16Little(fine);
}
template <> void SoundMacro::CmdSetPitch::Enumerate<DNAOpBinarySize>(size_t& s) { s += 5; }
template <> void SoundMacro::CmdSetPitch::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdSetPitch::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdSetPitch)
std::string_view SoundMacro::CmdSetPitch::DNAType() { return "amuse::SoundMacro::CmdSetPitch"; }

// SetPitchAdsr: table(2) seek(1) keys(1) cents(1)
template <> void SoundMacro::CmdSetPitchAdsr::Enumerate<DNAOpRead>(IStreamReader& r) {
    table.read(r); r.seek(1, athena::SeekOrigin::Current); keys = r.readByte(); cents = r.readByte();
}
template <> void SoundMacro::CmdSetPitchAdsr::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    table.write(w); uint8_t pad = 0; w.writeUByte(pad); w.writeByte(keys); w.writeByte(cents);
}
template <> void SoundMacro::CmdSetPitchAdsr::Enumerate<DNAOpBinarySize>(size_t& s) { s += 5; }
template <> void SoundMacro::CmdSetPitchAdsr::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdSetPitchAdsr::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdSetPitchAdsr)
std::string_view SoundMacro::CmdSetPitchAdsr::DNAType() { return "amuse::SoundMacro::CmdSetPitchAdsr"; }

// ScaleVolumeDLS: scale(2) originalVol(1)
template <> void SoundMacro::CmdScaleVolumeDLS::Enumerate<DNAOpRead>(IStreamReader& r) {
    scale = r.readInt16Little(); originalVol = r.readBool();
}
template <> void SoundMacro::CmdScaleVolumeDLS::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeInt16Little(scale); w.writeBool(originalVol);
}
template <> void SoundMacro::CmdScaleVolumeDLS::Enumerate<DNAOpBinarySize>(size_t& s) { s += 3; }
template <> void SoundMacro::CmdScaleVolumeDLS::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdScaleVolumeDLS::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdScaleVolumeDLS)
std::string_view SoundMacro::CmdScaleVolumeDLS::DNAType() { return "amuse::SoundMacro::CmdScaleVolumeDLS"; }

// Mod2Vibrange: keys(1) cents(1)
template <> void SoundMacro::CmdMod2Vibrange::Enumerate<DNAOpRead>(IStreamReader& r) {
    keys = r.readByte(); cents = r.readByte();
}
template <> void SoundMacro::CmdMod2Vibrange::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeByte(keys); w.writeByte(cents);
}
template <> void SoundMacro::CmdMod2Vibrange::Enumerate<DNAOpBinarySize>(size_t& s) { s += 2; }
template <> void SoundMacro::CmdMod2Vibrange::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdMod2Vibrange::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdMod2Vibrange)
std::string_view SoundMacro::CmdMod2Vibrange::DNAType() { return "amuse::SoundMacro::CmdMod2Vibrange"; }

// SetupTremolo: scale(2) seek(1) modwAddScale(2)
template <> void SoundMacro::CmdSetupTremolo::Enumerate<DNAOpRead>(IStreamReader& r) {
    scale = r.readInt16Little(); r.seek(1, athena::SeekOrigin::Current); modwAddScale = r.readInt16Little();
}
template <> void SoundMacro::CmdSetupTremolo::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeInt16Little(scale); uint8_t pad = 0; w.writeUByte(pad); w.writeInt16Little(modwAddScale);
}
template <> void SoundMacro::CmdSetupTremolo::Enumerate<DNAOpBinarySize>(size_t& s) { s += 5; }
template <> void SoundMacro::CmdSetupTremolo::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdSetupTremolo::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdSetupTremolo)
std::string_view SoundMacro::CmdSetupTremolo::DNAType() { return "amuse::SoundMacro::CmdSetupTremolo"; }

// GoSub: seek(1) macro(2) macroStep(2)
template <> void SoundMacro::CmdGoSub::Enumerate<DNAOpRead>(IStreamReader& r) {
    r.seek(1, athena::SeekOrigin::Current); macro.read(r); macroStep.read(r);
}
template <> void SoundMacro::CmdGoSub::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    uint8_t pad = 0; w.writeUByte(pad); macro.write(w); macroStep.write(w);
}
template <> void SoundMacro::CmdGoSub::Enumerate<DNAOpBinarySize>(size_t& s) { s += 5; }
template <> void SoundMacro::CmdGoSub::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdGoSub::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdGoSub)
std::string_view SoundMacro::CmdGoSub::DNAType() { return "amuse::SoundMacro::CmdGoSub"; }

// TrapEvent: event(1) macro(2) macroStep(2)
template <> void SoundMacro::CmdTrapEvent::Enumerate<DNAOpRead>(IStreamReader& r) {
    event = static_cast<EventType>(r.readByte()); macro.read(r); macroStep.read(r);
}
template <> void SoundMacro::CmdTrapEvent::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeByte(static_cast<int8_t>(event)); macro.write(w); macroStep.write(w);
}
template <> void SoundMacro::CmdTrapEvent::Enumerate<DNAOpBinarySize>(size_t& s) { s += 5; }
template <> void SoundMacro::CmdTrapEvent::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdTrapEvent::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdTrapEvent)
std::string_view SoundMacro::CmdTrapEvent::DNAType() { return "amuse::SoundMacro::CmdTrapEvent"; }

// UntrapEvent: event(1)
template <> void SoundMacro::CmdUntrapEvent::Enumerate<DNAOpRead>(IStreamReader& r) {
    event = static_cast<CmdTrapEvent::EventType>(r.readByte());
}
template <> void SoundMacro::CmdUntrapEvent::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeByte(static_cast<int8_t>(event));
}
template <> void SoundMacro::CmdUntrapEvent::Enumerate<DNAOpBinarySize>(size_t& s) { s += 1; }
template <> void SoundMacro::CmdUntrapEvent::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdUntrapEvent::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdUntrapEvent)
std::string_view SoundMacro::CmdUntrapEvent::DNAType() { return "amuse::SoundMacro::CmdUntrapEvent"; }

// SendMessage: isVar(1) macro(2) voiceVar(1) valueVar(1)
template <> void SoundMacro::CmdSendMessage::Enumerate<DNAOpRead>(IStreamReader& r) {
    isVar = r.readBool(); macro.read(r); voiceVar = r.readUByte(); valueVar = r.readUByte();
}
template <> void SoundMacro::CmdSendMessage::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeBool(isVar); macro.write(w); w.writeUByte(voiceVar); w.writeUByte(valueVar);
}
template <> void SoundMacro::CmdSendMessage::Enumerate<DNAOpBinarySize>(size_t& s) { s += 5; }
template <> void SoundMacro::CmdSendMessage::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdSendMessage::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdSendMessage)
std::string_view SoundMacro::CmdSendMessage::DNAType() { return "amuse::SoundMacro::CmdSendMessage"; }

// GetMessage: variable(1)
template <> void SoundMacro::CmdGetMessage::Enumerate<DNAOpRead>(IStreamReader& r) {
    variable = r.readUByte();
}
template <> void SoundMacro::CmdGetMessage::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUByte(variable);
}
template <> void SoundMacro::CmdGetMessage::Enumerate<DNAOpBinarySize>(size_t& s) { s += 1; }
template <> void SoundMacro::CmdGetMessage::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdGetMessage::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdGetMessage)
std::string_view SoundMacro::CmdGetMessage::DNAType() { return "amuse::SoundMacro::CmdGetMessage"; }

// GetVid: variable(1) playMacro(1)
template <> void SoundMacro::CmdGetVid::Enumerate<DNAOpRead>(IStreamReader& r) {
    variable = r.readUByte(); playMacro = r.readBool();
}
template <> void SoundMacro::CmdGetVid::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUByte(variable); w.writeBool(playMacro);
}
template <> void SoundMacro::CmdGetVid::Enumerate<DNAOpBinarySize>(size_t& s) { s += 2; }
template <> void SoundMacro::CmdGetVid::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdGetVid::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdGetVid)
std::string_view SoundMacro::CmdGetVid::DNAType() { return "amuse::SoundMacro::CmdGetVid"; }

// AddAgeCount: seek(1) add(2)
template <> void SoundMacro::CmdAddAgeCount::Enumerate<DNAOpRead>(IStreamReader& r) {
    r.seek(1, athena::SeekOrigin::Current); add = r.readInt16Little();
}
template <> void SoundMacro::CmdAddAgeCount::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    uint8_t pad = 0; w.writeUByte(pad); w.writeInt16Little(add);
}
template <> void SoundMacro::CmdAddAgeCount::Enumerate<DNAOpBinarySize>(size_t& s) { s += 3; }
template <> void SoundMacro::CmdAddAgeCount::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdAddAgeCount::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdAddAgeCount)
std::string_view SoundMacro::CmdAddAgeCount::DNAType() { return "amuse::SoundMacro::CmdAddAgeCount"; }

// SetAgeCount: seek(1) counter(2)
template <> void SoundMacro::CmdSetAgeCount::Enumerate<DNAOpRead>(IStreamReader& r) {
    r.seek(1, athena::SeekOrigin::Current); counter = r.readUint16Little();
}
template <> void SoundMacro::CmdSetAgeCount::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    uint8_t pad = 0; w.writeUByte(pad); w.writeUint16Little(counter);
}
template <> void SoundMacro::CmdSetAgeCount::Enumerate<DNAOpBinarySize>(size_t& s) { s += 3; }
template <> void SoundMacro::CmdSetAgeCount::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdSetAgeCount::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdSetAgeCount)
std::string_view SoundMacro::CmdSetAgeCount::DNAType() { return "amuse::SoundMacro::CmdSetAgeCount"; }

// SendFlag: flagId(1) value(1)
template <> void SoundMacro::CmdSendFlag::Enumerate<DNAOpRead>(IStreamReader& r) {
    flagId = r.readUByte(); value = r.readUByte();
}
template <> void SoundMacro::CmdSendFlag::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUByte(flagId); w.writeUByte(value);
}
template <> void SoundMacro::CmdSendFlag::Enumerate<DNAOpBinarySize>(size_t& s) { s += 2; }
template <> void SoundMacro::CmdSendFlag::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdSendFlag::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdSendFlag)
std::string_view SoundMacro::CmdSendFlag::DNAType() { return "amuse::SoundMacro::CmdSendFlag"; }

// PitchWheelR: rangeUp(1) rangeDown(1)
template <> void SoundMacro::CmdPitchWheelR::Enumerate<DNAOpRead>(IStreamReader& r) {
    rangeUp = r.readByte(); rangeDown = r.readByte();
}
template <> void SoundMacro::CmdPitchWheelR::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeByte(rangeUp); w.writeByte(rangeDown);
}
template <> void SoundMacro::CmdPitchWheelR::Enumerate<DNAOpBinarySize>(size_t& s) { s += 2; }
template <> void SoundMacro::CmdPitchWheelR::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdPitchWheelR::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdPitchWheelR)
std::string_view SoundMacro::CmdPitchWheelR::DNAType() { return "amuse::SoundMacro::CmdPitchWheelR"; }

// SetPriority: prio(1)
template <> void SoundMacro::CmdSetPriority::Enumerate<DNAOpRead>(IStreamReader& r) {
    prio = r.readUByte();
}
template <> void SoundMacro::CmdSetPriority::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUByte(prio);
}
template <> void SoundMacro::CmdSetPriority::Enumerate<DNAOpBinarySize>(size_t& s) { s += 1; }
template <> void SoundMacro::CmdSetPriority::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdSetPriority::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdSetPriority)
std::string_view SoundMacro::CmdSetPriority::DNAType() { return "amuse::SoundMacro::CmdSetPriority"; }

// AddPriority: seek(1) prio(2)
template <> void SoundMacro::CmdAddPriority::Enumerate<DNAOpRead>(IStreamReader& r) {
    r.seek(1, athena::SeekOrigin::Current); prio = r.readInt16Little();
}
template <> void SoundMacro::CmdAddPriority::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    uint8_t pad = 0; w.writeUByte(pad); w.writeInt16Little(prio);
}
template <> void SoundMacro::CmdAddPriority::Enumerate<DNAOpBinarySize>(size_t& s) { s += 3; }
template <> void SoundMacro::CmdAddPriority::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdAddPriority::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdAddPriority)
std::string_view SoundMacro::CmdAddPriority::DNAType() { return "amuse::SoundMacro::CmdAddPriority"; }

// AgeCntSpeed: seek(3) time(4)
template <> void SoundMacro::CmdAgeCntSpeed::Enumerate<DNAOpRead>(IStreamReader& r) {
    r.seek(3, athena::SeekOrigin::Current); time = r.readUint32Little();
}
template <> void SoundMacro::CmdAgeCntSpeed::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    uint8_t pad[3] = {}; w.writeBytes(pad, 3); w.writeUint32Little(time);
}
template <> void SoundMacro::CmdAgeCntSpeed::Enumerate<DNAOpBinarySize>(size_t& s) { s += 7; }
template <> void SoundMacro::CmdAgeCntSpeed::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdAgeCntSpeed::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdAgeCntSpeed)
std::string_view SoundMacro::CmdAgeCntSpeed::DNAType() { return "amuse::SoundMacro::CmdAgeCntSpeed"; }

// AgeCntVel: seek(1) ageBase(2) ageScale(2)
template <> void SoundMacro::CmdAgeCntVel::Enumerate<DNAOpRead>(IStreamReader& r) {
    r.seek(1, athena::SeekOrigin::Current); ageBase = r.readUint16Little(); ageScale = r.readUint16Little();
}
template <> void SoundMacro::CmdAgeCntVel::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    uint8_t pad = 0; w.writeUByte(pad); w.writeUint16Little(ageBase); w.writeUint16Little(ageScale);
}
template <> void SoundMacro::CmdAgeCntVel::Enumerate<DNAOpBinarySize>(size_t& s) { s += 5; }
template <> void SoundMacro::CmdAgeCntVel::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdAgeCntVel::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdAgeCntVel)
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
template <> void SoundMacro::CmdAuxAFXSelect::Enumerate<DNAOpRead>(IStreamReader& r) {
    midiControl = r.readUByte(); scalingPercentage = r.readInt16Little();
    combine = static_cast<Combine>(r.readByte()); isVar = r.readBool();
    fineScaling = r.readByte(); paramIndex = r.readUByte();
}
template <> void SoundMacro::CmdAuxAFXSelect::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUByte(midiControl); w.writeInt16Little(scalingPercentage);
    w.writeByte(static_cast<int8_t>(combine)); w.writeBool(isVar);
    w.writeByte(fineScaling); w.writeUByte(paramIndex);
}
template <> void SoundMacro::CmdAuxAFXSelect::Enumerate<DNAOpBinarySize>(size_t& s) { s += 7; }
template <> void SoundMacro::CmdAuxAFXSelect::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdAuxAFXSelect::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdAuxAFXSelect)
std::string_view SoundMacro::CmdAuxAFXSelect::DNAType() { return "amuse::SoundMacro::CmdAuxAFXSelect"; }

template <> void SoundMacro::CmdAuxBFXSelect::Enumerate<DNAOpRead>(IStreamReader& r) {
    midiControl = r.readUByte(); scalingPercentage = r.readInt16Little();
    combine = static_cast<Combine>(r.readByte()); isVar = r.readBool();
    fineScaling = r.readByte(); paramIndex = r.readUByte();
}
template <> void SoundMacro::CmdAuxBFXSelect::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUByte(midiControl); w.writeInt16Little(scalingPercentage);
    w.writeByte(static_cast<int8_t>(combine)); w.writeBool(isVar);
    w.writeByte(fineScaling); w.writeUByte(paramIndex);
}
template <> void SoundMacro::CmdAuxBFXSelect::Enumerate<DNAOpBinarySize>(size_t& s) { s += 7; }
template <> void SoundMacro::CmdAuxBFXSelect::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdAuxBFXSelect::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdAuxBFXSelect)
std::string_view SoundMacro::CmdAuxBFXSelect::DNAType() { return "amuse::SoundMacro::CmdAuxBFXSelect"; }

// SetupLFO: lfoNumber(1) periodInMs(2)
template <> void SoundMacro::CmdSetupLFO::Enumerate<DNAOpRead>(IStreamReader& r) {
    lfoNumber = r.readUByte(); periodInMs = r.readInt16Little();
}
template <> void SoundMacro::CmdSetupLFO::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUByte(lfoNumber); w.writeInt16Little(periodInMs);
}
template <> void SoundMacro::CmdSetupLFO::Enumerate<DNAOpBinarySize>(size_t& s) { s += 3; }
template <> void SoundMacro::CmdSetupLFO::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdSetupLFO::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdSetupLFO)
std::string_view SoundMacro::CmdSetupLFO::DNAType() { return "amuse::SoundMacro::CmdSetupLFO"; }

// ModeSelect: dlsVol(1) itd(1)
template <> void SoundMacro::CmdModeSelect::Enumerate<DNAOpRead>(IStreamReader& r) {
    dlsVol = r.readBool(); itd = r.readBool();
}
template <> void SoundMacro::CmdModeSelect::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeBool(dlsVol); w.writeBool(itd);
}
template <> void SoundMacro::CmdModeSelect::Enumerate<DNAOpBinarySize>(size_t& s) { s += 2; }
template <> void SoundMacro::CmdModeSelect::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdModeSelect::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdModeSelect)
std::string_view SoundMacro::CmdModeSelect::DNAType() { return "amuse::SoundMacro::CmdModeSelect"; }

// SetKeygroup: group(1) killNow(1)
template <> void SoundMacro::CmdSetKeygroup::Enumerate<DNAOpRead>(IStreamReader& r) {
    group = r.readUByte(); killNow = r.readBool();
}
template <> void SoundMacro::CmdSetKeygroup::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUByte(group); w.writeBool(killNow);
}
template <> void SoundMacro::CmdSetKeygroup::Enumerate<DNAOpBinarySize>(size_t& s) { s += 2; }
template <> void SoundMacro::CmdSetKeygroup::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdSetKeygroup::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdSetKeygroup)
std::string_view SoundMacro::CmdSetKeygroup::DNAType() { return "amuse::SoundMacro::CmdSetKeygroup"; }

// SRCmodeSelect: srcType(1) type0SrcFilter(1)
template <> void SoundMacro::CmdSRCmodeSelect::Enumerate<DNAOpRead>(IStreamReader& r) {
    srcType = r.readUByte(); type0SrcFilter = r.readUByte();
}
template <> void SoundMacro::CmdSRCmodeSelect::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUByte(srcType); w.writeUByte(type0SrcFilter);
}
template <> void SoundMacro::CmdSRCmodeSelect::Enumerate<DNAOpBinarySize>(size_t& s) { s += 2; }
template <> void SoundMacro::CmdSRCmodeSelect::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdSRCmodeSelect::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdSRCmodeSelect)
std::string_view SoundMacro::CmdSRCmodeSelect::DNAType() { return "amuse::SoundMacro::CmdSRCmodeSelect"; }

// WiiUnknown: flag(1)
template <> void SoundMacro::CmdWiiUnknown::Enumerate<DNAOpRead>(IStreamReader& r) {
    flag = r.readBool();
}
template <> void SoundMacro::CmdWiiUnknown::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeBool(flag);
}
template <> void SoundMacro::CmdWiiUnknown::Enumerate<DNAOpBinarySize>(size_t& s) { s += 1; }
template <> void SoundMacro::CmdWiiUnknown::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdWiiUnknown::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdWiiUnknown)
std::string_view SoundMacro::CmdWiiUnknown::DNAType() { return "amuse::SoundMacro::CmdWiiUnknown"; }

template <> void SoundMacro::CmdWiiUnknown2::Enumerate<DNAOpRead>(IStreamReader& r) {
    flag = r.readBool();
}
template <> void SoundMacro::CmdWiiUnknown2::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeBool(flag);
}
template <> void SoundMacro::CmdWiiUnknown2::Enumerate<DNAOpBinarySize>(size_t& s) { s += 1; }
template <> void SoundMacro::CmdWiiUnknown2::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdWiiUnknown2::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdWiiUnknown2)
std::string_view SoundMacro::CmdWiiUnknown2::DNAType() { return "amuse::SoundMacro::CmdWiiUnknown2"; }

// AddVars: varCtrlA(1) a(1) varCtrlB(1) b(1) varCtrlC(1) c(1)
template <> void SoundMacro::CmdAddVars::Enumerate<DNAOpRead>(IStreamReader& r) {
    varCtrlA = r.readBool(); a = r.readUByte(); varCtrlB = r.readBool();
    b = r.readUByte(); varCtrlC = r.readBool(); c = r.readUByte();
}
template <> void SoundMacro::CmdAddVars::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeBool(varCtrlA); w.writeUByte(a); w.writeBool(varCtrlB);
    w.writeUByte(b); w.writeBool(varCtrlC); w.writeUByte(c);
}
template <> void SoundMacro::CmdAddVars::Enumerate<DNAOpBinarySize>(size_t& s) { s += 6; }
template <> void SoundMacro::CmdAddVars::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdAddVars::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdAddVars)
std::string_view SoundMacro::CmdAddVars::DNAType() { return "amuse::SoundMacro::CmdAddVars"; }

// SubVars/MulVars/DivVars: same as AddVars but signed a/b/c
template <> void SoundMacro::CmdSubVars::Enumerate<DNAOpRead>(IStreamReader& r) {
    varCtrlA = r.readBool(); a = r.readByte(); varCtrlB = r.readBool();
    b = r.readByte(); varCtrlC = r.readBool(); c = r.readByte();
}
template <> void SoundMacro::CmdSubVars::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeBool(varCtrlA); w.writeByte(a); w.writeBool(varCtrlB);
    w.writeByte(b); w.writeBool(varCtrlC); w.writeByte(c);
}
template <> void SoundMacro::CmdSubVars::Enumerate<DNAOpBinarySize>(size_t& s) { s += 6; }
template <> void SoundMacro::CmdSubVars::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdSubVars::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdSubVars)
std::string_view SoundMacro::CmdSubVars::DNAType() { return "amuse::SoundMacro::CmdSubVars"; }

template <> void SoundMacro::CmdMulVars::Enumerate<DNAOpRead>(IStreamReader& r) {
    varCtrlA = r.readBool(); a = r.readByte(); varCtrlB = r.readBool();
    b = r.readByte(); varCtrlC = r.readBool(); c = r.readByte();
}
template <> void SoundMacro::CmdMulVars::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeBool(varCtrlA); w.writeByte(a); w.writeBool(varCtrlB);
    w.writeByte(b); w.writeBool(varCtrlC); w.writeByte(c);
}
template <> void SoundMacro::CmdMulVars::Enumerate<DNAOpBinarySize>(size_t& s) { s += 6; }
template <> void SoundMacro::CmdMulVars::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdMulVars::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdMulVars)
std::string_view SoundMacro::CmdMulVars::DNAType() { return "amuse::SoundMacro::CmdMulVars"; }

template <> void SoundMacro::CmdDivVars::Enumerate<DNAOpRead>(IStreamReader& r) {
    varCtrlA = r.readBool(); a = r.readByte(); varCtrlB = r.readBool();
    b = r.readByte(); varCtrlC = r.readBool(); c = r.readByte();
}
template <> void SoundMacro::CmdDivVars::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeBool(varCtrlA); w.writeByte(a); w.writeBool(varCtrlB);
    w.writeByte(b); w.writeBool(varCtrlC); w.writeByte(c);
}
template <> void SoundMacro::CmdDivVars::Enumerate<DNAOpBinarySize>(size_t& s) { s += 6; }
template <> void SoundMacro::CmdDivVars::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdDivVars::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdDivVars)
std::string_view SoundMacro::CmdDivVars::DNAType() { return "amuse::SoundMacro::CmdDivVars"; }

// AddIVars: varCtrlA(1) a(1) varCtrlB(1) b(1) imm(2)
template <> void SoundMacro::CmdAddIVars::Enumerate<DNAOpRead>(IStreamReader& r) {
    varCtrlA = r.readBool(); a = r.readByte(); varCtrlB = r.readBool();
    b = r.readByte(); imm = r.readInt16Little();
}
template <> void SoundMacro::CmdAddIVars::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeBool(varCtrlA); w.writeByte(a); w.writeBool(varCtrlB);
    w.writeByte(b); w.writeInt16Little(imm);
}
template <> void SoundMacro::CmdAddIVars::Enumerate<DNAOpBinarySize>(size_t& s) { s += 6; }
template <> void SoundMacro::CmdAddIVars::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdAddIVars::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdAddIVars)
std::string_view SoundMacro::CmdAddIVars::DNAType() { return "amuse::SoundMacro::CmdAddIVars"; }

// SetVar: varCtrlA(1) a(1) seek(1) imm(2)
template <> void SoundMacro::CmdSetVar::Enumerate<DNAOpRead>(IStreamReader& r) {
    varCtrlA = r.readBool(); a = r.readByte();
    r.seek(1, athena::SeekOrigin::Current); imm = r.readInt16Little();
}
template <> void SoundMacro::CmdSetVar::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeBool(varCtrlA); w.writeByte(a);
    uint8_t pad = 0; w.writeUByte(pad); w.writeInt16Little(imm);
}
template <> void SoundMacro::CmdSetVar::Enumerate<DNAOpBinarySize>(size_t& s) { s += 5; }
template <> void SoundMacro::CmdSetVar::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdSetVar::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdSetVar)
std::string_view SoundMacro::CmdSetVar::DNAType() { return "amuse::SoundMacro::CmdSetVar"; }

// IfEqual: varCtrlA(1) a(1) varCtrlB(1) b(1) notEq(1) macroStep(2)
template <> void SoundMacro::CmdIfEqual::Enumerate<DNAOpRead>(IStreamReader& r) {
    varCtrlA = r.readBool(); a = r.readByte(); varCtrlB = r.readBool();
    b = r.readByte(); notEq = r.readBool(); macroStep.read(r);
}
template <> void SoundMacro::CmdIfEqual::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeBool(varCtrlA); w.writeByte(a); w.writeBool(varCtrlB);
    w.writeByte(b); w.writeBool(notEq); macroStep.write(w);
}
template <> void SoundMacro::CmdIfEqual::Enumerate<DNAOpBinarySize>(size_t& s) { s += 7; }
template <> void SoundMacro::CmdIfEqual::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdIfEqual::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdIfEqual)
std::string_view SoundMacro::CmdIfEqual::DNAType() { return "amuse::SoundMacro::CmdIfEqual"; }

// IfLess: varCtrlA(1) a(1) varCtrlB(1) b(1) notLt(1) macroStep(2)
template <> void SoundMacro::CmdIfLess::Enumerate<DNAOpRead>(IStreamReader& r) {
    varCtrlA = r.readBool(); a = r.readByte(); varCtrlB = r.readBool();
    b = r.readByte(); notLt = r.readBool(); macroStep.read(r);
}
template <> void SoundMacro::CmdIfLess::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeBool(varCtrlA); w.writeByte(a); w.writeBool(varCtrlB);
    w.writeByte(b); w.writeBool(notLt); macroStep.write(w);
}
template <> void SoundMacro::CmdIfLess::Enumerate<DNAOpBinarySize>(size_t& s) { s += 7; }
template <> void SoundMacro::CmdIfLess::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void SoundMacro::CmdIfLess::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(SoundMacro::CmdIfLess)
std::string_view SoundMacro::CmdIfLess::DNAType() { return "amuse::SoundMacro::CmdIfLess"; }

// =============================================================================
// ITable – base class, no fields
// =============================================================================
template <> void ITable::Enumerate<DNAOpRead>(IStreamReader&) {}
template <> void ITable::Enumerate<DNAOpWrite>(IStreamWriter&) {}
template <> void ITable::Enumerate<DNAOpBinarySize>(size_t&) {}
template <> void ITable::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void ITable::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(ITable)
std::string_view ITable::DNAType() { return "amuse::ITable"; }

// ADSR: attack(2) decay(2) sustain(2) release(2) – little-endian
template <> void ADSR::Enumerate<DNAOpRead>(IStreamReader& r) {
    attack = r.readUint16Little(); decay = r.readUint16Little();
    sustain = r.readUint16Little(); release = r.readUint16Little();
}
template <> void ADSR::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUint16Little(attack); w.writeUint16Little(decay);
    w.writeUint16Little(sustain); w.writeUint16Little(release);
}
template <> void ADSR::Enumerate<DNAOpBinarySize>(size_t& s) { s += 8; }
template <> void ADSR::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void ADSR::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(ADSR)
std::string_view ADSR::DNAType() { return "amuse::ADSR"; }

// ADSRDLS: attack(4) decay(4) sustain(2) release(2) velToAttack(4) keyToDecay(4)
template <> void ADSRDLS::Enumerate<DNAOpRead>(IStreamReader& r) {
    attack = r.readUint32Little(); decay = r.readUint32Little();
    sustain = r.readUint16Little(); release = r.readUint16Little();
    velToAttack = r.readUint32Little(); keyToDecay = r.readUint32Little();
}
template <> void ADSRDLS::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    w.writeUint32Little(attack); w.writeUint32Little(decay);
    w.writeUint16Little(sustain); w.writeUint16Little(release);
    w.writeUint32Little(velToAttack); w.writeUint32Little(keyToDecay);
}
template <> void ADSRDLS::Enumerate<DNAOpBinarySize>(size_t& s) { s += 20; }
template <> void ADSRDLS::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void ADSRDLS::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
IMPL_YAMLV(ADSRDLS)
std::string_view ADSRDLS::DNAType() { return "amuse::ADSRDLS"; }

// Curve virtual methods (Enumerate is defined in AudioGroupPool.cpp)
IMPL_YAMLV(Curve)

// =============================================================================
// KeymapDNA<E>
// =============================================================================
template <> template <> void KeymapDNA<athena::Endian::Big>::Enumerate<DNAOpRead>(IStreamReader& r) {
    macro.read(r); transpose = r.readByte(); pan = r.readByte(); prioOffset = r.readByte();
    r.seek(3, athena::SeekOrigin::Current);
}
template <> template <> void KeymapDNA<athena::Endian::Big>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    macro.write(w); w.writeByte(transpose); w.writeByte(pan); w.writeByte(prioOffset);
    uint8_t pad[3] = {}; w.writeBytes(pad, 3);
}
template <> template <> void KeymapDNA<athena::Endian::Big>::Enumerate<DNAOpBinarySize>(size_t& s) { s += 8; }
template <> template <> void KeymapDNA<athena::Endian::Big>::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> template <> void KeymapDNA<athena::Endian::Big>::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}

template <> template <> void KeymapDNA<athena::Endian::Little>::Enumerate<DNAOpRead>(IStreamReader& r) {
    macro.read(r); transpose = r.readByte(); pan = r.readByte(); prioOffset = r.readByte();
    r.seek(3, athena::SeekOrigin::Current);
}
template <> template <> void KeymapDNA<athena::Endian::Little>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    macro.write(w); w.writeByte(transpose); w.writeByte(pan); w.writeByte(prioOffset);
    uint8_t pad[3] = {}; w.writeBytes(pad, 3);
}
template <> template <> void KeymapDNA<athena::Endian::Little>::Enumerate<DNAOpBinarySize>(size_t& s) { s += 8; }
template <> template <> void KeymapDNA<athena::Endian::Little>::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> template <> void KeymapDNA<athena::Endian::Little>::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}

template <athena::Endian DNAE>
std::string_view KeymapDNA<DNAE>::DNAType() { return "amuse::KeymapDNA"; }
template struct KeymapDNA<athena::Endian::Big>;
template struct KeymapDNA<athena::Endian::Little>;

// =============================================================================
// LayerMappingDNA<E>
// =============================================================================
template <> template <> void LayerMappingDNA<athena::Endian::Big>::Enumerate<DNAOpRead>(IStreamReader& r) {
    macro.read(r); keyLo = r.readByte(); keyHi = r.readByte(); transpose = r.readByte();
    volume = r.readByte(); prioOffset = r.readByte(); span = r.readByte(); pan = r.readByte();
    r.seek(3, athena::SeekOrigin::Current);
}
template <> template <> void LayerMappingDNA<athena::Endian::Big>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    macro.write(w); w.writeByte(keyLo); w.writeByte(keyHi); w.writeByte(transpose);
    w.writeByte(volume); w.writeByte(prioOffset); w.writeByte(span); w.writeByte(pan);
    uint8_t pad[3] = {}; w.writeBytes(pad, 3);
}
template <> template <> void LayerMappingDNA<athena::Endian::Big>::Enumerate<DNAOpBinarySize>(size_t& s) { s += 12; }
template <> template <> void LayerMappingDNA<athena::Endian::Big>::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> template <> void LayerMappingDNA<athena::Endian::Big>::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}

template <> template <> void LayerMappingDNA<athena::Endian::Little>::Enumerate<DNAOpRead>(IStreamReader& r) {
    macro.read(r); keyLo = r.readByte(); keyHi = r.readByte(); transpose = r.readByte();
    volume = r.readByte(); prioOffset = r.readByte(); span = r.readByte(); pan = r.readByte();
    r.seek(3, athena::SeekOrigin::Current);
}
template <> template <> void LayerMappingDNA<athena::Endian::Little>::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    macro.write(w); w.writeByte(keyLo); w.writeByte(keyHi); w.writeByte(transpose);
    w.writeByte(volume); w.writeByte(prioOffset); w.writeByte(span); w.writeByte(pan);
    uint8_t pad[3] = {}; w.writeBytes(pad, 3);
}
template <> template <> void LayerMappingDNA<athena::Endian::Little>::Enumerate<DNAOpBinarySize>(size_t& s) { s += 12; }
template <> template <> void LayerMappingDNA<athena::Endian::Little>::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> template <> void LayerMappingDNA<athena::Endian::Little>::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}

template <athena::Endian DNAE>
std::string_view LayerMappingDNA<DNAE>::DNAType() { return "amuse::LayerMappingDNA"; }
template struct LayerMappingDNA<athena::Endian::Big>;
template struct LayerMappingDNA<athena::Endian::Little>;

// =============================================================================
// Keymap (BigDNA, AT_DECL_DNA_YAML)
// =============================================================================
template <> void Keymap::Enumerate<DNAOpRead>(IStreamReader& r) {
    macro.read(r); transpose = r.readByte(); pan = r.readByte(); prioOffset = r.readByte();
}
template <> void Keymap::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    macro.write(w); w.writeByte(transpose); w.writeByte(pan); w.writeByte(prioOffset);
}
template <> void Keymap::Enumerate<DNAOpBinarySize>(size_t& s) { s += 5; }
template <> void Keymap::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void Keymap::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
std::string_view Keymap::DNAType() { return "amuse::Keymap"; }

// =============================================================================
// LayerMapping (BigDNA, AT_DECL_DNA_YAML)
// =============================================================================
template <> void LayerMapping::Enumerate<DNAOpRead>(IStreamReader& r) {
    macro.read(r); keyLo = r.readByte(); keyHi = r.readByte(); transpose = r.readByte();
    volume = r.readByte(); prioOffset = r.readByte(); span = r.readByte(); pan = r.readByte();
}
template <> void LayerMapping::Enumerate<DNAOpWrite>(IStreamWriter& w) {
    macro.write(w); w.writeByte(keyLo); w.writeByte(keyHi); w.writeByte(transpose);
    w.writeByte(volume); w.writeByte(prioOffset); w.writeByte(span); w.writeByte(pan);
}
template <> void LayerMapping::Enumerate<DNAOpBinarySize>(size_t& s) { s += 9; }
template <> void LayerMapping::Enumerate<DNAOpReadYaml>(YAMLDocReader&) {}
template <> void LayerMapping::Enumerate<DNAOpWriteYaml>(YAMLDocWriter&) {}
std::string_view LayerMapping::DNAType() { return "amuse::LayerMapping"; }

} // namespace amuse
