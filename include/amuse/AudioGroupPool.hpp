#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "amuse/Common.hpp"
#include "amuse/Entity.hpp"

#include <athena/DNA.hpp>
#include <athena/MemoryReader.hpp>

#include <fluidsynth.h>

namespace amuse {
class AudioGroupData;
struct SoundMacroState;
class Voice;
struct MacroExecContext;

/** Header at the top of the pool file */
template <std::endian DNAEn>
struct AT_SPECIALIZE_PARMS(std::endian::big, std::endian::little) PoolHeader : BigDNA {
  AT_DECL_DNA
  Value<uint32_t, DNAEn> soundMacrosOffset;
  Value<uint32_t, DNAEn> tablesOffset;
  Value<uint32_t, DNAEn> keymapsOffset;
  Value<uint32_t, DNAEn> layersOffset;
};

/** Header present at the top of each pool object */
template <std::endian DNAEn>
struct AT_SPECIALIZE_PARMS(std::endian::big, std::endian::little) ObjectHeader : BigDNA {
  AT_DECL_DNA
  Value<uint32_t, DNAEn> size;
  ObjectIdDNA<DNAEn> objectId;
  Seek<2, athena::SeekOrigin::Current> pad;
};

struct SoundMacro {
  /** SoundMacro command operations */
  enum class CmdOp : uint8_t {
    End,
    Stop,
    SplitKey,
    SplitVel,
    WaitTicks,
    Loop,
    Goto,
    WaitMs,
    PlayMacro,
    SendKeyOff,
    SplitMod,
    PianoPan,
    SetAdsr,
    ScaleVolume,
    Panning,
    Envelope,
    StartSample,
    StopSample,
    KeyOff,
    SplitRnd,
    FadeIn,
    Spanning,
    SetAdsrCtrl,
    RndNote,
    AddNote,
    SetNote,
    LastNote,
    Portamento,
    Vibrato,
    PitchSweep1,
    PitchSweep2,
    SetPitch,
    SetPitchAdsr,
    ScaleVolumeDLS,
    Mod2Vibrange,
    SetupTremolo,
    Return,
    GoSub,
    TrapEvent = 0x28,
    UntrapEvent,
    SendMessage,
    GetMessage,
    GetVid,
    AddAgeCount = 0x30, /* unimplemented */
    SetAgeCount,        /* unimplemented */
    SendFlag,           /* unimplemented */
    PitchWheelR,
    SetPriority = 0x36, /* unimplemented */
    AddPriority,        /* unimplemented */
    AgeCntSpeed,        /* unimplemented */
    AgeCntVel,          /* unimplemented */
    VolSelect = 0x40,
    PanSelect,
    PitchWheelSelect,
    ModWheelSelect,
    PedalSelect,
    PortamentoSelect,
    ReverbSelect, /* serves as PostASelect */
    SpanSelect,
    DopplerSelect,
    TremoloSelect,
    PreASelect,
    PreBSelect,
    PostBSelect,
    AuxAFXSelect, /* unimplemented */
    AuxBFXSelect, /* unimplemented */
    SetupLFO = 0x50,
    ModeSelect = 0x58,
    SetKeygroup,
    SRCmodeSelect, /* unimplemented */
    WiiUnknown = 0x5E,
    WiiUnknown2 = 0x5F,
    AddVars = 0x60,
    SubVars,
    MulVars,
    DivVars,
    AddIVars,
    SetVar,
    IfEqual = 0x70,
    IfLess,
    CmdOpMAX,
    Invalid = 0xff
  };

  enum class CmdType : uint8_t { Control, Pitch, Sample, Setup, Special, Structure, Volume, CmdTypeMAX };

  /** Introspection structure used by editors to define user interactivity per command */
  struct CmdIntrospection {
    struct Field {
      enum class Type {
        Invalid,
        Bool,
        Int8,
        UInt8,
        Int16,
        UInt16,
        Int32,
        UInt32,
        SoundMacroId,
        SoundMacroStep,
        TableId,
        SampleId,
        Choice
      };
      Type m_tp;
      size_t m_offset;
      std::string_view m_name;
      int64_t m_min, m_max, m_default;
      std::array<std::string_view, 4> m_choices;
    };
    CmdType m_tp;
    std::string_view m_name;
    std::string_view m_description;
    std::array<Field, 7> m_fields;
  };

