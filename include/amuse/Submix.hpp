#ifndef __AMUSE_SUBMIX_HPP__
#define __AMUSE_SUBMIX_HPP__

#include <memory>
#include "SoundMacroState.hpp"
#include "IBackendSubmix.hpp"
#include "IBackendVoice.hpp"
#include "EffectBase.hpp"
#include "EffectChorus.hpp"
#include "EffectDelay.hpp"
#include "EffectReverb.hpp"
#include <unordered_set>

namespace amuse
{
class IBackendSubmix;
class Sequencer;

/** Intermediate mix of voices for applying auxiliary effects */
class Submix
{
    friend class Engine;
    friend class Voice;
    friend class Sequencer;
    Engine& m_root;
    std::unique_ptr<IBackendSubmix> m_backendSubmix;                /**< Handle to client-implemented backend submix */
    std::vector<std::unique_ptr<EffectBaseTypeless>> m_effectStack; /**< Ordered list of effects to apply to submix */

public:
    Submix(Engine& engine);

    /** Add new effect to effect stack and assume ownership */
    template <class T, class... Args>
    T& makeEffect(Args... args)
    {
        switch (m_backendSubmix->getSampleFormat())
        {
        case SubmixFormat::Int16:
        {
            using ImpType = typename T::template ImpType<int16_t>;
            m_effectStack.emplace_back(new ImpType(args..., m_backendSubmix->getSampleRate()));
            return static_cast<ImpType&>(*m_effectStack.back());
        }
        case SubmixFormat::Int32:
        default:
        {
            using ImpType = typename T::template ImpType<int32_t>;
            m_effectStack.emplace_back(new ImpType(args..., m_backendSubmix->getSampleRate()));
            return static_cast<ImpType&>(*m_effectStack.back());
        }
        case SubmixFormat::Float:
        {
            using ImpType = typename T::template ImpType<float>;
            m_effectStack.emplace_back(new ImpType(args..., m_backendSubmix->getSampleRate()));
            return static_cast<ImpType&>(*m_effectStack.back());
        }
        }
    }

    /** Add new chorus effect to effect stack and assume ownership */
    EffectChorus& makeChorus(uint32_t baseDelay, uint32_t variation, uint32_t period);

    /** Add new chorus effect to effect stack and assume ownership */
    EffectChorus& makeChorus(const EffectChorusInfo& info);

    /** Add new delay effect to effect stack and assume ownership */
    EffectDelay& makeDelay(uint32_t initDelay, uint32_t initFeedback, uint32_t initOutput);

    /** Add new delay effect to effect stack and assume ownership */
    EffectDelay& makeDelay(const EffectDelayInfo& info);

    /** Add new standard-quality reverb effect to effect stack and assume ownership */
    EffectReverbStd& makeReverbStd(float coloration, float mix, float time, float damping, float preDelay);

    /** Add new standard-quality reverb effect to effect stack and assume ownership */
    EffectReverbStd& makeReverbStd(const EffectReverbStdInfo& info);

    /** Add new high-quality reverb effect to effect stack and assume ownership */
    EffectReverbHi& makeReverbHi(float coloration, float mix, float time, float damping, float preDelay,
                                 float crosstalk);

    /** Add new high-quality reverb effect to effect stack and assume ownership */
    EffectReverbHi& makeReverbHi(const EffectReverbHiInfo& info);

    /** Remove and deallocate all effects from effect stack */
    void clearEffects() { m_effectStack.clear(); }

    /** Returns true when an effect callback is bound */
    bool canApplyEffect() const { return m_effectStack.size() != 0; }

    /** in/out transformation entry for audio effect */
    void applyEffect(int16_t* audio, size_t frameCount, const ChannelMap& chanMap) const;

    /** in/out transformation entry for audio effect */
    void applyEffect(int32_t* audio, size_t frameCount, const ChannelMap& chanMap) const;

    /** in/out transformation entry for audio effect */
    void applyEffect(float* audio, size_t frameCount, const ChannelMap& chanMap) const;

    /** advice effects of changing sample rate */
    void resetOutputSampleRate(double sampleRate);

    Engine& getEngine() { return m_root; }
};
}

#endif // __AMUSE_SUBMIX_HPP__
