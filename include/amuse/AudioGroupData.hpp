#ifndef __AMUSE_AUDIOGROUPDATA_HPP__
#define __AMUSE_AUDIOGROUPDATA_HPP__

namespace amuse
{

/**
 * @brief Simple pointer-container of the four Audio Group chunks
 */
class AudioGroupData
{
protected:
    unsigned char* m_pool;
    unsigned char* m_proj;
    unsigned char* m_sdir;
    unsigned char* m_samp;
public:
    AudioGroupData(unsigned char* pool, unsigned char* proj,
                   unsigned char* sdir, unsigned char* samp)
    : m_pool(pool), m_proj(proj), m_sdir(sdir), m_samp(samp) {}

    const unsigned char* getPool() const {return m_pool;}
    const unsigned char* getProj() const {return m_proj;}
    const unsigned char* getSdir() const {return m_sdir;}
    const unsigned char* getSamp() const {return m_samp;}
};

/**
 * @brief A buffer-owning version of AudioGroupData
 */
class IntrusiveAudioGroupData : public AudioGroupData
{
public:
    using AudioGroupData::AudioGroupData;
    ~IntrusiveAudioGroupData();
};

}

#endif // __AMUSE_AUDIOGROUPDATA_HPP__