  /** Base command interface. All versions of MusyX encode little-endian parameters */
  struct ICmd : LittleDNAV {
    AT_DECL_DNA_YAMLV
    virtual bool Do(SoundMacroState& st, Voice& vox) const = 0;
    virtual CmdOp Isa() const = 0;
    /** Execute this command for fluidsyX.  Returns delay in ms (0 = continue immediately). */
    virtual unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const;
  };
  struct CmdEnd : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::End; }
  };
  struct CmdStop : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::Stop; }
  };
  struct CmdSplitKey : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<int8_t> key;
    SoundMacroIdDNA<std::endian::little> macro;
    SoundMacroStepDNA<std::endian::little> macroStep;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::SplitKey; }
  };
  struct CmdSplitVel : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<int8_t> velocity;
    SoundMacroIdDNA<std::endian::little> macro;
    SoundMacroStepDNA<std::endian::little> macroStep;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::SplitVel; }
  };
  struct CmdWaitTicks : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<bool> keyOff;
    Value<bool> random;
    Value<bool> sampleEnd;
    Value<bool> absolute;
    Value<bool> msSwitch;
    Value<uint16_t> ticksOrMs;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::WaitTicks; }
  };
  struct CmdLoop : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<bool> keyOff;
    Value<bool> random;
    Value<bool> sampleEnd;
    SoundMacroStepDNA<std::endian::little> macroStep;
    Value<uint16_t> times;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::Loop; }
  };
  struct CmdGoto : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Seek<1, athena::SeekOrigin::Current> dummy;
    SoundMacroIdDNA<std::endian::little> macro;
    SoundMacroStepDNA<std::endian::little> macroStep;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::Goto; }
  };
  struct CmdWaitMs : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<bool> keyOff;
    Value<bool> random;
    Value<bool> sampleEnd;
    Value<bool> absolute;
    Seek<1, athena::SeekOrigin::Current> dummy;
    Value<uint16_t> ms;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::WaitMs; }
  };
  struct CmdPlayMacro : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<int8_t> addNote;
    SoundMacroIdDNA<std::endian::little> macro;
    SoundMacroStepDNA<std::endian::little> macroStep;
    Value<uint8_t> priority;
    Value<uint8_t> maxVoices;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::PlayMacro; }
  };
  struct CmdSendKeyOff : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<uint8_t> variable;
    Value<bool> lastStarted;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::SendKeyOff; }
  };
  struct CmdSplitMod : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<int8_t> modValue;
    SoundMacroIdDNA<std::endian::little> macro;
    SoundMacroStepDNA<std::endian::little> macroStep;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::SplitMod; }
  };
  struct CmdPianoPan : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<int8_t> scale;
    Value<int8_t> centerKey;
    Value<int8_t> centerPan;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::PianoPan; }
  };
  struct CmdSetAdsr : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    TableIdDNA<std::endian::little> table;
    Value<bool> dlsMode;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::SetAdsr; }
  };
  struct CmdScaleVolume : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<int8_t> scale;
    Value<int8_t> add;
    TableIdDNA<std::endian::little> table;
    Value<bool> originalVol;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::ScaleVolume; }
  };
  struct CmdPanning : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<int8_t> panPosition;
    Value<uint16_t> timeMs;
    Value<int8_t> width;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::Panning; }
  };
  struct CmdEnvelope : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<int8_t> scale;
    Value<int8_t> add;
    TableIdDNA<std::endian::little> table;
    Value<bool> msSwitch;
    Value<uint16_t> ticksOrMs;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::Envelope; }
  };
  struct CmdStartSample : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    enum class Mode : int8_t { NoScale = 0, Negative = 1, Positive = 2 };
    SampleIdDNA<std::endian::little> sample;
    Value<Mode> mode;
    Value<uint32_t> offset;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::StartSample; }
  };
  struct CmdStopSample : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::StopSample; }
  };
  struct CmdKeyOff : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::KeyOff; }
  };
  struct CmdSplitRnd : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<uint8_t> rnd;
    SoundMacroIdDNA<std::endian::little> macro;
    SoundMacroStepDNA<std::endian::little> macroStep;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::SplitRnd; }
  };
  struct CmdFadeIn : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<int8_t> scale;
    Value<int8_t> add;
    TableIdDNA<std::endian::little> table;
    Value<bool> msSwitch;
    Value<uint16_t> ticksOrMs;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::FadeIn; }
  };
  struct CmdSpanning : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<int8_t> spanPosition;
    Value<uint16_t> timeMs;
    Value<int8_t> width;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::Spanning; }
  };
  struct CmdSetAdsrCtrl : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<uint8_t> attack;
    Value<uint8_t> decay;
    Value<uint8_t> sustain;
    Value<uint8_t> release;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::SetAdsrCtrl; }
  };
  struct CmdRndNote : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<int8_t> noteLo;
    Value<int8_t> detune;
    Value<int8_t> noteHi;
    Value<bool> fixedFree;
    Value<bool> absRel;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::RndNote; }
  };
  struct CmdAddNote : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<int8_t> add;
    Value<int8_t> detune;
    Value<bool> originalKey;
    Seek<1, athena::SeekOrigin::Current> seek;
    Value<bool> msSwitch;
    Value<uint16_t> ticksOrMs;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::AddNote; }
  };
  struct CmdSetNote : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<int8_t> key;
    Value<int8_t> detune;
    Seek<2, athena::SeekOrigin::Current> seek;
    Value<bool> msSwitch;
    Value<uint16_t> ticksOrMs;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::SetNote; }
  };
  struct CmdLastNote : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<int8_t> add;
    Value<int8_t> detune;
    Seek<2, athena::SeekOrigin::Current> seek;
    Value<bool> msSwitch;
    Value<uint16_t> ticksOrMs;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::LastNote; }
  };
  struct CmdPortamento : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    enum class PortState : int8_t { Disable, Enable, MIDIControlled };
    Value<PortState> portState;
    enum class PortType : int8_t { LastPressed, Always };
    Value<PortType> portType;
    Seek<2, athena::SeekOrigin::Current> seek;
    Value<bool> msSwitch;
    Value<uint16_t> ticksOrMs;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::Portamento; }
  };
  struct CmdVibrato : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<int8_t> levelNote;
    Value<int8_t> levelFine;
    Value<bool> modwheelFlag;
    Seek<1, athena::SeekOrigin::Current> seek;
    Value<bool> msSwitch;
    Value<uint16_t> ticksOrMs;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::Vibrato; }
  };
  struct CmdPitchSweep1 : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<int8_t> times;
    Value<int16_t> add;
    Seek<1, athena::SeekOrigin::Current> seek;
    Value<bool> msSwitch;
    Value<uint16_t> ticksOrMs;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::PitchSweep1; }
  };
  struct CmdPitchSweep2 : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<int8_t> times;
    Value<int16_t> add;
    Seek<1, athena::SeekOrigin::Current> seek;
    Value<bool> msSwitch;
    Value<uint16_t> ticksOrMs;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::PitchSweep2; }
  };
  struct CmdSetPitch : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    LittleUInt24 hz;
    Value<uint16_t> fine;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::SetPitch; }
  };
  struct CmdSetPitchAdsr : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    TableIdDNA<std::endian::little> table;
    Seek<1, athena::SeekOrigin::Current> seek;
    Value<int8_t> keys;
    Value<int8_t> cents;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::SetPitchAdsr; }
  };
  struct CmdScaleVolumeDLS : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<int16_t> scale;
    Value<bool> originalVol;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::ScaleVolumeDLS; }
  };
  struct CmdMod2Vibrange : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<int8_t> keys;
    Value<int8_t> cents;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::Mod2Vibrange; }
  };
  struct CmdSetupTremolo : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<int16_t> scale;
    Seek<1, athena::SeekOrigin::Current> seek;
    Value<int16_t> modwAddScale;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::SetupTremolo; }
  };
  struct CmdReturn : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::Return; }
  };
  struct CmdGoSub : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Seek<1, athena::SeekOrigin::Current> seek;
    SoundMacroIdDNA<std::endian::little> macro;
    SoundMacroStepDNA<std::endian::little> macroStep;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::GoSub; }
  };
  struct CmdTrapEvent : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    enum class EventType : int8_t { KeyOff, SampleEnd, MessageRecv };
    Value<EventType> event;
    SoundMacroIdDNA<std::endian::little> macro;
    SoundMacroStepDNA<std::endian::little> macroStep;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::TrapEvent; }
  };
  struct CmdUntrapEvent : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<CmdTrapEvent::EventType> event;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::UntrapEvent; }
  };
  struct CmdSendMessage : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<bool> isVar;
    SoundMacroIdDNA<std::endian::little> macro;
    Value<uint8_t> voiceVar;
    Value<uint8_t> valueVar;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::SendMessage; }
  };
  struct CmdGetMessage : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<uint8_t> variable;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::GetMessage; }
  };
  struct CmdGetVid : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<uint8_t> variable;
    Value<bool> playMacro;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::GetVid; }
  };
  struct CmdAddAgeCount : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Seek<1, athena::SeekOrigin::Current> seek;
    Value<int16_t> add;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::AddAgeCount; }
  };
  struct CmdSetAgeCount : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Seek<1, athena::SeekOrigin::Current> seek;
    Value<uint16_t> counter;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::SetAgeCount; }
  };
  struct CmdSendFlag : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<uint8_t> flagId;
    Value<uint8_t> value;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::SendFlag; }
  };
  struct CmdPitchWheelR : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<int8_t> rangeUp;
    Value<int8_t> rangeDown;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::PitchWheelR; }
  };
  struct CmdSetPriority : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<uint8_t> prio;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::SetPriority; }
  };
  struct CmdAddPriority : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Seek<1, athena::SeekOrigin::Current> seek;
    Value<int16_t> prio;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::AddPriority; }
  };
  struct CmdAgeCntSpeed : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Seek<3, athena::SeekOrigin::Current> seek;
    Value<uint32_t> time;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::AgeCntSpeed; }
  };
  struct CmdAgeCntVel : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Seek<1, athena::SeekOrigin::Current> seek;
    Value<uint16_t> ageBase;
    Value<uint16_t> ageScale;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::AgeCntVel; }
  };
  enum class Combine : int8_t { Set, Add, Mult };
  struct CmdVolSelect : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<uint8_t> midiControl;
    Value<int16_t> scalingPercentage;
    Value<Combine> combine;
    Value<bool> isVar;
    Value<int8_t> fineScaling;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::VolSelect; }
  };
  struct CmdPanSelect : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<uint8_t> midiControl;
    Value<int16_t> scalingPercentage;
    Value<Combine> combine;
    Value<bool> isVar;
    Value<int8_t> fineScaling;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::PanSelect; }
  };
  struct CmdPitchWheelSelect : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<uint8_t> midiControl;
    Value<int16_t> scalingPercentage;
    Value<Combine> combine;
    Value<bool> isVar;
    Value<int8_t> fineScaling;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::PitchWheelSelect; }
  };
  struct CmdModWheelSelect : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<uint8_t> midiControl;
    Value<int16_t> scalingPercentage;
    Value<Combine> combine;
    Value<bool> isVar;
    Value<int8_t> fineScaling;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::ModWheelSelect; }
  };
  struct CmdPedalSelect : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<uint8_t> midiControl;
    Value<int16_t> scalingPercentage;
    Value<Combine> combine;
    Value<bool> isVar;
    Value<int8_t> fineScaling;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::PedalSelect; }
  };
  struct CmdPortamentoSelect : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<uint8_t> midiControl;
    Value<int16_t> scalingPercentage;
    Value<Combine> combine;
    Value<bool> isVar;
    Value<int8_t> fineScaling;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::PortamentoSelect; }
  };
  struct CmdReverbSelect : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<uint8_t> midiControl;
    Value<int16_t> scalingPercentage;
    Value<Combine> combine;
    Value<bool> isVar;
    Value<int8_t> fineScaling;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::ReverbSelect; }
  };
  struct CmdSpanSelect : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<uint8_t> midiControl;
    Value<int16_t> scalingPercentage;
    Value<Combine> combine;
    Value<bool> isVar;
    Value<int8_t> fineScaling;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::SpanSelect; }
  };
  struct CmdDopplerSelect : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<uint8_t> midiControl;
    Value<int16_t> scalingPercentage;
    Value<Combine> combine;
    Value<bool> isVar;
    Value<int8_t> fineScaling;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::DopplerSelect; }
  };
  struct CmdTremoloSelect : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<uint8_t> midiControl;
    Value<int16_t> scalingPercentage;
    Value<Combine> combine;
    Value<bool> isVar;
    Value<int8_t> fineScaling;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::TremoloSelect; }
  };
  struct CmdPreASelect : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<uint8_t> midiControl;
    Value<int16_t> scalingPercentage;
    Value<Combine> combine;
    Value<bool> isVar;
    Value<int8_t> fineScaling;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::PreASelect; }
  };
  struct CmdPreBSelect : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<uint8_t> midiControl;
    Value<int16_t> scalingPercentage;
    Value<Combine> combine;
    Value<bool> isVar;
    Value<int8_t> fineScaling;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::PreBSelect; }
  };
  struct CmdPostBSelect : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<uint8_t> midiControl;
    Value<int16_t> scalingPercentage;
    Value<Combine> combine;
    Value<bool> isVar;
    Value<int8_t> fineScaling;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::PostBSelect; }
  };
  struct CmdAuxAFXSelect : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<uint8_t> midiControl;
    Value<int16_t> scalingPercentage;
    Value<Combine> combine;
    Value<bool> isVar;
    Value<int8_t> fineScaling;
    Value<uint8_t> paramIndex;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::AuxAFXSelect; }
  };
  struct CmdAuxBFXSelect : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<uint8_t> midiControl;
    Value<int16_t> scalingPercentage;
    Value<Combine> combine;
    Value<bool> isVar;
    Value<int8_t> fineScaling;
    Value<uint8_t> paramIndex;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::AuxBFXSelect; }
  };
  struct CmdSetupLFO : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<uint8_t> lfoNumber;
    Value<int16_t> periodInMs;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    bool fluid(fluid_voice_t* v) const;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::SetupLFO; }
  };
  struct CmdModeSelect : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<bool> dlsVol;
    Value<bool> itd;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::ModeSelect; }
  };
  struct CmdSetKeygroup : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<uint8_t> group;
    Value<bool> killNow;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::SetKeygroup; }
  };
  struct CmdSRCmodeSelect : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<uint8_t> srcType;
    Value<uint8_t> type0SrcFilter;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::SRCmodeSelect; }
  };
  struct CmdWiiUnknown : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<bool> flag;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::WiiUnknown; }
  };
  struct CmdWiiUnknown2 : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<bool> flag;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    CmdOp Isa() const override { return CmdOp::WiiUnknown2; }
  };
  struct CmdAddVars : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<bool> varCtrlA;
    Value<uint8_t> a;
    Value<bool> varCtrlB;
    Value<uint8_t> b;
    Value<bool> varCtrlC;
    Value<uint8_t> c;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::AddVars; }
  };
  struct CmdSubVars : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<bool> varCtrlA;
    Value<int8_t> a;
    Value<bool> varCtrlB;
    Value<int8_t> b;
    Value<bool> varCtrlC;
    Value<int8_t> c;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::SubVars; }
  };
  struct CmdMulVars : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<bool> varCtrlA;
    Value<int8_t> a;
    Value<bool> varCtrlB;
    Value<int8_t> b;
    Value<bool> varCtrlC;
    Value<int8_t> c;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::MulVars; }
  };
  struct CmdDivVars : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<bool> varCtrlA;
    Value<int8_t> a;
    Value<bool> varCtrlB;
    Value<int8_t> b;
    Value<bool> varCtrlC;
    Value<int8_t> c;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::DivVars; }
  };
  struct CmdAddIVars : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<bool> varCtrlA;
    Value<int8_t> a;
    Value<bool> varCtrlB;
    Value<int8_t> b;
    Value<int16_t> imm;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::AddIVars; }
  };
  struct CmdSetVar : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<bool> varCtrlA;
    Value<int8_t> a;
    Seek<1, athena::SeekOrigin::Current> pad;
    Value<int16_t> imm;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::SetVar; }
  };
  struct CmdIfEqual : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<bool> varCtrlA;
    Value<int8_t> a;
    Value<bool> varCtrlB;
    Value<int8_t> b;
    Value<bool> notEq;
    SoundMacroStepDNA<std::endian::little> macroStep;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::IfEqual; }
  };
  struct CmdIfLess : ICmd {
    AT_DECL_DNA_YAMLV
    static const CmdIntrospection Introspective;
    Value<bool> varCtrlA;
    Value<int8_t> a;
    Value<bool> varCtrlB;
    Value<int8_t> b;
    Value<bool> notLt;
    SoundMacroStepDNA<std::endian::little> macroStep;
    bool Do(SoundMacroState& st, Voice& vox) const override;
    unsigned int DoFluid(MacroExecContext& ctx, fluid_voice_t* v) const override;
    CmdOp Isa() const override { return CmdOp::IfLess; }
  };

  template <class Op, class O, class... _Args>
  static O CmdDo(_Args&&... args);
  static std::unique_ptr<SoundMacro::ICmd> MakeCmd(CmdOp op);
  static const CmdIntrospection* GetCmdIntrospection(CmdOp op);
  static std::string_view CmdOpToStr(CmdOp op);
  static CmdOp CmdStrToOp(std::string_view op);

  std::vector<std::unique_ptr<ICmd>> m_cmds;
  int assertPC(int pc) const;

  const ICmd& getCmd(int i) const { return *m_cmds[assertPC(i)]; }

  template <std::endian DNAE>
  void readCmds(athena::io::IStreamReader& r, uint32_t size);
  template <std::endian DNAE>
  void writeCmds(athena::io::IStreamWriter& w) const;

  ICmd* insertNewCmd(int idx, CmdOp op) { return m_cmds.insert(m_cmds.begin() + idx, MakeCmd(op))->get(); }
  ICmd* insertCmd(int idx, std::unique_ptr<ICmd>&& cmd) {
    return m_cmds.insert(m_cmds.begin() + idx, std::move(cmd))->get();
  }
  std::unique_ptr<ICmd> deleteCmd(int idx) {
    std::unique_ptr<ICmd> ret = std::move(m_cmds[idx]);
    m_cmds.erase(m_cmds.begin() + idx);
    return ret;
  }
  void swapPositions(int a, int b) {
    if (a == b)
      return;
    std::swap(m_cmds[a], m_cmds[b]);
  }
  void buildFromPrototype(const SoundMacro& other);

  void toYAML(athena::io::YAMLDocWriter& w) const;
  void fromYAML(athena::io::YAMLDocReader& r, size_t cmdCount);
};

