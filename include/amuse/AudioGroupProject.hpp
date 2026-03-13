#pragma once

#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

#include "amuse/Common.hpp"

#include <athena/DNA.hpp>

namespace amuse {
class AudioGroupData;
class AudioGroupPool;
class AudioGroupSampleDirectory;

enum class GroupType : uint16_t { Song, SFX };

/** Header at top of project file */
template <std::endian DNAEn>
struct AT_SPECIALIZE_PARMS(std::endian::big, std::endian::little) GroupHeader : BigDNA {
  AT_DECL_DNA
  Value<uint32_t, DNAEn> groupEndOff;
  GroupIdDNA<DNAEn> groupId;
  Value<GroupType, DNAEn> type;
  Value<uint32_t, DNAEn> soundMacroIdsOff;
  Value<uint32_t, DNAEn> samplIdsOff;
  Value<uint32_t, DNAEn> tableIdsOff;
  Value<uint32_t, DNAEn> keymapIdsOff;
  Value<uint32_t, DNAEn> layerIdsOff;
  Value<uint32_t, DNAEn> pageTableOff;
  Value<uint32_t, DNAEn> drumTableOff;
  Value<uint32_t, DNAEn> midiSetupsOff;
};

/** Common index members of SongGroups and SFXGroups */
struct AudioGroupIndex {};

/** Root index of SongGroup */
struct SongGroupIndex : AudioGroupIndex {
  /** Maps GM program numbers to sound entities */
  template <std::endian DNAEn>
  struct AT_SPECIALIZE_PARMS(std::endian::big, std::endian::little) PageEntryDNA : BigDNA {
    AT_DECL_DNA_YAML
    PageObjectIdDNA<DNAEn> objId;
    Value<uint8_t> priority;
    Value<uint8_t> maxVoices;
    Value<uint8_t> programNo;
    Seek<1, athena::SeekOrigin::Current> pad;
  };
  template <std::endian DNAEn>
  struct AT_SPECIALIZE_PARMS(std::endian::big, std::endian::little) MusyX1PageEntryDNA : BigDNA {
    AT_DECL_DNA
    PageObjectIdDNA<DNAEn> objId;
    Value<uint8_t> priority;
    Value<uint8_t> maxVoices;
    Value<uint8_t> unk;
    Value<uint8_t> programNo;
    Seek<2, athena::SeekOrigin::Current> pad;
  };
  struct PageEntry : BigDNA {
    AT_DECL_DNA_YAML
    PageObjectIdDNA<std::endian::big> objId;
    Value<uint8_t> priority = 0;
    Value<uint8_t> maxVoices = 255;

    PageEntry() = default;

    template <std::endian DNAE>
    PageEntry(const PageEntryDNA<DNAE>& in) : objId(in.objId.id), priority(in.priority), maxVoices(in.maxVoices) {}

    template <std::endian DNAE>
    PageEntry(const MusyX1PageEntryDNA<DNAE>& in)
    : objId(in.objId.id), priority(in.priority), maxVoices(in.maxVoices) {}

    template <std::endian DNAEn>
    PageEntryDNA<DNAEn> toDNA(uint8_t programNo) const {
      PageEntryDNA<DNAEn> ret;
      ret.objId = objId;
      ret.priority = priority;
      ret.maxVoices = maxVoices;
      ret.programNo = programNo;
      return ret;
    }
  };
  std::unordered_map<uint8_t, PageEntry> m_normPages;
  std::unordered_map<uint8_t, PageEntry> m_drumPages;

  /** Maps SongID to 16 MIDI channel numbers to GM program numbers and settings */
  struct MusyX1MIDISetup : BigDNA {
    AT_DECL_DNA_YAML
    Value<uint8_t> programNo;
    Value<uint8_t> volume;
    Value<uint8_t> panning;
    Value<uint8_t> reverb;
    Value<uint8_t> chorus;
    Seek<3, athena::SeekOrigin::Current> pad;
  };
  struct MIDISetup : BigDNA {
    AT_DECL_DNA_YAML
    Value<uint8_t> programNo = 0;
    Value<uint8_t> volume = 127;
    Value<uint8_t> panning = 64;
    Value<uint8_t> reverb = 0;
    Value<uint8_t> chorus = 0;
    MIDISetup() = default;
    MIDISetup(const MusyX1MIDISetup& setup)
    : programNo(setup.programNo)
    , volume(setup.volume)
    , panning(setup.panning)
    , reverb(setup.reverb)
    , chorus(setup.chorus) {}
  };
  std::unordered_map<SongId, std::array<MIDISetup, 16>> m_midiSetups;

