#include "test_kit_config.h"

#include "test_kit_panel.h"
#include "test_kit_teleport.h"

#include <emc/mod_hub_client.h>
#include <ois/OISKeyboard.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

namespace test_kit
{
std::string g_configPath;
bool g_pluginEnabled = true;
bool g_developerMode = false;
LoggingLevel g_loggingLevel = LoggingLevel_Info;
bool g_togglePanelRequireCtrl = true;
bool g_togglePanelRequireShift = true;
bool g_togglePanelRequireAlt = false;
std::string g_togglePanelKey = kDefaultTogglePanelKey;
bool g_hotkeyEnabled = true;
int g_hotkeyVirtualKey = 'D';
std::string g_hotkeyDisplay = "CTRL+SHIFT+D";
bool g_hotkeyPrevDown = false;
bool g_confirmDangerousActions = true;
bool g_panelHidden = false;
bool g_panelCollapsed = false;
int g_panelMinExpandedHeight = kPanelMinExpandedHeightDefault;
int g_panelMaxExpandedHeight = kPanelExpandedHeight;
int g_panelHeaderTitleFontHeight = kPanelHeaderTitleFontHeightDefault;
int g_panelCollapseButtonSize = kPanelCollapseButtonSizeDefault;
int g_panelCloseButtonSize = kPanelCloseButtonSizeDefault;
int g_panelBodyOverlap = kPanelBodyOverlapDefault;
std::vector<SavedLocation> g_savedLocations;

namespace
{
const char* kModHubNamespaceId = "emkej.qol";
const char* kModHubNamespaceDisplayName = "Emkej QoL";
const char* kModHubModId = "emkejs_test_kit";
const char* kModHubModDisplayName = "Emkejs Test Kit";
const char* kModHubTogglePanelKeyLabel = "Debug Panel Key";
const char* kModHubTogglePanelKeyDescription =
    "Primary key for showing or hiding the debug panel. Use the modifier toggles below for Ctrl, Shift, and Alt. Unbind to disable.";
const char* kModHubEnabledLabel = "Enabled";
const char* kModHubEnabledDescription =
    "Basic master switch for the mod. Disabling it keeps Mod Hub settings visible but turns off the panel and hotkey behavior.";
const char* kModHubTogglePanelCtrlLabel = "Require Ctrl";
const char* kModHubTogglePanelCtrlDescription = "Require Ctrl for the debug panel hotkey.";
const char* kModHubTogglePanelShiftLabel = "Require Shift";
const char* kModHubTogglePanelShiftDescription = "Require Shift for the debug panel hotkey.";
const char* kModHubTogglePanelAltLabel = "Require Alt";
const char* kModHubTogglePanelAltDescription = "Require Alt for the debug panel hotkey.";
const char* kModHubPanelMinHeightLabel = "Min Panel Height";
const char* kModHubPanelMinHeightDescription =
    "Minimum expanded height for the debug panel. Smaller active tabs still keep at least this height.";
const char* kModHubPanelWidthLabel = "Panel Width";
const char* kModHubPanelWidthDescription =
    "Overall width of the debug panel. Wider values give long labels, lists, and previews more room.";
const char* kModHubPanelMaxHeightLabel = "Max Panel Height";
const char* kModHubPanelMaxHeightDescription =
    "Maximum expanded height for the debug panel. Taller tab content scrolls inside the panel.";
const char* kModHubPanelHeaderTitleFontHeightLabel = "Header Title Size";
const char* kModHubPanelHeaderTitleFontHeightDescription =
    "Snaps the debug panel title to the nearest native Kenshi painted font size to keep it crisp.";
const char* kModHubPanelCollapseButtonSizeLabel = "Collapse Button Size";
const char* kModHubPanelCollapseButtonSizeDescription =
    "Square size for the header collapse button.";
const char* kModHubPanelCloseButtonSizeLabel = "Close Button Size";
const char* kModHubPanelCloseButtonSizeDescription =
    "Square size for the header close button.";
const char* kModHubPanelBodyOverlapLabel = "Header Body Overlap";
const char* kModHubPanelBodyOverlapDescription =
    "How many pixels the panel body tucks under the header to hide the seam.";

emc::ModHubClient g_modHubClient;
bool g_modHubClientConfigured = false;

bool TryResolveModConfigPath(std::string* outPath)
{
    if (!outPath || g_configPath.empty())
    {
        return false;
    }

    *outPath = g_configPath;
    return true;
}

bool TryReadTextFile(const std::string& path, std::string* outContent)
{
    if (!outContent)
    {
        return false;
    }

    std::ifstream input(path.c_str(), std::ios::in | std::ios::binary);
    if (!input)
    {
        return false;
    }

    std::stringstream buffer;
    buffer << input.rdbuf();
    if (!input.good() && !input.eof())
    {
        return false;
    }

    *outContent = buffer.str();
    return true;
}

bool TryWriteTextFile(const std::string& path, const std::string& content)
{
    std::ofstream output(path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!output)
    {
        return false;
    }

    output.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!output.good())
    {
        return false;
    }

    output.close();
    return output.good();
}

std::string EscapeJsonStringValue(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size() + 8);

    for (size_t index = 0; index < value.size(); ++index)
    {
        const char current = value[index];
        switch (current)
        {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped.push_back(current);
            break;
        }
    }

    return escaped;
}

bool TryParseJsonBoolByKey(const std::string& content, const char* key, bool* outValue)
{
    if (!key || !outValue)
    {
        return false;
    }

    const std::string needle = std::string("\"") + key + "\"";
    const std::string::size_type keyPos = content.find(needle);
    if (keyPos == std::string::npos)
    {
        return false;
    }

    std::string::size_type valuePos = content.find(':', keyPos + needle.size());
    if (valuePos == std::string::npos)
    {
        return false;
    }

    ++valuePos;
    while (valuePos < content.size()
        && std::isspace(static_cast<unsigned char>(content[valuePos])) != 0)
    {
        ++valuePos;
    }

    if (content.compare(valuePos, 4, "true") == 0)
    {
        *outValue = true;
        return true;
    }

    if (content.compare(valuePos, 5, "false") == 0)
    {
        *outValue = false;
        return true;
    }

    return false;
}

bool TryReplaceJsonBoolByKey(std::string* content, const char* key, bool value)
{
    if (!content || !key)
    {
        return false;
    }

    const std::string needle = std::string("\"") + key + "\"";
    const std::string::size_type keyPos = content->find(needle);
    if (keyPos == std::string::npos)
    {
        return false;
    }

    std::string::size_type valuePos = content->find(':', keyPos + needle.size());
    if (valuePos == std::string::npos)
    {
        return false;
    }

    ++valuePos;
    while (valuePos < content->size()
        && std::isspace(static_cast<unsigned char>((*content)[valuePos])) != 0)
    {
        ++valuePos;
    }

    const std::string replacement = value ? "true" : "false";
    if (content->compare(valuePos, 4, "true") == 0)
    {
        content->replace(valuePos, 4, replacement);
        return true;
    }

    if (content->compare(valuePos, 5, "false") == 0)
    {
        content->replace(valuePos, 5, replacement);
        return true;
    }

    return false;
}

bool TryReplaceJsonStringByKey(std::string* content, const char* key, const std::string& value)
{
    if (!content || !key)
    {
        return false;
    }

    const std::string needle = std::string("\"") + key + "\"";
    const std::string::size_type keyPos = content->find(needle);
    if (keyPos == std::string::npos)
    {
        return false;
    }

    std::string::size_type valuePos = content->find(':', keyPos + needle.size());
    if (valuePos == std::string::npos)
    {
        return false;
    }

    ++valuePos;
    while (valuePos < content->size()
        && std::isspace(static_cast<unsigned char>((*content)[valuePos])) != 0)
    {
        ++valuePos;
    }

    if (valuePos >= content->size() || (*content)[valuePos] != '"')
    {
        return false;
    }

    std::string::size_type endPos = valuePos + 1;
    while (endPos < content->size())
    {
        if ((*content)[endPos] == '"' && (*content)[endPos - 1] != '\\')
        {
            break;
        }

        ++endPos;
    }

    if (endPos >= content->size())
    {
        return false;
    }

    const std::string replacement = std::string("\"") + EscapeJsonStringValue(value) + "\"";
    content->replace(valuePos, endPos - valuePos + 1, replacement);
    return true;
}

bool TryParseJsonIntByKey(const std::string& content, const char* key, int* outValue)
{
    if (!key || !outValue)
    {
        return false;
    }

    const std::string needle = std::string("\"") + key + "\"";
    const std::string::size_type keyPos = content.find(needle);
    if (keyPos == std::string::npos)
    {
        return false;
    }

    std::string::size_type valuePos = content.find(':', keyPos + needle.size());
    if (valuePos == std::string::npos)
    {
        return false;
    }

    ++valuePos;
    while (valuePos < content.size()
        && std::isspace(static_cast<unsigned char>(content[valuePos])) != 0)
    {
        ++valuePos;
    }

    std::string::size_type endPos = valuePos;
    if (endPos < content.size() && content[endPos] == '-')
    {
        ++endPos;
    }
    while (endPos < content.size()
        && std::isdigit(static_cast<unsigned char>(content[endPos])) != 0)
    {
        ++endPos;
    }

    if (endPos == valuePos || (endPos == valuePos + 1 && content[valuePos] == '-'))
    {
        return false;
    }

    std::stringstream valueStream(content.substr(valuePos, endPos - valuePos));
    int parsedValue = 0;
    valueStream >> parsedValue;
    if (!valueStream || !valueStream.eof())
    {
        return false;
    }

    *outValue = parsedValue;
    return true;
}