template <typename T>
inline T& AccessField(SoundMacro::ICmd* cmd, const SoundMacro::CmdIntrospection::Field& field) {
  return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(std::addressof(*cmd)) + field.m_offset);
}

/** Converts time-cents representation to seconds */
inline double TimeCentsToSeconds(int32_t tc) {
  if (uint32_t(tc) == 0x80000000)
    return 0.0;
  return std::exp2(tc / (1200.0 * 65536.0));
}

/** Converts seconds representation to time-cents */
inline int32_t SecondsToTimeCents(double sec) {
  if (sec == 0.0)
    return 0x80000000;
  return int32_t(std::log2(sec) * (1200.0 * 65536.0));
}

/** Polymorphic interface for representing table data */
struct ITable : LittleDNAV {
  AT_DECL_DNA_YAMLV
  enum class Type { ADSR, ADSRDLS, Curve };
  virtual Type Isa() const = 0;
};

/** Defines phase-based volume curve for macro volume control */
struct ADSR : ITable {
  AT_DECL_DNA_YAMLV
  Value<uint16_t> attack = 0;
  Value<uint16_t> decay = 0x8000;
  Value<uint16_t> sustain = 0; /* 0x1000 == 100% */
  Value<uint16_t> release = 0; /* milliseconds */

  double getAttack() const { return attack / 1000.0; }
  void setAttack(double v) { attack = v * 1000.0; }
  double getDecay() const { return (decay == 0x8000) ? 0.0 : (decay / 1000.0); }
  void setDecay(double v) { decay = v == 0.0 ? 0x8000 : v * 1000.0; }
  double getSustain() const { return sustain / double(0x1000); }
  void setSustain(double v) { sustain = v * double(0x1000); }
  double getRelease() const { return release / 1000.0; }
  void setRelease(double v) { release = v * 1000.0; }

