#ifndef __AMUSE_VOICE_HPP__
#define __AMUSE_VOICE_HPP__

#include <stdint.h>
#include <stdlib.h>
#include <memory>
#include <list>
#include "SoundMacroState.hpp"
#include "Entity.hpp"

namespace amuse
{
class IBackendVoice;

/** State of voice over lifetime */
enum class VoiceState
{
    Playing,
    KeyOff,
    Finished
};

/** Individual source of audio */
class Voice : public Entity
{
    friend class Engine;
    template <class U, class A>
    friend class std::list;
    int m_vid; /**< VoiceID of this voice instance */
    bool m_emitter; /**< Voice is part of an Emitter */
    std::list<Voice>::iterator m_engineIt; /**< Iterator to self within Engine's list for quick deletion */
    std::unique_ptr<IBackendVoice> m_backendVoice; /**< Handle to client-implemented backend voice */
    SoundMacroState m_state; /**< State container for SoundMacro playback */
    Voice *m_nextSibling = nullptr, *m_prevSibling = nullptr; /**< Sibling voice links for PLAYMACRO usage */
    uint8_t m_lastNote = 0; /**< Last MIDI semitone played by voice */
    uint8_t m_keygroup = 0; /**< Keygroup voice is a member of */

    void _destroy();

public:
    Voice(Engine& engine, const AudioGroup& group, int vid, bool emitter);
    Voice(Engine& engine, const AudioGroup& group, ObjectId oid, int vid, bool emitter);

    /** Request specified count of audio frames (samples) from voice,
     *  internally advancing the voice stream */
    size_t supplyAudio(size_t frames, int16_t* data);

    /** Get current state of voice */
    VoiceState state() const;

    /** Get VoiceId of this voice (unique to all currently-playing voices) */
    int vid() const {return m_vid;}

    /** Allocate parallel macro and tie to voice for possible emitter influence */
    Voice* startSiblingMacro(int8_t addNote, ObjectId macroId, int macroStep);

    /** Load specified SoundMacro ID of within group into voice */
    bool loadSoundMacro(ObjectId macroId, int macroStep=0, bool pushPc=false);

    /** Signals voice to begin fade-out, eventually reaching silence */
    void keyOff();

    /** Sends numeric message to voice and all siblings */
    void message(int32_t val);

    void startSample(int16_t sampId, int32_t offset);
    void stopSample();
    void setVolume(float vol);
    void setPanning(float pan);
    void setSurroundPanning(float span);
    void setPitchKey(int32_t cents);
    void setModulation(float mod);
    void setPedal(bool pedal);
    void setDoppler(float doppler);
    void setReverbVol(float rvol);
    void setAdsr(ObjectId adsrId);
    void setPitchFrequency(uint32_t hz, uint16_t fine);
    void setPitchAdsr(ObjectId adsrId, int32_t cents);
    void setPitchWheelRange(int8_t up, int8_t down);
    void setKeygroup(uint8_t kg) {m_keygroup = kg;}

    uint8_t getLastNote() const {return m_lastNote;}
    int8_t getCtrlValue(uint8_t ctrl) const;
    void setCtrlValue(uint8_t ctrl, int8_t val);
    int8_t getPitchWheel() const;
    int8_t getModWheel() const;
    int8_t getAftertouch() const;

};

}

#endif // __AMUSE_VOICE_HPP__