bool TryReplaceJsonIntByKey(std::string* content, const char* key, int value)
{
    if (!content || !key)
    {
        return false;
    }

    const std::string needle = std::string("\"") + key + "\"";
    const std::string::size_type keyPos = content->find(needle);
    if (keyPos == std::string::npos)
    {
        return false;
    }

    std::string::size_type valuePos = content->find(':', keyPos + needle.size());
    if (valuePos == std::string::npos)
    {
        return false;
    }

    ++valuePos;
    while (valuePos < content->size()
        && std::isspace(static_cast<unsigned char>((*content)[valuePos])) != 0)
    {
        ++valuePos;
    }

    std::string::size_type endPos = valuePos;
    if (endPos < content->size() && (*content)[endPos] == '-')
    {
        ++endPos;
    }
    while (endPos < content->size()
        && std::isdigit(static_cast<unsigned char>((*content)[endPos])) != 0)
    {
        ++endPos;
    }

    if (endPos == valuePos || (endPos == valuePos + 1 && (*content)[valuePos] == '-'))
    {
        return false;
    }

    std::stringstream replacement;
    replacement << value;
    content->replace(valuePos, endPos - valuePos, replacement.str());
    return true;
}

bool TryInsertJsonIntByKey(std::string* content, const char* key, int value)
{
    if (!content || !key)
    {
        return false;
    }

    const std::string::size_type objectEnd = content->rfind('}');
    if (objectEnd == std::string::npos)
    {
        return false;
    }

    std::string::size_type insertPos = objectEnd;
    while (insertPos > 0
        && std::isspace(static_cast<unsigned char>((*content)[insertPos - 1])) != 0)
    {
        --insertPos;
    }

    std::string::size_type previousPos = insertPos;
    while (previousPos > 0
        && std::isspace(static_cast<unsigned char>((*content)[previousPos - 1])) != 0)
    {
        --previousPos;
    }

    const bool needsComma = previousPos > 0 && (*content)[previousPos - 1] != '{';
    std::stringstream insertion;
    if (needsComma)
    {
        insertion << ",";
    }
    insertion << "\n  \"" << key << "\": " << value;

    content->insert(insertPos, insertion.str());
    return true;
}

bool TryUpsertJsonIntByKey(std::string* content, const char* key, int value)
{
    return TryReplaceJsonIntByKey(content, key, value) || TryInsertJsonIntByKey(content, key, value);
}

bool TryParseJsonStringByKey(const std::string& content, const char* key, std::string* outValue)
{
    if (!key || !outValue)
    {
        return false;
    }

    const std::string needle = std::string("\"") + key + "\"";
    const std::string::size_type keyPos = content.find(needle);
    if (keyPos == std::string::npos)
    {
        return false;
    }

    std::string::size_type valuePos = content.find(':', keyPos + needle.size());
    if (valuePos == std::string::npos)
    {
        return false;
    }

    ++valuePos;
    while (valuePos < content.size()
        && std::isspace(static_cast<unsigned char>(content[valuePos])) != 0)
    {
        ++valuePos;
    }

    if (valuePos >= content.size() || content[valuePos] != '"')
    {
        return false;
    }

    ++valuePos;
    outValue->clear();

    while (valuePos < content.size())
    {
        char current = content[valuePos];
        if (current == '"')
        {
            return true;
        }

        if (current == '\\')
        {
            ++valuePos;
            if (valuePos >= content.size())
            {
                return false;
            }

            current = content[valuePos];
        }

        outValue->push_back(current);
        ++valuePos;
    }

    return false;
}

void SkipJsonWhitespace(const std::string& content, std::string::size_type* position)
{
    if (!position)
    {
        return;
    }

    while (*position < content.size()
        && std::isspace(static_cast<unsigned char>(content[*position])) != 0)
    {
        ++(*position);
    }
}

bool TryFindJsonStringEnd(
    const std::string& content,
    std::string::size_type openingQuotePos,
    std::string::size_type* closingQuotePosOut)
{
    if (openingQuotePos >= content.size() || content[openingQuotePos] != '"' || !closingQuotePosOut)
    {
        return false;
    }

    bool escaped = false;
    std::string::size_type position = openingQuotePos + 1;
    while (position < content.size())
    {
        const char current = content[position];
        if (escaped)
        {
            escaped = false;
        }
        else if (current == '\\')
        {
            escaped = true;
        }
        else if (current == '"')
        {
            *closingQuotePosOut = position;
            return true;
        }

        ++position;
    }

    return false;
}

bool TryFindMatchingJsonDelimiter(
    const std::string& content,
    std::string::size_type openingPos,
    char openingChar,
    char closingChar,
    std::string::size_type* closingPosOut)
{
    if (openingPos >= content.size() || content[openingPos] != openingChar || !closingPosOut)
    {
        return false;
    }

    int depth = 0;
    std::string::size_type position = openingPos;
    while (position < content.size())
    {
        const char current = content[position];
        if (current == '"')
        {
            std::string::size_type stringEnd = 0;
            if (!TryFindJsonStringEnd(content, position, &stringEnd))
            {
                return false;
            }

            position = stringEnd;
        }
        else if (current == openingChar)
        {
            ++depth;
        }
        else if (current == closingChar)
        {
            --depth;
            if (depth == 0)
            {
                *closingPosOut = position;
                return true;
            }

            if (depth < 0)
            {
                return false;
            }
        }

        ++position;
    }

    return false;
}

bool TryFindJsonValueStartByKey(
    const std::string& content,
    const char* key,
    std::string::size_type* valuePosOut)
{
    if (!key || !valuePosOut)
    {
        return false;
    }

    const std::string needle = std::string("\"") + key + "\"";
    const std::string::size_type keyPos = content.find(needle);
    if (keyPos == std::string::npos)
    {
        return false;
    }

    std::string::size_type valuePos = content.find(':', keyPos + needle.size());
    if (valuePos == std::string::npos)
    {
        return false;
    }

    ++valuePos;
    SkipJsonWhitespace(content, &valuePos);
    if (valuePos >= content.size())
    {
        return false;
    }

    *valuePosOut = valuePos;
    return true;
}

bool TryFindJsonValueEnd(
    const std::string& content,
    std::string::size_type valuePos,
    std::string::size_type* valueEndOut)
{
    if (valuePos >= content.size() || !valueEndOut)
    {
        return false;
    }

    const char valueLead = content[valuePos];
    if (valueLead == '"')
    {
        std::string::size_type stringEnd = 0;
        if (!TryFindJsonStringEnd(content, valuePos, &stringEnd))
        {
            return false;
        }

        *valueEndOut = stringEnd + 1;
        return true;
    }

    if (valueLead == '[')
    {
        std::string::size_type arrayEnd = 0;
        if (!TryFindMatchingJsonDelimiter(content, valuePos, '[', ']', &arrayEnd))
        {
            return false;
        }

        *valueEndOut = arrayEnd + 1;
        return true;
    }

    if (valueLead == '{')
    {
        std::string::size_type objectEnd = 0;
        if (!TryFindMatchingJsonDelimiter(content, valuePos, '{', '}', &objectEnd))
        {
            return false;
        }

        *valueEndOut = objectEnd + 1;
        return true;
    }

    std::string::size_type valueEnd = valuePos;
    while (valueEnd < content.size()
        && content[valueEnd] != ','
        && content[valueEnd] != '}'
        && content[valueEnd] != ']')
    {
        ++valueEnd;
    }

    while (valueEnd > valuePos
        && std::isspace(static_cast<unsigned char>(content[valueEnd - 1])) != 0)
    {
        --valueEnd;
    }

    if (valueEnd <= valuePos)
    {
        return false;
    }

    *valueEndOut = valueEnd;
    return true;
}

bool TryReplaceJsonRawValueByKey(std::string* content, const char* key, const std::string& rawValue)
{
    if (!content || !key)
    {
        return false;
    }

    std::string::size_type valuePos = 0;
    if (!TryFindJsonValueStartByKey(*content, key, &valuePos))
    {
        return false;
    }

    std::string::size_type valueEnd = 0;
    if (!TryFindJsonValueEnd(*content, valuePos, &valueEnd))
    {
        return false;
    }

    content->replace(valuePos, valueEnd - valuePos, rawValue);
    return true;
}

bool TryInsertJsonRawValueByKey(std::string* content, const char* key, const std::string& rawValue)
{
    if (!content || !key)
    {
        return false;
    }

    const std::string::size_type objectEnd = content->rfind('}');
    if (objectEnd == std::string::npos)
    {
        return false;
    }

    std::string::size_type insertPos = objectEnd;
    while (insertPos > 0
        && std::isspace(static_cast<unsigned char>((*content)[insertPos - 1])) != 0)
    {
        --insertPos;
    }

    std::string::size_type previousPos = insertPos;
    while (previousPos > 0
        && std::isspace(static_cast<unsigned char>((*content)[previousPos - 1])) != 0)
    {
        --previousPos;
    }

    const bool needsComma = previousPos > 0 && (*content)[previousPos - 1] != '{';
    std::stringstream insertion;
    if (needsComma)
    {
        insertion << ",";
    }
    insertion << "\n  \"" << key << "\": " << rawValue;

    content->insert(insertPos, insertion.str());
    return true;
}

bool TryUpsertJsonRawValueByKey(std::string* content, const char* key, const std::string& rawValue)
{
    return TryReplaceJsonRawValueByKey(content, key, rawValue) || TryInsertJsonRawValueByKey(content, key, rawValue);
}

bool TryParseJsonFloatByKey(const std::string& content, const char* key, float* outValue)
{
    if (!key || !outValue)
    {
        return false;
    }

    std::string::size_type valuePos = 0;
    if (!TryFindJsonValueStartByKey(content, key, &valuePos))
    {
        return false;
    }

    std::string::size_type endPos = valuePos;
    bool sawDigit = false;

    if (endPos < content.size() && (content[endPos] == '-' || content[endPos] == '+'))
    {
        ++endPos;
    }

    while (endPos < content.size() && std::isdigit(static_cast<unsigned char>(content[endPos])) != 0)
    {
        sawDigit = true;
        ++endPos;
    }

    if (endPos < content.size() && content[endPos] == '.')
    {
        ++endPos;
        while (endPos < content.size() && std::isdigit(static_cast<unsigned char>(content[endPos])) != 0)
        {
            sawDigit = true;
            ++endPos;
        }
    }

    if (endPos < content.size() && (content[endPos] == 'e' || content[endPos] == 'E'))
    {
        std::string::size_type exponentPos = endPos + 1;
        if (exponentPos < content.size() && (content[exponentPos] == '-' || content[exponentPos] == '+'))
        {
            ++exponentPos;
        }

        bool exponentDigit = false;
        while (exponentPos < content.size() && std::isdigit(static_cast<unsigned char>(content[exponentPos])) != 0)
        {
            exponentDigit = true;
            ++exponentPos;
        }

        if (!exponentDigit)
        {
            return false;
        }

        endPos = exponentPos;
    }

    if (!sawDigit)
    {
        return false;
    }

    std::stringstream valueStream(content.substr(valuePos, endPos - valuePos));
    double parsedValue = 0.0;
    valueStream >> parsedValue;
    if (!valueStream || !valueStream.eof())
    {
        return false;
    }

    *outValue = static_cast<float>(parsedValue);
    return true;
}

