#include <Debug.h>

#include <core/Functions.h>
#include <kenshi/Character.h>
#include <kenshi/Damages.h>
#include <kenshi/Dialogue.h>
#include <kenshi/Faction.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/Kenshi.h>
#include <kenshi/MedicalSystem.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/RootObject.h>
#include <kenshi/SaveManager.h>
#include <mygui/MyGUI_Button.h>
#include <mygui/MyGUI_Delegate.h>
#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_InputManager.h>
#include <mygui/MyGUI_RenderManager.h>
#include <mygui/MyGUI_TextBox.h>
#include <mygui/MyGUI_Widget.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cctype>
#include <fstream>
#include <sstream>
#include <string>

namespace
{
const char* kPluginName = "Emkejs-Test-Kit";
const char* kConfigFileName = "mod-config.json";
const char* kDefaultTogglePanelKey = "D";
const int kPanelLeft = 18;
const int kPanelTop = 140;
const int kPanelWidth = 360;
const int kPanelExpandedHeight = 548;
const int kPanelCollapsedHeight = 42;
const int kPanelViewportPadding = 16;
const int kPanelDragThreshold = 3;
const DWORD kDangerArmTimeoutMs = 3000;
const float kForceUnconsciousDurationSeconds = 30.0f;
const float kForceDyingBloodOffset = 8.0f;
const float kForceDyingAliveBloodMargin = 1.0f;
const float kProbablyDyingBloodMax = 50.0f;
const float kLimbDamageFraction = 0.35f;
const float kMinimumLimbDamageAmount = 5.0f;
const char* kTeleportDestinationLabel = "Test Spot";
const Ogre::Vector3 kTeleportDestinationCenter(-56164.4f, 1605.11f, 20653.6f);
const float kFloatChangeEpsilon = 0.001f;

enum LoggingLevel
{
    LoggingLevel_Info = 0,
    LoggingLevel_Debug = 1
};

enum TargetSource
{
    TargetSource_None = 0,
    TargetSource_Selected = 1,
    TargetSource_Hovered = 2,
    TargetSource_Conversation = 3
};

enum PanelTab
{
    PanelTab_Health = 0,
    PanelTab_Teleport = 1
};

struct TargetSnapshot
{
    bool hasTarget;
    TargetSource source;
    Character* target;
    std::string name;
    std::string factionName;
    std::string alignment;
    std::string membership;
    std::string stateLabel;
    bool unconscious;
    bool playingDead;
    bool dying;
    bool dead;
};

std::string g_configPath;
bool g_pluginEnabled = true;
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
bool g_runtimePanelPositionSet = false;
int g_runtimePanelLeft = kPanelLeft;
int g_runtimePanelTop = kPanelTop;
bool g_panelDragging = false;
bool g_panelDragMoved = false;
int g_panelDragLastMouseX = 0;
int g_panelDragLastMouseY = 0;
int g_panelDragMovedDistance = 0;
bool g_forceDyingArmed = false;
DWORD g_forceDyingArmedAtMs = 0;
std::string g_lastStatusMessage = "Ready";
TargetSnapshot g_lastTargetSnapshot;
bool g_hasLastTargetSnapshot = false;
PanelTab g_activePanelTab = PanelTab_Health;

PlayerInterface* g_lastPlayerInterface = 0;
bool g_loggedPanelCreateFailure = false;

MyGUI::Widget* g_panel = 0;
MyGUI::Button* g_headerFrame = 0;
MyGUI::TextBox* g_headerTitleText = 0;
MyGUI::Button* g_collapseButton = 0;
MyGUI::Button* g_bodyFrame = 0;
MyGUI::TextBox* g_targetSectionText = 0;
MyGUI::TextBox* g_targetNameText = 0;
MyGUI::TextBox* g_targetFactionText = 0;
MyGUI::TextBox* g_targetAlignmentText = 0;
MyGUI::TextBox* g_targetMembershipText = 0;
MyGUI::TextBox* g_targetStateText = 0;
MyGUI::TextBox* g_noTargetText = 0;
MyGUI::Button* g_healthTabButton = 0;
MyGUI::Button* g_teleportTabButton = 0;
MyGUI::TextBox* g_statesSectionText = 0;
MyGUI::Button* g_forceUnconsciousButton = 0;
MyGUI::Button* g_forcePlayingDeadButton = 0;
MyGUI::TextBox* g_limbDamageSectionText = 0;
MyGUI::Button* g_damageLeftArmButton = 0;
MyGUI::Button* g_damageRightArmButton = 0;
MyGUI::Button* g_damageLeftLegButton = 0;
MyGUI::Button* g_damageRightLegButton = 0;
MyGUI::TextBox* g_teleportSectionText = 0;
MyGUI::Button* g_teleportSelectedToCameraButton = 0;
MyGUI::TextBox* g_dangerousSectionText = 0;
MyGUI::Button* g_forceDyingButton = 0;
MyGUI::TextBox* g_statusText = 0;

void (*PlayerInterface_updateUT_orig)(PlayerInterface*) = 0;
void (*SaveManager_loadByInfo_orig)(SaveManager*, const SaveInfo&, bool) = 0;
void (*SaveManager_loadByName_orig)(SaveManager*, const std::string&) = 0;

bool IsSupportedVersion(KenshiLib::BinaryVersion& versionInfo)
{
    const unsigned int platform = versionInfo.GetPlatform();
    const std::string version = versionInfo.GetVersion();

    return platform != KenshiLib::BinaryVersion::UNKNOWN
        && (version == "1.0.65" || version == "1.0.68");
}

void LogInfoLine(const std::string& message)
{
    std::stringstream line;
    line << kPluginName << " INFO: " << message;
    DebugLog(line.str().c_str());
}

void LogWarnLine(const std::string& message)
{
    std::stringstream line;
    line << kPluginName << " WARN: " << message;
    ErrorLog(line.str().c_str());
}

void LogErrorLine(const std::string& message)
{
    std::stringstream line;
    line << kPluginName << " ERROR: " << message;
    ErrorLog(line.str().c_str());
}

bool ShouldLogDebug()
{
    return g_loggingLevel == LoggingLevel_Debug;
}

void LogDebugLine(const std::string& message)
{
    if (ShouldLogDebug())
    {
        LogInfoLine(message);
    }
}

std::string TrimAscii(const std::string& value)
{
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0)
    {
        ++start;
    }

    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0)
    {
        --end;
    }

    return value.substr(start, end - start);
}

std::string ToUpperAscii(const std::string& value)
{
    std::string upper;
    upper.reserve(value.size());

    for (size_t index = 0; index < value.size(); ++index)
    {
        upper.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(value[index]))));
    }

    return upper;
}

std::string SanitizeLogValue(const std::string& value)
{
    std::string sanitized;
    sanitized.reserve(value.size());

    for (size_t index = 0; index < value.size(); ++index)
    {
        const char current = value[index];
        if (current == '"')
        {
            sanitized.push_back('\'');
            continue;
        }

        sanitized.push_back(current);
    }

    return sanitized;
}

const char* TargetSourceToUiLabel(TargetSource source)
{
    switch (source)
    {
    case TargetSource_Selected:
        return "Selected";
    case TargetSource_Hovered:
        return "Hovered";
    case TargetSource_Conversation:
        return "Conversation";
    default:
        return "None";
    }
}

const char* TargetSourceToLogLabel(TargetSource source)
{
    switch (source)
    {
    case TargetSource_Selected:
        return "selected";
    case TargetSource_Hovered:
        return "hovered";
    case TargetSource_Conversation:
        return "conversation";
    default:
        return "none";
    }
}

void ResetTargetSnapshot(TargetSnapshot* snapshot)
{
    if (!snapshot)
    {
        return;
    }

    snapshot->hasTarget = false;
    snapshot->source = TargetSource_None;
    snapshot->target = 0;
    snapshot->name.clear();
    snapshot->factionName.clear();
    snapshot->alignment.clear();
    snapshot->membership.clear();
    snapshot->stateLabel = "Unknown";
    snapshot->unconscious = false;
    snapshot->playingDead = false;
    snapshot->dying = false;
    snapshot->dead = false;
}

bool AreTargetSnapshotsEqual(const TargetSnapshot& left, const TargetSnapshot& right)
{
    return left.hasTarget == right.hasTarget
        && left.source == right.source
        && left.target == right.target
        && left.name == right.name
        && left.factionName == right.factionName
        && left.alignment == right.alignment
        && left.membership == right.membership
        && left.stateLabel == right.stateLabel
        && left.unconscious == right.unconscious
        && left.playingDead == right.playingDead
        && left.dying == right.dying
        && left.dead == right.dead;
}

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

void RefreshStatusWidget()
{
    if (!g_statusText)
    {
        return;
    }

    std::stringstream caption;
    caption << "Status: " << g_lastStatusMessage;
    g_statusText->setCaption(caption.str());
}

void SetStatusMessage(const std::string& message)
{
    g_lastStatusMessage = message;
    RefreshStatusWidget();
}

void UpdateForceDyingButtonCaption()
{
    if (!g_forceDyingButton)
    {
        return;
    }

    if (g_confirmDangerousActions && g_forceDyingArmed)
    {
        g_forceDyingButton->setCaption("Confirm Force Dying");
        return;
    }

    g_forceDyingButton->setCaption("Force Dying");
}

void UpdateCollapseButtonCaption()
{
    if (!g_collapseButton)
    {
        return;
    }

    g_collapseButton->setCaption(g_panelCollapsed ? "Expand" : "Collapse");
}

void SetWidgetVisible(MyGUI::Widget* widget, bool visible)
{
    if (widget)
    {
        widget->setVisible(visible);
    }
}

void UpdatePanelTabButtonCaptions()
{
    if (g_healthTabButton)
    {
        g_healthTabButton->setCaption(g_activePanelTab == PanelTab_Health ? "[Health]" : "Health");
    }

    if (g_teleportTabButton)
    {
        g_teleportTabButton->setCaption(g_activePanelTab == PanelTab_Teleport ? "[Teleport]" : "Teleport");
    }
}

void UpdatePanelBodyWidgetVisibility(bool bodyVisible)
{
    const bool healthVisible = bodyVisible && g_activePanelTab == PanelTab_Health;
    const bool teleportVisible = bodyVisible && g_activePanelTab == PanelTab_Teleport;

    UpdatePanelTabButtonCaptions();

    SetWidgetVisible(g_targetSectionText, bodyVisible);
    SetWidgetVisible(g_targetNameText, bodyVisible);
    SetWidgetVisible(g_targetFactionText, bodyVisible);
    SetWidgetVisible(g_targetAlignmentText, bodyVisible);
    SetWidgetVisible(g_targetMembershipText, bodyVisible);
    SetWidgetVisible(g_targetStateText, bodyVisible);
    SetWidgetVisible(g_noTargetText, bodyVisible);
    SetWidgetVisible(g_healthTabButton, bodyVisible);
    SetWidgetVisible(g_teleportTabButton, bodyVisible);

    SetWidgetVisible(g_statesSectionText, healthVisible);
    SetWidgetVisible(g_forceUnconsciousButton, healthVisible);
    SetWidgetVisible(g_forcePlayingDeadButton, healthVisible);
    SetWidgetVisible(g_limbDamageSectionText, healthVisible);
    SetWidgetVisible(g_damageLeftArmButton, healthVisible);
    SetWidgetVisible(g_damageRightArmButton, healthVisible);
    SetWidgetVisible(g_damageLeftLegButton, healthVisible);
    SetWidgetVisible(g_damageRightLegButton, healthVisible);
    SetWidgetVisible(g_dangerousSectionText, healthVisible);
    SetWidgetVisible(g_forceDyingButton, healthVisible);

    SetWidgetVisible(g_teleportSectionText, teleportVisible);
    SetWidgetVisible(g_teleportSelectedToCameraButton, teleportVisible);

    SetWidgetVisible(g_statusText, bodyVisible);
}

