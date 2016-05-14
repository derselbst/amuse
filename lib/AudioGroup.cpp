#include "amuse/AudioGroup.hpp"
#include "amuse/AudioGroupData.hpp"

namespace amuse
{

AudioGroup::AudioGroup(int groupId, const AudioGroupData& data)
: m_groupId(groupId),
  m_proj(data.getProj()),
  m_pool(data.getPool()),
  m_sdir(data.getSdir()),
  m_samp(data.getSamp())
{}

const Sample* AudioGroup::getSample(int sfxId) const
{
    for (const auto& ent : m_sdir.m_entries)
        if (ent.second.first.m_sfxId == sfxId)
            return &ent.second;
    return nullptr;
}

const unsigned char* AudioGroup::getSampleData(uint32_t offset) const
{
    return m_samp + offset;
}

}