bool TryParseJsonUInt64ByKey(const std::string& content, const char* key, unsigned long long* outValue)
{
    if (!key || !outValue)
    {
        return false;
    }

    std::string::size_type valuePos = 0;
    if (!TryFindJsonValueStartByKey(content, key, &valuePos))
    {
        return false;
    }

    std::string::size_type endPos = valuePos;
    while (endPos < content.size() && std::isdigit(static_cast<unsigned char>(content[endPos])) != 0)
    {
        ++endPos;
    }

    if (endPos == valuePos)
    {
        return false;
    }

    std::stringstream valueStream(content.substr(valuePos, endPos - valuePos));
    unsigned long long parsedValue = 0u;
    valueStream >> parsedValue;
    if (!valueStream || !valueStream.eof())
    {
        return false;
    }

    *outValue = parsedValue;
    return true;
}

bool TryParseSavedLocationObject(const std::string& content, SavedLocation* outLocation)
{
    if (!outLocation)
    {
        return false;
    }

    SavedLocation parsedLocation;
    if (!TryParseJsonStringByKey(content, "id", &parsedLocation.id)
        || !TryParseJsonStringByKey(content, "name", &parsedLocation.name)
        || !TryParseJsonFloatByKey(content, "x", &parsedLocation.position.x)
        || !TryParseJsonFloatByKey(content, "y", &parsedLocation.position.y)
        || !TryParseJsonFloatByKey(content, "z", &parsedLocation.position.z))
    {
        return false;
    }

    parsedLocation.id = TrimAscii(parsedLocation.id);
    parsedLocation.name = TrimAscii(parsedLocation.name);
    if (parsedLocation.id.empty() || parsedLocation.name.empty())
    {
        return false;
    }

    bool parsedBool = false;
    if (TryParseJsonBoolByKey(content, "pinned", &parsedBool))
    {
        parsedLocation.pinned = parsedBool;
    }

    unsigned long long parsedLastUsedUtc = 0u;
    if (TryParseJsonUInt64ByKey(content, "last_used_utc", &parsedLastUsedUtc))
    {
        parsedLocation.lastUsedUtc = parsedLastUsedUtc;
    }

    *outLocation = parsedLocation;
    return true;
}

bool CompareSavedLocationsForDisplay(const SavedLocation& left, const SavedLocation& right)
{
    if (left.pinned != right.pinned)
    {
        return left.pinned;
    }

    const std::string leftNameUpper = ToUpperAscii(left.name);
    const std::string rightNameUpper = ToUpperAscii(right.name);
    if (leftNameUpper != rightNameUpper)
    {
        return leftNameUpper < rightNameUpper;
    }

    return left.id < right.id;
}

std::string BuildSavedLocationsJsonValue(const std::vector<SavedLocation>& locations)
{
    if (locations.empty())
    {
        return "[]";
    }

    std::stringstream value;
    value << "[";

    for (size_t index = 0; index < locations.size(); ++index)
    {
        const SavedLocation& location = locations[index];
        if (index == 0u)
        {
            value << "\n";
        }

        value << "    {\n"
              << "      \"id\": \"" << EscapeJsonStringValue(location.id) << "\",\n"
              << "      \"name\": \"" << EscapeJsonStringValue(location.name) << "\",\n"
              << "      \"x\": " << location.position.x << ",\n"
              << "      \"y\": " << location.position.y << ",\n"
              << "      \"z\": " << location.position.z << ",\n"
              << "      \"pinned\": " << (location.pinned ? "true" : "false") << ",\n"
              << "      \"last_used_utc\": " << location.lastUsedUtc << "\n"
              << "    }";

        if (index + 1u < locations.size())
        {
            value << ",";
        }
        value << "\n";
    }

    value << "  ]";
    return value.str();
}

bool TryParseSavedLocationsByKey(
    const std::string& content,
    const char* key,
    std::vector<SavedLocation>* outLocations)
{
    if (!key || !outLocations)
    {
        return false;
    }

    std::string::size_type arrayPos = 0;
    if (!TryFindJsonValueStartByKey(content, key, &arrayPos))
    {
        return false;
    }

    if (arrayPos >= content.size() || content[arrayPos] != '[')
    {
        return false;
    }

    std::string::size_type arrayEnd = 0;
    if (!TryFindMatchingJsonDelimiter(content, arrayPos, '[', ']', &arrayEnd))
    {
        return false;
    }

    outLocations->clear();
    std::string::size_type position = arrayPos + 1;
    size_t skippedCount = 0u;
    while (position < arrayEnd)
    {
        SkipJsonWhitespace(content, &position);
        if (position >= arrayEnd)
        {
            break;
        }

        if (content[position] == ',')
        {
            ++position;
            continue;
        }

        if (content[position] != '{')
        {
            return false;
        }

        std::string::size_type objectEnd = 0;
        if (!TryFindMatchingJsonDelimiter(content, position, '{', '}', &objectEnd))
        {
            return false;
        }

        SavedLocation parsedLocation;
        if (TryParseSavedLocationObject(content.substr(position, objectEnd - position + 1), &parsedLocation))
        {
            outLocations->push_back(parsedLocation);
        }
        else
        {
            ++skippedCount;
        }

        position = objectEnd + 1;
    }

    SortSavedLocationsForDisplay(outLocations);

    if (skippedCount > 0u)
    {
        std::stringstream line;
        line << "saved locations skipped invalid_entries=" << skippedCount;
        LogWarnLine(line.str());
    }

    return true;
}

void CopyModHubErrorMessage(char* errBuf, uint32_t errBufSize, const char* message)
{
    if (!errBuf || errBufSize == 0u)
    {
        return;
    }

    errBuf[0] = '\0';
    if (!message)
    {
        return;
    }

    const size_t maxCopyLength = static_cast<size_t>(errBufSize - 1u);
    std::strncpy(errBuf, message, maxCopyLength);
    errBuf[maxCopyLength] = '\0';
}