void ClearForceDyingArm(const char* reason, bool updateStatus)
{
    if (!g_forceDyingArmed)
    {
        return;
    }

    g_forceDyingArmed = false;
    g_forceDyingArmedAtMs = 0;
    UpdateForceDyingButtonCaption();

    if (reason)
    {
        std::stringstream line;
        line << "event=testkit_action_arm action=\"force_dying\" armed=false reason=\"" << reason << "\"";
        LogInfoLine(line.str());
    }

    if (updateStatus)
    {
        SetStatusMessage("Force Dying arm cleared");
    }
}

bool TryGetViewportSize(int* widthOut, int* heightOut)
{
    if (!widthOut || !heightOut)
    {
        return false;
    }

    MyGUI::RenderManager* renderManager = MyGUI::RenderManager::getInstancePtr();
    if (!renderManager)
    {
        return false;
    }

    const MyGUI::IntSize view = renderManager->getViewSize();
    if (view.width <= 0 || view.height <= 0)
    {
        return false;
    }

    *widthOut = view.width;
    *heightOut = view.height;
    return true;
}

bool TryGetMousePosition(int* xOut, int* yOut)
{
    if (!xOut || !yOut)
    {
        return false;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    if (!inputManager)
    {
        return false;
    }

    const MyGUI::IntPoint mouse = inputManager->getMousePosition();
    *xOut = mouse.left;
    *yOut = mouse.top;
    return true;
}

int ClampIntValue(int value, int minValue, int maxValue)
{
    if (value < minValue)
    {
        return minValue;
    }
    if (value > maxValue)
    {
        return maxValue;
    }
    return value;
}

MyGUI::IntCoord BuildPanelCoordFromAnchor(int left, int top)
{
    const int height = g_panelCollapsed ? kPanelCollapsedHeight : kPanelExpandedHeight;
    return MyGUI::IntCoord(left, top, kPanelWidth, height);
}

MyGUI::IntCoord ClampPanelCoordToViewport(const MyGUI::IntCoord& inputCoord)
{
    int left = inputCoord.left;
    int top = inputCoord.top;
    const int width = (inputCoord.width > 0) ? inputCoord.width : kPanelWidth;
    const int height = (inputCoord.height > 0)
        ? inputCoord.height
        : (g_panelCollapsed ? kPanelCollapsedHeight : kPanelExpandedHeight);

    int viewWidth = 0;
    int viewHeight = 0;
    if (!TryGetViewportSize(&viewWidth, &viewHeight))
    {
        if (left < kPanelViewportPadding)
        {
            left = kPanelViewportPadding;
        }
        if (top < kPanelViewportPadding)
        {
            top = kPanelViewportPadding;
        }
        return MyGUI::IntCoord(left, top, width, height);
    }

    int minLeft = kPanelViewportPadding;
    int minTop = kPanelViewportPadding;
    int maxLeft = viewWidth - width - kPanelViewportPadding;
    int maxTop = viewHeight - height - kPanelViewportPadding;

    if (maxLeft < minLeft)
    {
        minLeft = 0;
        maxLeft = viewWidth - width;
        if (maxLeft < minLeft)
        {
            maxLeft = minLeft;
        }
    }

    if (maxTop < minTop)
    {
        minTop = 0;
        maxTop = viewHeight - height;
        if (maxTop < minTop)
        {
            maxTop = minTop;
        }
    }

    left = ClampIntValue(left, minLeft, maxLeft);
    top = ClampIntValue(top, minTop, maxTop);
    return MyGUI::IntCoord(left, top, width, height);
}

void StorePanelRuntimePosition(const MyGUI::IntCoord& panelCoord)
{
    g_runtimePanelPositionSet = true;
    g_runtimePanelLeft = panelCoord.left;
    g_runtimePanelTop = panelCoord.top;
}

MyGUI::IntCoord ResolvePanelCoord()
{
    if (g_runtimePanelPositionSet)
    {
        return ClampPanelCoordToViewport(BuildPanelCoordFromAnchor(g_runtimePanelLeft, g_runtimePanelTop));
    }

    return ClampPanelCoordToViewport(BuildPanelCoordFromAnchor(kPanelLeft, kPanelTop));
}

void ApplyPanelLayout(const MyGUI::IntCoord& panelCoord)
{
    if (!g_panel)
    {
        return;
    }

    const MyGUI::IntCoord clampedCoord = ClampPanelCoordToViewport(panelCoord);
    StorePanelRuntimePosition(clampedCoord);

    g_panel->setCoord(clampedCoord);
    g_panel->setVisible(!g_panelHidden);

    const bool bodyVisible = !g_panelHidden && !g_panelCollapsed;

    if (g_headerFrame)
    {
        g_headerFrame->setCoord(MyGUI::IntCoord(0, 0, kPanelWidth, 38));
    }

    if (g_headerTitleText)
    {
        g_headerTitleText->setCoord(MyGUI::IntCoord(12, 8, kPanelWidth - 116, 22));
    }

    if (g_collapseButton)
    {
        g_collapseButton->setCoord(MyGUI::IntCoord(kPanelWidth - 96, 4, 84, 30));
    }

    if (g_bodyFrame)
    {
        g_bodyFrame->setCoord(MyGUI::IntCoord(0, 40, kPanelWidth, kPanelExpandedHeight - 40));
        g_bodyFrame->setVisible(bodyVisible);
    }

    UpdatePanelBodyWidgetVisibility(bodyVisible);
}

void ApplyPanelLayout()
{
    if (!g_panel)
    {
        return;
    }

    ApplyPanelLayout(ResolvePanelCoord());
}

void MovePanelByDelta(int deltaX, int deltaY)
{
    if (!g_panel || (deltaX == 0 && deltaY == 0))
    {
        return;
    }

    const int moveX = (deltaX < 0) ? -deltaX : deltaX;
    const int moveY = (deltaY < 0) ? -deltaY : deltaY;
    g_panelDragMovedDistance += moveX + moveY;
    if (g_panelDragMovedDistance >= kPanelDragThreshold)
    {
        g_panelDragMoved = true;
    }

    const MyGUI::IntCoord currentCoord = g_panel->getCoord();
    const MyGUI::IntCoord movedCoord = ClampPanelCoordToViewport(
        BuildPanelCoordFromAnchor(currentCoord.left + deltaX, currentCoord.top + deltaY));
    ApplyPanelLayout(movedCoord);
}

void FinalizePanelDrag(const char* source)
{
    if (!g_panelDragging)
    {
        return;
    }

    g_panelDragging = false;
    if (!g_panel)
    {
        g_panelDragMoved = false;
        g_panelDragMovedDistance = 0;
        return;
    }

    const MyGUI::IntCoord clampedCoord = ClampPanelCoordToViewport(g_panel->getCoord());
    ApplyPanelLayout(clampedCoord);

    if (g_panelDragMoved)
    {
        std::stringstream line;
        line << "event=testkit_panel_moved left=" << clampedCoord.left
             << " top=" << clampedCoord.top;
        if (source)
        {
            line << " source=\"" << source << "\"";
        }
        LogInfoLine(line.str());
    }

    g_panelDragMoved = false;
    g_panelDragMovedDistance = 0;
}

void OnHeaderMousePressed(MyGUI::Widget*, int left, int top, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    g_panelDragging = true;
    g_panelDragMoved = false;
    if (!TryGetMousePosition(&g_panelDragLastMouseX, &g_panelDragLastMouseY))
    {
        g_panelDragLastMouseX = left;
        g_panelDragLastMouseY = top;
    }
    g_panelDragMovedDistance = 0;
}

void OnHeaderMouseDrag(MyGUI::Widget*, int left, int top, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    if (!g_panelDragging || !g_panel)
    {
        return;
    }

    int mouseX = left;
    int mouseY = top;
    TryGetMousePosition(&mouseX, &mouseY);

    const int deltaX = mouseX - g_panelDragLastMouseX;
    const int deltaY = mouseY - g_panelDragLastMouseY;
    if (deltaX == 0 && deltaY == 0)
    {
        return;
    }

    MovePanelByDelta(deltaX, deltaY);
    g_panelDragLastMouseX = mouseX;
    g_panelDragLastMouseY = mouseY;
}

void OnHeaderMouseMove(MyGUI::Widget*, int left, int top)
{
    if (!g_panelDragging)
    {
        return;
    }

    OnHeaderMouseDrag(0, left, top, MyGUI::MouseButton::Left);
}

void OnHeaderMouseReleased(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    FinalizePanelDrag("drag_release");
}

void TickPanelDrag()
{
    if (!g_panelDragging || !g_panel)
    {
        return;
    }

    int mouseX = 0;
    int mouseY = 0;
    if (TryGetMousePosition(&mouseX, &mouseY))
    {
        const int deltaX = mouseX - g_panelDragLastMouseX;
        const int deltaY = mouseY - g_panelDragLastMouseY;
        MovePanelByDelta(deltaX, deltaY);
        g_panelDragLastMouseX = mouseX;
        g_panelDragLastMouseY = mouseY;
    }

    if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0)
    {
        FinalizePanelDrag("drag_release_poll");
    }
}

void ResetPanelWidgetPointers()
{
    g_panel = 0;
    g_headerFrame = 0;
    g_headerTitleText = 0;
    g_collapseButton = 0;
    g_bodyFrame = 0;
    g_targetSectionText = 0;
    g_targetNameText = 0;
    g_targetFactionText = 0;
    g_targetAlignmentText = 0;
    g_targetMembershipText = 0;
    g_targetStateText = 0;
    g_noTargetText = 0;
    g_healthTabButton = 0;
    g_teleportTabButton = 0;
    g_statesSectionText = 0;
    g_forceUnconsciousButton = 0;
    g_forcePlayingDeadButton = 0;
    g_limbDamageSectionText = 0;
    g_damageLeftArmButton = 0;
    g_damageRightArmButton = 0;
    g_damageLeftLegButton = 0;
    g_damageRightLegButton = 0;
    g_teleportSectionText = 0;
    g_teleportSelectedToCameraButton = 0;
    g_dangerousSectionText = 0;
    g_forceDyingButton = 0;
    g_statusText = 0;
}