  Type Isa() const override { return ITable::Type::ADSR; }
};

/** Defines phase-based volume curve for macro volume control (modified DLS standard) */
struct ADSRDLS : ITable {
  AT_DECL_DNA_YAMLV
  Value<uint32_t> attack = 0x80000000;      /* 16.16 Time-cents */
  Value<uint32_t> decay = 0x80000000;       /* 16.16 Time-cents */
  Value<uint16_t> sustain = 0;              /* 0x1000 == 100% */
  Value<uint16_t> release = 0;              /* milliseconds */
  Value<uint32_t> velToAttack = 0x80000000; /* 16.16, 1000.0 == 100%; attack = <attack> + (vel/128) * <velToAttack> */
  Value<uint32_t> keyToDecay = 0x80000000;  /* 16.16, 1000.0 == 100%; decay = <decay> + (note/128) * <keyToDecay> */

  double getAttack() const { return TimeCentsToSeconds(attack); }
  void setAttack(double v) { attack = SecondsToTimeCents(v); }
  double getDecay() const { return TimeCentsToSeconds(decay); }
  void setDecay(double v) { decay = SecondsToTimeCents(v); }
  double getSustain() const { return sustain / double(0x1000); }
  void setSustain(double v) { sustain = v * double(0x1000); }
  double getRelease() const { return release / 1000.0; }
  void setRelease(double v) { release = v * 1000.0; }

