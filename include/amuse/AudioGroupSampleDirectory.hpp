#ifndef __AMUSE_AUDIOGROUPSAMPLEDIR_HPP__
#define __AMUSE_AUDIOGROUPSAMPLEDIR_HPP__

#include <unordered_map>
#include <stdint.h>

namespace amuse
{

/** Indexes individual samples in SAMP chunk */
class AudioGroupSampleDirectory
{
    friend class AudioGroup;
public:
    struct Entry
    {
        uint16_t m_sfxId;
        uint32_t m_sampleOff;
        uint32_t m_unk;
        uint8_t m_pitch;
        uint16_t m_sampleRate;
        uint32_t m_numSamples;
        uint32_t m_loopStartSample;
        uint32_t m_loopLengthSamples;
        uint32_t m_adpcmParmOffset;
        void swapBig();
    };
    struct ADPCMParms
    {
        uint16_t m_bytesPerFrame;
        uint8_t m_ps;
        uint8_t m_lps;
        int16_t m_hist1;
        int16_t m_hist2;
        int16_t m_coefs[8][2];
        void swapBig();
    };
private:
    std::unordered_map<uint16_t, std::pair<Entry, ADPCMParms>> m_entries;
public:
    AudioGroupSampleDirectory(const unsigned char* data);
};

}

#endif // __AMUSE_AUDIOGROUPSAMPLEDIR_HPP__