void DestroyPanel()
{
    if (g_panel)
    {
        StorePanelRuntimePosition(ClampPanelCoordToViewport(g_panel->getCoord()));
    }

    MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
    if (gui && g_panel)
    {
        gui->destroyWidget(g_panel);
    }

    ResetPanelWidgetPointers();
    g_panelDragging = false;
    g_panelDragMoved = false;
    g_panelDragLastMouseX = 0;
    g_panelDragLastMouseY = 0;
    g_panelDragMovedDistance = 0;
    g_forceDyingArmed = false;
    g_forceDyingArmedAtMs = 0;
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
    if (keyUpper == "NONE")
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

void LoadConfig()
{
    g_pluginEnabled = true;
    g_loggingLevel = LoggingLevel_Info;
    g_togglePanelRequireCtrl = true;
    g_togglePanelRequireShift = true;
    g_togglePanelRequireAlt = false;
    g_togglePanelKey = kDefaultTogglePanelKey;
    g_confirmDangerousActions = true;
    g_panelHidden = false;
    g_panelCollapsed = false;

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
    std::string parsedString;

    if (TryParseJsonBoolByKey(configText, "enabled", &parsedBool))
    {
        g_pluginEnabled = parsedBool;
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

    RefreshHotkeyBinding();

    std::stringstream info;
    info << "mod config loaded enabled=" << (g_pluginEnabled ? "true" : "false")
         << " hotkey=\"" << g_hotkeyDisplay << "\""
         << " start_hidden=" << (g_panelHidden ? "true" : "false")
         << " start_collapsed=" << (g_panelCollapsed ? "true" : "false")
         << " confirm_dangerous_actions=" << (g_confirmDangerousActions ? "true" : "false");
    LogInfoLine(info.str());
}

bool IsAnyVirtualKeyDown(int primaryVk, int leftVk, int rightVk)
{
    return (GetAsyncKeyState(primaryVk) & 0x8000) != 0
        || (GetAsyncKeyState(leftVk) & 0x8000) != 0
        || (GetAsyncKeyState(rightVk) & 0x8000) != 0;
}

bool IsPanelToggleHotkeyDown()
{
    if (!g_hotkeyEnabled || g_hotkeyVirtualKey == 0)
    {
        return false;
    }

    if (g_togglePanelRequireCtrl && !IsAnyVirtualKeyDown(VK_CONTROL, VK_LCONTROL, VK_RCONTROL))
    {
        return false;
    }

    if (g_togglePanelRequireAlt && !IsAnyVirtualKeyDown(VK_MENU, VK_LMENU, VK_RMENU))
    {
        return false;
    }

    if (g_togglePanelRequireShift && !IsAnyVirtualKeyDown(VK_SHIFT, VK_LSHIFT, VK_RSHIFT))
    {
        return false;
    }

    return (GetAsyncKeyState(g_hotkeyVirtualKey) & 0x8000) != 0;
}

void LogPanelToggleEvent(bool visible, const char* source)
{
    std::stringstream line;
    line << "event=testkit_panel_toggled visible=" << (visible ? "true" : "false");
    if (source)
    {
        line << " source=\"" << source << "\"";
    }
    line << " hotkey=\"" << g_hotkeyDisplay << "\"";
    LogInfoLine(line.str());
}

void LogPanelCollapsedEvent(bool collapsed, const char* source)
{
    std::stringstream line;
    line << "event=testkit_panel_collapsed collapsed=" << (collapsed ? "true" : "false");
    if (source)
    {
        line << " source=\"" << source << "\"";
    }
    LogInfoLine(line.str());
}

void TogglePanelHidden(const char* source)
{
    g_panelHidden = !g_panelHidden;

    if (g_panelHidden)
    {
        ClearForceDyingArm("panel_hidden", false);
    }
    else
    {
        SetStatusMessage("Panel shown");
    }

    ApplyPanelLayout();
    LogPanelToggleEvent(!g_panelHidden, source);
}

void TogglePanelCollapsed(const char* source)
{
    g_panelCollapsed = !g_panelCollapsed;
    if (g_panelCollapsed)
    {
        ClearForceDyingArm("panel_collapsed", false);
    }

    UpdateCollapseButtonCaption();
    ApplyPanelLayout();
    SetStatusMessage(g_panelCollapsed ? "Panel collapsed" : "Panel expanded");
    LogPanelCollapsedEvent(g_panelCollapsed, source);
}

void TickPanelToggleHotkey()
{
    const bool hotkeyDown = IsPanelToggleHotkeyDown();
    if (hotkeyDown && !g_hotkeyPrevDown)
    {
        std::stringstream line;
        line << "event=testkit_hotkey_triggered hotkey=\"" << g_hotkeyDisplay << "\"";
        LogInfoLine(line.str());
        TogglePanelHidden("hotkey");
    }

    g_hotkeyPrevDown = hotkeyDown;
}

bool TryGetSelectedTarget(PlayerInterface* player, Character** outTarget)
{
    if (!player || !outTarget)
    {
        return false;
    }

    Character* target = 0;
    __try
    {
        if (player->selectedObject && player->selectedObject.isValid())
        {
            target = player->selectedObject.getCharacter();
        }

        if (!target)
        {
            target = player->selectedCharacter.getCharacter();
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (!target)
    {
        return false;
    }

    *outTarget = target;
    return true;
}

bool TryGetSelectedPlayerCharacter(PlayerInterface* player, Character** outTarget)
{
    if (!player || !outTarget)
    {
        return false;
    }

    Character* target = 0;
    __try
    {
        target = player->selectedCharacter.getCharacter();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (!target)
    {
        return false;
    }

    *outTarget = target;
    return true;
}

bool TryGetHoveredTarget(PlayerInterface* player, Character** outTarget)
{
    if (!player || !outTarget)
    {
        return false;
    }

    bool hasMouseRightTarget = false;
    RootObject* root = 0;
    __try
    {
        hasMouseRightTarget = player->mouseRightTargetSet;
        root = player->mouseRightTarget;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (!hasMouseRightTarget || !root)
    {
        return false;
    }

    Character* target = 0;
    __try
    {
        target = root->getHandle().getCharacter();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (!target)
    {
        return false;
    }

    *outTarget = target;
    return true;
}

bool TryGetConversationTargetFromSpeaker(Character* speaker, Character** outTarget)
{
    if (!speaker || !outTarget)
    {
        return false;
    }

    Dialogue* dialogue = 0;
    bool hasEnded = true;
    Character* target = 0;
    __try
    {
        dialogue = speaker->dialogue;
        if (!dialogue)
        {
            return false;
        }

        hasEnded = dialogue->conversationHasEndedPrettyMuch();
        if (hasEnded)
        {
            return false;
        }

        target = dialogue->getConversationTarget().getCharacter();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (!target)
    {
        return false;
    }

    *outTarget = target;
    return true;
}

bool TryGetConversationTarget(PlayerInterface* player, Character** outTarget)
{
    if (!player || !outTarget)
    {
        return false;
    }

    Character* selected = 0;
    if (TryGetSelectedPlayerCharacter(player, &selected) && TryGetConversationTargetFromSpeaker(selected, outTarget))
    {
        return true;
    }

    __try
    {
        const lektor<Character*>& playerCharacters = player->getAllPlayerCharacters();
        if (!playerCharacters.valid())
        {
            return false;
        }

        for (lektor<Character*>::const_iterator it = playerCharacters.begin(); it != playerCharacters.end(); ++it)
        {
            Character* speaker = *it;
            if (!speaker || speaker == selected)
            {
                continue;
            }

            if (TryGetConversationTargetFromSpeaker(speaker, outTarget))
            {
                return true;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    return false;
}

bool ResolveInspectionTarget(PlayerInterface* player, Character** outTarget, TargetSource* outSource)
{
    if (!outTarget || !outSource)
    {
        return false;
    }

    *outTarget = 0;
    *outSource = TargetSource_None;

    Character* target = 0;
    if (TryGetSelectedTarget(player, &target))
    {
        *outTarget = target;
        *outSource = TargetSource_Selected;
        return true;
    }

    if (TryGetHoveredTarget(player, &target))
    {
        *outTarget = target;
        *outSource = TargetSource_Hovered;
        return true;
    }

    if (TryGetConversationTarget(player, &target))
    {
        *outTarget = target;
        *outSource = TargetSource_Conversation;
        return true;
    }

    return false;
}

std::string SafeCharacterName(Character* target)
{
    if (!target)
    {
        return "Unknown";
    }

    const std::string name = target->getName();
    if (name.empty())
    {
        return "Unknown";
    }

    return name;
}

std::string SafeFactionName(Character* target)
{
    if (!target || !target->owner)
    {
        return "Unknown";
    }

    const std::string name = target->owner->getName();
    if (name.empty())
    {
        return "Unknown";
    }

    return name;
}

bool TryResolveMembershipLabel(Character* target, std::string* outLabel)
{
    if (!target || !outLabel)
    {
        return false;
    }

    __try
    {
        *outLabel = target->isWithThePlayer() ? "Squad" : "Non-squad";
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool TryResolveAlignmentLabel(PlayerInterface* player, Character* target, std::string* outLabel)
{
    if (!player || !target || !outLabel)
    {
        return false;
    }

    __try
    {
        if (target->isWithThePlayer())
        {
            *outLabel = "Ally";
            return true;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    __try
    {
        if (player->isEnemy(target))
        {
            *outLabel = "Enemy";
            return true;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    bool sawPlayerCharacter = false;
    bool sawAlly = false;
    __try
    {
        const lektor<Character*>& playerCharacters = player->getAllPlayerCharacters();
        if (playerCharacters.valid())
        {
            for (lektor<Character*>::const_iterator it = playerCharacters.begin(); it != playerCharacters.end(); ++it)
            {
                Character* playerCharacter = *it;
                if (!playerCharacter || playerCharacter == target)
                {
                    continue;
                }

                sawPlayerCharacter = true;
                if (target->isAlly(playerCharacter, true))
                {
                    sawAlly = true;
                    break;
                }
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (sawAlly)
    {
        *outLabel = "Ally";
        return true;
    }

    *outLabel = sawPlayerCharacter ? "Neutral" : "Unknown";
    return true;
}

bool TryResolveStateSummary(
    Character* target,
    std::string* outLabel,
    bool* unconsciousOut,
    bool* playingDeadOut,
    bool* dyingOut,
    bool* deadOut)
{
    if (!target || !outLabel || !unconsciousOut || !playingDeadOut || !dyingOut || !deadOut)
    {
        return false;
    }

    bool unconscious = false;
    bool playingDead = false;
    bool dying = false;
    bool dead = false;
    bool recoveryComa = false;
    __try
    {
        const ProneState proneState = target->_currentProneState;
        const bool deadByCharacter = target->isDead();
        const bool deadByMedicalMethod = target->medical.isDead();
        const bool deadByMedicalFlag = target->medical.dead;
        dead = deadByCharacter || deadByMedicalMethod || deadByMedicalFlag;

        unconscious = target->medical.unconcious || proneState == PS_KO;
        playingDead = (proneState == PS_PLAYING_DEAD);
        if (!dead && unconscious && !playingDead)
        {
            const bool isProbablyDying = target->medical.isProbablyDying();
            const bool dyingByTrauma = target->medical.isInBloodlossTrauma();
            const bool sub50Ko = target->medical.sub50KO;
            const float bloodLevel = target->medical.blood;
            const float pointOfNoReturn = target->medical.pointOfNoReturn();
            const bool dyingByBloodThreshold = (bloodLevel <= pointOfNoReturn);
            const bool dyingByProbablyLowBlood = (isProbablyDying && bloodLevel <= kProbablyDyingBloodMax);
            const bool dyingByActiveBleed =
                isProbablyDying
                && (target->medical.currentBleedRate > 0.0f || target->medical.extraBloodLossFromBodyparts > 0.0f);
            const bool recoveryComaByCannotWake = (!target->medical.canGetUpWakeUp() && sub50Ko);
            const bool knockoutTimerElapsed = (target->medical.knockoutTimer <= 0.0f);

            recoveryComa = recoveryComaByCannotWake
                && knockoutTimerElapsed
                && !isProbablyDying
                && !dyingByBloodThreshold
                && !dyingByTrauma
                && !dyingByActiveBleed
                && !dyingByProbablyLowBlood;

            dying = dyingByBloodThreshold || sub50Ko;
            if (recoveryComa)
            {
                dying = false;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    *unconsciousOut = unconscious;
    *playingDeadOut = playingDead;
    *dyingOut = dying;
    *deadOut = dead;

    if (dead)
    {
        *outLabel = "Dead";
    }
    else if (dying)
    {
        *outLabel = "Dying";
    }
    else if (recoveryComa)
    {
        *outLabel = "Recovery Coma";
    }
    else if (playingDead)
    {
        *outLabel = "Playing Dead";
    }
    else if (unconscious)
    {
        *outLabel = "Unconscious";
    }
    else
    {
        *outLabel = "None detected";
    }

    return true;
}

void SetActionButtonsEnabled(bool enabled)
{
    if (g_forceUnconsciousButton)
    {
        g_forceUnconsciousButton->setEnabled(enabled);
    }
    if (g_forcePlayingDeadButton)
    {
        g_forcePlayingDeadButton->setEnabled(enabled);
    }
    if (g_damageLeftArmButton)
    {
        g_damageLeftArmButton->setEnabled(enabled);
    }
    if (g_damageRightArmButton)
    {
        g_damageRightArmButton->setEnabled(enabled);
    }
    if (g_damageLeftLegButton)
    {
        g_damageLeftLegButton->setEnabled(enabled);
    }
    if (g_damageRightLegButton)
    {
        g_damageRightLegButton->setEnabled(enabled);
    }
    if (g_forceDyingButton)
    {
        g_forceDyingButton->setEnabled(enabled);
    }
}

void SetSelectionActionButtonsEnabled(bool enabled)
{
    if (g_teleportSelectedToCameraButton)
    {
        g_teleportSelectedToCameraButton->setEnabled(enabled);
    }
}

int GetSelectedCharacterCount(PlayerInterface* player)
{
    if (!player)
    {
        return 0;
    }

    int selectedCharacterCount = 0;

    __try
    {
        const ogre_unordered_set<hand>::type& selectedCharacters = player->selectedCharacters;
        ogre_unordered_set<hand>::type::const_iterator it = selectedCharacters.begin();
        for (; it != selectedCharacters.end(); ++it)
        {
            if (it->getCharacter())
            {
                ++selectedCharacterCount;
            }
        }

        if (selectedCharacterCount <= 0 && player->selectedCharacter.isValid() && player->selectedCharacter.getCharacter())
        {
            selectedCharacterCount = 1;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }

    return selectedCharacterCount;
}

void UpdateSelectionActionButtons(PlayerInterface* player)
{
    SetSelectionActionButtonsEnabled(GetSelectedCharacterCount(player) > 0);
}

void ApplyTargetSnapshotToUi(const TargetSnapshot& snapshot)
{
    if (!g_targetNameText || !g_targetFactionText || !g_targetAlignmentText
        || !g_targetMembershipText || !g_targetStateText || !g_noTargetText)
    {
        return;
    }

    if (!snapshot.hasTarget)
    {
        g_targetNameText->setCaption("Name: No target");
        g_targetFactionText->setCaption("Faction: Unknown");
        g_targetAlignmentText->setCaption("Alignment: Unknown");
        g_targetMembershipText->setCaption("Membership: Unknown");
        g_targetStateText->setCaption("State: Unknown");
        g_noTargetText->setCaption("No target - select a character");
        SetActionButtonsEnabled(false);
        return;
    }

    g_targetNameText->setCaption("Name: " + snapshot.name);
    g_targetFactionText->setCaption("Faction: " + snapshot.factionName);
    g_targetAlignmentText->setCaption("Alignment: " + snapshot.alignment);
    g_targetMembershipText->setCaption("Membership: " + snapshot.membership);
    g_targetStateText->setCaption("State: " + snapshot.stateLabel);
    g_noTargetText->setCaption(std::string("Source: ") + TargetSourceToUiLabel(snapshot.source));
    SetActionButtonsEnabled(true);
}

void LogTargetSnapshotIfChanged(const TargetSnapshot& snapshot)
{
    if (g_hasLastTargetSnapshot && AreTargetSnapshotsEqual(g_lastTargetSnapshot, snapshot))
    {
        return;
    }

    std::stringstream line;
    line << "event=testkit_target_snapshot target_present=" << (snapshot.hasTarget ? "true" : "false");
    if (snapshot.hasTarget)
    {
        line << " source=\"" << TargetSourceToLogLabel(snapshot.source) << "\""
             << " name=\"" << SanitizeLogValue(snapshot.name) << "\""
             << " faction=\"" << SanitizeLogValue(snapshot.factionName) << "\""
             << " alignment=\"" << snapshot.alignment << "\""
             << " membership=\"" << snapshot.membership << "\""
             << " state=\"" << snapshot.stateLabel << "\""
             << " unconscious=" << (snapshot.unconscious ? "true" : "false")
             << " playing_dead=" << (snapshot.playingDead ? "true" : "false")
             << " dying=" << (snapshot.dying ? "true" : "false")
             << " dead=" << (snapshot.dead ? "true" : "false");
    }
    LogDebugLine(line.str());
}

void BuildTargetSnapshot(PlayerInterface* player, Character* target, TargetSource source, TargetSnapshot* snapshotOut)
{
    if (!snapshotOut)
    {
        return;
    }

    ResetTargetSnapshot(snapshotOut);
    if (!target)
    {
        return;
    }

    snapshotOut->hasTarget = true;
    snapshotOut->source = source;
    snapshotOut->target = target;

    snapshotOut->name = SafeCharacterName(target);
    snapshotOut->factionName = SafeFactionName(target);

    if (!TryResolveAlignmentLabel(player, target, &snapshotOut->alignment))
    {
        snapshotOut->alignment = "Unknown";
    }
    if (!TryResolveMembershipLabel(target, &snapshotOut->membership))
    {
        snapshotOut->membership = "Unknown";
    }
    if (!TryResolveStateSummary(
            target,
            &snapshotOut->stateLabel,
            &snapshotOut->unconscious,
            &snapshotOut->playingDead,
            &snapshotOut->dying,
            &snapshotOut->dead))
    {
        snapshotOut->stateLabel = "Unknown";
        snapshotOut->unconscious = false;
        snapshotOut->playingDead = false;
        snapshotOut->dying = false;
        snapshotOut->dead = false;
    }
}

void UpdateTargetInspection(PlayerInterface* player)
{
    TargetSnapshot snapshot;
    ResetTargetSnapshot(&snapshot);

    Character* target = 0;
    TargetSource source = TargetSource_None;
    if (ResolveInspectionTarget(player, &target, &source))
    {
        BuildTargetSnapshot(player, target, source, &snapshot);
    }

    if (g_forceDyingArmed
        && (!snapshot.hasTarget
            || (g_hasLastTargetSnapshot
                && (snapshot.target != g_lastTargetSnapshot.target || snapshot.source != g_lastTargetSnapshot.source))))
    {
        ClearForceDyingArm(snapshot.hasTarget ? "target_changed" : "target_lost", true);
    }

    ApplyTargetSnapshotToUi(snapshot);
    UpdateSelectionActionButtons(player);
    LogTargetSnapshotIfChanged(snapshot);
    g_lastTargetSnapshot = snapshot;
    g_hasLastTargetSnapshot = true;
}

void LogActionRequested(const char* actionId)
{
    std::stringstream requested;
    requested << "event=testkit_action_requested action=\"" << actionId << "\"";
    if (g_hasLastTargetSnapshot && g_lastTargetSnapshot.hasTarget)
    {
        requested << " source=\"" << TargetSourceToLogLabel(g_lastTargetSnapshot.source) << "\""
                  << " target_name=\"" << SanitizeLogValue(g_lastTargetSnapshot.name) << "\"";
    }
    LogInfoLine(requested.str());
}

bool TryForceUnconscious(Character* target, bool alreadyUnconscious, float* knockoutTimerOut)
{
    if (!target)
    {
        return false;
    }

    if (knockoutTimerOut)
    {
        *knockoutTimerOut = 0.0f;
    }

    __try
    {
        MedicalSystem* medical = target->getMedical();
        if (!medical)
        {
            return false;
        }

        if (!alreadyUnconscious)
        {
            medical->knockout(0.0f);
        }
        medical->knockoutForceTimer(kForceUnconsciousDurationSeconds);

        if (knockoutTimerOut)
        {
            *knockoutTimerOut = medical->knockoutTimer;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    return true;
}

bool TryInvokeSelectedCharactersLayingLow(
    PlayerInterface* player,
    Character* target,
    bool* attemptedOut,
    bool* commandAcceptedOut)
{
    if (attemptedOut)
    {
        *attemptedOut = false;
    }
    if (commandAcceptedOut)
    {
        *commandAcceptedOut = false;
    }

    if (!player || !target)
    {
        return true;
    }

    __try
    {
        if (!target->isPlayerCharacter())
        {
            return true;
        }

        if (!player->selectedCharacter.isValid() || player->selectedCharacter.getCharacter() != target)
        {
            return true;
        }

        if (attemptedOut)
        {
            *attemptedOut = true;
        }
        if (commandAcceptedOut)
        {
            *commandAcceptedOut = player->selectedCharactersLayingLow();
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    return true;
}

bool TryForcePlayingDeadFallback(
    Character* target,
    bool alreadyUnconscious,
    bool alreadyPlayingDead,
    bool* knockedOutOut,
    float* knockoutTimerOut)
{
    if (knockedOutOut)
    {
        *knockedOutOut = false;
    }
    if (knockoutTimerOut)
    {
        *knockoutTimerOut = 0.0f;
    }

    if (!target)
    {
        return false;
    }

    __try
    {
        target->playerWantsMeToGetUp = false;

        if (!alreadyUnconscious && !alreadyPlayingDead)
        {
            MedicalSystem* medical = target->getMedical();
            if (!medical)
            {
                return false;
            }

            medical->knockout(0.0f);

            if (knockedOutOut)
            {
                *knockedOutOut = true;
            }
            if (knockoutTimerOut)
            {
                *knockoutTimerOut = medical->knockoutTimer;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    return true;
}

bool TryForceDying(
    Character* target,
    bool alreadyUnconscious,
    float* knockoutTimerOut,
    float* bloodOut,
    float* pointOfNoReturnOut,
    bool* usedSub50KoOut,
    float* currentBleedRateOut,
    float* extraBloodLossOut,
    bool* probablyDyingOut,
    bool* canWakeOut,
    bool* bloodlossTraumaOut)
{
    if (knockoutTimerOut)
    {
        *knockoutTimerOut = 0.0f;
    }
    if (bloodOut)
    {
        *bloodOut = 0.0f;
    }
    if (pointOfNoReturnOut)
    {
        *pointOfNoReturnOut = 0.0f;
    }
    if (usedSub50KoOut)
    {
        *usedSub50KoOut = false;
    }
    if (currentBleedRateOut)
    {
        *currentBleedRateOut = 0.0f;
    }
    if (extraBloodLossOut)
    {
        *extraBloodLossOut = 0.0f;
    }
    if (probablyDyingOut)
    {
        *probablyDyingOut = false;
    }
    if (canWakeOut)
    {
        *canWakeOut = false;
    }
    if (bloodlossTraumaOut)
    {
        *bloodlossTraumaOut = false;
    }

    if (!target)
    {
        return false;
    }

    __try
    {
        MedicalSystem* medical = target->getMedical();
        if (!medical)
        {
            return false;
        }

        const float pointOfNoReturn = medical->pointOfNoReturn();
        const float maxBlood = medical->getMaxBlood();
        const float aliveFloor = -maxBlood + kForceDyingAliveBloodMargin;
        float forcedBlood = pointOfNoReturn - kForceDyingBloodOffset;

        if (!alreadyUnconscious)
        {
            medical->knockout(0.0f);
        }

        if (forcedBlood <= aliveFloor)
        {
            forcedBlood = aliveFloor;
        }

        medical->blood = forcedBlood;
        medical->knockoutTimer = 0.0f;
        medical->currentBleedRate = 1.0f;
        medical->extraBloodLossFromBodyparts = 1.0f;
        medical->bloodlossTrauma = true;
        medical->reassessCollapseMode(false, true);
        target->playerWantsMeToGetUp = false;

        if (knockoutTimerOut)
        {
            *knockoutTimerOut = medical->knockoutTimer;
        }
        if (bloodOut)
        {
            *bloodOut = medical->blood;
        }
        if (pointOfNoReturnOut)
        {
            *pointOfNoReturnOut = pointOfNoReturn;
        }
        if (usedSub50KoOut)
        {
            *usedSub50KoOut = medical->sub50KO;
        }
        if (currentBleedRateOut)
        {
            *currentBleedRateOut = medical->currentBleedRate;
        }
        if (extraBloodLossOut)
        {
            *extraBloodLossOut = medical->extraBloodLossFromBodyparts;
        }
        if (probablyDyingOut)
        {
            *probablyDyingOut = medical->isProbablyDying();
        }
        if (canWakeOut)
        {
            *canWakeOut = medical->canGetUpWakeUp();
        }
        if (bloodlossTraumaOut)
        {
            *bloodlossTraumaOut = medical->isInBloodlossTrauma();
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    return true;
}

bool TryApplyLimbDamage(
    Character* target,
    MedicalSystem::HealthPartStatus::PartType partType,
    LeftRight side,
    float* partMaxHealthOut,
    float* beforeFleshOut,
    float* afterFleshOut,
    float* beforeFleshStunOut,
    float* afterFleshStunOut,
    float* beforeDerivedHealthOut,
    float* afterDerivedHealthOut,
    float* appliedDamageOut)
{
    if (partMaxHealthOut)
    {
        *partMaxHealthOut = 0.0f;
    }
    if (beforeFleshOut)
    {
        *beforeFleshOut = 0.0f;
    }
    if (afterFleshOut)
    {
        *afterFleshOut = 0.0f;
    }
    if (appliedDamageOut)
    {
        *appliedDamageOut = 0.0f;
    }
    if (beforeFleshStunOut)
    {
        *beforeFleshStunOut = 0.0f;
    }
    if (afterFleshStunOut)
    {
        *afterFleshStunOut = 0.0f;
    }
    if (beforeDerivedHealthOut)
    {
        *beforeDerivedHealthOut = 0.0f;
    }
    if (afterDerivedHealthOut)
    {
        *afterDerivedHealthOut = 0.0f;
    }

    if (!target)
    {
        return false;
    }

    __try
    {
        MedicalSystem* medical = target->getMedical();
        if (!medical)
        {
            return false;
        }

        MedicalSystem::HealthPartStatus* part = medical->getPart(partType, side);
        if (!part)
        {
            return false;
        }

        const float partMaxHealth = part->maxHealth();
        const float beforeFlesh = part->flesh;
        const float beforeFleshStun = part->fleshStun;
        const float beforeDerivedHealth = part->derivedFleshHealthPercent;
        float damageAmount = partMaxHealth * kLimbDamageFraction;
        if (damageAmount < kMinimumLimbDamageAmount)
        {
            damageAmount = kMinimumLimbDamageAmount;
        }

        __declspec(align(16)) unsigned char damageStorage[sizeof(Damages)] = {};
        Damages* damage = reinterpret_cast<Damages*>(damageStorage);
        damage->cut = 0.0f;
        damage->blunt = damageAmount;
        damage->pierce = 0.0f;
        damage->extraStun = 0.0f;
        damage->bleedMult = 0.0f;
        damage->armourPenetration = 0.0f;

        part->applyDamage(*damage);
        part->updateDerivedHealths();
        medical->updateDamageState();
        medical->reassessCollapseMode(false, false);

        if (partMaxHealthOut)
        {
            *partMaxHealthOut = partMaxHealth;
        }
        if (beforeFleshOut)
        {
            *beforeFleshOut = beforeFlesh;
        }
        if (afterFleshOut)
        {
            *afterFleshOut = part->flesh;
        }
        if (beforeFleshStunOut)
        {
            *beforeFleshStunOut = beforeFleshStun;
        }
        if (afterFleshStunOut)
        {
            *afterFleshStunOut = part->fleshStun;
        }
        if (beforeDerivedHealthOut)
        {
            *beforeDerivedHealthOut = beforeDerivedHealth;
        }
        if (afterDerivedHealthOut)
        {
            *afterDerivedHealthOut = part->derivedFleshHealthPercent;
        }
        if (appliedDamageOut)
        {
            *appliedDamageOut = damageAmount;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    return true;
}

bool TryTeleportSelectedCharactersToCamera(
    PlayerInterface* player,
    int* selectedCountOut,
    int* teleportedCountOut,
    Ogre::Vector3* destinationCenterOut)
{
    if (selectedCountOut)
    {
        *selectedCountOut = 0;
    }
    if (teleportedCountOut)
    {
        *teleportedCountOut = 0;
    }
    if (destinationCenterOut)
    {
        *destinationCenterOut = Ogre::Vector3(0.0f, 0.0f, 0.0f);
    }

    if (!player || !ou)
    {
        return false;
    }

    __try
    {
        const ogre_unordered_set<hand>::type& selectedCharacters = player->selectedCharacters;
        int selectedCharacterCount = 0;

        ogre_unordered_set<hand>::type::const_iterator countIt = selectedCharacters.begin();
        for (; countIt != selectedCharacters.end(); ++countIt)
        {
            Character* character = countIt->getCharacter();
            if (character)
            {
                ++selectedCharacterCount;
            }
        }

        const bool useSelectedCharacterFallback =
            selectedCharacterCount <= 0 && player->selectedCharacter.isValid() && player->selectedCharacter.getCharacter() != 0;
        if (useSelectedCharacterFallback)
        {
            selectedCharacterCount = 1;
        }

        if (selectedCountOut)
        {
            *selectedCountOut = selectedCharacterCount;
        }

        const Ogre::Vector3 destinationCenter = kTeleportDestinationCenter;
        if (destinationCenterOut)
        {
            *destinationCenterOut = destinationCenter;
        }

        Ogre::Vector3 resolvedDestination = destinationCenter;
        if (ou)
        {
            Ogre::Vector3 validatedDestination = resolvedDestination;
            if (ou->findValidSpawnPos(validatedDestination, resolvedDestination))
            {
                resolvedDestination = validatedDestination;
            }
        }

        int teleportedCount = 0;

        ogre_unordered_set<hand>::type::const_iterator teleportIt = selectedCharacters.begin();
        for (; teleportIt != selectedCharacters.end(); ++teleportIt)
        {
            Character* character = teleportIt->getCharacter();
            if (!character)
            {
                continue;
            }

            character->teleport(resolvedDestination - character->getPosition(), character->getOrientation());
            character->setRagdollNavmeshSafePos();
            ++teleportedCount;
        }

        if (useSelectedCharacterFallback)
        {
            Character* character = player->selectedCharacter.getCharacter();
            if (character)
            {
                character->teleport(resolvedDestination - character->getPosition(), character->getOrientation());
                character->setRagdollNavmeshSafePos();
                ++teleportedCount;
            }
        }

        if (teleportedCountOut)
        {
            *teleportedCountOut = teleportedCount;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    return true;
}

void ReportShellOnlyAction(const char* actionId, const char* actionLabel)
{
    LogActionRequested(actionId);

    if (!g_hasLastTargetSnapshot || !g_lastTargetSnapshot.hasTarget)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"" << actionId
               << "\" success=false reason=\"no_target\"";
        LogInfoLine(result.str());
        SetStatusMessage("No target - select a character");
        return;
    }

    std::stringstream result;
    result << "event=testkit_action_result action=\"" << actionId
           << "\" success=false reason=\"slice2_shell_only\""
           << " target_name=\"" << SanitizeLogValue(g_lastTargetSnapshot.name) << "\"";
    LogInfoLine(result.str());

    std::stringstream status;
    status << actionLabel << " unavailable in slice 2 shell";
    SetStatusMessage(status.str());
}

void OnCollapseButtonClicked(MyGUI::Widget*)
{
    TogglePanelCollapsed("button");
}

void OnForceUnconsciousButtonClicked(MyGUI::Widget*)
{
    const char* actionId = "force_unconscious";
    LogActionRequested(actionId);

    if (!g_hasLastTargetSnapshot || !g_lastTargetSnapshot.hasTarget || !g_lastTargetSnapshot.target)
    {
        LogInfoLine("event=testkit_action_result action=\"force_unconscious\" success=false reason=\"no_target\"");
        SetStatusMessage("No target - select a character");
        return;
    }

    if (g_lastTargetSnapshot.dead)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"force_unconscious\" success=false reason=\"target_dead\""
               << " target_name=\"" << SanitizeLogValue(g_lastTargetSnapshot.name) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Force Unconscious failed - target is dead");
        return;
    }

    const Character* const expectedTarget = g_lastTargetSnapshot.target;
    const std::string targetName = g_lastTargetSnapshot.name;
    const bool alreadyUnconscious = g_lastTargetSnapshot.unconscious;

    float knockoutTimer = 0.0f;
    if (!TryForceUnconscious(g_lastTargetSnapshot.target, alreadyUnconscious, &knockoutTimer))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"force_unconscious\" success=false reason=\"apply_failed\""
               << " target_name=\"" << SanitizeLogValue(targetName) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Force Unconscious failed - apply path unavailable");
        return;
    }

    bool observedUnconscious = false;
    if (g_lastPlayerInterface)
    {
        UpdateTargetInspection(g_lastPlayerInterface);
        observedUnconscious = g_hasLastTargetSnapshot
            && g_lastTargetSnapshot.hasTarget
            && g_lastTargetSnapshot.target == expectedTarget
            && g_lastTargetSnapshot.unconscious;
    }
    else
    {
        std::string stateLabel;
        bool unconscious = false;
        bool playingDead = false;
        bool dying = false;
        bool dead = false;
        observedUnconscious = TryResolveStateSummary(
                g_lastTargetSnapshot.target,
                &stateLabel,
                &unconscious,
                &playingDead,
                &dying,
                &dead)
            && unconscious;
    }

    std::stringstream result;
    result << "event=testkit_action_result action=\"force_unconscious\" success="
           << (observedUnconscious ? "true" : "false")
           << " target_name=\"" << SanitizeLogValue(targetName) << "\""
           << " already_unconscious=" << (alreadyUnconscious ? "true" : "false")
           << " observed_unconscious=" << (observedUnconscious ? "true" : "false")
           << " knockout_timer=" << knockoutTimer;
    if (!observedUnconscious)
    {
        result << " reason=\"not_observed_after_apply\"";
    }
    LogInfoLine(result.str());

    std::stringstream status;
    if (observedUnconscious)
    {
        status << (alreadyUnconscious ? "Force Unconscious refreshed for " : "Force Unconscious applied to ")
               << targetName;
    }
    else
    {
        status << "Force Unconscious requested for " << targetName << " - no KO readback yet";
    }
    SetStatusMessage(status.str());
}

void OnForceUnconsciousButtonPressed(MyGUI::Widget* widget, int left, int top, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnForceUnconsciousButtonClicked(0);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnForcePlayingDeadButtonClicked(MyGUI::Widget*)
{
    const char* actionId = "force_playing_dead";
    LogActionRequested(actionId);

    if (!g_hasLastTargetSnapshot || !g_lastTargetSnapshot.hasTarget || !g_lastTargetSnapshot.target)
    {
        LogInfoLine("event=testkit_action_result action=\"force_playing_dead\" success=false reason=\"no_target\"");
        SetStatusMessage("No target - select a character");
        return;
    }

    if (g_lastTargetSnapshot.dead)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"force_playing_dead\" success=false reason=\"target_dead\""
               << " target_name=\"" << SanitizeLogValue(g_lastTargetSnapshot.name) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Force Playing Dead failed - target is dead");
        return;
    }

    const Character* const expectedTarget = g_lastTargetSnapshot.target;
    const std::string targetName = g_lastTargetSnapshot.name;
    const bool alreadyPlayingDead = g_lastTargetSnapshot.playingDead;
    const bool alreadyUnconscious = g_lastTargetSnapshot.unconscious;

    if (alreadyPlayingDead)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"force_playing_dead\" success=true"
               << " target_name=\"" << SanitizeLogValue(targetName) << "\""
               << " already_playing_dead=true";
        LogInfoLine(result.str());
        SetStatusMessage("Force Playing Dead confirmed for " + targetName);
        return;
    }

    bool selectionPathAttempted = false;
    bool selectionCommandAccepted = false;
    if (!TryInvokeSelectedCharactersLayingLow(
            g_lastPlayerInterface,
            g_lastTargetSnapshot.target,
            &selectionPathAttempted,
            &selectionCommandAccepted))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"force_playing_dead\" success=false reason=\"selection_path_failed\""
               << " target_name=\"" << SanitizeLogValue(targetName) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Force Playing Dead failed - selection path crashed");
        return;
    }

    bool knockedOut = false;
    float knockoutTimer = 0.0f;
    if (!selectionCommandAccepted
        && !TryForcePlayingDeadFallback(
                g_lastTargetSnapshot.target,
                alreadyUnconscious,
                alreadyPlayingDead,
                &knockedOut,
                &knockoutTimer))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"force_playing_dead\" success=false reason=\"apply_failed\""
               << " target_name=\"" << SanitizeLogValue(targetName) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Force Playing Dead failed - apply path unavailable");
        return;
    }

    bool observedPlayingDead = false;
    bool observedUnconscious = false;
    if (g_lastPlayerInterface)
    {
        UpdateTargetInspection(g_lastPlayerInterface);
        observedPlayingDead = g_hasLastTargetSnapshot
            && g_lastTargetSnapshot.hasTarget
            && g_lastTargetSnapshot.target == expectedTarget
            && g_lastTargetSnapshot.playingDead;
        observedUnconscious = g_hasLastTargetSnapshot
            && g_lastTargetSnapshot.hasTarget
            && g_lastTargetSnapshot.target == expectedTarget
            && g_lastTargetSnapshot.unconscious;
    }
    else
    {
        std::string stateLabel;
        bool unconscious = false;
        bool playingDead = false;
        bool dying = false;
        bool dead = false;
        if (TryResolveStateSummary(
                g_lastTargetSnapshot.target,
                &stateLabel,
                &unconscious,
                &playingDead,
                &dying,
                &dead))
        {
            observedPlayingDead = playingDead;
            observedUnconscious = unconscious;
        }
    }

    std::stringstream result;
    result << "event=testkit_action_result action=\"force_playing_dead\" success="
           << (observedPlayingDead ? "true" : "false")
           << " target_name=\"" << SanitizeLogValue(targetName) << "\""
           << " selection_path_attempted=" << (selectionPathAttempted ? "true" : "false")
           << " selection_command_accepted=" << (selectionCommandAccepted ? "true" : "false")
           << " fallback_knockout=" << (knockedOut ? "true" : "false")
           << " observed_playing_dead=" << (observedPlayingDead ? "true" : "false")
           << " observed_unconscious=" << (observedUnconscious ? "true" : "false")
           << " knockout_timer=" << knockoutTimer;
    if (!observedPlayingDead)
    {
        result << " reason=\"not_observed_after_apply\"";
    }
    LogInfoLine(result.str());

    if (observedPlayingDead)
    {
        SetStatusMessage("Force Playing Dead applied to " + targetName);
        return;
    }

    if (observedUnconscious)
    {
        SetStatusMessage("Force Playing Dead requested for " + targetName + " - waiting for recovery into play dead");
        return;
    }

    SetStatusMessage("Force Playing Dead requested for " + targetName + " - no readback yet");
}

void OnForcePlayingDeadButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnForcePlayingDeadButtonClicked(0);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnDamageLimbButtonClicked(
    const char* actionId,
    const char* actionLabel,
    MedicalSystem::HealthPartStatus::PartType partType,
    LeftRight side)
{
    LogActionRequested(actionId);

    if (!g_hasLastTargetSnapshot || !g_lastTargetSnapshot.hasTarget || !g_lastTargetSnapshot.target)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"" << actionId << "\" success=false reason=\"no_target\"";
        LogInfoLine(result.str());
        SetStatusMessage("No target - select a character");
        return;
    }

    if (g_lastTargetSnapshot.dead)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"" << actionId << "\" success=false reason=\"target_dead\""
               << " target_name=\"" << SanitizeLogValue(g_lastTargetSnapshot.name) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage(std::string(actionLabel) + " failed - target is dead");
        return;
    }

    const Character* const expectedTarget = g_lastTargetSnapshot.target;
    const std::string targetName = g_lastTargetSnapshot.name;

    float partMaxHealth = 0.0f;
    float beforeFlesh = 0.0f;
    float afterFlesh = 0.0f;
    float beforeFleshStun = 0.0f;
    float afterFleshStun = 0.0f;
    float beforeDerivedHealth = 0.0f;
    float afterDerivedHealth = 0.0f;
    float appliedDamage = 0.0f;
    if (!TryApplyLimbDamage(
            g_lastTargetSnapshot.target,
            partType,
            side,
            &partMaxHealth,
            &beforeFlesh,
            &afterFlesh,
            &beforeFleshStun,
            &afterFleshStun,
            &beforeDerivedHealth,
            &afterDerivedHealth,
            &appliedDamage))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"" << actionId << "\" success=false reason=\"apply_failed\""
               << " target_name=\"" << SanitizeLogValue(targetName) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage(std::string(actionLabel) + " failed - apply path unavailable");
        return;
    }

    std::string observedStateLabel = "Unknown";
    bool observedUnconscious = false;
    bool observedPlayingDead = false;
    bool observedDying = false;
    bool observedDead = false;
    if (g_lastPlayerInterface)
    {
        UpdateTargetInspection(g_lastPlayerInterface);
        if (g_hasLastTargetSnapshot
            && g_lastTargetSnapshot.hasTarget
            && g_lastTargetSnapshot.target == expectedTarget)
        {
            observedStateLabel = g_lastTargetSnapshot.stateLabel;
            observedUnconscious = g_lastTargetSnapshot.unconscious;
            observedPlayingDead = g_lastTargetSnapshot.playingDead;
            observedDying = g_lastTargetSnapshot.dying;
            observedDead = g_lastTargetSnapshot.dead;
        }
    }
    else
    {
        TryResolveStateSummary(
            g_lastTargetSnapshot.target,
            &observedStateLabel,
            &observedUnconscious,
            &observedPlayingDead,
            &observedDying,
            &observedDead);
    }

    const bool fleshChanged = (beforeFlesh - afterFlesh > kFloatChangeEpsilon) || (afterFlesh - beforeFlesh > kFloatChangeEpsilon);
    const bool fleshStunChanged =
        (beforeFleshStun - afterFleshStun > kFloatChangeEpsilon) || (afterFleshStun - beforeFleshStun > kFloatChangeEpsilon);
    const bool derivedHealthChanged =
        (beforeDerivedHealth - afterDerivedHealth > kFloatChangeEpsilon)
        || (afterDerivedHealth - beforeDerivedHealth > kFloatChangeEpsilon);
    const bool observedEffect =
        fleshChanged || fleshStunChanged || derivedHealthChanged || observedUnconscious || observedDying || observedDead;

    std::stringstream result;
    result << "event=testkit_action_result action=\"" << actionId << "\" success="
           << (observedEffect ? "true" : "false")
           << " target_name=\"" << SanitizeLogValue(targetName) << "\""
           << " part_max_health=" << partMaxHealth
           << " before_flesh=" << beforeFlesh
           << " after_flesh=" << afterFlesh
           << " before_flesh_stun=" << beforeFleshStun
           << " after_flesh_stun=" << afterFleshStun
           << " before_derived_health=" << beforeDerivedHealth
           << " after_derived_health=" << afterDerivedHealth
           << " applied_damage=" << appliedDamage
           << " observed_state=\"" << observedStateLabel << "\""
           << " observed_unconscious=" << (observedUnconscious ? "true" : "false")
           << " observed_playing_dead=" << (observedPlayingDead ? "true" : "false")
           << " observed_dying=" << (observedDying ? "true" : "false")
           << " observed_dead=" << (observedDead ? "true" : "false");
    if (!observedEffect)
    {
        result << " reason=\"not_observed_after_apply\"";
    }
    LogInfoLine(result.str());

    if (observedEffect)
    {
        SetStatusMessage(std::string(actionLabel) + " applied to " + targetName);
        return;
    }

    SetStatusMessage(std::string(actionLabel) + " requested for " + targetName + " - no limb change readback yet");
}

void OnDamageLeftArmButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnDamageLimbButtonClicked(
        "damage_left_arm",
        "Damage Left Arm",
        MedicalSystem::HealthPartStatus::PART_ARM,
        SIDE_LEFT);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnDamageRightArmButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnDamageLimbButtonClicked(
        "damage_right_arm",
        "Damage Right Arm",
        MedicalSystem::HealthPartStatus::PART_ARM,
        SIDE_RIGHT);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnDamageLeftLegButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnDamageLimbButtonClicked(
        "damage_left_leg",
        "Damage Left Leg",
        MedicalSystem::HealthPartStatus::PART_LEG,
        SIDE_LEFT);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnDamageRightLegButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnDamageLimbButtonClicked(
        "damage_right_leg",
        "Damage Right Leg",
        MedicalSystem::HealthPartStatus::PART_LEG,
        SIDE_RIGHT);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnTeleportSelectedToCameraButtonClicked(MyGUI::Widget*)
{
    std::stringstream requested;
    requested << "event=testkit_action_requested action=\"teleport_selected_to_test_spot\"";
    LogInfoLine(requested.str());

    if (!g_lastPlayerInterface)
    {
        LogInfoLine("event=testkit_action_result action=\"teleport_selected_to_test_spot\" success=false reason=\"no_player_interface\"");
        SetStatusMessage("Teleport failed - player interface unavailable");
        return;
    }

    int selectedCount = 0;
    int teleportedCount = 0;
    Ogre::Vector3 destinationCenter(0.0f, 0.0f, 0.0f);
    if (!TryTeleportSelectedCharactersToCamera(
            g_lastPlayerInterface,
            &selectedCount,
            &teleportedCount,
            &destinationCenter))
    {
        LogInfoLine("event=testkit_action_result action=\"teleport_selected_to_test_spot\" success=false reason=\"apply_failed\"");
        SetStatusMessage(std::string("Teleport to ") + kTeleportDestinationLabel + " failed - apply path unavailable");
        return;
    }

    if (selectedCount <= 0)
    {
        LogInfoLine("event=testkit_action_result action=\"teleport_selected_to_test_spot\" success=false reason=\"no_selection\"");
        SetStatusMessage(std::string("No selected characters to teleport to ") + kTeleportDestinationLabel);
        return;
    }

    std::stringstream result;
    result << "event=testkit_action_result action=\"teleport_selected_to_test_spot\" success="
           << (teleportedCount > 0 ? "true" : "false")
           << " selected_count=" << selectedCount
           << " teleported_count=" << teleportedCount
           << " destination_x=" << destinationCenter.x
           << " destination_y=" << destinationCenter.y
           << " destination_z=" << destinationCenter.z;
    if (teleportedCount <= 0)
    {
        result << " reason=\"not_observed_after_apply\"";
    }
    LogInfoLine(result.str());

    if (teleportedCount == selectedCount)
    {
        std::stringstream status;
        status << "Teleported " << teleportedCount << " selected character(s) to " << kTeleportDestinationLabel;
        SetStatusMessage(status.str());
    }
    else if (teleportedCount > 0)
    {
        std::stringstream status;
        status << "Teleported " << teleportedCount << " of " << selectedCount << " selected characters to "
               << kTeleportDestinationLabel;
        SetStatusMessage(status.str());
    }
    else
    {
        SetStatusMessage(std::string("Teleport to ") + kTeleportDestinationLabel + " requested - no selected characters moved");
    }

    UpdateTargetInspection(g_lastPlayerInterface);
}

void OnTeleportSelectedToCameraButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnTeleportSelectedToCameraButtonClicked(0);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void SetActivePanelTab(PanelTab tab)
{
    if (g_activePanelTab == tab)
    {
        return;
    }

    g_activePanelTab = tab;
    ApplyPanelLayout();
}

void OnHealthTabButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    SetActivePanelTab(PanelTab_Health);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnTeleportTabButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    SetActivePanelTab(PanelTab_Teleport);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnForceDyingButtonClicked(MyGUI::Widget*)
{
    if (!g_hasLastTargetSnapshot || !g_lastTargetSnapshot.hasTarget || !g_lastTargetSnapshot.target)
    {
        LogInfoLine("event=testkit_action_result action=\"force_dying\" success=false reason=\"no_target\"");
        SetStatusMessage("No target - select a character");
        return;
    }

    if (g_lastTargetSnapshot.dead)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"force_dying\" success=false reason=\"target_dead\""
               << " target_name=\"" << SanitizeLogValue(g_lastTargetSnapshot.name) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Force Dying failed - target is already dead");
        return;
    }

    if (g_confirmDangerousActions && !g_forceDyingArmed)
    {
        g_forceDyingArmed = true;
        g_forceDyingArmedAtMs = GetTickCount();
        UpdateForceDyingButtonCaption();

        LogInfoLine("event=testkit_action_arm action=\"force_dying\" armed=true");
        SetStatusMessage("Force Dying armed - click again to confirm");
        return;
    }

    LogActionRequested("force_dying");

    const Character* const expectedTarget = g_lastTargetSnapshot.target;
    const std::string targetName = g_lastTargetSnapshot.name;
    const bool alreadyUnconscious = g_lastTargetSnapshot.unconscious;
    ClearForceDyingArm("confirmed", false);

    float knockoutTimer = 0.0f;
    float bloodLevel = 0.0f;
    float pointOfNoReturn = 0.0f;
    bool usedSub50Ko = false;
    float currentBleedRate = 0.0f;
    float extraBloodLoss = 0.0f;
    bool probablyDying = false;
    bool canWake = false;
    bool bloodlossTrauma = false;
    if (!TryForceDying(
            g_lastTargetSnapshot.target,
            alreadyUnconscious,
            &knockoutTimer,
            &bloodLevel,
            &pointOfNoReturn,
            &usedSub50Ko,
            &currentBleedRate,
            &extraBloodLoss,
            &probablyDying,
            &canWake,
            &bloodlossTrauma))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"force_dying\" success=false reason=\"apply_failed\""
               << " target_name=\"" << SanitizeLogValue(targetName) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Force Dying failed - apply path unavailable");
        return;
    }

    bool observedDying = false;
    if (g_lastPlayerInterface)
    {
        UpdateTargetInspection(g_lastPlayerInterface);
        observedDying = g_hasLastTargetSnapshot
            && g_lastTargetSnapshot.hasTarget
            && g_lastTargetSnapshot.target == expectedTarget
            && g_lastTargetSnapshot.dying;
    }
    else
    {
        std::string stateLabel;
        bool unconscious = false;
        bool playingDead = false;
        bool dying = false;
        bool dead = false;
        observedDying = TryResolveStateSummary(
                g_lastTargetSnapshot.target,
                &stateLabel,
                &unconscious,
                &playingDead,
                &dying,
                &dead)
            && dying;
    }

    std::stringstream result;
    result << "event=testkit_action_result action=\"force_dying\" success="
           << (observedDying ? "true" : "false")
           << " target_name=\"" << SanitizeLogValue(targetName) << "\""
           << " observed_dying=" << (observedDying ? "true" : "false")
           << " knockout_timer=" << knockoutTimer
           << " blood=" << bloodLevel
           << " point_of_no_return=" << pointOfNoReturn
           << " used_sub50ko_fallback=" << (usedSub50Ko ? "true" : "false")
           << " current_bleed_rate=" << currentBleedRate
           << " extra_blood_loss=" << extraBloodLoss
           << " probably_dying=" << (probablyDying ? "true" : "false")
           << " can_wake=" << (canWake ? "true" : "false")
           << " bloodloss_trauma=" << (bloodlossTrauma ? "true" : "false");
    if (!observedDying)
    {
        result << " reason=\"not_observed_after_apply\"";
    }
    LogInfoLine(result.str());

    if (observedDying)
    {
        SetStatusMessage("Force Dying applied to " + targetName);
        return;
    }

    SetStatusMessage("Force Dying requested for " + targetName + " - no dying readback yet");
}

void OnForceDyingButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnForceDyingButtonClicked(0);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void TickForceDyingArmTimeout()
{
    if (!g_forceDyingArmed)
    {
        return;
    }

    const DWORD nowMs = GetTickCount();
    if (nowMs - g_forceDyingArmedAtMs < kDangerArmTimeoutMs)
    {
        return;
    }

    ClearForceDyingArm("timeout", true);
}

void ConfigureTextWidget(MyGUI::TextBox* widget)
{
    if (!widget)
    {
        return;
    }

    widget->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
}

bool HasAllPanelWidgets()
{
    return g_panel
        && g_headerFrame
        && g_headerTitleText
        && g_collapseButton
        && g_bodyFrame
        && g_targetSectionText
        && g_targetNameText
        && g_targetFactionText
        && g_targetAlignmentText
        && g_targetMembershipText
        && g_targetStateText
        && g_noTargetText
        && g_healthTabButton
        && g_teleportTabButton
        && g_statesSectionText
        && g_forceUnconsciousButton
        && g_forcePlayingDeadButton
        && g_limbDamageSectionText
        && g_damageLeftArmButton
        && g_damageRightArmButton
        && g_damageLeftLegButton
        && g_damageRightLegButton
        && g_teleportSectionText
        && g_teleportSelectedToCameraButton
        && g_dangerousSectionText
        && g_forceDyingButton
        && g_statusText;
}

void InitializePanelWidgets()
{
    if (!HasAllPanelWidgets())
    {
        return;
    }

    g_headerFrame->setCaption("");
    g_headerFrame->setEnabled(true);
    g_headerFrame->setNeedMouseFocus(true);
    g_headerFrame->setNeedKeyFocus(true);
    g_panel->setNeedMouseFocus(true);
    g_headerTitleText->setCaption("Emkejs Test Kit");
    g_headerTitleText->setNeedMouseFocus(false);
    ConfigureTextWidget(g_headerTitleText);

    g_bodyFrame->setCaption("");
    g_bodyFrame->setEnabled(false);
    g_bodyFrame->setNeedMouseFocus(true);

    ConfigureTextWidget(g_targetSectionText);
    ConfigureTextWidget(g_targetNameText);
    ConfigureTextWidget(g_targetFactionText);
    ConfigureTextWidget(g_targetAlignmentText);
    ConfigureTextWidget(g_targetMembershipText);
    ConfigureTextWidget(g_targetStateText);
    ConfigureTextWidget(g_noTargetText);
    ConfigureTextWidget(g_statesSectionText);
    ConfigureTextWidget(g_limbDamageSectionText);
    ConfigureTextWidget(g_teleportSectionText);
    ConfigureTextWidget(g_dangerousSectionText);
    ConfigureTextWidget(g_statusText);

    g_targetSectionText->setCaption("Target");
    g_targetNameText->setCaption("Name: Pending target inspection");
    g_targetFactionText->setCaption("Faction: Unknown");
    g_targetAlignmentText->setCaption("Alignment: Unknown");
    g_targetMembershipText->setCaption("Membership: Unknown");
    g_targetStateText->setCaption("State: Unknown");
    g_noTargetText->setCaption("No target - select a character");
    UpdatePanelTabButtonCaptions();
    g_statesSectionText->setCaption("States");
    g_limbDamageSectionText->setCaption("Limb Damage");
    g_teleportSectionText->setCaption("Teleport");
    g_dangerousSectionText->setCaption("Dangerous");

    g_forceUnconsciousButton->setCaption("Force Unconscious");
    g_forcePlayingDeadButton->setCaption("Force Playing Dead");
    g_damageLeftArmButton->setCaption("Damage Left Arm");
    g_damageRightArmButton->setCaption("Damage Right Arm");
    g_damageLeftLegButton->setCaption("Damage Left Leg");
    g_damageRightLegButton->setCaption("Damage Right Leg");
    g_teleportSelectedToCameraButton->setCaption(std::string("Teleport Selected To ") + kTeleportDestinationLabel);
    UpdateForceDyingButtonCaption();
    UpdateCollapseButtonCaption();
    RefreshStatusWidget();
    SetActionButtonsEnabled(false);
    SetSelectionActionButtonsEnabled(false);

    g_collapseButton->eventMouseButtonClick += MyGUI::newDelegate(&OnCollapseButtonClicked);
    g_healthTabButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnHealthTabButtonPressed);
    g_teleportTabButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnTeleportTabButtonPressed);
    g_forceUnconsciousButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnForceUnconsciousButtonPressed);
    g_forcePlayingDeadButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnForcePlayingDeadButtonPressed);
    g_damageLeftArmButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnDamageLeftArmButtonPressed);
    g_damageRightArmButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnDamageRightArmButtonPressed);
    g_damageLeftLegButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnDamageLeftLegButtonPressed);
    g_damageRightLegButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnDamageRightLegButtonPressed);
    g_teleportSelectedToCameraButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnTeleportSelectedToCameraButtonPressed);
    g_forceDyingButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnForceDyingButtonPressed);
    g_headerFrame->eventMouseButtonPressed += MyGUI::newDelegate(&OnHeaderMousePressed);
    g_headerFrame->eventMouseDrag += MyGUI::newDelegate(&OnHeaderMouseDrag);
    g_headerFrame->eventMouseMove += MyGUI::newDelegate(&OnHeaderMouseMove);
    g_headerFrame->eventMouseButtonReleased += MyGUI::newDelegate(&OnHeaderMouseReleased);

    TargetSnapshot snapshot;
    ResetTargetSnapshot(&snapshot);
    ApplyTargetSnapshotToUi(snapshot);
}

void CreatePanelWidgets()
{
    MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
    if (!gui)
    {
        return;
    }

    const MyGUI::IntCoord panelCoord = ResolvePanelCoord();

    g_panel = gui->createWidget<MyGUI::Widget>(
        "PanelEmpty",
        panelCoord,
        MyGUI::Align::Default,
        "Main");
    if (!g_panel)
    {
        g_panel = gui->createWidget<MyGUI::Widget>(
            "PanelEmpty",
            panelCoord,
            MyGUI::Align::Default,
            "Overlapped");
    }

    if (!g_panel)
    {
        if (!g_loggedPanelCreateFailure)
        {
            LogErrorLine("failed to create panel root");
            g_loggedPanelCreateFailure = true;
        }
        return;
    }

    g_headerFrame = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(0, 0, kPanelWidth, 38),
        MyGUI::Align::Default);
    g_headerTitleText = g_headerFrame->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(12, 8, kPanelWidth - 116, 22),
        MyGUI::Align::Default);
    g_collapseButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(kPanelWidth - 96, 4, 84, 30),
        MyGUI::Align::Default);
    g_bodyFrame = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(0, 40, kPanelWidth, kPanelExpandedHeight - 40),
        MyGUI::Align::Default);
    g_targetSectionText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(14, 52, kPanelWidth - 28, 18),
        MyGUI::Align::Default);
    g_targetNameText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(20, 74, 156, 18),
        MyGUI::Align::Default);
    g_targetFactionText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(184, 74, 156, 18),
        MyGUI::Align::Default);
    g_targetAlignmentText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(20, 96, 156, 18),
        MyGUI::Align::Default);
    g_targetMembershipText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(184, 96, 156, 18),
        MyGUI::Align::Default);
    g_targetStateText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(20, 118, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_noTargetText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(20, 140, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_healthTabButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(20, 170, 156, 28),
        MyGUI::Align::Default);
    g_teleportTabButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(184, 170, 156, 28),
        MyGUI::Align::Default);
    g_statesSectionText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(14, 208, kPanelWidth - 28, 18),
        MyGUI::Align::Default);
    g_forceUnconsciousButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(20, 230, kPanelWidth - 40, 28),
        MyGUI::Align::Default);
    g_forcePlayingDeadButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(20, 264, kPanelWidth - 40, 28),
        MyGUI::Align::Default);
    g_limbDamageSectionText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(14, 298, kPanelWidth - 28, 18),
        MyGUI::Align::Default);
    g_damageLeftArmButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(20, 320, 156, 28),
        MyGUI::Align::Default);
    g_damageRightArmButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(184, 320, 156, 28),
        MyGUI::Align::Default);
    g_damageLeftLegButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(20, 354, 156, 28),
        MyGUI::Align::Default);
    g_damageRightLegButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(184, 354, 156, 28),
        MyGUI::Align::Default);
    g_teleportSectionText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(14, 208, kPanelWidth - 28, 18),
        MyGUI::Align::Default);
    g_teleportSelectedToCameraButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(20, 230, kPanelWidth - 40, 28),
        MyGUI::Align::Default);
    g_dangerousSectionText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(14, 394, kPanelWidth - 28, 18),
        MyGUI::Align::Default);
    g_forceDyingButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(20, 416, kPanelWidth - 40, 28),
        MyGUI::Align::Default);
    g_statusText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(20, 514, kPanelWidth - 40, 18),
        MyGUI::Align::Default);

    if (!HasAllPanelWidgets())
    {
        DestroyPanel();
        if (!g_loggedPanelCreateFailure)
        {
            LogErrorLine("failed to create panel widgets");
            g_loggedPanelCreateFailure = true;
        }
        return;
    }

    InitializePanelWidgets();
    ApplyPanelLayout();
    g_loggedPanelCreateFailure = false;
    LogInfoLine(std::string("event=testkit_panel_created visible=") + (!g_panelHidden ? "true" : "false"));
}

