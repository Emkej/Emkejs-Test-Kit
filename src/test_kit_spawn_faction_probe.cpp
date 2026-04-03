#include "test_kit_spawn_faction_probe.h"

#include "test_kit_internal.h"

#include <kenshi/Faction.h>
#include <kenshi/GameDataManager.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>

#include <sstream>

namespace test_kit
{
namespace
{
const bool kEnableSpawnTemplateNaturalFactionProbe = false;
const size_t kFactionProbeMaxMatchesPerField = 3u;
const char* const kFactionProbeRootSlots[] = {
    "FACTION",
    "OWNER FACTION",
    "SQUAD",
    "LEADER",
    "TOWN",
    "LAW",
    "BOUNTY FACTIONS"
};
const char* const kFactionProbeChildSlots[] = {
    "FACTION",
    "OWNER FACTION"
};

std::string NormalizeProbeSlotName(const std::string& value)
{
    return ToUpperAscii(TrimAscii(value));
}

bool DoesProbeSlotMatchExact(
    const std::string& slotName,
    const char* const* candidates,
    size_t candidateCount)
{
    const std::string normalizedSlotName = NormalizeProbeSlotName(slotName);
    for (size_t index = 0; index < candidateCount; ++index)
    {
        if (normalizedSlotName == candidates[index])
        {
            return true;
        }
    }

    return false;
}

bool IsFactionProbeRootSlot(const std::string& slotName)
{
    return DoesProbeSlotMatchExact(
        slotName,
        kFactionProbeRootSlots,
        sizeof(kFactionProbeRootSlots) / sizeof(kFactionProbeRootSlots[0]));
}

bool IsFactionProbeChildSlot(const std::string& slotName)
{
    return DoesProbeSlotMatchExact(
        slotName,
        kFactionProbeChildSlots,
        sizeof(kFactionProbeChildSlots) / sizeof(kFactionProbeChildSlots[0]));
}

bool IsFactionProbeSquadSlot(const std::string& slotName)
{
    return NormalizeProbeSlotName(slotName) == "SQUAD";
}

bool DoesProbeTextContainTraversalTerm(const std::string& value)
{
    return IsFactionProbeRootSlot(value) || IsFactionProbeChildSlot(value);
}

bool DoesProbeKeyMatchFaction(const std::string& key)
{
    return DoesProbeTextContainTraversalTerm(key);
}

GameData* TryGetFactionDataSafe(Faction* faction)
{
    if (!faction)
    {
        return 0;
    }

    __try
    {
        return faction->getData();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

GameDataContainer* TryGetSourceContainerSafe(GameData* templateData)
{
    if (!templateData)
    {
        return 0;
    }

    __try
    {
        return templateData->getSourceContainer();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

std::string BuildGameDataIdentity(GameData* data)
{
    if (!data)
    {
        return "";
    }

    const std::string name = TrimAscii(data->name);
    const std::string stringId = TrimAscii(data->stringID);
    if (!stringId.empty() && !name.empty())
    {
        return stringId + "|" + name;
    }

    if (!stringId.empty())
    {
        return stringId;
    }

    if (!name.empty())
    {
        return name;
    }

    std::stringstream fallback;
    fallback << "id:" << data->id;
    return fallback.str();
}

std::string BuildReferenceIdentity(const GameDataReference& reference, GameData* resolvedData)
{
    if (resolvedData)
    {
        return BuildGameDataIdentity(resolvedData);
    }

    if (!reference.sid.empty())
    {
        return TrimAscii(reference.sid);
    }

    return "<unresolved>";
}

GameData* TryResolveReferenceTargetSafe(const GameDataReference& reference, GameDataContainer* sourceContainer)
{
    __try
    {
        if (reference.ptr)
        {
            return reference.ptr;
        }

        if (sourceContainer && !reference.sid.empty())
        {
            return sourceContainer->getData(reference.sid);
        }

        return 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

void AppendFactionProbeFields(std::stringstream& line, const char* prefix, Faction* faction)
{
    const std::string fieldPrefix = prefix ? prefix : "faction";
    std::string factionName = "Unknown";
    std::string factionStringId;
    GameData* factionData = TryGetFactionDataSafe(faction);

    if (faction && IsProbablyReadableEnginePointer(faction))
    {
        factionName = SafeFactionName(faction);
        if (factionData)
        {
            factionStringId = TrimAscii(factionData->stringID);
        }
    }
    else if (faction)
    {
        factionName = "Invalid";
    }

    line << " " << fieldPrefix << "_ptr=" << FormatPointerValue(faction)
         << " " << fieldPrefix << "_name=\"" << SanitizeLogValue(factionName) << "\""
         << " " << fieldPrefix << "_sid=\"" << SanitizeLogValue(factionStringId) << "\""
         << " " << fieldPrefix << "_data_ptr=" << FormatPointerValue(factionData);
}

void AppendStringMapFactionMatches(
    std::stringstream& line,
    const char* prefix,
    const boost::unordered::unordered_map<
        std::string,
        std::string,
        boost::hash<std::string>,
        std::equal_to<std::string>,
        Ogre::STLAllocator<std::pair<std::string const, std::string>, Ogre::GeneralAllocPolicy> >& map)
{
    size_t matchCount = 0u;
    size_t loggedMatches = 0u;
    std::stringstream matches;
    for (boost::unordered::unordered_map<
             std::string,
             std::string,
             boost::hash<std::string>,
             std::equal_to<std::string>,
             Ogre::STLAllocator<std::pair<std::string const, std::string>, Ogre::GeneralAllocPolicy> >::const_iterator it = map.begin();
         it != map.end();
         ++it)
    {
        if (!DoesProbeKeyMatchFaction(it->first))
        {
            continue;
        }

        ++matchCount;
        if (loggedMatches >= kFactionProbeMaxMatchesPerField)
        {
            continue;
        }

        if (loggedMatches > 0u)
        {
            matches << "|";
        }

        matches << SanitizeLogValue(it->first) << "=" << SanitizeLogValue(it->second);
        ++loggedMatches;
    }

    line << " " << prefix << "_count=" << matchCount
         << " " << prefix << "_samples=\"" << matches.str() << "\"";
}

void AppendIntMapFactionMatches(
    std::stringstream& line,
    const char* prefix,
    const boost::unordered::unordered_map<
        std::string,
        int,
        boost::hash<std::string>,
        std::equal_to<std::string>,
        Ogre::STLAllocator<std::pair<std::string const, int>, Ogre::GeneralAllocPolicy> >& map)
{
    size_t matchCount = 0u;
    size_t loggedMatches = 0u;
    std::stringstream matches;
    for (boost::unordered::unordered_map<
             std::string,
             int,
             boost::hash<std::string>,
             std::equal_to<std::string>,
             Ogre::STLAllocator<std::pair<std::string const, int>, Ogre::GeneralAllocPolicy> >::const_iterator it = map.begin();
         it != map.end();
         ++it)
    {
        if (!DoesProbeKeyMatchFaction(it->first))
        {
            continue;
        }

        ++matchCount;
        if (loggedMatches >= kFactionProbeMaxMatchesPerField)
        {
            continue;
        }

        if (loggedMatches > 0u)
        {
            matches << "|";
        }

        matches << SanitizeLogValue(it->first) << "=" << it->second;
        ++loggedMatches;
    }

    line << " " << prefix << "_count=" << matchCount
         << " " << prefix << "_samples=\"" << matches.str() << "\"";
}

void AppendObjectReferenceFactionMatches(
    std::stringstream& line,
    const char* prefix,
    const boost::unordered::unordered_map<
        std::string,
        Ogre::vector<GameDataReference>::type,
        boost::hash<std::string>,
        std::equal_to<std::string>,
        Ogre::STLAllocator<
            std::pair<std::string const, Ogre::vector<GameDataReference>::type>,
            Ogre::GeneralAllocPolicy> >& map)
{
    size_t matchCount = 0u;
    size_t loggedMatches = 0u;
    std::stringstream matches;
    for (boost::unordered::unordered_map<
             std::string,
             Ogre::vector<GameDataReference>::type,
             boost::hash<std::string>,
             std::equal_to<std::string>,
             Ogre::STLAllocator<
                 std::pair<std::string const, Ogre::vector<GameDataReference>::type>,
                 Ogre::GeneralAllocPolicy> >::const_iterator it = map.begin();
         it != map.end();
         ++it)
    {
        if (!DoesProbeKeyMatchFaction(it->first))
        {
            continue;
        }

        ++matchCount;
        if (loggedMatches >= kFactionProbeMaxMatchesPerField)
        {
            continue;
        }

        if (loggedMatches > 0u)
        {
            matches << "|";
        }

        matches << SanitizeLogValue(it->first) << "#"
                << it->second.size();
        if (!it->second.empty())
        {
            matches << ":" << SanitizeLogValue(it->second[0].sid);
        }
        ++loggedMatches;
    }

    line << " " << prefix << "_count=" << matchCount
         << " " << prefix << "_samples=\"" << matches.str() << "\"";
}

Faction* TryGetFactionByStringIdSafe(const std::string& factionStringId)
{
    if (!ou || !ou->factionMgr || factionStringId.empty())
    {
        return 0;
    }

    __try
    {
        return ou->factionMgr->getFactionByStringID(factionStringId);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

Faction* TryGetFactionByNameSafe(const std::string& factionName)
{
    if (!ou || !ou->factionMgr || factionName.empty())
    {
        return 0;
    }

    __try
    {
        return ou->factionMgr->getFactionByName(factionName);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

Faction* TryGetFactionBySquadSafe(GameData* templateData)
{
    if (!ou || !ou->factionMgr || !templateData)
    {
        return 0;
    }

    __try
    {
        return ou->factionMgr->getFactionBySquad(templateData);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

Faction* TryGetFactionForDataIdentity(GameData* data)
{
    if (!data)
    {
        return 0;
    }

    const std::string stringId = TrimAscii(data->stringID);
    if (!stringId.empty())
    {
        Faction* faction = TryGetFactionByStringIdSafe(stringId);
        if (faction)
        {
            return faction;
        }
    }

    const std::string name = TrimAscii(data->name);
    if (!name.empty())
    {
        return TryGetFactionByNameSafe(name);
    }

    return 0;
}

std::string GetSourceContainerNameSafe(GameData* templateData)
{
    GameDataContainer* sourceContainer = TryGetSourceContainerSafe(templateData);
    if (!sourceContainer)
    {
        return "";
    }
    return TrimAscii(sourceContainer->name);
}

void AppendBoolMapHintMatches(
    std::stringstream& line,
    const char* prefix,
    const boost::unordered::unordered_map<
        std::string,
        bool,
        boost::hash<std::string>,
        std::equal_to<std::string>,
        Ogre::STLAllocator<std::pair<std::string const, bool>, Ogre::GeneralAllocPolicy> >& map)
{
    size_t matchCount = 0u;
    size_t loggedMatches = 0u;
    std::stringstream matches;
    for (boost::unordered::unordered_map<
             std::string,
             bool,
             boost::hash<std::string>,
             std::equal_to<std::string>,
             Ogre::STLAllocator<std::pair<std::string const, bool>, Ogre::GeneralAllocPolicy> >::const_iterator it = map.begin();
         it != map.end();
         ++it)
    {
        if (!DoesProbeTextContainTraversalTerm(it->first))
        {
            continue;
        }

        ++matchCount;
        if (loggedMatches >= kFactionProbeMaxMatchesPerField)
        {
            continue;
        }

        if (loggedMatches > 0u)
        {
            matches << "|";
        }

        matches << SanitizeLogValue(it->first) << "=" << (it->second ? "true" : "false");
        ++loggedMatches;
    }

    line << " " << prefix << "_count=" << matchCount
         << " " << prefix << "_samples=\"" << matches.str() << "\"";
}

std::string BuildSelectedOutgoingReferenceSummary(
    GameData* data,
    bool rootNode)
{
    if (!data)
    {
        return "";
    }

    GameDataContainer* sourceContainer = TryGetSourceContainerSafe(data);
    std::stringstream summary;
    bool firstEntry = true;
    for (boost::unordered::unordered_map<
             std::string,
             Ogre::vector<GameDataReference>::type,
             boost::hash<std::string>,
             std::equal_to<std::string>,
             Ogre::STLAllocator<
                 std::pair<std::string const, Ogre::vector<GameDataReference>::type>,
                 Ogre::GeneralAllocPolicy> >::const_iterator slotIt = data->objectReferences.begin();
         slotIt != data->objectReferences.end();
         ++slotIt)
    {
        const bool slotSelected = rootNode
            ? IsFactionProbeRootSlot(slotIt->first)
            : IsFactionProbeChildSlot(slotIt->first);
        if (!slotSelected)
        {
            continue;
        }

        for (size_t refIndex = 0u; refIndex < slotIt->second.size(); ++refIndex)
        {
            if (!firstEntry)
            {
                summary << "|";
            }

            GameData* resolvedData = TryResolveReferenceTargetSafe(slotIt->second[refIndex], sourceContainer);
            summary << SanitizeLogValue(slotIt->first)
                    << "[" << refIndex << "]="
                    << SanitizeLogValue(BuildReferenceIdentity(slotIt->second[refIndex], resolvedData));
            firstEntry = false;
        }
    }

    return summary.str();
}

bool HasVisitedData(const std::vector<GameData*>& visitedDatas, GameData* data)
{
    for (size_t index = 0u; index < visitedDatas.size(); ++index)
    {
        if (visitedDatas[index] == data)
        {
            return true;
        }
    }

    return false;
}

void AppendUniqueString(std::vector<std::string>* values, const std::string& value)
{
    if (!values || value.empty())
    {
        return;
    }

    for (size_t index = 0u; index < values->size(); ++index)
    {
        if ((*values)[index] == value)
        {
            return;
        }
    }

    values->push_back(value);
}

std::string JoinValues(const std::vector<std::string>& values)
{
    std::stringstream joined;
    for (size_t index = 0u; index < values.size(); ++index)
    {
        if (index > 0u)
        {
            joined << "|";
        }
        joined << SanitizeLogValue(values[index]);
    }
    return joined.str();
}

void LogTemplateWalkNode(
    const std::string& templateName,
    const std::string& path,
    int depth,
    GameData* node,
    bool rootNode)
{
    if (!node)
    {
        return;
    }

    Faction* candidateFaction = TryGetFactionForDataIdentity(node);

    std::stringstream line;
    line << "[investigate][spawn][faction_probe_node]"
         << " template_name=\"" << SanitizeLogValue(templateName) << "\""
         << " depth=" << depth
         << " path=\"" << SanitizeLogValue(path) << "\""
         << " node_ptr=" << FormatPointerValue(node)
         << " node_id=" << node->id
         << " node_type=" << node->type
         << " node_name=\"" << SanitizeLogValue(TrimAscii(node->name)) << "\""
         << " node_sid=\"" << SanitizeLogValue(TrimAscii(node->stringID)) << "\""
         << " node_source_container_name=\"" << SanitizeLogValue(GetSourceContainerNameSafe(node)) << "\""
         << " node_identity=\"" << SanitizeLogValue(BuildGameDataIdentity(node)) << "\""
         << " node_selected_refs=\"" << BuildSelectedOutgoingReferenceSummary(node, rootNode) << "\"";
    AppendFactionProbeFields(line, "node_identity_faction_candidate", candidateFaction);
    LogInfoLine(line.str());
}

void CollectFactionProbeCandidate(
    GameData* node,
    std::vector<std::string>* walkedNodeIdentities,
    std::vector<std::string>* walkedFactionCandidates)
{
    if (!node)
    {
        return;
    }

    const std::string nodeIdentity = BuildGameDataIdentity(node);
    AppendUniqueString(walkedNodeIdentities, nodeIdentity);

    Faction* candidateFaction = TryGetFactionForDataIdentity(node);
    if (candidateFaction)
    {
        GameData* factionData = TryGetFactionDataSafe(candidateFaction);
        AppendUniqueString(walkedFactionCandidates, BuildGameDataIdentity(factionData));
        AppendUniqueString(walkedFactionCandidates, SafeFactionName(candidateFaction));
    }
}

void WalkTemplateFactionGraph(
    GameData* templateData,
    const std::string& templateName,
    bool emitLogs,
    std::vector<std::string>* walkedNodeIdentities,
    std::vector<std::string>* walkedFactionCandidates)
{
    if (!templateData)
    {
        return;
    }

    if (emitLogs)
    {
        LogTemplateWalkNode(templateName, "template", 0, templateData, true);
    }

    CollectFactionProbeCandidate(templateData, walkedNodeIdentities, walkedFactionCandidates);

    GameDataContainer* sourceContainer = TryGetSourceContainerSafe(templateData);
    for (boost::unordered::unordered_map<
             std::string,
             Ogre::vector<GameDataReference>::type,
             boost::hash<std::string>,
             std::equal_to<std::string>,
             Ogre::STLAllocator<
                 std::pair<std::string const, Ogre::vector<GameDataReference>::type>,
                 Ogre::GeneralAllocPolicy> >::const_iterator slotIt = templateData->objectReferences.begin();
         slotIt != templateData->objectReferences.end();
         ++slotIt)
    {
        if (!IsFactionProbeRootSlot(slotIt->first))
        {
            continue;
        }

        for (size_t refIndex = 0u; refIndex < slotIt->second.size(); ++refIndex)
        {
            const GameDataReference& reference = slotIt->second[refIndex];
            GameData* childNode = TryResolveReferenceTargetSafe(reference, sourceContainer);
            const std::string childIdentity = BuildReferenceIdentity(reference, childNode);

            std::stringstream childPath;
            childPath << "template"
                      << " -> " << slotIt->first
                      << "[" << refIndex << "]"
                      << " -> " << childIdentity;

            if (emitLogs)
            {
                std::stringstream chainLine;
                chainLine << "[investigate][spawn][faction_probe_chain]"
                          << " template_name=\"" << SanitizeLogValue(templateName) << "\""
                          << " depth=1"
                          << " path=\"" << SanitizeLogValue(childPath.str()) << "\"";
                LogInfoLine(chainLine.str());
            }

            if (!childNode)
            {
                AppendUniqueString(walkedNodeIdentities, childIdentity);
                continue;
            }

            if (emitLogs)
            {
                LogTemplateWalkNode(templateName, childPath.str(), 1, childNode, false);
            }
            CollectFactionProbeCandidate(childNode, walkedNodeIdentities, walkedFactionCandidates);

            if (!IsFactionProbeSquadSlot(slotIt->first))
            {
                continue;
            }

            GameDataContainer* childSourceContainer = TryGetSourceContainerSafe(childNode);
            for (boost::unordered::unordered_map<
                     std::string,
                     Ogre::vector<GameDataReference>::type,
                     boost::hash<std::string>,
                     std::equal_to<std::string>,
                     Ogre::STLAllocator<
                         std::pair<std::string const, Ogre::vector<GameDataReference>::type>,
                         Ogre::GeneralAllocPolicy> >::const_iterator childSlotIt = childNode->objectReferences.begin();
                 childSlotIt != childNode->objectReferences.end();
                 ++childSlotIt)
            {
                if (!IsFactionProbeChildSlot(childSlotIt->first))
                {
                    continue;
                }

                for (size_t childRefIndex = 0u; childRefIndex < childSlotIt->second.size(); ++childRefIndex)
                {
                    const GameDataReference& childReference = childSlotIt->second[childRefIndex];
                    GameData* grandchildNode = TryResolveReferenceTargetSafe(childReference, childSourceContainer);
                    const std::string grandchildIdentity = BuildReferenceIdentity(childReference, grandchildNode);

                    std::stringstream grandchildPath;
                    grandchildPath << childPath.str()
                                   << " -> " << childSlotIt->first
                                   << "[" << childRefIndex << "]"
                                   << " -> " << grandchildIdentity;

                    if (emitLogs)
                    {
                        std::stringstream chainLine;
                        chainLine << "[investigate][spawn][faction_probe_chain]"
                                  << " template_name=\"" << SanitizeLogValue(templateName) << "\""
                                  << " depth=2"
                                  << " path=\"" << SanitizeLogValue(grandchildPath.str()) << "\"";
                        LogInfoLine(chainLine.str());
                    }

                    if (!grandchildNode)
                    {
                        AppendUniqueString(walkedNodeIdentities, grandchildIdentity);
                        continue;
                    }

                    if (emitLogs)
                    {
                        LogTemplateWalkNode(templateName, grandchildPath.str(), 2, grandchildNode, false);
                    }
                    CollectFactionProbeCandidate(grandchildNode, walkedNodeIdentities, walkedFactionCandidates);
                }
            }
        }
    }
}

void CollectTemplateFactionWalkSummary(
    GameData* templateData,
    const std::string& templateName,
    bool emitLogs,
    std::vector<std::string>* walkedNodeIdentities,
    std::vector<std::string>* walkedFactionCandidates)
{
    if (!templateData)
    {
        return;
    }

    WalkTemplateFactionGraph(
        templateData,
        templateName,
        emitLogs,
        walkedNodeIdentities,
        walkedFactionCandidates);
}
} // namespace

bool ShouldUseSpawnTemplateNaturalFactionProbe()
{
    return g_developerMode && kEnableSpawnTemplateNaturalFactionProbe;
}

void LogSpawnTemplateFactionProbe(GameData* templateData, const std::string& templateName, Character* target)
{
    if (!g_developerMode || !templateData)
    {
        return;
    }

    Faction* targetFaction = 0;
    TryResolveCharacterFaction(target, &targetFaction);

    const std::string templateDataName = TrimAscii(templateData->name);
    const std::string templateStringId = TrimAscii(templateData->stringID);
    const std::string sourceContainerName = GetSourceContainerNameSafe(templateData);
    Faction* byTemplateStringId = TryGetFactionByStringIdSafe(templateStringId);
    Faction* byTemplateName = TryGetFactionByNameSafe(templateDataName);
    Faction* byTemplateSquad = TryGetFactionBySquadSafe(templateData);
    std::vector<std::string> walkedNodeIdentities;
    std::vector<std::string> walkedFactionCandidates;
    CollectTemplateFactionWalkSummary(
        templateData,
        templateName,
        true,
        &walkedNodeIdentities,
        &walkedFactionCandidates);

    std::stringstream line;
    line << "[investigate][spawn][faction_probe]"
         << " template_name=\"" << SanitizeLogValue(templateName) << "\""
         << " template_ptr=" << FormatPointerValue(templateData)
         << " template_id=" << templateData->id
         << " template_type=" << templateData->type
         << " template_name_raw=\"" << SanitizeLogValue(templateDataName) << "\""
         << " template_sid=\"" << SanitizeLogValue(templateStringId) << "\""
         << " template_source_container_ptr=" << FormatPointerValue(TryGetSourceContainerSafe(templateData))
         << " template_source_container_name=\"" << SanitizeLogValue(sourceContainerName) << "\""
         << " template_identity=\"" << SanitizeLogValue(BuildGameDataIdentity(templateData)) << "\""
         << " template_root_selected_refs=\"" << BuildSelectedOutgoingReferenceSummary(templateData, true) << "\""
         << " target_name=\"" << SanitizeLogValue(SafeCharacterName(target)) << "\"";
    AppendFactionProbeFields(line, "target_faction", targetFaction);
    AppendFactionProbeFields(line, "candidate_by_template_sid", byTemplateStringId);
    AppendFactionProbeFields(line, "candidate_by_template_name", byTemplateName);
    AppendFactionProbeFields(line, "candidate_by_template_squad", byTemplateSquad);
    line << " walked_node_identities=\"" << JoinValues(walkedNodeIdentities) << "\""
         << " walked_faction_candidates=\"" << JoinValues(walkedFactionCandidates) << "\"";
    LogInfoLine(line.str());
}

void LogSpawnTemplateFactionProbeComparison(
    GameData* templateData,
    const std::string& templateName,
    Character* spawnedCharacter,
    const char* phase)
{
    if (!g_developerMode || !templateData || !spawnedCharacter)
    {
        return;
    }

    Faction* spawnedFaction = 0;
    TryResolveCharacterFaction(spawnedCharacter, &spawnedFaction);
    GameData* spawnedFactionData = TryGetFactionDataSafe(spawnedFaction);
    const std::string spawnedFactionName = SafeFactionName(spawnedFaction);
    const std::string spawnedFactionIdentity = BuildGameDataIdentity(spawnedFactionData);

    std::vector<std::string> walkedNodeIdentities;
    std::vector<std::string> walkedFactionCandidates;
    CollectTemplateFactionWalkSummary(
        templateData,
        templateName,
        false,
        &walkedNodeIdentities,
        &walkedFactionCandidates);

    bool walkedNodeSidMatch = false;
    bool walkedNodeNameMatch = false;
    bool walkedFactionCandidateMatch = false;
    for (size_t index = 0u; index < walkedNodeIdentities.size(); ++index)
    {
        if (!spawnedFactionIdentity.empty() && walkedNodeIdentities[index].find(spawnedFactionIdentity) != std::string::npos)
        {
            walkedNodeSidMatch = true;
        }
        if (!spawnedFactionName.empty() && walkedNodeIdentities[index].find(spawnedFactionName) != std::string::npos)
        {
            walkedNodeNameMatch = true;
        }
    }

    for (size_t index = 0u; index < walkedFactionCandidates.size(); ++index)
    {
        if ((!spawnedFactionIdentity.empty() && walkedFactionCandidates[index].find(spawnedFactionIdentity) != std::string::npos)
            || (!spawnedFactionName.empty() && walkedFactionCandidates[index].find(spawnedFactionName) != std::string::npos))
        {
            walkedFactionCandidateMatch = true;
            break;
        }
    }

    std::stringstream line;
    line << "[investigate][spawn][faction_probe_compare]"
         << " template_name=\"" << SanitizeLogValue(templateName) << "\""
         << " phase=\"" << SanitizeLogValue(phase ? phase : "unknown") << "\""
         << " spawned_name=\"" << SanitizeLogValue(SafeCharacterName(spawnedCharacter)) << "\"";
    AppendFactionProbeFields(line, "spawned_faction", spawnedFaction);
    line << " spawned_faction_identity=\"" << SanitizeLogValue(spawnedFactionIdentity) << "\""
         << " walked_node_identities=\"" << JoinValues(walkedNodeIdentities) << "\""
         << " walked_faction_candidates=\"" << JoinValues(walkedFactionCandidates) << "\""
         << " walked_node_sid_match=" << (walkedNodeSidMatch ? "true" : "false")
         << " walked_node_name_match=" << (walkedNodeNameMatch ? "true" : "false")
         << " walked_faction_candidate_match=" << (walkedFactionCandidateMatch ? "true" : "false");
    LogInfoLine(line.str());
}
}