  void toYAML(athena::io::YAMLDocWriter& w) const;
  void fromYAML(athena::io::YAMLDocReader& r);
};

/** Root index of SFXGroup */
struct SFXGroupIndex : AudioGroupIndex {
  /** Maps game-side SFX define IDs to sound entities */
  template <std::endian DNAEn>
  struct AT_SPECIALIZE_PARMS(std::endian::big, std::endian::little) SFXEntryDNA : BigDNA {
    AT_DECL_DNA
    SFXIdDNA<DNAEn> sfxId;
    PageObjectIdDNA<DNAEn> objId;
    Value<uint8_t> priority;
    Value<uint8_t> maxVoices;
    Value<uint8_t> defVel;
    Value<uint8_t> panning;
    Value<uint8_t> defKey;
    Seek<1, athena::SeekOrigin::Current> pad;
  };
  struct SFXEntry : BigDNA {
    AT_DECL_DNA_YAML
    PageObjectIdDNA<std::endian::big> objId;
    Value<uint8_t> priority = 0;
    Value<uint8_t> maxVoices = 255;
    Value<uint8_t> defVel = 127;
    Value<uint8_t> panning = 64;
    Value<uint8_t> defKey = 60;

    SFXEntry() = default;

    template <std::endian DNAE>
    SFXEntry(const SFXEntryDNA<DNAE>& in)
    : objId(in.objId.id)
    , priority(in.priority)
    , maxVoices(in.maxVoices)
    , defVel(in.defVel)
    , panning(in.panning)
    , defKey(in.defKey) {}

    template <std::endian DNAEn>
    SFXEntryDNA<DNAEn> toDNA(SFXId id) const {
      SFXEntryDNA<DNAEn> ret;
      ret.sfxId.id = id;
      ret.objId = objId;
      ret.priority = priority;
      ret.maxVoices = maxVoices;
      ret.defVel = defVel;
      ret.panning = panning;
      ret.defKey = defKey;
      return ret;
    }
  };
  std::unordered_map<SFXId, SFXEntry> m_sfxEntries;

  SFXGroupIndex() = default;
  SFXGroupIndex(const SFXGroupIndex& other);

  void toYAML(athena::io::YAMLDocWriter& w) const;
  void fromYAML(athena::io::YAMLDocReader& r);
};

/** Collection of SongGroup and SFXGroup indexes */
class AudioGroupProject {
  std::unordered_map<GroupId, ObjToken<SongGroupIndex>> m_songGroups;
  std::unordered_map<GroupId, ObjToken<SFXGroupIndex>> m_sfxGroups;

  AudioGroupProject(athena::io::IStreamReader& r, GCNDataTag);
  template <std::endian DNAE>
  static AudioGroupProject _AudioGroupProject(athena::io::IStreamReader& r, bool absOffs);

  static void BootstrapObjectIDs(athena::io::IStreamReader& r, GCNDataTag);
  template <std::endian DNAE>
  static void BootstrapObjectIDs(athena::io::IStreamReader& r, bool absOffs);

public:
  AudioGroupProject() = default;
  static AudioGroupProject CreateAudioGroupProject(const AudioGroupData& data);
  static AudioGroupProject CreateAudioGroupProject(std::string_view groupPath);
  static AudioGroupProject CreateAudioGroupProject(const AudioGroupProject& oldProj);
  static void BootstrapObjectIDs(const AudioGroupData& data);

  const SongGroupIndex* getSongGroupIndex(GroupId groupId) const;
  const SFXGroupIndex* getSFXGroupIndex(GroupId groupId) const;

  const std::unordered_map<GroupId, ObjToken<SongGroupIndex>>& songGroups() const { return m_songGroups; }
  const std::unordered_map<GroupId, ObjToken<SFXGroupIndex>>& sfxGroups() const { return m_sfxGroups; }
  std::unordered_map<GroupId, ObjToken<SongGroupIndex>>& songGroups() { return m_songGroups; }
  std::unordered_map<GroupId, ObjToken<SFXGroupIndex>>& sfxGroups() { return m_sfxGroups; }

  std::vector<uint8_t> toYAML() const;
  std::vector<uint8_t> toGCNData(const AudioGroupPool& pool, const AudioGroupSampleDirectory& sdir) const;

  AudioGroupProject(const AudioGroupProject&) = delete;
  AudioGroupProject& operator=(const AudioGroupProject&) = delete;
  AudioGroupProject(AudioGroupProject&&) = default;
  AudioGroupProject& operator=(AudioGroupProject&&) = default;
};
} // namespace amuse
