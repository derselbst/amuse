#include "amuse/BooBackend.hpp"
#include "amuse/Voice.hpp"
#include "amuse/Submix.hpp"
#include "amuse/Engine.hpp"

namespace amuse
{

void BooBackendVoice::VoiceCallback::preSupplyAudio(boo::IAudioVoice&,
                                                    double dt)
{
    m_parent.m_clientVox.preSupplyAudio(dt);
}

size_t BooBackendVoice::VoiceCallback::supplyAudio(boo::IAudioVoice&,
                                                   size_t frames, int16_t* data)
{
    return m_parent.m_clientVox.supplyAudio(frames, data);
}

BooBackendVoice::BooBackendVoice(boo::IAudioVoiceEngine& engine, Voice& clientVox,
                                 double sampleRate, bool dynamicPitch)
: m_clientVox(clientVox), m_cb(*this),
  m_booVoice(engine.allocateNewMonoVoice(sampleRate, &m_cb, dynamicPitch))
{}

void BooBackendVoice::resetSampleRate(double sampleRate)
{
    m_booVoice->resetSampleRate(sampleRate);
}

void BooBackendVoice::resetChannelLevels()
{
    m_booVoice->resetChannelLevels();
}

void BooBackendVoice::setChannelLevels(IBackendSubmix* submix, const float coefs[8], bool slew)
{
    BooBackendSubmix& smx = *reinterpret_cast<BooBackendSubmix*>(submix);
    m_booVoice->setMonoChannelLevels(smx.m_booSubmix.get(), coefs, slew);
}

void BooBackendVoice::setPitchRatio(double ratio, bool slew)
{
    m_booVoice->setPitchRatio(ratio, slew);
}

void BooBackendVoice::start()
{
    m_booVoice->start();
}

void BooBackendVoice::stop()
{
    m_booVoice->stop();
}

bool BooBackendSubmix::SubmixCallback::canApplyEffect() const
{
    return m_parent.m_clientSmx.canApplyEffect();
}

void BooBackendSubmix::SubmixCallback::applyEffect(int16_t* audio, size_t frameCount,
                                                   const boo::ChannelMap& chanMap, double) const
{
    return m_parent.m_clientSmx.applyEffect(audio, frameCount, reinterpret_cast<const ChannelMap&>(chanMap));
}

void BooBackendSubmix::SubmixCallback::applyEffect(int32_t* audio, size_t frameCount,
                                                   const boo::ChannelMap& chanMap, double) const
{
    return m_parent.m_clientSmx.applyEffect(audio, frameCount, reinterpret_cast<const ChannelMap&>(chanMap));
}

void BooBackendSubmix::SubmixCallback::applyEffect(float* audio, size_t frameCount,
                                                   const boo::ChannelMap& chanMap, double) const
{
    return m_parent.m_clientSmx.applyEffect(audio, frameCount, reinterpret_cast<const ChannelMap&>(chanMap));
}

void BooBackendSubmix::SubmixCallback::resetOutputSampleRate(double sampleRate)
{
    m_parent.m_clientSmx.resetOutputSampleRate(sampleRate);
}

BooBackendSubmix::BooBackendSubmix(boo::IAudioVoiceEngine& engine, Submix& clientSmx, bool mainOut)
: m_clientSmx(clientSmx), m_cb(*this), m_booSubmix(engine.allocateNewSubmix(mainOut, &m_cb))
{}

void BooBackendSubmix::setSendLevel(IBackendSubmix* submix, float level, bool slew)
{
    BooBackendSubmix& smx = *reinterpret_cast<BooBackendSubmix*>(submix);
    m_booSubmix->setSendLevel(smx.m_booSubmix.get(), level, slew);
}

double BooBackendSubmix::getSampleRate() const
{
    return m_booSubmix->getSampleRate();
}

SubmixFormat BooBackendSubmix::getSampleFormat() const
{
    return SubmixFormat(m_booSubmix->getSampleFormat());
}


std::string BooBackendMIDIReader::description()
{
    return m_midiIn->description();
}

BooBackendMIDIReader::~BooBackendMIDIReader() {}

BooBackendMIDIReader::BooBackendMIDIReader(Engine& engine, const char* name, bool useLock)
: m_engine(engine), m_decoder(*this), m_useLock(useLock)
{
    BooBackendVoiceAllocator& voxAlloc = static_cast<BooBackendVoiceAllocator&>(engine.getBackend());
    if (!name)
    {
        auto devices = voxAlloc.m_booEngine.enumerateMIDIDevices();
        for (const auto& dev : devices)
        {
            m_midiIn = voxAlloc.m_booEngine.newRealMIDIIn(dev.first.c_str(),
                                                          std::bind(&BooBackendMIDIReader::_MIDIReceive, this,
                                                                    std::placeholders::_1, std::placeholders::_2));
            if (m_midiIn)
                return;
        }
        m_midiIn = voxAlloc.m_booEngine.newVirtualMIDIIn(std::bind(&BooBackendMIDIReader::_MIDIReceive, this,
                                                                   std::placeholders::_1, std::placeholders::_2));
    }
    else
        m_midiIn = voxAlloc.m_booEngine.newRealMIDIIn(name,
                                                      std::bind(&BooBackendMIDIReader::_MIDIReceive, this,
                                                                std::placeholders::_1, std::placeholders::_2));
}

void BooBackendMIDIReader::_MIDIReceive(std::vector<uint8_t>&& bytes, double time)
{
    std::unique_lock<std::mutex> lk(m_midiMutex, std::defer_lock_t{});
    if (m_useLock) lk.lock();
    m_queue.emplace_back(time, std::move(bytes));
#if 0
    openlog("LogIt", (LOG_CONS|LOG_PERROR|LOG_PID), LOG_DAEMON);
    syslog(LOG_EMERG, "MIDI receive %f\n", time);
    closelog();
#endif
}

void BooBackendMIDIReader::pumpReader(double dt)
{
    dt += 0.001; /* Add 1ms to ensure consumer keeps up with producer */

    std::unique_lock<std::mutex> lk(m_midiMutex, std::defer_lock_t{});
    if (m_useLock) lk.lock();
    if (m_queue.empty())
        return;

    /* Determine range of buffer updates within this period */
    auto periodEnd = m_queue.cbegin();
    double startPt = m_queue.front().first;
    for (; periodEnd != m_queue.cend() ; ++periodEnd)
    {
        double delta = periodEnd->first - startPt;
        if (delta > dt)
            break;
    }

    if (m_queue.cbegin() == periodEnd)
        return;

    /* Dispatch buffers */
    for (auto it = m_queue.begin() ; it != periodEnd ;)
    {
#if 0
        char str[64];
        sprintf(str, "MIDI %zu %f ", it->second.size(), it->first);
        for (uint8_t byte : it->second)
            sprintf(str + strlen(str), "%02X ", byte);
        openlog("LogIt", (LOG_CONS|LOG_PERROR|LOG_PID), LOG_DAEMON);
        syslog(LOG_EMERG, "%s\n", str);
        closelog();
#endif
        m_decoder.receiveBytes(it->second.cbegin(), it->second.cend());
        it = m_queue.erase(it);
    }
}

void BooBackendMIDIReader::noteOff(uint8_t chan, uint8_t key, uint8_t velocity)
{
    for (std::shared_ptr<Sequencer>& seq : m_engine.getActiveSequencers())
        seq->keyOff(chan, key, velocity);
#if 0
    openlog("LogIt", (LOG_CONS|LOG_PERROR|LOG_PID), LOG_DAEMON);
    syslog(LOG_EMERG, "NoteOff %d", key);
    closelog();
#endif
}

void BooBackendMIDIReader::noteOn(uint8_t chan, uint8_t key, uint8_t velocity)
{
    for (std::shared_ptr<Sequencer>& seq : m_engine.getActiveSequencers())
        seq->keyOn(chan, key, velocity);
#if 0
    openlog("LogIt", (LOG_CONS|LOG_PERROR|LOG_PID), LOG_DAEMON);
    syslog(LOG_EMERG, "NoteOn %d", key);
    closelog();
#endif
}

void BooBackendMIDIReader::notePressure(uint8_t /*chan*/, uint8_t /*key*/, uint8_t /*pressure*/)
{
}

void BooBackendMIDIReader::controlChange(uint8_t chan, uint8_t control, uint8_t value)
{
    for (std::shared_ptr<Sequencer>& seq : m_engine.getActiveSequencers())
        seq->setCtrlValue(chan, control, value);
}

void BooBackendMIDIReader::programChange(uint8_t chan, uint8_t program)
{
    for (std::shared_ptr<Sequencer>& seq : m_engine.getActiveSequencers())
        seq->setChanProgram(chan, program);
}

void BooBackendMIDIReader::channelPressure(uint8_t /*chan*/, uint8_t /*pressure*/)
{
}

void BooBackendMIDIReader::pitchBend(uint8_t chan, int16_t pitch)
{
    for (std::shared_ptr<Sequencer>& seq : m_engine.getActiveSequencers())
        seq->setPitchWheel(chan, (pitch - 0x2000) / float(0x2000));
}


void BooBackendMIDIReader::allSoundOff(uint8_t chan)
{
    for (std::shared_ptr<Sequencer>& seq : m_engine.getActiveSequencers())
        seq->allOff(chan, true);
}

void BooBackendMIDIReader::resetAllControllers(uint8_t /*chan*/)
{
}

void BooBackendMIDIReader::localControl(uint8_t /*chan*/, bool /*on*/)
{
}

void BooBackendMIDIReader::allNotesOff(uint8_t chan)
{
    for (std::shared_ptr<Sequencer>& seq : m_engine.getActiveSequencers())
        seq->allOff(chan, false);
}

void BooBackendMIDIReader::omniMode(uint8_t /*chan*/, bool /*on*/)
{
}

void BooBackendMIDIReader::polyMode(uint8_t /*chan*/, bool /*on*/)
{
}


void BooBackendMIDIReader::sysex(const void* /*data*/, size_t /*len*/)
{
}

void BooBackendMIDIReader::timeCodeQuarterFrame(uint8_t /*message*/, uint8_t /*value*/)
{
}

void BooBackendMIDIReader::songPositionPointer(uint16_t /*pointer*/)
{
}

void BooBackendMIDIReader::songSelect(uint8_t /*song*/)
{
}

void BooBackendMIDIReader::tuneRequest()
{
}


void BooBackendMIDIReader::startSeq()
{
}

void BooBackendMIDIReader::continueSeq()
{
}

void BooBackendMIDIReader::stopSeq()
{
}


void BooBackendMIDIReader::reset()
{
}


BooBackendVoiceAllocator::BooBackendVoiceAllocator(boo::IAudioVoiceEngine& booEngine)
: m_booEngine(booEngine)
{}

std::unique_ptr<IBackendVoice>
BooBackendVoiceAllocator::allocateVoice(Voice& clientVox, double sampleRate, bool dynamicPitch)
{
    return std::make_unique<BooBackendVoice>(m_booEngine, clientVox, sampleRate, dynamicPitch);
}

std::unique_ptr<IBackendSubmix> BooBackendVoiceAllocator::allocateSubmix(Submix& clientSmx, bool mainOut)
{
    return std::make_unique<BooBackendSubmix>(m_booEngine, clientSmx, mainOut);
}

std::vector<std::pair<std::string, std::string>> BooBackendVoiceAllocator::enumerateMIDIDevices()
{
    return m_booEngine.enumerateMIDIDevices();
}

std::unique_ptr<IMIDIReader> BooBackendVoiceAllocator::allocateMIDIReader(Engine& engine, const char* name)
{
    std::unique_ptr<IMIDIReader> ret = std::make_unique<BooBackendMIDIReader>(engine, name, m_booEngine.useMIDILock());
    if (!static_cast<BooBackendMIDIReader&>(*ret).m_midiIn)
        return {};
    return ret;
}

void BooBackendVoiceAllocator::register5MsCallback(std::function<void(double)>&& callback)
{
    m_booEngine.register5MsCallback(std::move(callback));
}

AudioChannelSet BooBackendVoiceAllocator::getAvailableSet()
{
    return AudioChannelSet(m_booEngine.getAvailableSet());
}

void BooBackendVoiceAllocator::pumpAndMixVoices()
{
    m_booEngine.pumpAndMixVoices();
}

}