  double _getVelToAttack() const {
    if (velToAttack == 0x80000000)
      return 0.0;
    else
      return velToAttack / 65536.0 / 1000.0;
  }

  void _setVelToAttack(double v) {
    if (v == 0.0)
      velToAttack = 0x80000000;
    else
      velToAttack = uint32_t(v * 1000.0 * 65536.0);
  }

  double _getKeyToDecay() const {
    if (keyToDecay == 0x80000000)
      return 0.0;
    else
      return keyToDecay / 65536.0 / 1000.0;
  }

  void _setKeyToDecay(double v) {
    if (v == 0.0)
      keyToDecay = 0x80000000;
    else
      keyToDecay = uint32_t(v * 1000.0 * 65536.0);
  }

  double getVelToAttack(int8_t vel) const {
    if (velToAttack == 0x80000000)
      return getAttack();
    return getAttack() + vel * (velToAttack / 65536.0 / 1000.0) / 128.0;
  }

  double getKeyToDecay(int8_t note) const {
    if (keyToDecay == 0x80000000)
      return getDecay();
    return getDecay() + note * (keyToDecay / 65536.0 / 1000.0) / 128.0;
  }

  Type Isa() const override  { return ITable::Type::ADSRDLS; }
};

/** Defines arbitrary data for use as volume curve */
struct Curve : ITable {
  AT_DECL_EXPLICIT_DNA_YAMLV
  std::vector<uint8_t> data;

