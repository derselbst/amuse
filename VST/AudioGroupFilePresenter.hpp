#ifndef __AMUSE_AUDIOGROUPFILEPRESENTER_HPP__
#define __AMUSE_AUDIOGROUPFILEPRESENTER_HPP__

#include <map>
#include <memory>
#include "optional.hpp"
#include <amuse/amuse.hpp>
#include <athena/FileReader.hpp>

namespace amuse
{
class VSTBackend;
class VSTEditor;
class AudioGroupFilePresenter;

struct AudioGroupDataCollection
{
    std::wstring m_path;
    std::wstring m_name;

    std::vector<uint8_t> m_projData;
    std::vector<uint8_t> m_poolData;
    std::vector<uint8_t> m_sdirData;
    std::vector<uint8_t> m_sampData;

    struct MetaData
    {
        amuse::DataFormat fmt;
        uint32_t absOffs;
        uint32_t active;
        MetaData(amuse::DataFormat fmtIn, uint32_t absOffsIn, uint32_t activeIn)
        : fmt(fmtIn), absOffs(absOffsIn), active(activeIn) {}
        MetaData(athena::io::FileReader& r)
        : fmt(amuse::DataFormat(r.readUint32Little())), absOffs(r.readUint32Little()), active(r.readUint32Little()) {}
    };
    std::experimental::optional<MetaData> m_metaData;

    std::experimental::optional<amuse::AudioGroupData> m_loadedData;
    const amuse::AudioGroup* m_loadedGroup;
    struct GroupToken
    {
        int m_groupId;
        const amuse::SongGroupIndex* m_song = nullptr;
        const amuse::SFXGroupIndex* m_sfx = nullptr;
        GroupToken(int id, const amuse::SongGroupIndex* song) : m_groupId(id), m_song(song) {}
        GroupToken(int id, const amuse::SFXGroupIndex* sfx) : m_groupId(id), m_sfx(sfx) {}
    };
    std::vector<GroupToken> m_groupTokens;

    bool loadProj();
    bool loadPool();
    bool loadSdir();
    bool loadSamp();
    bool loadMeta();

    AudioGroupDataCollection(const std::wstring& path, const std::wstring& name);
    bool isDataComplete() const {return m_projData.size() && m_poolData.size() && m_sdirData.size() && m_sampData.size() && m_metaData;}
    bool _attemptLoad(AudioGroupFilePresenter& presenter);
    bool _indexData(AudioGroupFilePresenter& presenter);
};

struct AudioGroupCollection
{
    std::wstring m_path;
    std::wstring m_name;

    std::map<std::wstring, std::unique_ptr<AudioGroupDataCollection>> m_groups;

    AudioGroupCollection(const std::wstring& path, const std::wstring& name);
    void addCollection(AudioGroupFilePresenter& presenter,
                       std::vector<std::pair<std::wstring, amuse::IntrusiveAudioGroupData>>&& collection);
    void update(AudioGroupFilePresenter& presenter);

    //void populate
};

class AudioGroupFilePresenter
{
    VSTBackend& m_backend;
    std::map<std::wstring, std::unique_ptr<AudioGroupCollection>> m_audioGroupCollections;
public:
    AudioGroupFilePresenter(VSTBackend& backend) : m_backend(backend) {}
    void update();
    void populateEditor(VSTEditor& editor);
    void addCollection(const std::wstring& name,
                       std::vector<std::pair<std::wstring, amuse::IntrusiveAudioGroupData>>&& collection);
    VSTBackend& getBackend() {return m_backend;}
};

}

#endif // __AMUSE_AUDIOGROUPFILEPRESENTER_HPP__