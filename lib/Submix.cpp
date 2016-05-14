#include "amuse/Submix.hpp"

namespace amuse
{

void Submix::_destroy()
{
    m_destroyed = true;
    if (m_submix)
        m_submix->m_activeSubmixes.erase(this);
}

Submix::Submix(Engine& engine, Submix* smx)
: m_root(engine), m_submix(smx)
{
    if (m_submix)
        m_submix->m_activeSubmixes.insert(this);
}

EffectChorus& Submix::makeChorus(uint32_t baseDelay, uint32_t variation, uint32_t period)
{
    return makeEffect<EffectChorus>(baseDelay, variation, period);
}

EffectDelay& Submix::makeDelay(uint32_t initDelay, uint32_t initFeedback, uint32_t initOutput)
{
    return makeEffect<EffectDelay>(initDelay, initFeedback, initOutput);
}

EffectReverb& Submix::makeReverbStd(float coloration, float mix, float time,
                                    float damping, float preDelay)
{
    return makeEffect<EffectReverb>(coloration, mix, time, damping, preDelay);
}

EffectReverbHi& Submix::makeReverbHi(float coloration, float mix, float time,
                                     float damping, float preDelay, float crosstalk)
{
    return makeEffect<EffectReverbHi>(coloration, mix, time, damping, preDelay, crosstalk);
}

void Submix::applyEffect(int16_t* audio, size_t frameCount, const ChannelMap& chanMap) const
{
}

void Submix::applyEffect(int32_t* audio, size_t frameCount, const ChannelMap& chanMap) const
{
}

void Submix::applyEffect(float* audio, size_t frameCount, const ChannelMap& chanMap) const
{
}

}