  Type Isa() const override  { return ITable::Type::Curve; }
};

/** Maps individual MIDI keys to sound-entity as indexed in table
 *  (macro-voice, keymap, layer) */
template <std::endian DNAEn>
struct AT_SPECIALIZE_PARMS(std::endian::big, std::endian::little) KeymapDNA : BigDNA {
  AT_DECL_DNA
  SoundMacroIdDNA<DNAEn> macro;
  Value<int8_t> transpose;
  Value<int8_t> pan; /* -128 for surround-channel only */
  Value<int8_t> prioOffset;
  Seek<3, athena::SeekOrigin::Current> pad;
};
struct Keymap : BigDNA {
  AT_DECL_DNA_YAML
  SoundMacroIdDNA<std::endian::big> macro;
  Value<int8_t> transpose = 0;
  Value<int8_t> pan = 64; /* -128 for surround-channel only */
  Value<int8_t> prioOffset = 0;

  Keymap() = default;

  template <std::endian DNAE>
  Keymap(const KeymapDNA<DNAE>& in)
  : macro(in.macro.id), transpose(in.transpose), pan(in.pan), prioOffset(in.prioOffset) {}

  template <std::endian DNAEn>
  KeymapDNA<DNAEn> toDNA() const {
    KeymapDNA<DNAEn> ret;
    ret.macro.id = macro;
    ret.transpose = transpose;
    ret.pan = pan;
    ret.prioOffset = prioOffset;
    return ret;
  }