void EnsurePanel(PlayerInterface* thisptr)
{
    g_lastPlayerInterface = thisptr;

    if (!g_panel)
    {
        CreatePanelWidgets();
    }

    TickPanelDrag();
    TickForceDyingArmTimeout();
    UpdateCollapseButtonCaption();
    UpdateForceDyingButtonCaption();
    ApplyPanelLayout();

    if (!g_panelHidden && !g_panelCollapsed)
    {
        UpdateTargetInspection(thisptr);
    }
}

void OnSaveLoadTransitionStart(const char* source)
{
    if (source)
    {
        std::stringstream line;
        line << "event=testkit_panel_destroyed reason=\"" << source << "\"";
        LogInfoLine(line.str());
    }

    DestroyPanel();
    g_lastPlayerInterface = 0;
    g_hotkeyPrevDown = false;
    g_lastStatusMessage = "Ready";
    ResetTargetSnapshot(&g_lastTargetSnapshot);
    g_hasLastTargetSnapshot = false;
}

void PlayerInterface_updateUT_hook(PlayerInterface* thisptr)
{
    PlayerInterface_updateUT_orig(thisptr);

    TickPanelToggleHotkey();
    EnsurePanel(thisptr);
}

void SaveManager_loadByInfo_hook(SaveManager* thisptr, const SaveInfo& saveInfo, bool resetPos)
{
    OnSaveLoadTransitionStart("SaveManager::load(saveInfo,bool)");
    if (SaveManager_loadByInfo_orig)
    {
        SaveManager_loadByInfo_orig(thisptr, saveInfo, resetPos);
    }
}