bool TryParsePrimaryKeyToken(const std::string& tokenValue, int* virtualKeyOut, std::string* canonicalTokenOut)
{
    if (!virtualKeyOut || !canonicalTokenOut)
    {
        return false;
    }

    const std::string tokenUpper = ToUpperAscii(TrimAscii(tokenValue));
    if (tokenUpper.empty())
    {
        return false;
    }

    if (tokenUpper.size() == 1)
    {
        const char ch = tokenUpper[0];
        if ((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9'))
        {
            *virtualKeyOut = static_cast<int>(ch);
            canonicalTokenOut->assign(1, ch);
            return true;
        }
    }

    if (tokenUpper == "SPACE")
    {
        *virtualKeyOut = VK_SPACE;
        *canonicalTokenOut = "SPACE";
        return true;
    }

    if (tokenUpper == "TAB")
    {
        *virtualKeyOut = VK_TAB;
        *canonicalTokenOut = "TAB";
        return true;
    }

    if (tokenUpper == "ENTER" || tokenUpper == "RETURN")
    {
        *virtualKeyOut = VK_RETURN;
        *canonicalTokenOut = "ENTER";
        return true;
    }

    if (tokenUpper == "ESC" || tokenUpper == "ESCAPE")
    {
        *virtualKeyOut = VK_ESCAPE;
        *canonicalTokenOut = "ESC";
        return true;
    }

    if (tokenUpper == "BACKSPACE")
    {
        *virtualKeyOut = VK_BACK;
        *canonicalTokenOut = "BACKSPACE";
        return true;
    }

    if (tokenUpper == "DELETE")
    {
        *virtualKeyOut = VK_DELETE;
        *canonicalTokenOut = "DELETE";
        return true;
    }

    if (tokenUpper == "INSERT")
    {
        *virtualKeyOut = VK_INSERT;
        *canonicalTokenOut = "INSERT";
        return true;
    }

    if (tokenUpper == "HOME")
    {
        *virtualKeyOut = VK_HOME;
        *canonicalTokenOut = "HOME";
        return true;
    }

    if (tokenUpper == "END")
    {
        *virtualKeyOut = VK_END;
        *canonicalTokenOut = "END";
        return true;
    }

    if (tokenUpper == "PAGEUP" || tokenUpper == "PGUP")
    {
        *virtualKeyOut = VK_PRIOR;
        *canonicalTokenOut = "PAGEUP";
        return true;
    }

    if (tokenUpper == "PAGEDOWN" || tokenUpper == "PGDN")
    {
        *virtualKeyOut = VK_NEXT;
        *canonicalTokenOut = "PAGEDOWN";
        return true;
    }

    if (tokenUpper == "UP")
    {
        *virtualKeyOut = VK_UP;
        *canonicalTokenOut = "UP";
        return true;
    }

    if (tokenUpper == "DOWN")
    {
        *virtualKeyOut = VK_DOWN;
        *canonicalTokenOut = "DOWN";
        return true;
    }

    if (tokenUpper == "LEFT")
    {
        *virtualKeyOut = VK_LEFT;
        *canonicalTokenOut = "LEFT";
        return true;
    }

    if (tokenUpper == "RIGHT")
    {
        *virtualKeyOut = VK_RIGHT;
        *canonicalTokenOut = "RIGHT";
        return true;
    }

    if (tokenUpper.size() >= 2 && tokenUpper[0] == 'F')
    {
        int functionIndex = 0;
        for (size_t index = 1; index < tokenUpper.size(); ++index)
        {
            const unsigned char ch = static_cast<unsigned char>(tokenUpper[index]);
            if (std::isdigit(ch) == 0)
            {
                return false;
            }

            functionIndex = (functionIndex * 10) + (tokenUpper[index] - '0');
        }

        if (functionIndex >= 1 && functionIndex <= 24)
        {
            *virtualKeyOut = VK_F1 + (functionIndex - 1);

            std::stringstream label;
            label << "F" << functionIndex;
            *canonicalTokenOut = label.str();
            return true;
        }
    }

    return false;
}

bool TryMapTogglePanelTokenToOisKeycode(const std::string& tokenValue, int32_t* outKeycode)
{
    if (!outKeycode)
    {
        return false;
    }

    const std::string tokenUpper = ToUpperAscii(TrimAscii(tokenValue));
    if (tokenUpper == "NONE" || tokenUpper == "UNBOUND")
    {
        *outKeycode = EMC_KEY_UNBOUND;
        return true;
    }

    int virtualKey = 0;
    std::string canonicalToken;
    if (!TryParsePrimaryKeyToken(tokenValue, &virtualKey, &canonicalToken))
    {
        return false;
    }

    if (canonicalToken.size() == 1)
    {
        switch (canonicalToken[0])
        {
        case '0': *outKeycode = OIS::KC_0; return true;
        case '1': *outKeycode = OIS::KC_1; return true;
        case '2': *outKeycode = OIS::KC_2; return true;
        case '3': *outKeycode = OIS::KC_3; return true;
        case '4': *outKeycode = OIS::KC_4; return true;
        case '5': *outKeycode = OIS::KC_5; return true;
        case '6': *outKeycode = OIS::KC_6; return true;
        case '7': *outKeycode = OIS::KC_7; return true;
        case '8': *outKeycode = OIS::KC_8; return true;
        case '9': *outKeycode = OIS::KC_9; return true;
        case 'A': *outKeycode = OIS::KC_A; return true;
        case 'B': *outKeycode = OIS::KC_B; return true;
        case 'C': *outKeycode = OIS::KC_C; return true;
        case 'D': *outKeycode = OIS::KC_D; return true;
        case 'E': *outKeycode = OIS::KC_E; return true;
        case 'F': *outKeycode = OIS::KC_F; return true;
        case 'G': *outKeycode = OIS::KC_G; return true;
        case 'H': *outKeycode = OIS::KC_H; return true;
        case 'I': *outKeycode = OIS::KC_I; return true;
        case 'J': *outKeycode = OIS::KC_J; return true;
        case 'K': *outKeycode = OIS::KC_K; return true;
        case 'L': *outKeycode = OIS::KC_L; return true;
        case 'M': *outKeycode = OIS::KC_M; return true;
        case 'N': *outKeycode = OIS::KC_N; return true;
        case 'O': *outKeycode = OIS::KC_O; return true;
        case 'P': *outKeycode = OIS::KC_P; return true;
        case 'Q': *outKeycode = OIS::KC_Q; return true;
        case 'R': *outKeycode = OIS::KC_R; return true;
        case 'S': *outKeycode = OIS::KC_S; return true;
        case 'T': *outKeycode = OIS::KC_T; return true;
        case 'U': *outKeycode = OIS::KC_U; return true;
        case 'V': *outKeycode = OIS::KC_V; return true;
        case 'W': *outKeycode = OIS::KC_W; return true;
        case 'X': *outKeycode = OIS::KC_X; return true;
        case 'Y': *outKeycode = OIS::KC_Y; return true;
        case 'Z': *outKeycode = OIS::KC_Z; return true;
        default:
            return false;
        }
    }

    if (canonicalToken.size() >= 2 && canonicalToken[0] == 'F')
    {
        const int functionIndex = std::atoi(canonicalToken.c_str() + 1);
        switch (functionIndex)
        {
        case 1: *outKeycode = OIS::KC_F1; return true;
        case 2: *outKeycode = OIS::KC_F2; return true;
        case 3: *outKeycode = OIS::KC_F3; return true;
        case 4: *outKeycode = OIS::KC_F4; return true;
        case 5: *outKeycode = OIS::KC_F5; return true;
        case 6: *outKeycode = OIS::KC_F6; return true;
        case 7: *outKeycode = OIS::KC_F7; return true;
        case 8: *outKeycode = OIS::KC_F8; return true;
        case 9: *outKeycode = OIS::KC_F9; return true;
        case 10: *outKeycode = OIS::KC_F10; return true;
        case 11: *outKeycode = OIS::KC_F11; return true;
        case 12: *outKeycode = OIS::KC_F12; return true;
        case 13: *outKeycode = OIS::KC_F13; return true;
        case 14: *outKeycode = OIS::KC_F14; return true;
        case 15: *outKeycode = OIS::KC_F15; return true;
        default:
            return false;
        }
    }

    if (canonicalToken == "SPACE")
    {
        *outKeycode = OIS::KC_SPACE;
        return true;
    }
    if (canonicalToken == "TAB")
    {
        *outKeycode = OIS::KC_TAB;
        return true;
    }
    if (canonicalToken == "ENTER")
    {
        *outKeycode = OIS::KC_RETURN;
        return true;
    }
    if (canonicalToken == "ESC")
    {
        *outKeycode = OIS::KC_ESCAPE;
        return true;
    }
    if (canonicalToken == "BACKSPACE")
    {
        *outKeycode = OIS::KC_BACK;
        return true;
    }
    if (canonicalToken == "DELETE")
    {
        *outKeycode = OIS::KC_DELETE;
        return true;
    }
    if (canonicalToken == "INSERT")
    {
        *outKeycode = OIS::KC_INSERT;
        return true;
    }
    if (canonicalToken == "HOME")
    {
        *outKeycode = OIS::KC_HOME;
        return true;
    }
    if (canonicalToken == "END")
    {
        *outKeycode = OIS::KC_END;
        return true;
    }
    if (canonicalToken == "PAGEUP")
    {
        *outKeycode = OIS::KC_PGUP;
        return true;
    }
    if (canonicalToken == "PAGEDOWN")
    {
        *outKeycode = OIS::KC_PGDOWN;
        return true;
    }
    if (canonicalToken == "UP")
    {
        *outKeycode = OIS::KC_UP;
        return true;
    }
    if (canonicalToken == "DOWN")
    {
        *outKeycode = OIS::KC_DOWN;
        return true;
    }
    if (canonicalToken == "LEFT")
    {
        *outKeycode = OIS::KC_LEFT;
        return true;
    }
    if (canonicalToken == "RIGHT")
    {
        *outKeycode = OIS::KC_RIGHT;
        return true;
    }

    return false;
}

bool TryMapOisKeycodeToTogglePanelToken(int32_t keycode, std::string* outToken)
{
    if (!outToken)
    {
        return false;
    }

    outToken->clear();

    if (keycode == EMC_KEY_UNBOUND)
    {
        *outToken = "NONE";
        return true;
    }

    switch (keycode)
    {
    case OIS::KC_0: *outToken = "0"; return true;
    case OIS::KC_1: *outToken = "1"; return true;
    case OIS::KC_2: *outToken = "2"; return true;
    case OIS::KC_3: *outToken = "3"; return true;
    case OIS::KC_4: *outToken = "4"; return true;
    case OIS::KC_5: *outToken = "5"; return true;
    case OIS::KC_6: *outToken = "6"; return true;
    case OIS::KC_7: *outToken = "7"; return true;
    case OIS::KC_8: *outToken = "8"; return true;
    case OIS::KC_9: *outToken = "9"; return true;
    case OIS::KC_A: *outToken = "A"; return true;
    case OIS::KC_B: *outToken = "B"; return true;
    case OIS::KC_C: *outToken = "C"; return true;
    case OIS::KC_D: *outToken = "D"; return true;
    case OIS::KC_E: *outToken = "E"; return true;
    case OIS::KC_F: *outToken = "F"; return true;
    case OIS::KC_G: *outToken = "G"; return true;
    case OIS::KC_H: *outToken = "H"; return true;
    case OIS::KC_I: *outToken = "I"; return true;
    case OIS::KC_J: *outToken = "J"; return true;
    case OIS::KC_K: *outToken = "K"; return true;
    case OIS::KC_L: *outToken = "L"; return true;
    case OIS::KC_M: *outToken = "M"; return true;
    case OIS::KC_N: *outToken = "N"; return true;
    case OIS::KC_O: *outToken = "O"; return true;
    case OIS::KC_P: *outToken = "P"; return true;
    case OIS::KC_Q: *outToken = "Q"; return true;
    case OIS::KC_R: *outToken = "R"; return true;
    case OIS::KC_S: *outToken = "S"; return true;
    case OIS::KC_T: *outToken = "T"; return true;
    case OIS::KC_U: *outToken = "U"; return true;
    case OIS::KC_V: *outToken = "V"; return true;
    case OIS::KC_W: *outToken = "W"; return true;
    case OIS::KC_X: *outToken = "X"; return true;
    case OIS::KC_Y: *outToken = "Y"; return true;
    case OIS::KC_Z: *outToken = "Z"; return true;
    case OIS::KC_F1: *outToken = "F1"; return true;
    case OIS::KC_F2: *outToken = "F2"; return true;
    case OIS::KC_F3: *outToken = "F3"; return true;
    case OIS::KC_F4: *outToken = "F4"; return true;
    case OIS::KC_F5: *outToken = "F5"; return true;
    case OIS::KC_F6: *outToken = "F6"; return true;
    case OIS::KC_F7: *outToken = "F7"; return true;
    case OIS::KC_F8: *outToken = "F8"; return true;
    case OIS::KC_F9: *outToken = "F9"; return true;
    case OIS::KC_F10: *outToken = "F10"; return true;
    case OIS::KC_F11: *outToken = "F11"; return true;
    case OIS::KC_F12: *outToken = "F12"; return true;
    case OIS::KC_F13: *outToken = "F13"; return true;
    case OIS::KC_F14: *outToken = "F14"; return true;
    case OIS::KC_F15: *outToken = "F15"; return true;
    case OIS::KC_SPACE: *outToken = "SPACE"; return true;
    case OIS::KC_TAB: *outToken = "TAB"; return true;
    case OIS::KC_RETURN: *outToken = "ENTER"; return true;
    case OIS::KC_ESCAPE: *outToken = "ESC"; return true;
    case OIS::KC_BACK: *outToken = "BACKSPACE"; return true;
    case OIS::KC_DELETE: *outToken = "DELETE"; return true;
    case OIS::KC_INSERT: *outToken = "INSERT"; return true;
    case OIS::KC_HOME: *outToken = "HOME"; return true;
    case OIS::KC_END: *outToken = "END"; return true;
    case OIS::KC_PGUP: *outToken = "PAGEUP"; return true;
    case OIS::KC_PGDOWN: *outToken = "PAGEDOWN"; return true;
    case OIS::KC_UP: *outToken = "UP"; return true;
    case OIS::KC_DOWN: *outToken = "DOWN"; return true;
    case OIS::KC_LEFT: *outToken = "LEFT"; return true;
    case OIS::KC_RIGHT: *outToken = "RIGHT"; return true;
    default:
        return false;
    }
}

bool TrySaveTogglePanelHotkeyConfig(const char** outError)
{
    if (outError)
    {
        *outError = "";
    }

    std::string configPath;
    if (!TryResolveModConfigPath(&configPath))
    {
        if (outError)
        {
            *outError = "config_path_unavailable";
        }
        return false;
    }

    std::string configText;
    if (!TryReadTextFile(configPath, &configText))
    {
        if (outError)
        {
            *outError = "config_read_failed";
        }
        return false;
    }

    if (!TryReplaceJsonBoolByKey(&configText, "enabled", g_pluginEnabled)
        || !TryReplaceJsonStringByKey(&configText, "toggle_panel_key", g_togglePanelKey)
        || !TryReplaceJsonBoolByKey(&configText, "toggle_panel_ctrl", g_togglePanelRequireCtrl)
        || !TryReplaceJsonBoolByKey(&configText, "toggle_panel_shift", g_togglePanelRequireShift)
        || !TryReplaceJsonBoolByKey(&configText, "toggle_panel_alt", g_togglePanelRequireAlt)
        || !TryUpsertJsonIntByKey(&configText, "panel_width", kPanelWidth)
        || !TryUpsertJsonIntByKey(&configText, "panel_min_expanded_height", g_panelMinExpandedHeight)
        || !TryUpsertJsonIntByKey(&configText, "panel_max_expanded_height", g_panelMaxExpandedHeight))
    {
        if (outError)
        {
            *outError = "config_key_missing";
        }
        return false;
    }

    if (g_developerMode
        && (!TryUpsertJsonIntByKey(&configText, "panel_header_title_font_height", g_panelHeaderTitleFontHeight)
            || !TryUpsertJsonIntByKey(&configText, "panel_collapse_button_size", g_panelCollapseButtonSize)
            || !TryUpsertJsonIntByKey(&configText, "panel_close_button_size", g_panelCloseButtonSize)
            || !TryUpsertJsonIntByKey(&configText, "panel_body_overlap", g_panelBodyOverlap)))
    {
        if (outError)
        {
            *outError = "config_key_missing";
        }
        return false;
    }

    if (!TryWriteTextFile(configPath, configText))
    {
        if (outError)
        {
            *outError = "config_write_failed";
        }
        return false;
    }

    return true;
}

void LogHotkeyBindingFallback()
{
    std::stringstream line;
    line << "invalid toggle_panel_key=\"" << g_togglePanelKey
         << "\"; falling back to \"" << kDefaultTogglePanelKey << "\"";
    LogWarnLine(line.str());
}

void RefreshHotkeyBinding()
{
    const std::string keyToken = TrimAscii(g_togglePanelKey);
    const std::string keyUpper = ToUpperAscii(keyToken);
    if (keyUpper == "NONE" || keyUpper == "UNBOUND")
    {
        g_hotkeyEnabled = false;
        g_hotkeyVirtualKey = 0;
        g_hotkeyDisplay = "NONE";
        g_hotkeyPrevDown = false;
        g_togglePanelKey = "NONE";
        return;
    }

    std::string canonicalKey;
    int virtualKey = 0;
    if (!TryParsePrimaryKeyToken(keyToken, &virtualKey, &canonicalKey))
    {
        LogHotkeyBindingFallback();
        canonicalKey = kDefaultTogglePanelKey;
        virtualKey = 'D';
    }

    g_hotkeyEnabled = true;
    g_hotkeyVirtualKey = virtualKey;
    g_togglePanelKey = canonicalKey;

    std::stringstream display;
    if (g_togglePanelRequireCtrl)
    {
        display << "CTRL+";
    }
    if (g_togglePanelRequireAlt)
    {
        display << "ALT+";
    }
    if (g_togglePanelRequireShift)
    {
        display << "SHIFT+";
    }
    display << canonicalKey;
    g_hotkeyDisplay = display.str();
}

void RebuildPanelForWidthChange()
{
    if (!g_panel)
    {
        return;
    }

    DestroyPanel();
    CreatePanelWidgets();
}

EMC_Result __cdecl GetTogglePanelHotkeyKeybind(void*, EMC_KeybindValueV1* outValue)
{
    if (!outValue)
    {
        return EMC_ERR_INVALID_ARGUMENT;
    }

    int32_t keycode = EMC_KEY_UNBOUND;
    if (g_hotkeyEnabled && !TryMapTogglePanelTokenToOisKeycode(g_togglePanelKey, &keycode))
    {
        keycode = EMC_KEY_UNBOUND;
    }

    outValue->keycode = keycode;
    outValue->modifiers = 0u;
    return EMC_OK;
}

EMC_Result __cdecl SetTogglePanelHotkeyKeybind(void*, EMC_KeybindValueV1 value, char* errBuf, uint32_t errBufSize)
{
    if (value.modifiers != 0u)
    {
        CopyModHubErrorMessage(errBuf, errBufSize, "use_modifier_toggles");
        return EMC_ERR_INVALID_ARGUMENT;
    }

    std::string updatedToken;
    if (!TryMapOisKeycodeToTogglePanelToken(value.keycode, &updatedToken))
    {
        CopyModHubErrorMessage(errBuf, errBufSize, "invalid_keybind");
        return EMC_ERR_INVALID_ARGUMENT;
    }

    const std::string previousToken = g_togglePanelKey;
    g_togglePanelKey = updatedToken;
    RefreshHotkeyBinding();

    const char* saveError = "";
    if (!TrySaveTogglePanelHotkeyConfig(&saveError))
    {
        g_togglePanelKey = previousToken;
        RefreshHotkeyBinding();
        CopyModHubErrorMessage(errBuf, errBufSize, saveError);
        return EMC_ERR_CALLBACK_FAILED;
    }

    CopyModHubErrorMessage(errBuf, errBufSize, 0);
    return EMC_OK;
}

EMC_Result __cdecl GetTogglePanelRequireCtrl(void*, int32_t* outValue)
{
    if (!outValue)
    {
        return EMC_ERR_INVALID_ARGUMENT;
    }

    *outValue = g_togglePanelRequireCtrl ? 1 : 0;
    return EMC_OK;
}

EMC_Result __cdecl SetTogglePanelRequireCtrl(void*, int32_t value, char* errBuf, uint32_t errBufSize)
{
    if (value != 0 && value != 1)
    {
        CopyModHubErrorMessage(errBuf, errBufSize, "value_must_be_bool");
        return EMC_ERR_INVALID_ARGUMENT;
    }

    const bool previousValue = g_togglePanelRequireCtrl;
    g_togglePanelRequireCtrl = value != 0;
    RefreshHotkeyBinding();

    const char* saveError = "";
    if (!TrySaveTogglePanelHotkeyConfig(&saveError))
    {
        g_togglePanelRequireCtrl = previousValue;
        RefreshHotkeyBinding();
        CopyModHubErrorMessage(errBuf, errBufSize, saveError);
        return EMC_ERR_CALLBACK_FAILED;
    }

    CopyModHubErrorMessage(errBuf, errBufSize, 0);
    return EMC_OK;
}

EMC_Result __cdecl GetTogglePanelRequireShift(void*, int32_t* outValue)
{
    if (!outValue)
    {
        return EMC_ERR_INVALID_ARGUMENT;
    }

    *outValue = g_togglePanelRequireShift ? 1 : 0;
    return EMC_OK;
}

EMC_Result __cdecl SetTogglePanelRequireShift(void*, int32_t value, char* errBuf, uint32_t errBufSize)
{
    if (value != 0 && value != 1)
    {
        CopyModHubErrorMessage(errBuf, errBufSize, "value_must_be_bool");
        return EMC_ERR_INVALID_ARGUMENT;
    }

    const bool previousValue = g_togglePanelRequireShift;
    g_togglePanelRequireShift = value != 0;
    RefreshHotkeyBinding();

    const char* saveError = "";
    if (!TrySaveTogglePanelHotkeyConfig(&saveError))
    {
        g_togglePanelRequireShift = previousValue;
        RefreshHotkeyBinding();
        CopyModHubErrorMessage(errBuf, errBufSize, saveError);
        return EMC_ERR_CALLBACK_FAILED;
    }

    CopyModHubErrorMessage(errBuf, errBufSize, 0);
    return EMC_OK;
}

EMC_Result __cdecl GetTogglePanelRequireAlt(void*, int32_t* outValue)
{
    if (!outValue)
    {
        return EMC_ERR_INVALID_ARGUMENT;
    }

    *outValue = g_togglePanelRequireAlt ? 1 : 0;
    return EMC_OK;
}

EMC_Result __cdecl SetTogglePanelRequireAlt(void*, int32_t value, char* errBuf, uint32_t errBufSize)
{
    if (value != 0 && value != 1)
    {
        CopyModHubErrorMessage(errBuf, errBufSize, "value_must_be_bool");
        return EMC_ERR_INVALID_ARGUMENT;
    }

    const bool previousValue = g_togglePanelRequireAlt;
    g_togglePanelRequireAlt = value != 0;
    RefreshHotkeyBinding();

    const char* saveError = "";
    if (!TrySaveTogglePanelHotkeyConfig(&saveError))
    {
        g_togglePanelRequireAlt = previousValue;
        RefreshHotkeyBinding();
        CopyModHubErrorMessage(errBuf, errBufSize, saveError);
        return EMC_ERR_CALLBACK_FAILED;
    }

    CopyModHubErrorMessage(errBuf, errBufSize, 0);
    return EMC_OK;
}

EMC_Result __cdecl GetPluginEnabled(void*, int32_t* outValue)
{
    if (!outValue)
    {
        return EMC_ERR_INVALID_ARGUMENT;
    }

    *outValue = g_pluginEnabled ? 1 : 0;
    return EMC_OK;
}

EMC_Result __cdecl SetPluginEnabled(void*, int32_t value, char* errBuf, uint32_t errBufSize)
{
    if (value != 0 && value != 1)
    {
        CopyModHubErrorMessage(errBuf, errBufSize, "value_must_be_bool");
        return EMC_ERR_INVALID_ARGUMENT;
    }

    const bool previousValue = g_pluginEnabled;
    g_pluginEnabled = value != 0;

    const char* saveError = "";
    if (!TrySaveTogglePanelHotkeyConfig(&saveError))
    {
        g_pluginEnabled = previousValue;
        CopyModHubErrorMessage(errBuf, errBufSize, saveError);
        return EMC_ERR_CALLBACK_FAILED;
    }

    if (!g_pluginEnabled && g_panel)
    {
        DestroyPanel();
    }
    else if (g_pluginEnabled && !g_panel && g_lastPlayerInterface)
    {
        CreatePanelWidgets();
    }

    CopyModHubErrorMessage(errBuf, errBufSize, 0);
    return EMC_OK;
}

EMC_Result __cdecl GetPanelWidth(void*, int32_t* outValue)
{
    if (!outValue)
    {
        return EMC_ERR_INVALID_ARGUMENT;
    }

    *outValue = kPanelWidth;
    return EMC_OK;
}

EMC_Result __cdecl SetPanelWidth(void*, int32_t value, char* errBuf, uint32_t errBufSize)
{
    const int previousValue = kPanelWidth;
    kPanelWidth = ClampPanelWidthValue(value);

    const char* saveError = "";
    if (!TrySaveTogglePanelHotkeyConfig(&saveError))
    {
        kPanelWidth = previousValue;
        CopyModHubErrorMessage(errBuf, errBufSize, saveError);
        return EMC_ERR_CALLBACK_FAILED;
    }

    RebuildPanelForWidthChange();
    CopyModHubErrorMessage(errBuf, errBufSize, 0);
    return EMC_OK;
}

EMC_Result __cdecl GetPanelMinExpandedHeight(void*, int32_t* outValue)
{
    if (!outValue)
    {
        return EMC_ERR_INVALID_ARGUMENT;
    }

    *outValue = g_panelMinExpandedHeight;
    return EMC_OK;
}

EMC_Result __cdecl SetPanelMinExpandedHeight(void*, int32_t value, char* errBuf, uint32_t errBufSize)
{
    const int previousMin = g_panelMinExpandedHeight;
    const int previousMax = g_panelMaxExpandedHeight;

    g_panelMinExpandedHeight = ClampIntValue(value, kPanelExpandedHeightLowerBound, kPanelExpandedHeightUpperBound);
    if (g_panelMinExpandedHeight > g_panelMaxExpandedHeight)
    {
        g_panelMaxExpandedHeight = g_panelMinExpandedHeight;
    }

    const char* saveError = "";
    if (!TrySaveTogglePanelHotkeyConfig(&saveError))
    {
        g_panelMinExpandedHeight = previousMin;
        g_panelMaxExpandedHeight = previousMax;
        CopyModHubErrorMessage(errBuf, errBufSize, saveError);
        return EMC_ERR_CALLBACK_FAILED;
    }

    ApplyPanelLayout();
    CopyModHubErrorMessage(errBuf, errBufSize, 0);
    return EMC_OK;
}

EMC_Result __cdecl GetPanelMaxExpandedHeight(void*, int32_t* outValue)
{
    if (!outValue)
    {
        return EMC_ERR_INVALID_ARGUMENT;
    }

    *outValue = g_panelMaxExpandedHeight;
    return EMC_OK;
}

EMC_Result __cdecl SetPanelMaxExpandedHeight(void*, int32_t value, char* errBuf, uint32_t errBufSize)
{
    const int previousMin = g_panelMinExpandedHeight;
    const int previousMax = g_panelMaxExpandedHeight;

    g_panelMaxExpandedHeight = ClampIntValue(value, kPanelExpandedHeightLowerBound, kPanelExpandedHeightUpperBound);
    if (g_panelMaxExpandedHeight < g_panelMinExpandedHeight)
    {
        g_panelMinExpandedHeight = g_panelMaxExpandedHeight;
    }

    const char* saveError = "";
    if (!TrySaveTogglePanelHotkeyConfig(&saveError))
    {
        g_panelMinExpandedHeight = previousMin;
        g_panelMaxExpandedHeight = previousMax;
        CopyModHubErrorMessage(errBuf, errBufSize, saveError);
        return EMC_ERR_CALLBACK_FAILED;
    }

    ApplyPanelLayout();
    CopyModHubErrorMessage(errBuf, errBufSize, 0);
    return EMC_OK;
}

EMC_Result __cdecl GetPanelHeaderTitleFontHeight(void*, int32_t* outValue)
{
    if (!outValue)
    {
        return EMC_ERR_INVALID_ARGUMENT;
    }

    *outValue = g_panelHeaderTitleFontHeight;
    return EMC_OK;
}

EMC_Result __cdecl SetPanelHeaderTitleFontHeight(void*, int32_t value, char* errBuf, uint32_t errBufSize)
{
    const int previousValue = g_panelHeaderTitleFontHeight;
    g_panelHeaderTitleFontHeight = ClampIntValue(value, kPanelHeaderTitleFontHeightLowerBound, kPanelHeaderTitleFontHeightUpperBound);

    const char* saveError = "";
    if (!TrySaveTogglePanelHotkeyConfig(&saveError))
    {
        g_panelHeaderTitleFontHeight = previousValue;
        CopyModHubErrorMessage(errBuf, errBufSize, saveError);
        return EMC_ERR_CALLBACK_FAILED;
    }

    ApplyPanelLayout();
    CopyModHubErrorMessage(errBuf, errBufSize, 0);
    return EMC_OK;
}

EMC_Result __cdecl GetPanelCollapseButtonSize(void*, int32_t* outValue)
{
    if (!outValue)
    {
        return EMC_ERR_INVALID_ARGUMENT;
    }

    *outValue = g_panelCollapseButtonSize;
    return EMC_OK;
}

EMC_Result __cdecl SetPanelCollapseButtonSize(void*, int32_t value, char* errBuf, uint32_t errBufSize)
{
    const int previousValue = g_panelCollapseButtonSize;
    g_panelCollapseButtonSize = ClampIntValue(value, kPanelHeaderButtonSizeLowerBound, kPanelHeaderButtonSizeUpperBound);

    const char* saveError = "";
    if (!TrySaveTogglePanelHotkeyConfig(&saveError))
    {
        g_panelCollapseButtonSize = previousValue;
        CopyModHubErrorMessage(errBuf, errBufSize, saveError);
        return EMC_ERR_CALLBACK_FAILED;
    }

    ApplyPanelLayout();
    CopyModHubErrorMessage(errBuf, errBufSize, 0);
    return EMC_OK;
}

EMC_Result __cdecl GetPanelCloseButtonSize(void*, int32_t* outValue)
{
    if (!outValue)
    {
        return EMC_ERR_INVALID_ARGUMENT;
    }

    *outValue = g_panelCloseButtonSize;
    return EMC_OK;
}

EMC_Result __cdecl SetPanelCloseButtonSize(void*, int32_t value, char* errBuf, uint32_t errBufSize)
{
    const int previousValue = g_panelCloseButtonSize;
    g_panelCloseButtonSize = ClampIntValue(value, kPanelHeaderButtonSizeLowerBound, kPanelHeaderButtonSizeUpperBound);

    const char* saveError = "";
    if (!TrySaveTogglePanelHotkeyConfig(&saveError))
    {
        g_panelCloseButtonSize = previousValue;
        CopyModHubErrorMessage(errBuf, errBufSize, saveError);
        return EMC_ERR_CALLBACK_FAILED;
    }

    ApplyPanelLayout();
    CopyModHubErrorMessage(errBuf, errBufSize, 0);
    return EMC_OK;
}

EMC_Result __cdecl GetPanelBodyOverlap(void*, int32_t* outValue)
{
    if (!outValue)
    {
        return EMC_ERR_INVALID_ARGUMENT;
    }

    *outValue = g_panelBodyOverlap;
    return EMC_OK;
}

EMC_Result __cdecl SetPanelBodyOverlap(void*, int32_t value, char* errBuf, uint32_t errBufSize)
{
    const int previousValue = g_panelBodyOverlap;
    g_panelBodyOverlap = ClampIntValue(value, kPanelBodyOverlapLowerBound, kPanelBodyOverlapUpperBound);

    const char* saveError = "";
    if (!TrySaveTogglePanelHotkeyConfig(&saveError))
    {
        g_panelBodyOverlap = previousValue;
        CopyModHubErrorMessage(errBuf, errBufSize, saveError);
        return EMC_ERR_CALLBACK_FAILED;
    }

    ApplyPanelLayout();
    CopyModHubErrorMessage(errBuf, errBufSize, 0);
    return EMC_OK;
}

const EMC_ModDescriptorV1 kModHubModDescriptor = {
    kModHubNamespaceId,
    kModHubNamespaceDisplayName,
    kModHubModId,
    kModHubModDisplayName,
    0
};

const EMC_KeybindSettingDefV1 kModHubTogglePanelKeySetting = {
    "toggle_panel_key",
    kModHubTogglePanelKeyLabel,
    kModHubTogglePanelKeyDescription,
    0,
    &GetTogglePanelHotkeyKeybind,
    &SetTogglePanelHotkeyKeybind
};

const EMC_BoolSettingDefV1 kModHubEnabledSetting = {
    "enabled",
    kModHubEnabledLabel,
    kModHubEnabledDescription,
    0,
    &GetPluginEnabled,
    &SetPluginEnabled
};

const EMC_BoolSettingDefV1 kModHubTogglePanelCtrlSetting = {
    "toggle_panel_ctrl",
    kModHubTogglePanelCtrlLabel,
    kModHubTogglePanelCtrlDescription,
    0,
    &GetTogglePanelRequireCtrl,
    &SetTogglePanelRequireCtrl
};

const EMC_BoolSettingDefV1 kModHubTogglePanelShiftSetting = {
    "toggle_panel_shift",
    kModHubTogglePanelShiftLabel,
    kModHubTogglePanelShiftDescription,
    0,
    &GetTogglePanelRequireShift,
    &SetTogglePanelRequireShift
};

const EMC_BoolSettingDefV1 kModHubTogglePanelAltSetting = {
    "toggle_panel_alt",
    kModHubTogglePanelAltLabel,
    kModHubTogglePanelAltDescription,
    0,
    &GetTogglePanelRequireAlt,
    &SetTogglePanelRequireAlt
};

const EMC_IntSettingDefV2 kModHubPanelWidthSetting = {
    "panel_width",
    kModHubPanelWidthLabel,
    kModHubPanelWidthDescription,
    0,
    kPanelWidthLowerBound,
    kPanelWidthUpperBound,
    10,
    { 40, 20, 10 },
    { 10, 20, 40 },
    &GetPanelWidth,
    &SetPanelWidth
};

const EMC_IntSettingDefV2 kModHubPanelMinHeightSetting = {
    "panel_min_expanded_height",
    kModHubPanelMinHeightLabel,
    kModHubPanelMinHeightDescription,
    0,
    kPanelExpandedHeightLowerBound,
    kPanelExpandedHeightUpperBound,
    10,
    { 50, 20, 10 },
    { 10, 20, 50 },
    &GetPanelMinExpandedHeight,
    &SetPanelMinExpandedHeight
};

const EMC_IntSettingDefV2 kModHubPanelMaxHeightSetting = {
    "panel_max_expanded_height",
    kModHubPanelMaxHeightLabel,
    kModHubPanelMaxHeightDescription,
    0,
    kPanelExpandedHeightLowerBound,
    kPanelExpandedHeightUpperBound,
    10,
    { 50, 20, 10 },
    { 10, 20, 50 },
    &GetPanelMaxExpandedHeight,
    &SetPanelMaxExpandedHeight
};

const EMC_IntSettingDefV2 kModHubPanelHeaderTitleFontHeightSetting = {
    "panel_header_title_font_height",
    kModHubPanelHeaderTitleFontHeightLabel,
    kModHubPanelHeaderTitleFontHeightDescription,
    0,
    kPanelHeaderTitleFontHeightLowerBound,
    kPanelHeaderTitleFontHeightUpperBound,
    1,
    { 4, 2, 1 },
    { 1, 2, 4 },
    &GetPanelHeaderTitleFontHeight,
    &SetPanelHeaderTitleFontHeight
};

const EMC_IntSettingDefV2 kModHubPanelCollapseButtonSizeSetting = {
    "panel_collapse_button_size",
    kModHubPanelCollapseButtonSizeLabel,
    kModHubPanelCollapseButtonSizeDescription,
    0,
    kPanelHeaderButtonSizeLowerBound,
    kPanelHeaderButtonSizeUpperBound,
    1,
    { 4, 2, 1 },
    { 1, 2, 4 },
    &GetPanelCollapseButtonSize,
    &SetPanelCollapseButtonSize
};

const EMC_IntSettingDefV2 kModHubPanelCloseButtonSizeSetting = {
    "panel_close_button_size",
    kModHubPanelCloseButtonSizeLabel,
    kModHubPanelCloseButtonSizeDescription,
    0,
    kPanelHeaderButtonSizeLowerBound,
    kPanelHeaderButtonSizeUpperBound,
    1,
    { 4, 2, 1 },
    { 1, 2, 4 },
    &GetPanelCloseButtonSize,
    &SetPanelCloseButtonSize
};

const EMC_IntSettingDefV2 kModHubPanelBodyOverlapSetting = {
    "panel_body_overlap",
    kModHubPanelBodyOverlapLabel,
    kModHubPanelBodyOverlapDescription,
    0,
    kPanelBodyOverlapLowerBound,
    kPanelBodyOverlapUpperBound,
    1,
    { 4, 2, 1 },
    { 1, 2, 4 },
    &GetPanelBodyOverlap,
    &SetPanelBodyOverlap
};

const emc::ModHubClientSettingRowV1 kModHubBaseRows[] = {
    { emc::MOD_HUB_CLIENT_SETTING_KIND_BOOL, &kModHubEnabledSetting },
    { emc::MOD_HUB_CLIENT_SETTING_KIND_KEYBIND, &kModHubTogglePanelKeySetting },
    { emc::MOD_HUB_CLIENT_SETTING_KIND_BOOL, &kModHubTogglePanelCtrlSetting },
    { emc::MOD_HUB_CLIENT_SETTING_KIND_BOOL, &kModHubTogglePanelShiftSetting },
    { emc::MOD_HUB_CLIENT_SETTING_KIND_BOOL, &kModHubTogglePanelAltSetting },
    { emc::MOD_HUB_CLIENT_SETTING_KIND_INT_V2, &kModHubPanelWidthSetting },
    { emc::MOD_HUB_CLIENT_SETTING_KIND_INT_V2, &kModHubPanelMinHeightSetting },
    { emc::MOD_HUB_CLIENT_SETTING_KIND_INT_V2, &kModHubPanelMaxHeightSetting }
};

const emc::ModHubClientSettingRowV1 kModHubDeveloperRows[] = {
    { emc::MOD_HUB_CLIENT_SETTING_KIND_BOOL, &kModHubEnabledSetting },
    { emc::MOD_HUB_CLIENT_SETTING_KIND_KEYBIND, &kModHubTogglePanelKeySetting },
    { emc::MOD_HUB_CLIENT_SETTING_KIND_BOOL, &kModHubTogglePanelCtrlSetting },
    { emc::MOD_HUB_CLIENT_SETTING_KIND_BOOL, &kModHubTogglePanelShiftSetting },
    { emc::MOD_HUB_CLIENT_SETTING_KIND_BOOL, &kModHubTogglePanelAltSetting },
    { emc::MOD_HUB_CLIENT_SETTING_KIND_INT_V2, &kModHubPanelWidthSetting },
    { emc::MOD_HUB_CLIENT_SETTING_KIND_INT_V2, &kModHubPanelMinHeightSetting },
    { emc::MOD_HUB_CLIENT_SETTING_KIND_INT_V2, &kModHubPanelMaxHeightSetting },
    { emc::MOD_HUB_CLIENT_SETTING_KIND_INT_V2, &kModHubPanelHeaderTitleFontHeightSetting },
    { emc::MOD_HUB_CLIENT_SETTING_KIND_INT_V2, &kModHubPanelCollapseButtonSizeSetting },
    { emc::MOD_HUB_CLIENT_SETTING_KIND_INT_V2, &kModHubPanelCloseButtonSizeSetting },
    { emc::MOD_HUB_CLIENT_SETTING_KIND_INT_V2, &kModHubPanelBodyOverlapSetting }
};

const emc::ModHubClientTableRegistrationV1 kModHubBaseRegistration = {
    &kModHubModDescriptor,
    kModHubBaseRows,
    static_cast<uint32_t>(sizeof(kModHubBaseRows) / sizeof(kModHubBaseRows[0]))
};

const emc::ModHubClientTableRegistrationV1 kModHubDeveloperRegistration = {
    &kModHubModDescriptor,
    kModHubDeveloperRows,
    static_cast<uint32_t>(sizeof(kModHubDeveloperRows) / sizeof(kModHubDeveloperRows[0]))
};

void LogModHubClientAttemptResult(const char* phase, emc::ModHubClient::AttemptResult result)
{
    std::stringstream line;
    line << "event=testkit_mod_hub_attach phase=\"" << (phase ? phase : "unknown") << "\"";

    switch (result)
    {
    case emc::ModHubClient::ATTACH_SUCCESS:
        line << " result=\"success\"";
        break;
    case emc::ModHubClient::ATTACH_FAILED:
        line << " result=\"attach_failed\"";
        break;
    case emc::ModHubClient::REGISTRATION_FAILED:
        line << " result=\"registration_failed\"";
        break;
    case emc::ModHubClient::INVALID_CONFIGURATION:
        line << " result=\"invalid_configuration\"";
        break;
    default:
        line << " result=\"unknown\"";
        break;
    }

    line << " failure_code=" << g_modHubClient.LastAttemptFailureResult()
         << " use_hub_ui=" << (g_modHubClient.UseHubUi() ? "true" : "false")
         << " retry_pending=" << (g_modHubClient.IsAttachRetryPending() ? "true" : "false")
         << " retried=" << (g_modHubClient.HasAttachRetryAttempted() ? "true" : "false");
    LogInfoLine(line.str());
}
}

void PersistCollapsedStateSetting()
{
    std::string configPath;
    if (!TryResolveModConfigPath(&configPath))
    {
        LogWarnLine("collapsed-state persistence skipped: could not resolve mod config path");
        return;
    }

    std::string configText;
    if (!TryReadTextFile(configPath, &configText))
    {
        std::stringstream line;
        line << "collapsed-state persistence skipped: could not read " << configPath;
        LogWarnLine(line.str());
        return;
    }

    if (!TryReplaceJsonBoolByKey(&configText, "start_collapsed", g_panelCollapsed))
    {
        std::stringstream line;
        line << "collapsed-state persistence skipped: missing start_collapsed in " << configPath;
        LogWarnLine(line.str());
        return;
    }

    if (!TryWriteTextFile(configPath, configText))
    {
        std::stringstream line;
        line << "collapsed-state persistence failed: could not write " << configPath;
        LogWarnLine(line.str());
        return;
    }

    std::stringstream line;
    line << "event=testkit_panel_collapsed_persisted collapsed=" << (g_panelCollapsed ? "true" : "false");
    LogDebugLine(line.str());
}

void SortSavedLocationsForDisplay(std::vector<SavedLocation>* locations)
{
    if (!locations || locations->size() < 2u)
    {
        return;
    }

    std::sort(locations->begin(), locations->end(), CompareSavedLocationsForDisplay);
}

void NormalizePanelHeightSettings()
{
    g_panelMinExpandedHeight = ClampIntValue(g_panelMinExpandedHeight, kPanelExpandedHeightLowerBound, kPanelExpandedHeightUpperBound);
    g_panelMaxExpandedHeight = ClampIntValue(g_panelMaxExpandedHeight, kPanelExpandedHeightLowerBound, kPanelExpandedHeightUpperBound);
    if (g_panelMinExpandedHeight > g_panelMaxExpandedHeight)
    {
        g_panelMaxExpandedHeight = g_panelMinExpandedHeight;
    }
}

void NormalizePanelVisualSettings()
{
    g_panelHeaderTitleFontHeight =
        ClampIntValue(g_panelHeaderTitleFontHeight, kPanelHeaderTitleFontHeightLowerBound, kPanelHeaderTitleFontHeightUpperBound);
    g_panelCollapseButtonSize =
        ClampIntValue(g_panelCollapseButtonSize, kPanelHeaderButtonSizeLowerBound, kPanelHeaderButtonSizeUpperBound);
    g_panelCloseButtonSize =
        ClampIntValue(g_panelCloseButtonSize, kPanelHeaderButtonSizeLowerBound, kPanelHeaderButtonSizeUpperBound);
    g_panelBodyOverlap = ClampIntValue(g_panelBodyOverlap, kPanelBodyOverlapLowerBound, kPanelBodyOverlapUpperBound);
}

bool TryPersistSavedLocationsConfig(const std::vector<SavedLocation>& locations, std::string* outError)
{
    std::string configPath;
    if (!TryResolveModConfigPath(&configPath))
    {
        if (outError)
        {
            *outError = "config_path_unavailable";
        }
        return false;
    }

    std::string configText;
    if (!TryReadTextFile(configPath, &configText))
    {
        if (outError)
        {
            *outError = "config_read_failed";
        }
        return false;
    }

    if (!TryUpsertJsonRawValueByKey(&configText, kSavedLocationsConfigKey, BuildSavedLocationsJsonValue(locations)))
    {
        if (outError)
        {
            *outError = "config_key_missing";
        }
        return false;
    }

    if (!TryWriteTextFile(configPath, configText))
    {
        if (outError)
        {
            *outError = "config_write_failed";
        }
        return false;
    }

    return true;
}

void LoadConfig()
{
    g_pluginEnabled = true;
    g_developerMode = false;
    g_loggingLevel = LoggingLevel_Info;
    g_togglePanelRequireCtrl = true;
    g_togglePanelRequireShift = true;
    g_togglePanelRequireAlt = false;
    g_togglePanelKey = kDefaultTogglePanelKey;
    g_confirmDangerousActions = true;
    g_panelHidden = false;
    g_panelCollapsed = false;
    kPanelWidth = kPanelWidthDefault;
    g_panelMinExpandedHeight = kPanelMinExpandedHeightDefault;
    g_panelMaxExpandedHeight = kPanelExpandedHeight;
    g_panelHeaderTitleFontHeight = kPanelHeaderTitleFontHeightDefault;
    g_panelCollapseButtonSize = kPanelCollapseButtonSizeDefault;
    g_panelCloseButtonSize = kPanelCloseButtonSizeDefault;
    g_panelBodyOverlap = kPanelBodyOverlapDefault;
    g_savedLocations.clear();

    std::string configPath;
    if (!TryResolveModConfigPath(&configPath))
    {
        LogWarnLine("mod config load skipped: could not resolve plugin directory (using defaults)");
        RefreshHotkeyBinding();
        return;
    }

    std::string configText;
    if (!TryReadTextFile(configPath, &configText))
    {
        std::stringstream line;
        line << "mod config load skipped: could not read " << configPath << " (using defaults)";
        LogWarnLine(line.str());
        RefreshHotkeyBinding();
        return;
    }

    bool parsedBool = false;
    int parsedInt = 0;
    std::string parsedString;

    if (TryParseJsonBoolByKey(configText, "enabled", &parsedBool))
    {
        g_pluginEnabled = parsedBool;
    }
    if (TryParseJsonBoolByKey(configText, kDeveloperModeConfigKey, &parsedBool))
    {
        g_developerMode = parsedBool;
    }
    if (TryParseJsonBoolByKey(configText, "toggle_panel_ctrl", &parsedBool))
    {
        g_togglePanelRequireCtrl = parsedBool;
    }
    if (TryParseJsonBoolByKey(configText, "toggle_panel_shift", &parsedBool))
    {
        g_togglePanelRequireShift = parsedBool;
    }
    if (TryParseJsonBoolByKey(configText, "toggle_panel_alt", &parsedBool))
    {
        g_togglePanelRequireAlt = parsedBool;
    }
    if (TryParseJsonBoolByKey(configText, "start_hidden", &parsedBool))
    {
        g_panelHidden = parsedBool;
    }
    if (TryParseJsonBoolByKey(configText, "start_collapsed", &parsedBool))
    {
        g_panelCollapsed = parsedBool;
    }
    if (TryParseJsonBoolByKey(configText, "confirm_dangerous_actions", &parsedBool))
    {
        g_confirmDangerousActions = parsedBool;
    }
    if (TryParseJsonIntByKey(configText, "panel_width", &parsedInt))
    {
        kPanelWidth = parsedInt;
    }
    if (TryParseJsonIntByKey(configText, "panel_min_expanded_height", &parsedInt))
    {
        g_panelMinExpandedHeight = parsedInt;
    }
    if (TryParseJsonIntByKey(configText, "panel_max_expanded_height", &parsedInt))
    {
        g_panelMaxExpandedHeight = parsedInt;
    }
    if (TryParseJsonIntByKey(configText, "panel_header_title_font_height", &parsedInt))
    {
        g_panelHeaderTitleFontHeight = parsedInt;
    }
    if (TryParseJsonIntByKey(configText, "panel_collapse_button_size", &parsedInt))
    {
        g_panelCollapseButtonSize = parsedInt;
    }
    if (TryParseJsonIntByKey(configText, "panel_close_button_size", &parsedInt))
    {
        g_panelCloseButtonSize = parsedInt;
    }
    if (TryParseJsonIntByKey(configText, "panel_body_overlap", &parsedInt))
    {
        g_panelBodyOverlap = parsedInt;
    }

    if (TryParseJsonStringByKey(configText, "toggle_panel_key", &parsedString))
    {
        g_togglePanelKey = parsedString;
    }

    if (TryParseJsonStringByKey(configText, "logging_level", &parsedString))
    {
        const std::string levelUpper = ToUpperAscii(TrimAscii(parsedString));
        if (levelUpper == "DEBUG")
        {
            g_loggingLevel = LoggingLevel_Debug;
        }
    }
    else if (TryParseJsonBoolByKey(configText, "debugLogging", &parsedBool) && parsedBool)
    {
        g_loggingLevel = LoggingLevel_Debug;
    }

    std::string::size_type savedLocationsValuePos = 0;
    if (TryFindJsonValueStartByKey(configText, kSavedLocationsConfigKey, &savedLocationsValuePos))
    {
        if (!TryParseSavedLocationsByKey(configText, kSavedLocationsConfigKey, &g_savedLocations))
        {
            LogWarnLine("saved locations load skipped: invalid saved_locations array");
            g_savedLocations.clear();
        }
    }
    else
    {
        const std::string savedLocationsValue = BuildSavedLocationsJsonValue(g_savedLocations);
        if (TryUpsertJsonRawValueByKey(&configText, kSavedLocationsConfigKey, savedLocationsValue))
        {
            if (TryWriteTextFile(configPath, configText))
            {
                LogInfoLine("saved locations config initialized entries=0");
            }
            else
            {
                LogWarnLine("saved locations config initialization failed: could not write config");
            }
        }
        else
        {
            LogWarnLine("saved locations config initialization failed: could not upsert saved_locations");
        }
    }

    kPanelWidth = ClampPanelWidthValue(kPanelWidth);
    NormalizePanelHeightSettings();
    NormalizePanelVisualSettings();
    RefreshHotkeyBinding();
    RefreshSavedLocationsListWidget();

    std::stringstream info;
    info << "mod config loaded enabled=" << (g_pluginEnabled ? "true" : "false")
         << " developer_mode=" << (g_developerMode ? "true" : "false")
         << " hotkey=\"" << g_hotkeyDisplay << "\""
         << " start_hidden=" << (g_panelHidden ? "true" : "false")
         << " start_collapsed=" << (g_panelCollapsed ? "true" : "false")
         << " confirm_dangerous_actions=" << (g_confirmDangerousActions ? "true" : "false")
         << " panel_width=" << kPanelWidth
         << " min_height=" << g_panelMinExpandedHeight
         << " max_height=" << g_panelMaxExpandedHeight
         << " title_font_height=" << g_panelHeaderTitleFontHeight
         << " collapse_button_size=" << g_panelCollapseButtonSize
         << " close_button_size=" << g_panelCloseButtonSize
         << " body_overlap=" << g_panelBodyOverlap
         << " saved_locations=" << g_savedLocations.size();
    LogInfoLine(info.str());
}

void EnsureModHubClientConfigured()
{
    if (g_modHubClientConfigured)
    {
        return;
    }

    emc::ModHubClient::Config config;
    config.table_registration = g_developerMode ? &kModHubDeveloperRegistration : &kModHubBaseRegistration;
    g_modHubClient.SetConfig(config);
    g_modHubClientConfigured = true;
}

void StartModHubClient()
{
    EnsureModHubClientConfigured();
    LogModHubClientAttemptResult("startup", g_modHubClient.OnStartup());
}

void TickModHubAttachRetry()
{
    if (!g_modHubClient.IsAttachRetryPending() || g_modHubClient.HasAttachRetryAttempted())
    {
        return;
    }

    LogModHubClientAttemptResult("retry", g_modHubClient.OnOptionsWindowInit());
}
}