  uint64_t configKey() const {
    return uint64_t(macro.id.id) | (uint64_t(transpose) << 16) | (uint64_t(pan) << 24) | (uint64_t(prioOffset) << 32);
  }
};

/** Maps ranges of MIDI keys to sound-entity (macro-voice, keymap, layer) */
template <std::endian DNAEn>
struct AT_SPECIALIZE_PARMS(std::endian::big, std::endian::little) LayerMappingDNA : BigDNA {
  AT_DECL_DNA
  SoundMacroIdDNA<DNAEn> macro;
  Value<int8_t> keyLo;
  Value<int8_t> keyHi;
  Value<int8_t> transpose;
  Value<int8_t> volume;
  Value<int8_t> prioOffset;
  Value<int8_t> span;
  Value<int8_t> pan;
  Seek<3, athena::SeekOrigin::Current> pad;
};
struct LayerMapping : BigDNA {
  AT_DECL_DNA_YAML
  SoundMacroIdDNA<std::endian::big> macro;
  Value<int8_t> keyLo = 0;
  Value<int8_t> keyHi = 127;
  Value<int8_t> transpose = 0;
  Value<int8_t> volume = 127;
  Value<int8_t> prioOffset = 0;
  Value<int8_t> span = 0;
  Value<int8_t> pan = 64;