void SaveManager_loadByName_hook(SaveManager* thisptr, const std::string& saveName)
{
    OnSaveLoadTransitionStart("SaveManager::load(name)");
    if (SaveManager_loadByName_orig)
    {
        SaveManager_loadByName_orig(thisptr, saveName);
    }
}
}

__declspec(dllexport) void startPlugin()
{
    LogInfoLine("startPlugin()");

    KenshiLib::BinaryVersion versionInfo = KenshiLib::GetKenshiVersion();
    if (!IsSupportedVersion(versionInfo))
    {
        std::stringstream error;
        error << "unsupported Kenshi version/platform"
              << " version=" << versionInfo.GetVersion()
              << " platform=" << versionInfo.GetPlatform();
        LogErrorLine(error.str());
        return;
    }

    std::stringstream versionLine;
    versionLine << "supported Kenshi version detected: " << versionInfo.GetVersion();
    LogInfoLine(versionLine.str());

    LoadConfig();
    if (!g_pluginEnabled)
    {
        LogInfoLine("plugin disabled by config");
        return;
    }

    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
        KenshiLib::GetRealAddress(&PlayerInterface::updateUT),
        PlayerInterface_updateUT_hook,
        &PlayerInterface_updateUT_orig))
    {
        LogErrorLine("Could not hook PlayerInterface::updateUT");
        return;
    }

    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
        KenshiLib::GetRealAddress(static_cast<void (SaveManager::*)(const SaveInfo&, bool)>(&SaveManager::load)),
        SaveManager_loadByInfo_hook,
        &SaveManager_loadByInfo_orig))
    {
        LogWarnLine("Could not hook SaveManager::load(SaveInfo,bool); panel teardown on load is reduced");
    }

    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
        KenshiLib::GetRealAddress(static_cast<void (SaveManager::*)(const std::string&)>(&SaveManager::load)),
        SaveManager_loadByName_hook,
        &SaveManager_loadByName_orig))
    {
        LogWarnLine("Could not hook SaveManager::load(std::string); panel teardown on load is reduced");
    }

    std::stringstream info;
    info << "panel framework initialized hotkey=\"" << g_hotkeyDisplay
         << "\" start_hidden=" << (g_panelHidden ? "true" : "false")
         << " start_collapsed=" << (g_panelCollapsed ? "true" : "false");
    LogInfoLine(info.str());

    if (ShouldLogDebug())
    {
        LogDebugLine("step 1 panel shell active; target inspection and state forcing remain unimplemented");
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        char dllPath[MAX_PATH] = { 0 };
        if (GetModuleFileNameA(hModule, dllPath, MAX_PATH) > 0)
        {
            const std::string fullPath(dllPath);
            const std::string::size_type separator = fullPath.find_last_of("\\/");
            if (separator != std::string::npos)
            {
                g_configPath = fullPath.substr(0, separator) + "\\" + kConfigFileName;
            }
        }
    }

    return TRUE;
}
