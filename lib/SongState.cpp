#include "amuse/SongState.hpp"
#include "amuse/Common.hpp"
#include "amuse/Sequencer.hpp"
#include <cmath>

namespace amuse
{

static uint32_t DecodeRLE(const unsigned char*& data)
{
    uint32_t ret = 0;

    while (true)
    {
        uint32_t thisPart = *data & 0x7f;
        if (*data & 0x80)
        {
            ++data;
            thisPart = thisPart * 256 + *data;
            if (thisPart == 0)
            {
                ++data;
                return -1;
            }
        }

        if (thisPart == 32767)
        {
            ret += 32767;
            data += 2;
            continue;
        }

        ret += thisPart;
        data += 1;
        break;
    }

    return ret;
}

static int32_t DecodeContinuousRLE(const unsigned char*& data)
{
    int32_t ret = int32_t(DecodeRLE(data));
    if (ret >= 16384)
        return ret - 32767;
    return ret;
}

static uint32_t DecodeTimeRLE(const unsigned char*& data)
{
    uint32_t ret = 0;

    while (true)
    {
        uint16_t thisPart = SBig(*reinterpret_cast<const uint16_t*>(data));
        if (thisPart == 0xffff)
        {
            ret += 65535;
            data += 4;
            continue;
        }

        ret += thisPart;
        data += 2;
        break;
    }

    return ret;
}

void SongState::Header::swapBig()
{
    m_trackIdxOff = SBig(m_trackIdxOff);
    m_regionIdxOff = SBig(m_regionIdxOff);
    m_chanMapOff = SBig(m_chanMapOff);
    m_tempoTableOff = SBig(m_tempoTableOff);
    m_initialTempo = SBig(m_initialTempo);
    m_unkOff = SBig(m_unkOff);
}

bool SongState::TrackRegion::indexValid(bool bigEndian) const
{
    return (bigEndian ? SBig(m_regionIndex) : m_regionIndex) >= 0;
}

void SongState::TempoChange::swapBig()
{
    m_tick = SBig(m_tick);
    m_tempo = SBig(m_tempo);
}

void SongState::Track::Header::swapBig()
{
    m_type = SBig(m_type);
    m_pitchOff = SBig(m_pitchOff);
    m_modOff = SBig(m_modOff);
}

SongState::Track::Track(SongState& parent, uint8_t midiChan, const TrackRegion* regions)
: m_parent(parent), m_midiChan(midiChan), m_curRegion(nullptr), m_nextRegion(regions)
{
    for (int i = 0; i < 128; ++i)
        m_remNoteLengths[i] = INT_MIN;
}

void SongState::Track::setRegion(Sequencer* seq, const TrackRegion* region)
{
    m_curRegion = region;
    uint32_t regionIdx = (m_parent.m_bigEndian ? SBig(m_curRegion->m_regionIndex) : m_curRegion->m_regionIndex);
    m_nextRegion = &m_curRegion[1];

    m_data = m_parent.m_songData +
             (m_parent.m_bigEndian ? SBig(m_parent.m_regionIdx[regionIdx]) : m_parent.m_regionIdx[regionIdx]);

    Header header = *reinterpret_cast<const Header*>(m_data);
    if (m_parent.m_bigEndian)
        header.swapBig();
    m_data += 12;

    if (header.m_pitchOff)
        m_pitchWheelData = m_parent.m_songData + header.m_pitchOff;
    if (header.m_modOff)
        m_modWheelData = m_parent.m_songData + header.m_modOff;

    m_eventWaitCountdown = 0;
    m_lastPitchTick = m_parent.m_curTick;
    m_lastPitchVal = 0;
    m_lastModTick = m_parent.m_curTick;
    m_lastModVal = 0;
    if (seq)
    {
        seq->setPitchWheel(m_midiChan, clamp(-1.f, m_lastPitchVal / 32768.f, 1.f));
        seq->setCtrlValue(m_midiChan, 1, clamp(0, m_lastModVal * 128 / 16384, 127));
    }
    if (m_parent.m_sngVersion == 1)
        m_eventWaitCountdown = int32_t(DecodeTimeRLE(m_data));
    else
    {
        int32_t absTick = (m_parent.m_bigEndian ? SBig(*reinterpret_cast<const int32_t*>(m_data))
                                                : *reinterpret_cast<const int32_t*>(m_data));
        m_eventWaitCountdown = absTick;
        m_lastN64EventTick = absTick;
        m_data += 4;
    }
}

void SongState::Track::advanceRegion(Sequencer* seq) { setRegion(seq, m_nextRegion); }

int SongState::DetectVersion(const unsigned char* ptr, bool& isBig)
{
    isBig = ptr[0] == 0;
    Header header = *reinterpret_cast<const Header*>(ptr);
    if (isBig)
        header.swapBig();
    const uint32_t* trackIdx = reinterpret_cast<const uint32_t*>(ptr + header.m_trackIdxOff);
    const uint32_t* regionIdxTable = reinterpret_cast<const uint32_t*>(ptr + header.m_regionIdxOff);

    /* First determine maximum index of MIDI regions across all tracks */
    uint32_t maxRegionIdx = 0;
    for (int i = 0; i < 64; ++i)
    {
        if (trackIdx[i])
        {
            const TrackRegion* region = nullptr;
            const TrackRegion* nextRegion =
                reinterpret_cast<const TrackRegion*>(ptr + (isBig ? SBig(trackIdx[i]) : trackIdx[i]));

            /* Iterate all regions */
            while (nextRegion->indexValid(isBig))
            {
                region = nextRegion;
                uint32_t regionIdx = (isBig ? SBig(region->m_regionIndex) : region->m_regionIndex);
                maxRegionIdx = std::max(maxRegionIdx, regionIdx);
                nextRegion = &region[1];
            }
        }
    }

    /* Perform 2 trials, first assuming revised format (more likely) */
    int v = 1;
    for (; v >= 0; --v)
    {
        bool bad = false;

        /* Validate all tracks */
        for (int i = 0; i < 64; ++i)
        {
            if (trackIdx[i])
            {
                const TrackRegion* region = nullptr;
                const TrackRegion* nextRegion =
                    reinterpret_cast<const TrackRegion*>(ptr + (isBig ? SBig(trackIdx[i]) : trackIdx[i]));

                /* Iterate all regions */
                while (nextRegion->indexValid(isBig))
                {
                    region = nextRegion;
                    uint32_t regionIdx = (isBig ? SBig(region->m_regionIndex) : region->m_regionIndex);
                    nextRegion = &region[1];

                    const unsigned char* data =
                        ptr + (isBig ? SBig(regionIdxTable[regionIdx]) : regionIdxTable[regionIdx]);

                    /* Can't reliably validate final region */
                    if (regionIdx == maxRegionIdx)
                        continue;

                    /* Expected end pointer (next region) */
                    const unsigned char* expectedEnd =
                        ptr + (isBig ? SBig(regionIdxTable[regionIdx + 1]) : regionIdxTable[regionIdx + 1]);

                    Track::Header header = *reinterpret_cast<const Track::Header*>(data);
                    if (isBig)
                        header.swapBig();
                    data += 12;

                    /* continuous pitch data */
                    if (header.m_pitchOff)
                    {
                        const unsigned char* dptr = ptr + header.m_pitchOff;
                        while (DecodeRLE(dptr) != 0xffffffff)
                        {
                            DecodeContinuousRLE(dptr);
                        }
                        if (dptr >= (expectedEnd - 4) && (dptr <= expectedEnd))
                            continue;
                    }

                    /* continuous modulation data */
                    if (header.m_modOff)
                    {
                        const unsigned char* dptr = ptr + header.m_modOff;
                        while (DecodeRLE(dptr) != 0xffffffff)
                        {
                            DecodeContinuousRLE(dptr);
                        }
                        if (dptr >= (expectedEnd - 4) && (dptr <= expectedEnd))
                            continue;
                    }

                    /* Loop through as many commands as we can for this time period */
                    if (v == 1)
                    {
                        /* Revised */
                        while (true)
                        {
                            /* Delta time */
                            DecodeTimeRLE(data);

                            /* Load next command */
                            if (*reinterpret_cast<const uint16_t*>(data) == 0xffff)
                            {
                                /* End of channel */
                                data += 2;
                                break;
                            }
                            else if (data[0] & 0x80 && data[1] & 0x80)
                            {
                                /* Control change */
                                data += 2;
                            }
                            else if (data[0] & 0x80)
                            {
                                /* Program change */
                                data += 2;
                            }
                            else
                            {
                                /* Note */
                                data += 4;
                            }
                        }
                    }
                    else
                    {
                        /* Legacy */
                        while (true)
                        {
                            /* Delta-time */
                            data += 4;

                            /* Load next command */
                            if (*reinterpret_cast<const uint16_t*>(&data[2]) == 0xffff)
                            {
                                /* End of channel */
                                data += 4;
                                break;
                            }
                            else
                            {
                                if ((data[2] & 0x80) != 0x80)
                                {
                                    /* Note */
                                }
                                else if (data[2] & 0x80 && data[3] & 0x80)
                                {
                                    /* Control change */
                                }
                                else if (data[2] & 0x80)
                                {
                                    /* Program change */
                                }
                                data += 4;
                            }
                        }
                    }

                    if (data < (expectedEnd - 4) || (data > expectedEnd))
                    {
                        bad = true;
                        break;
                    }
                }
                if (bad)
                    break;
            }
        }
        if (bad)
            continue;
        break;
    }

    return v;
}

bool SongState::initialize(const unsigned char* ptr)
{
    m_sngVersion = DetectVersion(ptr, m_bigEndian);
    if (m_sngVersion < 0)
        return false;

    m_songData = ptr;
    m_header = *reinterpret_cast<const Header*>(ptr);
    if (m_bigEndian)
        m_header.swapBig();
    const uint32_t* trackIdx = reinterpret_cast<const uint32_t*>(ptr + m_header.m_trackIdxOff);
    m_regionIdx = reinterpret_cast<const uint32_t*>(ptr + m_header.m_regionIdxOff);
    const uint8_t* chanMap = reinterpret_cast<const uint8_t*>(ptr + m_header.m_chanMapOff);

    /* Initialize all tracks */
    for (int i = 0; i < 64; ++i)
    {
        if (trackIdx[i])
        {
            const TrackRegion* region =
                reinterpret_cast<const TrackRegion*>(ptr + (m_bigEndian ? SBig(trackIdx[i]) : trackIdx[i]));
            m_tracks[i].emplace(*this, chanMap[i], region);
        }
        else
            m_tracks[i] = std::experimental::nullopt;
    }

    /* Initialize tempo */
    if (m_header.m_tempoTableOff)
        m_tempoPtr = reinterpret_cast<const TempoChange*>(ptr + m_header.m_tempoTableOff);
    else
        m_tempoPtr = nullptr;

    m_tempo = m_header.m_initialTempo & 0x7fffffff;
    m_curTick = 0;
    m_songState = SongPlayState::Playing;

    return true;
}

bool SongState::Track::advance(Sequencer& seq, int32_t ticks)
{
    int32_t endTick = m_parent.m_curTick + ticks;

    /* Advance region if needed */
    while (m_nextRegion->indexValid(m_parent.m_bigEndian))
    {
        uint32_t nextRegTick = (m_parent.m_bigEndian ? SBig(m_nextRegion->m_startTick) : m_nextRegion->m_startTick);
        if (uint32_t(endTick) > nextRegTick)
            advanceRegion(&seq);
        else
            break;
    }

    /* Stop finished notes */
    for (int i = 0; i < 128; ++i)
    {
        if (m_remNoteLengths[i] != INT_MIN)
        {
            m_remNoteLengths[i] -= ticks;
            if (m_remNoteLengths[i] <= 0)
            {
                seq.keyOff(m_midiChan, i, 0);
                m_remNoteLengths[i] = INT_MIN;
            }
        }
    }

    if (!m_data)
        return !m_nextRegion->indexValid(m_parent.m_bigEndian);

    /* Update continuous pitch data */
    if (m_pitchWheelData)
    {
        int32_t pitchTick = m_parent.m_curTick;
        int32_t remPitchTicks = ticks;
        while (pitchTick < endTick)
        {
            /* See if there's an upcoming pitch change in this interval */
            const unsigned char* ptr = m_pitchWheelData;
            uint32_t deltaTicks = DecodeRLE(ptr);
            if (deltaTicks != 0xffffffff)
            {
                int32_t nextTick = m_lastPitchTick + deltaTicks;
                if (pitchTick + remPitchTicks > nextTick)
                {
                    /* Update pitch */
                    int32_t pitchDelta = DecodeContinuousRLE(ptr);
                    m_lastPitchVal += pitchDelta;
                    m_pitchWheelData = ptr;
                    m_lastPitchTick = nextTick;
                    remPitchTicks -= (nextTick - pitchTick);
                    pitchTick = nextTick;
                    seq.setPitchWheel(m_midiChan, clamp(-1.f, m_lastPitchVal / 32768.f, 1.f));
                    continue;
                }
                remPitchTicks -= (nextTick - pitchTick);
                pitchTick = nextTick;
            }
            else
                break;
        }
    }

    /* Update continuous modulation data */
    if (m_modWheelData)
    {
        int32_t modTick = m_parent.m_curTick;
        int32_t remModTicks = ticks;
        while (modTick < endTick)
        {
            /* See if there's an upcoming modulation change in this interval */
            const unsigned char* ptr = m_modWheelData;
            uint32_t deltaTicks = DecodeRLE(ptr);
            if (deltaTicks != 0xffffffff)
            {
                int32_t nextTick = m_lastModTick + deltaTicks;
                if (modTick + remModTicks > nextTick)
                {
                    /* Update modulation */
                    int32_t modDelta = DecodeContinuousRLE(ptr);
                    m_lastModVal += modDelta;
                    m_modWheelData = ptr;
                    m_lastModTick = nextTick;
                    remModTicks -= (nextTick - modTick);
                    modTick = nextTick;
                    seq.setCtrlValue(m_midiChan, 1, clamp(0, m_lastModVal * 128 / 16384, 127));
                    continue;
                }
                remModTicks -= (nextTick - modTick);
                modTick = nextTick;
            }
            else
                break;
        }
    }

    /* Loop through as many commands as we can for this time period */
    if (m_parent.m_sngVersion == 1)
    {
        /* Revision */
        while (true)
        {
            /* Advance wait timer if active, returning if waiting */
            if (m_eventWaitCountdown)
            {
                m_eventWaitCountdown -= ticks;
                ticks = 0;
                if (m_eventWaitCountdown > 0)
                    return false;
            }

            /* Load next command */
            if (*reinterpret_cast<const uint16_t*>(m_data) == 0xffff)
            {
                /* End of channel */
                m_data = nullptr;
                return !m_nextRegion->indexValid(m_parent.m_bigEndian);
            }
            else if (m_data[0] & 0x80 && m_data[1] & 0x80)
            {
                /* Control change */
                uint8_t val = m_data[0] & 0x7f;
                uint8_t ctrl = m_data[1] & 0x7f;
                seq.setCtrlValue(m_midiChan, ctrl, val);
                m_data += 2;
            }
            else if (m_data[0] & 0x80)
            {
                /* Program change */
                uint8_t prog = m_data[0] & 0x7f;
                seq.setChanProgram(m_midiChan, prog);
                m_data += 2;
            }
            else
            {
                /* Note */
                uint8_t note = m_data[0] & 0x7f;
                uint8_t vel = m_data[1] & 0x7f;
                uint16_t length = (m_parent.m_bigEndian ? SBig(*reinterpret_cast<const uint16_t*>(m_data + 2))
                                                        : *reinterpret_cast<const uint16_t*>(m_data + 2));
                seq.keyOn(m_midiChan, note, vel);
                m_remNoteLengths[note] = length;
                m_data += 4;
            }

            /* Set next delta-time */
            m_eventWaitCountdown += int32_t(DecodeTimeRLE(m_data));
        }
    }
    else
    {
        /* Legacy */
        while (true)
        {
            /* Advance wait timer if active, returning if waiting */
            if (m_eventWaitCountdown)
            {
                m_eventWaitCountdown -= ticks;
                ticks = 0;
                if (m_eventWaitCountdown > 0)
                    return false;
            }

            /* Load next command */
            if (*reinterpret_cast<const uint16_t*>(&m_data[2]) == 0xffff)
            {
                /* End of channel */
                m_data = nullptr;
                return !m_nextRegion->indexValid(m_parent.m_bigEndian);
            }
            else
            {
                if ((m_data[2] & 0x80) != 0x80)
                {
                    /* Note */
                    uint16_t length = (m_parent.m_bigEndian ? SBig(*reinterpret_cast<const uint16_t*>(m_data))
                                                            : *reinterpret_cast<const uint16_t*>(m_data));
                    uint8_t note = m_data[2] & 0x7f;
                    uint8_t vel = m_data[3] & 0x7f;
                    seq.keyOn(m_midiChan, note, vel);
                    m_remNoteLengths[note] = length;
                }
                else if (m_data[2] & 0x80 && m_data[3] & 0x80)
                {
                    /* Control change */
                    uint8_t val = m_data[2] & 0x7f;
                    uint8_t ctrl = m_data[3] & 0x7f;
                    seq.setCtrlValue(m_midiChan, ctrl, val);
                }
                else if (m_data[2] & 0x80)
                {
                    /* Program change */
                    uint8_t prog = m_data[2] & 0x7f;
                    seq.setChanProgram(m_midiChan, prog);
                }
                m_data += 4;
            }

            /* Set next delta-time */
            int32_t absTick = (m_parent.m_bigEndian ? SBig(*reinterpret_cast<const int32_t*>(m_data))
                                                    : *reinterpret_cast<const int32_t*>(m_data));
            m_eventWaitCountdown += absTick - m_lastN64EventTick;
            m_lastN64EventTick = absTick;
            m_data += 4;
        }
    }

    return false;
}

bool SongState::advance(Sequencer& seq, double dt)
{
    /* Stopped */
    if (m_songState == SongPlayState::Stopped)
        return true;

    bool done = false;
    m_curDt += dt;
    while (m_curDt > 0.0)
    {
        done = true;

        /* Compute ticks to compute based on current tempo */
        double ticksPerSecond = m_tempo * 384 / 60;
        int32_t remTicks = std::ceil(m_curDt * ticksPerSecond);
        if (!remTicks)
            break;

        /* See if there's an upcoming tempo change in this interval */
        if (m_tempoPtr && m_tempoPtr->m_tick != 0xffffffff)
        {
            TempoChange change = *m_tempoPtr;
            if (m_bigEndian)
                change.swapBig();

            if (m_curTick + remTicks > change.m_tick)
                remTicks = change.m_tick - m_curTick;

            if (remTicks <= 0)
            {
                /* Turn over tempo */
                m_tempo = change.m_tempo & 0x7fffffff;
                seq.setTempo(m_tempo * 384 / 60);
                ++m_tempoPtr;
                continue;
            }
        }

        /* Advance all tracks */
        for (std::experimental::optional<Track>& trk : m_tracks)
            if (trk)
                done &= trk->advance(seq, remTicks);

        m_curTick += remTicks;

        if (m_tempo == 0)
            m_curDt = 0.0;
        else
            m_curDt -= remTicks / ticksPerSecond;
    }

    if (done)
        m_songState = SongPlayState::Stopped;
    return done;
}
}