  LayerMapping() = default;

  template <std::endian DNAE>
  LayerMapping(const LayerMappingDNA<DNAE>& in)
  : macro(in.macro.id)
  , keyLo(in.keyLo)
  , keyHi(in.keyHi)
  , transpose(in.transpose)
  , volume(in.volume)
  , prioOffset(in.prioOffset)
  , span(in.span)
  , pan(in.pan) {}

  template <std::endian DNAEn>
  LayerMappingDNA<DNAEn> toDNA() const {
    LayerMappingDNA<DNAEn> ret;
    ret.macro.id = macro;
    ret.keyLo = keyLo;
    ret.keyHi = keyHi;
    ret.transpose = transpose;
    ret.volume = volume;
    ret.prioOffset = prioOffset;
    ret.span = span;
    ret.pan = pan;
    return ret;
  }
};

/** Database of functional objects within Audio Group */
class AudioGroupPool {
  std::unordered_map<SoundMacroId, ObjToken<SoundMacro>> m_soundMacros;
  std::unordered_map<TableId, ObjToken<std::unique_ptr<ITable>>> m_tables;
  std::unordered_map<KeymapId, ObjToken<std::array<Keymap, 128>>> m_keymaps;
  std::unordered_map<LayersId, ObjToken<std::vector<LayerMapping>>> m_layers;

  template <std::endian DNAE>
  static AudioGroupPool _AudioGroupPool(athena::io::IStreamReader& r);

public:
  AudioGroupPool() = default;
  static AudioGroupPool CreateAudioGroupPool(const AudioGroupData& data);
  static AudioGroupPool CreateAudioGroupPool(std::string_view groupPath);

  const std::unordered_map<SoundMacroId, ObjToken<SoundMacro>>& soundMacros() const { return m_soundMacros; }
  const std::unordered_map<TableId, ObjToken<std::unique_ptr<ITable>>>& tables() const { return m_tables; }
  const std::unordered_map<KeymapId, ObjToken<std::array<Keymap, 128>>>& keymaps() const { return m_keymaps; }
  const std::unordered_map<LayersId, ObjToken<std::vector<LayerMapping>>>& layers() const { return m_layers; }
  std::unordered_map<SoundMacroId, ObjToken<SoundMacro>>& soundMacros() { return m_soundMacros; }
  std::unordered_map<TableId, ObjToken<std::unique_ptr<ITable>>>& tables() { return m_tables; }
  std::unordered_map<KeymapId, ObjToken<std::array<Keymap, 128>>>& keymaps() { return m_keymaps; }
  std::unordered_map<LayersId, ObjToken<std::vector<LayerMapping>>>& layers() { return m_layers; }

  const SoundMacro* soundMacro(ObjectId id) const;
  const Keymap* keymap(ObjectId id) const;
  const std::vector<LayerMapping>* layer(ObjectId id) const;
  const ADSR* tableAsAdsr(ObjectId id) const;
  const ADSRDLS* tableAsAdsrDLS(ObjectId id) const;
  const Curve* tableAsCurves(ObjectId id) const;

  std::vector<uint8_t> toYAML() const;
  template <std::endian DNAE>
  std::vector<uint8_t> toData() const;

  AudioGroupPool(const AudioGroupPool&) = delete;
  AudioGroupPool& operator=(const AudioGroupPool&) = delete;
  AudioGroupPool(AudioGroupPool&&) = default;
  AudioGroupPool& operator=(AudioGroupPool&&) = default;
};
} // namespace amuse
