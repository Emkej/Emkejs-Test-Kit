#include "test_kit_panel.h"

#include "test_kit_config.h"
#include "test_kit_health.h"
#include "test_kit_inventory.h"
#include "test_kit_inventory_quality.h"
#include "test_kit_spawn.h"
#include "test_kit_construction.h"
#include "test_kit_stats.h"
#include "test_kit_teleport.h"

#include <mygui/MyGUI_Delegate.h>
#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_InputManager.h>
#include <mygui/MyGUI_RenderManager.h>

#include <algorithm>
#include <sstream>

namespace test_kit
{
void ConfigureTextWidget(MyGUI::TextBox* widget);
const char* ResolvePanelHeaderTitleFontName(int fontHeight);
void ApplyPanelHeaderTitleFont();
void ApplyPanelTabButtonLayout();
void ApplyPanelResponsiveBodyLayout();

namespace
{
const int kSavedLocationsSectionContentHeight = 210;
const int kSavedLocationsListHeight = 120;
const int kSavedLocationEmptyHeight = 18;

void UpdateCollapseButtonCaption()
{
    if (!g_collapseButton)
    {
        return;
    }

    g_collapseButton->setCaption(g_panelCollapsed ? "+" : "-");
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

    if (g_statsTabButton)
    {
        g_statsTabButton->setCaption(g_activePanelTab == PanelTab_Stats ? "[Stats]" : "Stats");
    }

    if (g_teleportTabButton)
    {
        g_teleportTabButton->setCaption(g_activePanelTab == PanelTab_Teleport ? "[Teleport]" : "Teleport");
    }

    if (g_inventoryTabButton)
    {
        g_inventoryTabButton->setCaption(g_activePanelTab == PanelTab_Inventory ? "[Inventory]" : "Inventory");
    }

    if (g_spawnTabButton)
    {
        g_spawnTabButton->setCaption(g_activePanelTab == PanelTab_Spawn ? "[Spawn]" : "Spawn");
    }

    if (g_constructionTabButton)
    {
        g_constructionTabButton->setCaption(
            g_activePanelTab == PanelTab_Construction ? "[Construction]" : "Construction");
    }
}

void UpdatePanelBodyWidgetVisibility(bool bodyVisible)
{
    const bool healthVisible = bodyVisible && g_activePanelTab == PanelTab_Health;
    const bool statsVisible = bodyVisible && g_activePanelTab == PanelTab_Stats;
    const bool teleportVisible = bodyVisible && g_activePanelTab == PanelTab_Teleport;
    const bool inventoryVisible = bodyVisible && g_activePanelTab == PanelTab_Inventory;
    const bool spawnVisible = bodyVisible && g_activePanelTab == PanelTab_Spawn;
    const bool constructionVisible = bodyVisible && g_activePanelTab == PanelTab_Construction;
    const bool spawnCustomFactionVisible = spawnVisible && ShouldShowSpawnCustomFactionControls();

    UpdatePanelTabButtonCaptions();

    SetWidgetVisible(g_targetSectionText, bodyVisible);
    SetWidgetVisible(g_targetNameText, bodyVisible);
    SetWidgetVisible(g_targetFactionText, bodyVisible);
    SetWidgetVisible(g_targetAlignmentText, bodyVisible);
    SetWidgetVisible(g_targetMembershipText, bodyVisible);
    SetWidgetVisible(g_targetStateText, bodyVisible);
    SetWidgetVisible(g_noTargetText, bodyVisible);
    SetWidgetVisible(g_healthTabButton, bodyVisible);
    SetWidgetVisible(g_statsTabButton, bodyVisible);
    SetWidgetVisible(g_teleportTabButton, bodyVisible);
    SetWidgetVisible(g_inventoryTabButton, bodyVisible);
    SetWidgetVisible(g_spawnTabButton, bodyVisible);
    SetWidgetVisible(g_constructionTabButton, bodyVisible);

    SetWidgetVisible(g_statesSectionText, healthVisible);
    SetWidgetVisible(g_fullRestoreButton, healthVisible);
    SetWidgetVisible(g_forceUnconsciousButton, healthVisible);
    SetWidgetVisible(g_limbDamageSectionText, healthVisible);
    SetWidgetVisible(g_damageLeftArmButton, healthVisible);
    SetWidgetVisible(g_damageRightArmButton, healthVisible);
    SetWidgetVisible(g_damageLeftLegButton, healthVisible);
    SetWidgetVisible(g_damageRightLegButton, healthVisible);
    SetWidgetVisible(g_dangerousSectionText, healthVisible);
    SetWidgetVisible(g_forceDeadButton, healthVisible);
    SetWidgetVisible(g_forceDyingButton, healthVisible);

    SetWidgetVisible(g_statsSectionText, statsVisible);
    SetWidgetVisible(g_statsScopeText, statsVisible);
    SetWidgetVisible(g_statsApplyToAllButton, statsVisible);
    SetWidgetVisible(g_statsClipboardText, statsVisible);
    SetWidgetVisible(g_statsCopyButton, statsVisible);
    SetWidgetVisible(g_statsPasteButton, statsVisible);
    SetWidgetVisible(g_statsSectionFilterText, statsVisible);
    SetWidgetVisible(g_statsAllSectionButton, statsVisible);
    SetWidgetVisible(g_statsCommonSectionButton, statsVisible);
    SetWidgetVisible(g_statsCoreSectionButton, statsVisible);
    SetWidgetVisible(g_statsUtilitySectionButton, statsVisible);
    SetWidgetVisible(g_statsCombatSectionButton, statsVisible);
    SetWidgetVisible(g_statsWeaponsSectionButton, statsVisible);
    SetWidgetVisible(g_statsLaborSectionButton, statsVisible);
    SetWidgetVisible(g_statsSearchLabelText, statsVisible);
    SetWidgetVisible(g_statsSearchEdit, statsVisible);
    SetWidgetVisible(g_statsResultCountText, statsVisible);
    SetWidgetVisible(g_statsResultsList, statsVisible);
    SetWidgetVisible(g_statsSelectedSummaryText, statsVisible);
    SetWidgetVisible(g_statsCurrentValueText, statsVisible);
    SetWidgetVisible(g_statsInputLabelText, statsVisible);
    SetWidgetVisible(g_statsInputEdit, statsVisible);
    SetWidgetVisible(g_statsSetButton, statsVisible);
    SetWidgetVisible(g_statsAddButton, statsVisible);
    SetWidgetVisible(g_statsSubtractButton, statsVisible);
    SetWidgetVisible(g_statsPreviewText, statsVisible);

    SetWidgetVisible(g_teleportSectionText, teleportVisible);
    SetWidgetVisible(g_saveLocationNameLabelText, teleportVisible);
    SetWidgetVisible(g_saveLocationNameEdit, teleportVisible);
    SetWidgetVisible(g_saveSelectedLocationButton, teleportVisible);
    SetWidgetVisible(g_savedLocationsSectionText, teleportVisible);
    SetWidgetVisible(g_savedLocationsCollapseButton, teleportVisible);
    SetWidgetVisible(g_savedLocationsRowsRoot, teleportVisible && !g_savedLocationsCollapsed);

    SetWidgetVisible(g_inventorySectionText, false);
    SetWidgetVisible(g_moneyAmountLabelText, inventoryVisible);
    SetWidgetVisible(g_moneyAmountEdit, inventoryVisible);
    SetWidgetVisible(g_addMoneyButton, inventoryVisible);
    SetWidgetVisible(g_spawnFoodSectionText, inventoryVisible);
    SetWidgetVisible(g_itemCategoryLabelText, inventoryVisible);
    SetWidgetVisible(g_itemCategoryDropdown, inventoryVisible);
    SetWidgetVisible(g_itemSearchLabelText, inventoryVisible);
    SetWidgetVisible(g_itemSearchEdit, inventoryVisible);
    SetWidgetVisible(g_itemSearchResultsList, inventoryVisible);
    SetWidgetVisible(g_itemQualityLabelText, inventoryVisible);
    SetWidgetVisible(g_itemQualityDropdown, inventoryVisible);
    SetWidgetVisible(g_itemQuantityLabelText, inventoryVisible);
    SetWidgetVisible(g_itemQuantityEdit, inventoryVisible);
    SetWidgetVisible(g_spawnItemButton, inventoryVisible);

    SetWidgetVisible(g_spawnSectionText, spawnVisible);
    SetWidgetVisible(g_spawnCategoryLabelText, spawnVisible);
    SetWidgetVisible(g_spawnCategoryDropdown, spawnVisible);
    SetWidgetVisible(g_spawnSearchLabelText, spawnVisible);
    SetWidgetVisible(g_spawnSearchEdit, spawnVisible);
    SetWidgetVisible(g_spawnResultCountText, spawnVisible);
    SetWidgetVisible(g_spawnResultsList, spawnVisible);
    SetWidgetVisible(g_spawnSelectedSummaryText, spawnVisible);
    SetWidgetVisible(g_spawnQuantityLabelText, spawnVisible);
    SetWidgetVisible(g_spawnQuantityEdit, spawnVisible);
    SetWidgetVisible(g_spawnFactionLabelText, spawnVisible);
    SetWidgetVisible(g_spawnFactionDropdown, spawnVisible);
    SetWidgetVisible(g_spawnCustomFactionLabelText, spawnCustomFactionVisible);
    SetWidgetVisible(g_spawnCustomFactionSearchEdit, spawnCustomFactionVisible);
    SetWidgetVisible(g_spawnCustomFactionResultsList, spawnCustomFactionVisible);
    SetWidgetVisible(g_spawnAllegianceLabelText, spawnVisible);
    SetWidgetVisible(g_spawnAllegianceDropdown, spawnVisible);
    SetWidgetVisible(g_spawnRadiusLabelText, spawnVisible);
    SetWidgetVisible(g_spawnRadiusDropdown, spawnVisible);
    SetWidgetVisible(g_spawnCreatureAgeLabelText, spawnVisible);
    SetWidgetVisible(g_spawnCreatureAgeDropdown, spawnVisible);
    SetWidgetVisible(g_spawnModeLabelText, spawnVisible);
    SetWidgetVisible(g_spawnModeDropdown, spawnVisible);
    SetWidgetVisible(g_spawnPreviewText, spawnVisible);
    SetWidgetVisible(g_spawnCharactersButton, spawnVisible);

    SetWidgetVisible(g_constructionSectionText, constructionVisible);
    SetWidgetVisible(g_constructionSelectedText, constructionVisible);
    SetWidgetVisible(g_constructionStatusText, constructionVisible);
    SetWidgetVisible(g_constructionFinishButton, constructionVisible);

    SetWidgetVisible(g_statusText, bodyVisible);
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

int GetPanelBodyTop()
{
    NormalizePanelVisualSettings();
    return kPanelHeaderHeight - g_panelBodyOverlap;
}

MyGUI::IntCoord BuildBodyCoord(int left, int top, int width, int height)
{
    return MyGUI::IntCoord(left, top - GetPanelBodyTop(), width, height);
}

int GetWidgetBottom(MyGUI::Widget* widget, int fallbackBottom)
{
    if (!widget)
    {
        return fallbackBottom;
    }

    const MyGUI::IntCoord coord = widget->getCoord();
    return coord.top + coord.height;
}

int GetSavedLocationsContentBottomInBodyCoords()
{
    const int bodyTop = GetPanelBodyTop();
    if (g_savedLocationsCollapsed)
    {
        const int fallbackBottom = 378 - bodyTop;
        return GetWidgetBottom(
            g_savedLocationsCollapseButton,
            GetWidgetBottom(g_savedLocationsSectionText, fallbackBottom));
    }

    return GetWidgetBottom(g_savedLocationsRowsRoot, 592 - bodyTop);
}

int GetActivePanelContentBottomInBodyCoords()
{
    const int bodyTop = GetPanelBodyTop();
    int bottom = 226 - bodyTop;
    switch (g_activePanelTab)
    {
    case PanelTab_Stats:
        return GetWidgetBottom(
            g_statsPreviewText,
            GetWidgetBottom(g_statsSubtractButton, 726 - bodyTop));
    case PanelTab_Teleport:
        return GetSavedLocationsContentBottomInBodyCoords();
    case PanelTab_Inventory:
        return GetWidgetBottom(g_spawnItemButton, 636 - bodyTop);
    case PanelTab_Spawn:
        return GetWidgetBottom(
            g_spawnPreviewText,
            GetWidgetBottom(g_spawnCharactersButton, 886 - bodyTop));
    case PanelTab_Construction:
        return GetWidgetBottom(g_constructionFinishButton, 342 - bodyTop);
    case PanelTab_Health:
    default:
        bottom = GetWidgetBottom(g_forceDyingButton, 512 - bodyTop);
        break;
    }

    return bottom;
}

int GetStatusTopInPanelCoords()
{
    return GetPanelBodyTop() + GetActivePanelContentBottomInBodyCoords() + kPanelStatusGap;
}

int GetRequiredBodyContentHeight()
{
    const int bodyTop = GetPanelBodyTop();
    return (GetStatusTopInPanelCoords() - bodyTop) + 18 + kPanelBodyBottomPadding;
}

int ResolveExpandedPanelHeight()
{
    NormalizePanelHeightSettings();
    const int bodyTop = GetPanelBodyTop();
    const int desiredHeight = bodyTop + kPanelBodyScrollPadding + GetRequiredBodyContentHeight();
    return ClampIntValue(desiredHeight, g_panelMinExpandedHeight, g_panelMaxExpandedHeight);
}

MyGUI::IntCoord BuildPanelCoordFromAnchor(int left, int top)
{
    const int height = g_panelCollapsed ? kPanelCollapsedHeight : ResolveExpandedPanelHeight();
    return MyGUI::IntCoord(left, top, kPanelWidth, height);
}

MyGUI::IntCoord ClampPanelCoordToViewport(const MyGUI::IntCoord& inputCoord)
{
    int left = inputCoord.left;
    int top = inputCoord.top;
    const int width = (inputCoord.width > 0) ? inputCoord.width : kPanelWidth;
    const int height = (inputCoord.height > 0)
        ? inputCoord.height
        : (g_panelCollapsed ? kPanelCollapsedHeight : ResolveExpandedPanelHeight());

    int viewWidth = 0;
    int viewHeight = 0;
    if (!TryGetViewportSize(&viewWidth, &viewHeight))
    {
        const int minLeft = kPanelMinimumVisibleWidth - width;
        const int minTop = kPanelMinimumVisibleHeight - height;
        left = ClampIntValue(left, minLeft, kPanelViewportPadding);
        top = ClampIntValue(top, minTop, kPanelViewportPadding);
        return MyGUI::IntCoord(left, top, width, height);
    }

    int minVisibleWidth = kPanelMinimumVisibleWidth;
    if (minVisibleWidth > width)
    {
        minVisibleWidth = width;
    }

    int minVisibleHeight = kPanelMinimumVisibleHeight;
    if (minVisibleHeight < kPanelHeaderHeight)
    {
        minVisibleHeight = kPanelHeaderHeight;
    }
    if (minVisibleHeight > height)
    {
        minVisibleHeight = height;
    }

    int minLeft = minVisibleWidth - width;
    int minTop = minVisibleHeight - height;
    int maxLeft = viewWidth - minVisibleWidth;
    int maxTop = viewHeight - minVisibleHeight;

    if (maxLeft < minLeft)
    {
        minLeft = 0;
        maxLeft = 0;
    }
    if (maxTop < minTop)
    {
        minTop = 0;
        maxTop = 0;
    }

    left = ClampIntValue(left, minLeft, maxLeft);
    top = ClampIntValue(top, minTop, maxTop);

    const int snapLeft = 0;
    const int snapTop = 0;
    const int snapRight = viewWidth - width;
    const int snapBottom = viewHeight - height;

    if (left >= snapLeft - kPanelEdgeSnapDistance && left <= snapLeft + kPanelEdgeSnapDistance)
    {
        left = snapLeft;
    }
    else if (left >= snapRight - kPanelEdgeSnapDistance && left <= snapRight + kPanelEdgeSnapDistance)
    {
        left = snapRight;
    }

    if (top >= snapTop - kPanelEdgeSnapDistance && top <= snapTop + kPanelEdgeSnapDistance)
    {
        top = snapTop;
    }
    else if (top >= snapBottom - kPanelEdgeSnapDistance && top <= snapBottom + kPanelEdgeSnapDistance)
    {
        top = snapBottom;
    }

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

    NormalizePanelVisualSettings();
    MyGUI::IntCoord desiredCoord = panelCoord;
    desiredCoord.width = kPanelWidth;
    desiredCoord.height = g_panelCollapsed ? kPanelCollapsedHeight : ResolveExpandedPanelHeight();

    const MyGUI::IntCoord clampedCoord = ClampPanelCoordToViewport(desiredCoord);
    StorePanelRuntimePosition(clampedCoord);

    g_panel->setCoord(clampedCoord);
    g_panel->setVisible(!g_panelHidden);

    const bool bodyVisible = !g_panelHidden && !g_panelCollapsed;
    const int bodyTop = GetPanelBodyTop();
    const int bodyHeight = (clampedCoord.height > bodyTop) ? (clampedCoord.height - bodyTop) : 0;
    const int closeButtonTop = (kPanelHeaderHeight - g_panelCloseButtonSize) / 2;
    const int collapseButtonTop = (kPanelHeaderHeight - g_panelCollapseButtonSize) / 2;
    const int closeButtonLeft = kPanelWidth - kPanelHeaderButtonRightPadding - g_panelCloseButtonSize;
    const int collapseButtonLeft = closeButtonLeft - kPanelHeaderButtonGap - g_panelCollapseButtonSize;
    int titleInset = kPanelHeaderButtonRightPadding + g_panelCloseButtonSize + kPanelHeaderButtonGap + g_panelCollapseButtonSize + 8;
    if (titleInset < 36)
    {
        titleInset = 36;
    }
    int titleLeft = titleInset;
    int titleWidth = kPanelWidth - (titleInset * 2);
    if (titleWidth < 120)
    {
        titleLeft = 12;
        titleWidth = kPanelWidth - 24;
    }

    if (g_headerBackground)
    {
        g_headerBackground->setCoord(MyGUI::IntCoord(0, 0, kPanelWidth, kPanelHeaderHeight));
    }

    if (g_headerFrame)
    {
        g_headerFrame->setCoord(MyGUI::IntCoord(0, 0, kPanelWidth, kPanelHeaderHeight));
    }

    if (g_headerTitleText)
    {
        g_headerTitleText->setCoord(MyGUI::IntCoord(titleLeft, 0, titleWidth, kPanelHeaderHeight));
        ApplyPanelHeaderTitleFont();
    }

    if (g_collapseButton)
    {
        g_collapseButton->setCoord(MyGUI::IntCoord(
            collapseButtonLeft,
            collapseButtonTop,
            g_panelCollapseButtonSize,
            g_panelCollapseButtonSize));
    }

    if (g_closeButton)
    {
        g_closeButton->setCoord(MyGUI::IntCoord(
            closeButtonLeft,
            closeButtonTop,
            g_panelCloseButtonSize,
            g_panelCloseButtonSize));
    }

    if (g_bodyFrame)
    {
        g_bodyFrame->setCoord(MyGUI::IntCoord(0, bodyTop, kPanelWidth, bodyHeight));
        g_bodyFrame->setVisible(bodyVisible);
    }

    if (g_bodyScrollView)
    {
        g_bodyScrollView->setCoord(MyGUI::IntCoord(0, bodyTop, kPanelWidth, bodyHeight));
        g_bodyScrollView->setVisible(bodyVisible);
    }

    ApplyPanelTabButtonLayout();
    ApplyPanelResponsiveBodyLayout();

    UpdatePanelBodyWidgetVisibility(bodyVisible);

    if (g_statusText)
    {
        g_statusText->setCoord(BuildBodyCoord(20, GetStatusTopInPanelCoords(), kPanelWidth - 40, 18));
    }

    if (g_bodyScrollView)
    {
        const int minCanvasWidth = kPanelWidth - kPanelBodyScrollPadding;
        const int minCanvasHeight = bodyHeight - kPanelBodyScrollPadding;
        const int canvasWidth = (minCanvasWidth > 1) ? minCanvasWidth : 1;
        const int baseCanvasHeight = (minCanvasHeight > 1) ? minCanvasHeight : 1;
        const int requiredBodyContentHeight = GetRequiredBodyContentHeight();
        const int canvasHeight = (requiredBodyContentHeight > baseCanvasHeight)
            ? requiredBodyContentHeight
            : baseCanvasHeight;

        g_bodyScrollView->setCanvasSize(canvasWidth, canvasHeight);
        g_bodyScrollView->setVisibleHScroll(false);
        g_bodyScrollView->setVisibleVScroll(canvasHeight > baseCanvasHeight);

        if (!bodyVisible)
        {
            g_bodyScrollView->setViewOffset(MyGUI::IntPoint(0, 0));
        }
        else
        {
            MyGUI::IntPoint viewOffset = g_bodyScrollView->getViewOffset();
            const int maxViewTop = (canvasHeight > baseCanvasHeight) ? (canvasHeight - baseCanvasHeight) : 0;
            if (viewOffset.top > maxViewTop)
            {
                viewOffset.top = maxViewTop;
                g_bodyScrollView->setViewOffset(viewOffset);
            }
        }
    }
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
        LogDebugLine(line.str());
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
    ResetInventoryWidgetInteractionState();
    ResetConstructionUiState();

    g_panel = 0;
    g_headerBackground = 0;
    g_headerFrame = 0;
    g_headerTitleText = 0;
    g_collapseButton = 0;
    g_closeButton = 0;
    g_bodyFrame = 0;
    g_bodyScrollView = 0;
    g_targetSectionText = 0;
    g_targetNameText = 0;
    g_targetFactionText = 0;
    g_targetAlignmentText = 0;
    g_targetMembershipText = 0;
    g_targetStateText = 0;
    g_noTargetText = 0;
    g_healthTabButton = 0;
    g_statsTabButton = 0;
    g_teleportTabButton = 0;
    g_inventoryTabButton = 0;
    g_spawnTabButton = 0;
    g_constructionTabButton = 0;
    g_statesSectionText = 0;
    g_fullRestoreButton = 0;
    g_forceUnconsciousButton = 0;
    g_limbDamageSectionText = 0;
    g_damageLeftArmButton = 0;
    g_damageRightArmButton = 0;
    g_damageLeftLegButton = 0;
    g_damageRightLegButton = 0;
    g_statsSectionText = 0;
    g_statsScopeText = 0;
    g_statsApplyToAllButton = 0;
    g_statsClipboardText = 0;
    g_statsCopyButton = 0;
    g_statsPasteButton = 0;
    g_statsSectionFilterText = 0;
    g_statsAllSectionButton = 0;
    g_statsCommonSectionButton = 0;
    g_statsCoreSectionButton = 0;
    g_statsUtilitySectionButton = 0;
    g_statsCombatSectionButton = 0;
    g_statsWeaponsSectionButton = 0;
    g_statsLaborSectionButton = 0;
    g_statsSearchLabelText = 0;
    g_statsSearchEdit = 0;
    g_statsResultCountText = 0;
    g_statsResultsList = 0;
    g_statsSelectedSummaryText = 0;
    g_statsCurrentValueText = 0;
    g_statsInputLabelText = 0;
    g_statsInputEdit = 0;
    g_statsSetButton = 0;
    g_statsAddButton = 0;
    g_statsSubtractButton = 0;
    g_statsPreviewText = 0;
    g_teleportSectionText = 0;
    g_saveLocationNameLabelText = 0;
    g_saveLocationNameEdit = 0;
    g_saveSelectedLocationButton = 0;
    g_savedLocationsSectionText = 0;
    g_savedLocationsCollapseButton = 0;
    g_savedLocationsRowsRoot = 0;
    g_savedLocationSearchLabelText = 0;
    g_savedLocationSearchEdit = 0;
    g_savedLocationsListBox = 0;
    g_savedLocationsEmptyText = 0;
    g_savedLocationTeleportButton = 0;
    g_savedLocationPinButton = 0;
    g_savedLocationRenameButton = 0;
    g_savedLocationDeleteButton = 0;
    g_inventorySectionText = 0;
    g_moneyAmountLabelText = 0;
    g_moneyAmountEdit = 0;
    g_addMoneyButton = 0;
    g_spawnFoodSectionText = 0;
    g_itemCategoryLabelText = 0;
    g_itemCategoryDropdown = 0;
    g_itemSearchLabelText = 0;
    g_itemSearchEdit = 0;
    g_itemSearchResultsList = 0;
    g_itemQualityLabelText = 0;
    g_itemQualityDropdown = 0;
    g_itemQuantityLabelText = 0;
    g_itemQuantityEdit = 0;
    g_spawnItemButton = 0;
    g_spawnSectionText = 0;
    g_spawnCategoryLabelText = 0;
    g_spawnCategoryDropdown = 0;
    g_spawnSearchLabelText = 0;
    g_spawnSearchEdit = 0;
    g_spawnResultCountText = 0;
    g_spawnResultsList = 0;
    g_spawnSelectedSummaryText = 0;
    g_spawnQuantityLabelText = 0;
    g_spawnQuantityEdit = 0;
    g_spawnFactionLabelText = 0;
    g_spawnFactionDropdown = 0;
    g_spawnCustomFactionLabelText = 0;
    g_spawnCustomFactionSearchEdit = 0;
    g_spawnCustomFactionResultsList = 0;
    g_spawnAllegianceLabelText = 0;
    g_spawnAllegianceDropdown = 0;
    g_spawnRadiusLabelText = 0;
    g_spawnRadiusDropdown = 0;
    g_spawnCreatureAgeLabelText = 0;
    g_spawnCreatureAgeDropdown = 0;
    g_spawnModeLabelText = 0;
    g_spawnModeDropdown = 0;
    g_spawnPreviewText = 0;
    g_spawnCharactersButton = 0;
    g_constructionSectionText = 0;
    g_constructionSelectedText = 0;
    g_constructionStatusText = 0;
    g_constructionFinishButton = 0;
    g_dangerousSectionText = 0;
    g_forceDeadButton = 0;
    g_forceDyingButton = 0;
    g_statusText = 0;
    g_filteredStatsRegistryIndexes.clear();
    g_selectedStatsStat = STAT_NONE;
    g_statsApplyToAllSelected = false;
    g_activeStatsSectionFilter = StatsSectionFilter_All;
    g_filteredSavedLocationIndexes.clear();
    g_savedLocationRenameId.clear();
    g_savedLocationSearchText.clear();
    g_selectedSavedLocationId.clear();
    g_savedLocationsCollapsed = false;
}

void ConfigureEditBoxWidget(MyGUI::EditBox* widget)
{
    if (!widget)
    {
        return;
    }

    MyGUI::Widget* clientWidget = widget->getClientWidget();
    if (clientWidget)
    {
        clientWidget->setAlign(MyGUI::Align::Stretch);
    }

    widget->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
}

void ConfigureComboBoxWidget(MyGUI::ComboBox* widget)
{
    if (!widget)
    {
        return;
    }

    MyGUI::Widget* clientWidget = widget->getClientWidget();
    if (clientWidget)
    {
        clientWidget->setAlign(MyGUI::Align::Stretch);
    }

    widget->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
    widget->setComboModeDrop(true);
    widget->setMaxListLength(kInventoryItemDropdownMaxListLength);
}

void ConfigureListBoxWidget(MyGUI::ListBox* widget)
{
    if (!widget)
    {
        return;
    }

    widget->setScrollVisible(true);
    widget->setActivateOnClick(true);
}

void TrySetTextWidgetFontName(MyGUI::TextBox* widget, const char* fontName)
{
    if (!widget || !fontName || fontName[0] == '\0')
    {
        return;
    }

    try
    {
        widget->setFontName(fontName);
    }
    catch (...)
    {
    }
}

void ApplySectionHeaderFonts()
{
    MyGUI::TextBox* headers[] = {
        g_targetSectionText,
        g_statesSectionText,
        g_limbDamageSectionText,
        g_statsSectionText,
        g_teleportSectionText,
        g_savedLocationsSectionText,
        g_inventorySectionText,
        g_spawnFoodSectionText,
        g_spawnSectionText,
        g_dangerousSectionText
    };

    for (size_t index = 0; index < sizeof(headers) / sizeof(headers[0]); ++index)
    {
        TrySetTextWidgetFontName(headers[index], "Kenshi_PaintedTextFont_Medium");
    }
}

void ConfigureScrollViewWidget(MyGUI::ScrollView* widget)
{
    if (!widget)
    {
        return;
    }

    widget->setVisibleHScroll(false);
    widget->setVisibleVScroll(false);
    widget->setCanvasAlign(MyGUI::Align::Left | MyGUI::Align::Top);
    widget->setViewOffset(MyGUI::IntPoint(0, 0));
}

MyGUI::ScrollView* CreatePanelBodyScrollView(MyGUI::Widget* parent, const MyGUI::IntCoord& coord)
{
    if (!parent)
    {
        return 0;
    }

    const char* skins[] = {
        "Kenshi_ScrollViewEmpty",
        "Kenshi_ScrollViewEmptyLight",
        "Kenshi_ScrollView",
        "ScrollView"
    };

    for (size_t index = 0; index < sizeof(skins) / sizeof(skins[0]); ++index)
    {
        try
        {
            MyGUI::ScrollView* widget =
                parent->createWidget<MyGUI::ScrollView>(skins[index], coord, MyGUI::Align::Default);
            if (widget)
            {
                return widget;
            }
        }
        catch (...)
        {
        }
    }

    return 0;
}

MyGUI::Button* CreatePanelCloseButton(MyGUI::Widget* parent, const MyGUI::IntCoord& coord)
{
    if (!parent)
    {
        return 0;
    }

    try
    {
        return parent->createWidget<MyGUI::Button>(
            "Kenshi_CloseButtonSkin",
            coord,
            MyGUI::Align::Default);
    }
    catch (...)
    {
    }

    try
    {
        MyGUI::Button* button = parent->createWidget<MyGUI::Button>(
            "Kenshi_Button1",
            coord,
            MyGUI::Align::Default);
        if (button)
        {
            button->setCaption("X");
        }
        return button;
    }
    catch (...)
    {
    }

    return 0;
}

bool HasAllPanelWidgets()
{
    return g_panel
        && g_headerBackground
        && g_headerFrame
        && g_headerTitleText
        && g_collapseButton
        && g_closeButton
        && g_bodyFrame
        && g_bodyScrollView
        && g_targetSectionText
        && g_targetNameText
        && g_targetFactionText
        && g_targetAlignmentText
        && g_targetMembershipText
        && g_targetStateText
        && g_noTargetText
        && g_healthTabButton
        && g_statsTabButton
        && g_teleportTabButton
        && g_inventoryTabButton
        && g_spawnTabButton
        && g_constructionTabButton
        && g_statesSectionText
        && g_fullRestoreButton
        && g_forceUnconsciousButton
        && g_limbDamageSectionText
        && g_damageLeftArmButton
        && g_damageRightArmButton
        && g_damageLeftLegButton
        && g_damageRightLegButton
        && g_statsSectionText
        && g_statsScopeText
        && g_statsApplyToAllButton
        && g_statsClipboardText
        && g_statsCopyButton
        && g_statsPasteButton
        && g_statsSectionFilterText
        && g_statsAllSectionButton
        && g_statsCommonSectionButton
        && g_statsCoreSectionButton
        && g_statsUtilitySectionButton
        && g_statsCombatSectionButton
        && g_statsWeaponsSectionButton
        && g_statsLaborSectionButton
        && g_statsSearchLabelText
        && g_statsSearchEdit
        && g_statsResultCountText
        && g_statsResultsList
        && g_statsSelectedSummaryText
        && g_statsCurrentValueText
        && g_statsInputLabelText
        && g_statsInputEdit
        && g_statsSetButton
        && g_statsAddButton
        && g_statsSubtractButton
        && g_statsPreviewText
        && g_teleportSectionText
        && g_saveLocationNameLabelText
        && g_saveLocationNameEdit
        && g_saveSelectedLocationButton
        && g_savedLocationsSectionText
        && g_savedLocationsCollapseButton
        && g_savedLocationsRowsRoot
        && g_savedLocationSearchLabelText
        && g_savedLocationSearchEdit
        && g_savedLocationsListBox
        && g_savedLocationsEmptyText
        && g_savedLocationTeleportButton
        && g_savedLocationPinButton
        && g_savedLocationRenameButton
        && g_savedLocationDeleteButton
        && g_inventorySectionText
        && g_moneyAmountLabelText
        && g_moneyAmountEdit
        && g_addMoneyButton
        && g_spawnFoodSectionText
        && g_itemCategoryLabelText
        && g_itemCategoryDropdown
        && g_itemSearchLabelText
        && g_itemSearchEdit
        && g_itemSearchResultsList
        && g_itemQualityLabelText
        && g_itemQualityDropdown
        && g_itemQuantityLabelText
        && g_itemQuantityEdit
        && g_spawnItemButton
        && g_spawnSectionText
        && g_constructionSectionText
        && g_constructionSelectedText
        && g_constructionStatusText
        && g_constructionFinishButton
        && g_spawnCategoryLabelText
        && g_spawnCategoryDropdown
        && g_spawnSearchLabelText
        && g_spawnSearchEdit
        && g_spawnResultCountText
        && g_spawnResultsList
        && g_spawnSelectedSummaryText
        && g_spawnQuantityLabelText
        && g_spawnQuantityEdit
        && g_spawnFactionLabelText
        && g_spawnFactionDropdown
        && g_spawnCustomFactionLabelText
        && g_spawnCustomFactionSearchEdit
        && g_spawnCustomFactionResultsList
        && g_spawnAllegianceLabelText
        && g_spawnAllegianceDropdown
        && g_spawnRadiusLabelText
        && g_spawnRadiusDropdown
        && g_spawnCreatureAgeLabelText
        && g_spawnCreatureAgeDropdown
        && g_spawnModeLabelText
        && g_spawnModeDropdown
        && g_spawnPreviewText
        && g_spawnCharactersButton
        && g_dangerousSectionText
        && g_forceDeadButton
        && g_forceDyingButton
        && g_statusText;
}

void OnCollapseButtonClicked(MyGUI::Widget*)
{
    TogglePanelCollapsed("button");
}

void OnCloseButtonClicked(MyGUI::Widget*)
{
    if (!g_panelHidden)
    {
        TogglePanelHidden("close_button");
    }
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

void OnInventoryTabButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    EnsureInventoryFoodItemOptionsLoaded();
    RefreshInventoryFoodItemDropdown();
    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    SetActivePanelTab(PanelTab_Inventory);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnSpawnTabButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    EnsureSpawnTemplateOptionsLoaded();
    RefreshSpawnTemplateList();
    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    SetActivePanelTab(PanelTab_Spawn);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void InitializePanelWidgets()
{
    if (!HasAllPanelWidgets())
    {
        return;
    }

    g_headerBackground->setCaption("");
    g_headerBackground->setEnabled(true);
    g_headerBackground->setNeedMouseFocus(false);
    g_headerFrame->setEnabled(true);
    g_headerFrame->setNeedMouseFocus(true);
    g_headerFrame->setNeedKeyFocus(true);
    g_panel->setNeedMouseFocus(true);
    g_headerTitleText->setCaption("Emkejs Test Kit");
    g_headerTitleText->setNeedMouseFocus(false);
    g_headerTitleText->setTextAlign(MyGUI::Align::Center | MyGUI::Align::VCenter);
    ApplyPanelHeaderTitleFont();

    g_bodyFrame->setCaption("");
    g_bodyFrame->setEnabled(false);
    g_bodyFrame->setNeedMouseFocus(true);
    ConfigureScrollViewWidget(g_bodyScrollView);

    ConfigureTextWidget(g_targetSectionText);
    ConfigureTextWidget(g_targetNameText);
    ConfigureTextWidget(g_targetFactionText);
    ConfigureTextWidget(g_targetAlignmentText);
    ConfigureTextWidget(g_targetMembershipText);
    ConfigureTextWidget(g_targetStateText);
    ConfigureTextWidget(g_noTargetText);
    ConfigureTextWidget(g_statesSectionText);
    ConfigureTextWidget(g_limbDamageSectionText);
    ConfigureTextWidget(g_statsSectionText);
    ConfigureTextWidget(g_statsScopeText);
    ConfigureTextWidget(g_statsClipboardText);
    ConfigureTextWidget(g_statsSectionFilterText);
    ConfigureTextWidget(g_statsSearchLabelText);
    ConfigureTextWidget(g_statsResultCountText);
    ConfigureTextWidget(g_statsSelectedSummaryText);
    ConfigureTextWidget(g_statsCurrentValueText);
    ConfigureTextWidget(g_statsInputLabelText);
    ConfigureTextWidget(g_statsPreviewText);
    ConfigureTextWidget(g_teleportSectionText);
    ConfigureTextWidget(g_saveLocationNameLabelText);
    ConfigureTextWidget(g_savedLocationsSectionText);
    ConfigureTextWidget(g_savedLocationSearchLabelText);
    ConfigureTextWidget(g_savedLocationsEmptyText);
    ConfigureTextWidget(g_inventorySectionText);
    ConfigureTextWidget(g_moneyAmountLabelText);
    ConfigureTextWidget(g_spawnFoodSectionText);
    ConfigureTextWidget(g_itemCategoryLabelText);
    ConfigureTextWidget(g_itemSearchLabelText);
    ConfigureTextWidget(g_itemQualityLabelText);
    ConfigureTextWidget(g_itemQuantityLabelText);
    ConfigureTextWidget(g_spawnSectionText);
    ConfigureTextWidget(g_spawnCategoryLabelText);
    ConfigureTextWidget(g_spawnSearchLabelText);
    ConfigureTextWidget(g_spawnResultCountText);
    ConfigureTextWidget(g_spawnSelectedSummaryText);
    ConfigureTextWidget(g_spawnQuantityLabelText);
    ConfigureTextWidget(g_spawnFactionLabelText);
    ConfigureTextWidget(g_spawnCustomFactionLabelText);
    ConfigureTextWidget(g_spawnAllegianceLabelText);
    ConfigureTextWidget(g_spawnRadiusLabelText);
    ConfigureTextWidget(g_spawnCreatureAgeLabelText);
    ConfigureTextWidget(g_spawnModeLabelText);
    ConfigureTextWidget(g_spawnPreviewText);
    ConfigureTextWidget(g_constructionSectionText);
    ConfigureTextWidget(g_constructionSelectedText);
    ConfigureTextWidget(g_constructionStatusText);
    ConfigureTextWidget(g_dangerousSectionText);
    ConfigureTextWidget(g_statusText);
    ApplySectionHeaderFonts();
    ConfigureEditBoxWidget(g_saveLocationNameEdit);
    ConfigureEditBoxWidget(g_savedLocationSearchEdit);
    ConfigureEditBoxWidget(g_statsSearchEdit);
    ConfigureEditBoxWidget(g_statsInputEdit);
    ConfigureEditBoxWidget(g_moneyAmountEdit);
    ConfigureEditBoxWidget(g_itemSearchEdit);
    ConfigureEditBoxWidget(g_itemQuantityEdit);
    ConfigureEditBoxWidget(g_spawnSearchEdit);
    ConfigureEditBoxWidget(g_spawnQuantityEdit);
    ConfigureEditBoxWidget(g_spawnCustomFactionSearchEdit);
    ConfigureComboBoxWidget(g_itemCategoryDropdown);
    ConfigureComboBoxWidget(g_itemQualityDropdown);
    ConfigureComboBoxWidget(g_spawnCategoryDropdown);
    ConfigureComboBoxWidget(g_spawnFactionDropdown);
    ConfigureComboBoxWidget(g_spawnAllegianceDropdown);
    ConfigureComboBoxWidget(g_spawnRadiusDropdown);
    ConfigureComboBoxWidget(g_spawnCreatureAgeDropdown);
    ConfigureComboBoxWidget(g_spawnModeDropdown);
    ConfigureListBoxWidget(g_savedLocationsListBox);
    ConfigureListBoxWidget(g_statsResultsList);
    ConfigureListBoxWidget(g_itemSearchResultsList);
    ConfigureListBoxWidget(g_spawnResultsList);
    ConfigureListBoxWidget(g_spawnCustomFactionResultsList);

    g_savedLocationsRowsRoot->setNeedMouseFocus(false);
    g_savedLocationsRowsRoot->setInheritsPick(true);
    g_savedLocationsEmptyText->setNeedMouseFocus(false);

    g_targetSectionText->setCaption("Target");
    g_targetNameText->setCaption("Name: Pending target inspection");
    g_targetFactionText->setCaption("Faction: Unknown");
    g_targetAlignmentText->setCaption("Alignment: Unknown");
    g_targetMembershipText->setCaption("Membership: Unknown");
    g_targetStateText->setCaption("State: Unknown");
    g_noTargetText->setCaption("Source: None");
    UpdatePanelTabButtonCaptions();
    g_statesSectionText->setCaption("States");
    g_limbDamageSectionText->setCaption("Limb Damage");
    g_statsSectionText->setCaption("Stats");
    g_statsScopeText->setCaption("Scope: No selected character");
    g_statsClipboardText->setCaption("Clipboard: Empty");
    g_statsSectionFilterText->setCaption("Sections");
    g_statsSearchLabelText->setCaption("Search");
    g_statsResultCountText->setCaption("0 stats");
    g_statsSelectedSummaryText->setCaption("Selected: None");
    g_statsCurrentValueText->setCaption("Current: No stat selected");
    g_statsInputLabelText->setCaption("Value / Delta");
    g_statsPreviewText->setCaption("Preview: Select a stat");
    g_teleportSectionText->setCaption("Teleport");
    g_saveLocationNameLabelText->setCaption("Location Name (Enter to save)");
    g_savedLocationsSectionText->setCaption("Saved Locations");
    g_savedLocationSearchLabelText->setCaption("Search Saved Locations");
    g_inventorySectionText->setCaption("");
    g_moneyAmountLabelText->setCaption("Cats To Add");
    g_spawnFoodSectionText->setCaption("Spawn Items");
    g_itemCategoryLabelText->setCaption("Category");
    g_itemSearchLabelText->setCaption("Search");
    g_itemQualityLabelText->setCaption("Quality");
    g_itemQuantityLabelText->setCaption("Quantity");
    g_spawnSectionText->setCaption("Spawn");
    g_spawnCategoryLabelText->setCaption("Category");
    g_constructionSectionText->setCaption("Construction");
    g_constructionSelectedText->setCaption("Selected: None");
    g_constructionStatusText->setCaption("Status: No selection");
    g_spawnSearchLabelText->setCaption("Search");
    g_spawnResultCountText->setCaption("0 results");
    g_spawnSelectedSummaryText->setCaption("Selected: None");
    {
        std::stringstream caption;
        caption << "Quantity (1-" << kSpawnTemplateQuantityMax << ")";
        g_spawnQuantityLabelText->setCaption(caption.str());
    }
    g_spawnFactionLabelText->setCaption("Faction");
    g_spawnCustomFactionLabelText->setCaption("Custom faction");
    g_spawnAllegianceLabelText->setCaption("Allegiance");
    g_spawnRadiusLabelText->setCaption("Radius");
    g_spawnCreatureAgeLabelText->setCaption("Age (creatures)");
    g_spawnModeLabelText->setCaption("Mode");
    g_spawnPreviewText->setCaption("Preview: Select a spawn template");
    g_constructionFinishButton->setCaption("Finish selected construction");
    g_dangerousSectionText->setCaption("Dangerous");

    g_fullRestoreButton->setCaption("Full Restore");
    g_forceUnconsciousButton->setCaption("Force Unconscious");
    g_damageLeftArmButton->setCaption("Damage Left Arm");
    g_damageRightArmButton->setCaption("Damage Right Arm");
    g_damageLeftLegButton->setCaption("Damage Left Leg");
    g_damageRightLegButton->setCaption("Damage Right Leg");
    g_statsApplyToAllButton->setCaption("Apply To All Selected: Off");
    g_statsCopyButton->setCaption("Copy Current Stats");
    g_statsPasteButton->setCaption("Paste To Selected");
    UpdateStatsSectionButtonCaptions();
    g_statsSearchEdit->setEditStatic(false);
    g_statsSearchEdit->setMaxTextLength(48);
    g_statsSearchEdit->setOnlyText("");
    g_statsResultsList->removeAllItems();
    g_statsResultsList->clearIndexSelected();
    g_statsInputEdit->setEditStatic(false);
    g_statsInputEdit->setMaxTextLength(3);
    g_statsInputEdit->setOnlyText("10");
    g_statsSetButton->setCaption("Set");
    g_statsAddButton->setCaption("+");
    g_statsSubtractButton->setCaption("-");
    RefreshStatsList();
    RefreshStatsUi(g_lastPlayerInterface);
    g_saveLocationNameEdit->setEditStatic(false);
    g_saveLocationNameEdit->setMaxTextLength(64);
    g_saveLocationNameEdit->setOnlyText("");
    g_savedLocationSearchEdit->setEditStatic(false);
    g_savedLocationSearchEdit->setMaxTextLength(64);
    g_savedLocationSearchEdit->setOnlyText("");
    RefreshSaveLocationInputUi();
    g_savedLocationsEmptyText->setCaption("No saved locations yet");
    UpdateSavedLocationsCollapseButtonCaption();
    g_savedLocationTeleportButton->setCaption("Teleport");
    g_savedLocationPinButton->setCaption("Pin");
    g_savedLocationRenameButton->setCaption("Rename");
    g_savedLocationDeleteButton->setCaption("Delete");
    RefreshSavedLocationsListWidget();
    g_moneyAmountEdit->setEditStatic(false);
    g_moneyAmountEdit->setMaxTextLength(10);
    g_moneyAmountEdit->setOnlyText("1000");
    g_addMoneyButton->setCaption("Add Money");
    g_itemCategoryDropdown->removeAllItems();
    g_itemCategoryDropdown->addItem("All");
    g_itemCategoryDropdown->addItem("Food");
    g_itemCategoryDropdown->addItem("General");
    g_itemCategoryDropdown->addItem("Armor");
    g_itemCategoryDropdown->addItem("Weapons");
    g_itemCategoryDropdown->setIndexSelected(0);
    g_itemSearchEdit->setEditStatic(false);
    g_itemSearchEdit->setMaxTextLength(48);
    g_itemSearchEdit->setOnlyText("");
    g_itemSearchResultsList->removeAllItems();
    g_itemSearchResultsList->clearIndexSelected();
    ResetInventoryQualityWidgetState();
    g_itemQuantityEdit->setEditStatic(false);
    g_itemQuantityEdit->setMaxTextLength(10);
    g_itemQuantityEdit->setOnlyText("1");
    g_spawnItemButton->setCaption("Spawn Item");
    g_spawnCategoryDropdown->removeAllItems();
    g_spawnCategoryDropdown->addItem("All");
    g_spawnCategoryDropdown->addItem("Characters");
    g_spawnCategoryDropdown->addItem("Creatures");
    g_spawnCategoryDropdown->setIndexSelected(0);
    g_spawnSearchEdit->setEditStatic(false);
    g_spawnSearchEdit->setMaxTextLength(48);
    g_spawnSearchEdit->setOnlyText("");
    g_spawnResultsList->removeAllItems();
    g_spawnResultsList->clearIndexSelected();
    g_spawnQuantityEdit->setEditStatic(false);
    g_spawnQuantityEdit->setMaxTextLength(2);
    g_spawnQuantityEdit->setOnlyText("1");
    g_spawnFactionDropdown->removeAllItems();
    g_spawnFactionDropdown->addItem("None");
    g_spawnFactionDropdown->addItem("Custom");
    g_spawnFactionDropdown->setIndexSelected(0);
    g_spawnCustomFactionSearchEdit->setEditStatic(false);
    g_spawnCustomFactionSearchEdit->setMaxTextLength(64);
    g_spawnCustomFactionSearchEdit->setOnlyText("");
    g_spawnCustomFactionResultsList->removeAllItems();
    g_spawnCustomFactionResultsList->clearIndexSelected();
    g_spawnAllegianceDropdown->removeAllItems();
    g_spawnAllegianceDropdown->addItem("Same as target");
    g_spawnAllegianceDropdown->addItem("Friendly (player)");
    g_spawnAllegianceDropdown->addItem("Neutral");
    g_spawnAllegianceDropdown->addItem("Hostile");
    g_spawnAllegianceDropdown->setIndexSelected(2);
    g_spawnRadiusDropdown->removeAllItems();
    g_spawnRadiusDropdown->addItem("Close");
    g_spawnRadiusDropdown->addItem("Normal");
    g_spawnRadiusDropdown->addItem("Wide");
    g_spawnRadiusDropdown->setIndexSelected(1);
    g_spawnCreatureAgeDropdown->removeAllItems();
    g_spawnCreatureAgeDropdown->addItem("Pup");
    g_spawnCreatureAgeDropdown->addItem("Young");
    g_spawnCreatureAgeDropdown->addItem("Adult");
    g_spawnCreatureAgeDropdown->setIndexSelected(2);
    g_spawnModeDropdown->removeAllItems();
    g_spawnModeDropdown->addItem("Independent");
    g_spawnModeDropdown->addItem("Add to target squad");
    g_spawnModeDropdown->setIndexSelected(0);
    g_spawnCharactersButton->setCaption("Spawn");
    EnsureInventoryFoodItemOptionsLoaded();
    RefreshInventoryFoodItemDropdown();
    EnsureSpawnTemplateOptionsLoaded();
    RefreshSpawnTemplateList();
    RefreshSpawnCreatureAgeControlState();
    RefreshSpawnFactionControlState();
    UpdateDangerousActionButtonCaptions();
    UpdateCollapseButtonCaption();
    RefreshStatusWidget();
    SetActionButtonsEnabled(false);
    SetSelectionActionButtonsEnabled(false);

    g_collapseButton->eventMouseButtonClick += MyGUI::newDelegate(&OnCollapseButtonClicked);
    g_closeButton->eventMouseButtonClick += MyGUI::newDelegate(&OnCloseButtonClicked);
    g_healthTabButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnHealthTabButtonPressed);
    g_statsTabButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnStatsTabButtonPressed);
    g_teleportTabButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnTeleportTabButtonPressed);
    g_inventoryTabButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnInventoryTabButtonPressed);
    g_spawnTabButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnSpawnTabButtonPressed);
    g_fullRestoreButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnFullRestoreButtonPressed);
    g_forceUnconsciousButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnForceUnconsciousButtonPressed);
    g_damageLeftArmButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnDamageLeftArmButtonPressed);
    g_damageRightArmButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnDamageRightArmButtonPressed);
    g_damageLeftLegButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnDamageLeftLegButtonPressed);
    g_damageRightLegButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnDamageRightLegButtonPressed);
    g_forceDeadButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnForceDeadButtonPressed);
    g_statsApplyToAllButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnStatsApplyToAllButtonPressed);
    g_statsAllSectionButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnStatsAllSectionButtonPressed);
    g_statsCommonSectionButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnStatsCommonSectionButtonPressed);
    g_statsCoreSectionButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnStatsCoreSectionButtonPressed);
    g_statsUtilitySectionButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnStatsUtilitySectionButtonPressed);
    g_statsCombatSectionButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnStatsCombatSectionButtonPressed);
    g_statsWeaponsSectionButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnStatsWeaponsSectionButtonPressed);
    g_statsLaborSectionButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnStatsLaborSectionButtonPressed);
    g_statsCopyButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnStatsCopyButtonPressed);
    g_statsPasteButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnStatsPasteButtonPressed);
    g_statsSearchEdit->eventEditTextChange += MyGUI::newDelegate(&OnStatsSearchTextChanged);
    g_statsResultsList->eventListChangePosition += MyGUI::newDelegate(&OnStatsResultsSelectionChanged);
    g_statsInputEdit->eventEditTextChange += MyGUI::newDelegate(&OnStatsInputTextChanged);
    g_statsSetButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnStatsSetButtonPressed);
    g_statsAddButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnStatsAddButtonPressed);
    g_statsSubtractButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnStatsSubtractButtonPressed);
    g_saveLocationNameEdit->eventEditTextChange += MyGUI::newDelegate(&OnSaveLocationNameTextChanged);
    g_saveLocationNameEdit->eventEditSelectAccept += MyGUI::newDelegate(&OnSaveLocationNameAccepted);
    g_saveSelectedLocationButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnSaveSelectedLocationButtonPressed);
    g_savedLocationsCollapseButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnSavedLocationsCollapseButtonPressed);
    g_savedLocationSearchEdit->eventEditTextChange += MyGUI::newDelegate(&OnSavedLocationSearchTextChanged);
    g_savedLocationsListBox->eventListChangePosition += MyGUI::newDelegate(&OnSavedLocationsListSelectionChanged);
    g_savedLocationTeleportButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnSavedLocationTeleportButtonPressed);
    g_savedLocationPinButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnSavedLocationPinButtonPressed);
    g_savedLocationRenameButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnSavedLocationRenameButtonPressed);
    g_savedLocationDeleteButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnSavedLocationDeleteButtonPressed);
    g_addMoneyButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnAddMoneyButtonPressed);
    g_itemCategoryDropdown->eventComboChangePosition += MyGUI::newDelegate(&OnInventoryCategoryChanged);
    g_itemSearchEdit->eventEditTextChange += MyGUI::newDelegate(&OnInventoryItemSearchTextChanged);
    g_itemSearchEdit->eventKeySetFocus += MyGUI::newDelegate(&OnInventoryItemSearchFocusChanged);
    g_itemSearchEdit->eventKeyLostFocus += MyGUI::newDelegate(&OnInventoryItemSearchFocusChanged);
    g_itemSearchEdit->eventKeyButtonPressed += MyGUI::newDelegate(&OnInventoryItemSearchKeyPressed);
    g_itemSearchEdit->eventKeyButtonReleased += MyGUI::newDelegate(&OnInventoryItemSearchKeyReleased);
    g_itemSearchResultsList->eventListChangePosition += MyGUI::newDelegate(&OnInventorySearchResultsSelectionChanged);
    g_itemQualityDropdown->eventComboChangePosition += MyGUI::newDelegate(&OnInventoryQualityChanged);
    g_spawnItemButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnSpawnItemButtonPressed);
    g_spawnCategoryDropdown->eventComboChangePosition += MyGUI::newDelegate(&OnSpawnCategoryChanged);
    g_spawnSearchEdit->eventEditTextChange += MyGUI::newDelegate(&OnSpawnSearchTextChanged);
    g_spawnResultsList->eventListChangePosition += MyGUI::newDelegate(&OnSpawnResultsSelectionChanged);
    g_spawnQuantityEdit->eventEditTextChange += MyGUI::newDelegate(&OnSpawnQuantityTextChanged);
    g_spawnFactionDropdown->eventComboChangePosition += MyGUI::newDelegate(&OnSpawnFactionModeChanged);
    g_spawnCustomFactionSearchEdit->eventEditTextChange += MyGUI::newDelegate(&OnSpawnCustomFactionTextChanged);
    g_spawnCustomFactionResultsList->eventListChangePosition += MyGUI::newDelegate(&OnSpawnCustomFactionResultsSelectionChanged);
    g_spawnAllegianceDropdown->eventComboChangePosition += MyGUI::newDelegate(&OnSpawnAllegianceChanged);
    g_spawnRadiusDropdown->eventComboChangePosition += MyGUI::newDelegate(&OnSpawnRadiusChanged);
    g_spawnCreatureAgeDropdown->eventComboChangePosition += MyGUI::newDelegate(&OnSpawnCreatureAgeChanged);
    g_spawnModeDropdown->eventComboChangePosition += MyGUI::newDelegate(&OnSpawnModeChanged);
    g_spawnCharactersButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnSpawnCharactersButtonPressed);
    g_constructionTabButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnConstructionTabButtonPressed);
    g_constructionFinishButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnFinishSelectedConstructionButtonPressed);
    g_forceDyingButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnForceDyingButtonPressed);
    g_headerFrame->eventMouseButtonPressed += MyGUI::newDelegate(&OnHeaderMousePressed);
    g_headerFrame->eventMouseDrag += MyGUI::newDelegate(&OnHeaderMouseDrag);
    g_headerFrame->eventMouseMove += MyGUI::newDelegate(&OnHeaderMouseMove);
    g_headerFrame->eventMouseButtonReleased += MyGUI::newDelegate(&OnHeaderMouseReleased);

    ApplyPanelResponsiveBodyLayout();
    TargetSnapshot snapshot;
    ResetTargetSnapshot(&snapshot);
    ApplyTargetSnapshotToUi(snapshot);
    RefreshConstructionUi(g_lastPlayerInterface);
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
} // namespace

void ConfigureTextWidget(MyGUI::TextBox* widget)
{
    if (!widget)
    {
        return;
    }

    widget->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
}

const char* ResolvePanelHeaderTitleFontName(int fontHeight)
{
    if (fontHeight <= 17)
    {
        return "Kenshi_PaintedTextFont_Small";
    }

    if (fontHeight <= 23)
    {
        return "Kenshi_PaintedTextFont_Medium";
    }

    return "Kenshi_PaintedTextFont_Large";
}

void ApplyPanelHeaderTitleFont()
{
    if (!g_headerTitleText)
    {
        return;
    }

    TrySetTextWidgetFontName(g_headerTitleText, ResolvePanelHeaderTitleFontName(g_panelHeaderTitleFontHeight));
}

void ApplyPanelTabButtonLayout()
{
    if (!g_healthTabButton || !g_statsTabButton || !g_teleportTabButton || !g_inventoryTabButton || !g_spawnTabButton
        || !g_constructionTabButton)
    {
        return;
    }

    const int topTabRowWidth = g_healthTabButtonWidth + g_statsTabButtonWidth + g_teleportTabButtonWidth
        + g_inventoryTabButtonWidth + g_spawnTabButtonWidth;
    const int topTabRowLeft = std::max(0, (kPanelWidth - topTabRowWidth) / 2);
    const int constructionTabLeft = std::max(0, (kPanelWidth - g_constructionTabButtonWidth) / 2);

    g_healthTabButton->setCoord(BuildBodyCoord(topTabRowLeft + 0, 170, g_healthTabButtonWidth, 28));
    g_statsTabButton->setCoord(
        BuildBodyCoord(topTabRowLeft + g_healthTabButtonWidth, 170, g_statsTabButtonWidth, 28));
    g_teleportTabButton->setCoord(
        BuildBodyCoord(topTabRowLeft + g_healthTabButtonWidth + g_statsTabButtonWidth, 170, g_teleportTabButtonWidth, 28));
    g_inventoryTabButton->setCoord(BuildBodyCoord(
        topTabRowLeft + g_healthTabButtonWidth + g_statsTabButtonWidth + g_teleportTabButtonWidth,
        170,
        g_inventoryTabButtonWidth,
        28));
    g_spawnTabButton->setCoord(BuildBodyCoord(
        topTabRowLeft + g_healthTabButtonWidth + g_statsTabButtonWidth + g_teleportTabButtonWidth
            + g_inventoryTabButtonWidth,
        170,
        g_spawnTabButtonWidth,
        28));
    g_constructionTabButton->setCoord(BuildBodyCoord(constructionTabLeft, 198, g_constructionTabButtonWidth, 28));
}

void ApplyPanelResponsiveBodyLayout()
{
    const int bodyWidth = std::max(2, kPanelWidth - 40);
    const int splitGap = 8;
    const int leftColumnWidth = std::max(1, (bodyWidth - splitGap) / 2);
    const int rightColumnWidth = std::max(1, bodyWidth - leftColumnWidth - splitGap);
    const int leftColumnLeft = 20;
    const int rightColumnLeft = leftColumnLeft + leftColumnWidth + splitGap;

    const auto applySplitRow = [&](MyGUI::Widget* leftWidget, MyGUI::Widget* rightWidget, int top, int height) {
        if (leftWidget)
        {
            leftWidget->setCoord(BuildBodyCoord(leftColumnLeft, top, leftColumnWidth, height));
        }
        if (rightWidget)
        {
            rightWidget->setCoord(BuildBodyCoord(rightColumnLeft, top, rightColumnWidth, height));
        }
    };

    applySplitRow(g_targetNameText, g_targetFactionText, 74, 18);
    applySplitRow(g_targetAlignmentText, g_targetMembershipText, 96, 18);
    applySplitRow(g_targetStateText, g_noTargetText, 118, 18);
    applySplitRow(g_damageLeftArmButton, g_damageRightArmButton, 348, 28);
    applySplitRow(g_damageLeftLegButton, g_damageRightLegButton, 382, 28);
    applySplitRow(g_itemQuantityEdit, g_spawnItemButton, 608, 28);
    applySplitRow(g_moneyAmountEdit, g_addMoneyButton, 258, 28);
    applySplitRow(g_statsCopyButton, g_statsPasteButton, 338, 28);
    applySplitRow(g_spawnQuantityEdit, g_spawnRadiusDropdown, 554, 28);
    applySplitRow(g_spawnAllegianceDropdown, g_spawnModeDropdown, 610, 30);
    applySplitRow(g_spawnFactionDropdown, g_spawnCreatureAgeDropdown, 668, 30);

    if (g_saveLocationNameLabelText)
    {
        g_saveLocationNameLabelText->setCoord(BuildBodyCoord(20, 260, kPanelWidth - 40, 18));
    }

    if (g_saveLocationNameEdit)
    {
        g_saveLocationNameEdit->setCoord(BuildBodyCoord(20, 282, kPanelWidth - 40, 28));
    }

    if (g_saveSelectedLocationButton)
    {
        g_saveSelectedLocationButton->setCoord(BuildBodyCoord(20, 316, std::max(1, kPanelWidth - 76), 28));
    }

    if (g_savedLocationsSectionText)
    {
        g_savedLocationsSectionText->setCoord(BuildBodyCoord(14, 354, kPanelWidth - 60, 20));
    }

    if (g_savedLocationsCollapseButton)
    {
        g_savedLocationsCollapseButton->setCoord(BuildBodyCoord(kPanelWidth - 46, 322, 26, 28));
    }

    if (g_savedLocationsRowsRoot)
    {
        g_savedLocationsRowsRoot->setCoord(BuildBodyCoord(20, 382, kPanelWidth - 40, kSavedLocationsSectionContentHeight));
    }

    if (g_savedLocationSearchLabelText)
    {
        g_savedLocationSearchLabelText->setCoord(MyGUI::IntCoord(0, 0, kPanelWidth - 40, 18));
    }

    if (g_savedLocationSearchEdit)
    {
        g_savedLocationSearchEdit->setCoord(MyGUI::IntCoord(0, 22, kPanelWidth - 40, 28));
    }

    if (g_savedLocationsListBox)
    {
        g_savedLocationsListBox->setCoord(MyGUI::IntCoord(0, 56, kPanelWidth - 40, kSavedLocationsListHeight));
    }

    if (g_savedLocationsEmptyText)
    {
        g_savedLocationsEmptyText->setCoord(MyGUI::IntCoord(0, 56, kPanelWidth - 40, kSavedLocationEmptyHeight));
    }

    if (g_savedLocationTeleportButton)
    {
        g_savedLocationTeleportButton->setCoord(MyGUI::IntCoord(0, 182, 86, 28));
    }

    if (g_savedLocationPinButton)
    {
        g_savedLocationPinButton->setCoord(MyGUI::IntCoord(92, 182, 64, 28));
    }

    if (g_savedLocationRenameButton)
    {
        g_savedLocationRenameButton->setCoord(MyGUI::IntCoord(162, 182, 70, 28));
    }

    if (g_savedLocationDeleteButton)
    {
        g_savedLocationDeleteButton->setCoord(MyGUI::IntCoord(238, 182, 62, 28));
    }
}

void ApplyPanelLayout()
{
    if (!g_panel)
    {
        return;
    }

    ApplyPanelLayout(ResolvePanelCoord());
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
    g_armedDangerousHealthAction = DangerousHealthAction_None;
    g_dangerousHealthActionArmedAtMs = 0;
}

void SetActivePanelTab(PanelTab tab)
{
    if (g_activePanelTab == tab)
    {
        return;
    }

    g_activePanelTab = tab;
    if (g_bodyScrollView)
    {
        g_bodyScrollView->setViewOffset(MyGUI::IntPoint(0, 0));
    }
    ApplyPanelLayout();
    RefreshConstructionUi(g_lastPlayerInterface);
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

    const int initialBodyTop = GetPanelBodyTop();

    g_headerBackground = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(0, 0, kPanelWidth, kPanelHeaderHeight),
        MyGUI::Align::Default);
    g_headerFrame = g_panel->createWidget<MyGUI::Widget>(
        "PanelEmpty",
        MyGUI::IntCoord(0, 0, kPanelWidth, kPanelHeaderHeight),
        MyGUI::Align::Default);
    g_headerTitleText = g_headerFrame->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxPaintedText_Large",
        MyGUI::IntCoord(12, 0, kPanelWidth - 24, kPanelHeaderHeight),
        MyGUI::Align::Default);
    g_collapseButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(
            kPanelWidth - kPanelHeaderButtonRightPadding - g_panelCloseButtonSize - kPanelHeaderButtonGap - g_panelCollapseButtonSize,
            (kPanelHeaderHeight - g_panelCollapseButtonSize) / 2,
            g_panelCollapseButtonSize,
            g_panelCollapseButtonSize),
        MyGUI::Align::Default);
    g_closeButton = CreatePanelCloseButton(
        g_panel,
        MyGUI::IntCoord(
            kPanelWidth - kPanelHeaderButtonRightPadding - g_panelCloseButtonSize,
            (kPanelHeaderHeight - g_panelCloseButtonSize) / 2,
            g_panelCloseButtonSize,
            g_panelCloseButtonSize));
    g_bodyFrame = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(0, initialBodyTop, kPanelWidth, kPanelExpandedHeight - initialBodyTop),
        MyGUI::Align::Default);
    g_bodyScrollView = CreatePanelBodyScrollView(
        g_panel,
        MyGUI::IntCoord(0, initialBodyTop, kPanelWidth, kPanelExpandedHeight - initialBodyTop));

    MyGUI::Widget* bodyParent = 0;
    if (g_bodyScrollView)
    {
        bodyParent = g_bodyScrollView->getClientWidget();
        if (!bodyParent)
        {
            bodyParent = g_bodyScrollView;
        }
    }
    if (!bodyParent)
    {
        DestroyPanel();
        if (!g_loggedPanelCreateFailure)
        {
            LogErrorLine("failed to create panel body scroll view");
            g_loggedPanelCreateFailure = true;
        }
        return;
    }

    g_targetSectionText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxPaintedText",
        BuildBodyCoord(14, 52, kPanelWidth - 28, 20),
        MyGUI::Align::Default);
    g_targetNameText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 74, 156, 18),
        MyGUI::Align::Default);
    g_targetFactionText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(184, 74, 156, 18),
        MyGUI::Align::Default);
    g_targetAlignmentText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 96, 156, 18),
        MyGUI::Align::Default);
    g_targetMembershipText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(184, 96, 156, 18),
        MyGUI::Align::Default);
    g_targetStateText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 118, 156, 18),
        MyGUI::Align::Default);
    g_noTargetText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(184, 118, 156, 18),
        MyGUI::Align::Default);
    g_healthTabButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(0, 170, g_healthTabButtonWidth, 28),
        MyGUI::Align::Default);
    g_statsTabButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(0, 170, g_statsTabButtonWidth, 28),
        MyGUI::Align::Default);
    g_teleportTabButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(0, 170, g_teleportTabButtonWidth, 28),
        MyGUI::Align::Default);
    g_inventoryTabButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(0, 170, g_inventoryTabButtonWidth, 28),
        MyGUI::Align::Default);
    g_spawnTabButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(0, 170, g_spawnTabButtonWidth, 28),
        MyGUI::Align::Default);
    g_constructionTabButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(0, 198, g_constructionTabButtonWidth, 28),
        MyGUI::Align::Default);
    ApplyPanelTabButtonLayout();
    g_statesSectionText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxPaintedText",
        BuildBodyCoord(14, 236, kPanelWidth - 28, 20),
        MyGUI::Align::Default);
    g_fullRestoreButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(20, 258, kPanelWidth - 40, 28),
        MyGUI::Align::Default);
    g_forceUnconsciousButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(20, 292, kPanelWidth - 40, 28),
        MyGUI::Align::Default);
    g_limbDamageSectionText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxPaintedText",
        BuildBodyCoord(14, 326, kPanelWidth - 28, 20),
        MyGUI::Align::Default);
    g_damageLeftArmButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(20, 348, 156, 28),
        MyGUI::Align::Default);
    g_damageRightArmButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(184, 348, 156, 28),
        MyGUI::Align::Default);
    g_damageLeftLegButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(20, 382, 156, 28),
        MyGUI::Align::Default);
    g_damageRightLegButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(184, 382, 156, 28),
        MyGUI::Align::Default);
    g_statsSectionText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxPaintedText",
        BuildBodyCoord(14, 236, kPanelWidth - 28, 20),
        MyGUI::Align::Default);
    g_statsScopeText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 260, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_statsApplyToAllButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(20, 282, kPanelWidth - 40, 28),
        MyGUI::Align::Default);
    g_statsClipboardText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 316, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_statsCopyButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(20, 338, 156, 28),
        MyGUI::Align::Default);
    g_statsPasteButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(184, 338, 156, 28),
        MyGUI::Align::Default);
    g_statsSectionFilterText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 372, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_statsAllSectionButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(20, 394, 70, 28),
        MyGUI::Align::Default);
    g_statsCommonSectionButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(96, 394, 78, 28),
        MyGUI::Align::Default);
    g_statsCoreSectionButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(180, 394, 70, 28),
        MyGUI::Align::Default);
    g_statsUtilitySectionButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(256, 394, 84, 28),
        MyGUI::Align::Default);
    g_statsCombatSectionButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(20, 428, 94, 28),
        MyGUI::Align::Default);
    g_statsWeaponsSectionButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(120, 428, 96, 28),
        MyGUI::Align::Default);
    g_statsLaborSectionButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(222, 428, 118, 28),
        MyGUI::Align::Default);
    g_statsSearchLabelText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 462, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_statsSearchEdit = bodyParent->createWidget<MyGUI::EditBox>(
        "Kenshi_EditBox",
        BuildBodyCoord(20, 484, kPanelWidth - 40, 28),
        MyGUI::Align::Default);
    g_statsResultCountText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 518, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_statsResultsList = bodyParent->createWidget<MyGUI::ListBox>(
        "Kenshi_ListBox",
        BuildBodyCoord(20, 540, kPanelWidth - 40, 114),
        MyGUI::Align::Default);
    g_statsSelectedSummaryText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 660, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_statsCurrentValueText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 682, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_statsInputLabelText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 704, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_statsInputEdit = bodyParent->createWidget<MyGUI::EditBox>(
        "Kenshi_EditBox",
        BuildBodyCoord(20, 726, 96, 28),
        MyGUI::Align::Default);
    g_statsSetButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(124, 726, 64, 28),
        MyGUI::Align::Default);
    g_statsAddButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(196, 726, 64, 28),
        MyGUI::Align::Default);
    g_statsSubtractButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(268, 726, 72, 28),
        MyGUI::Align::Default);
    g_statsPreviewText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 760, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_teleportSectionText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxPaintedText",
        BuildBodyCoord(14, 236, kPanelWidth - 28, 20),
        MyGUI::Align::Default);
    g_saveLocationNameLabelText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 260, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_saveLocationNameEdit = bodyParent->createWidget<MyGUI::EditBox>(
        "Kenshi_EditBox",
        BuildBodyCoord(20, 282, kPanelWidth - 40, 28),
        MyGUI::Align::Default);
    g_saveSelectedLocationButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(20, 316, kPanelWidth - 40, 28),
        MyGUI::Align::Default);
    g_savedLocationsSectionText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxPaintedText",
        BuildBodyCoord(14, 354, kPanelWidth - 60, 20),
        MyGUI::Align::Default);
    g_savedLocationsCollapseButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(kPanelWidth - 46, 322, 26, 28),
        MyGUI::Align::Default);
    g_savedLocationsRowsRoot = bodyParent->createWidget<MyGUI::Widget>(
        "PanelEmpty",
        BuildBodyCoord(20, 382, kPanelWidth - 40, kSavedLocationsSectionContentHeight),
        MyGUI::Align::Default);
    g_savedLocationSearchLabelText = g_savedLocationsRowsRoot->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(0, 0, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_savedLocationSearchEdit = g_savedLocationsRowsRoot->createWidget<MyGUI::EditBox>(
        "Kenshi_EditBox",
        MyGUI::IntCoord(0, 22, kPanelWidth - 40, 28),
        MyGUI::Align::Default);
    g_savedLocationsListBox = g_savedLocationsRowsRoot->createWidget<MyGUI::ListBox>(
        "Kenshi_ListBox",
        MyGUI::IntCoord(0, 56, kPanelWidth - 40, kSavedLocationsListHeight),
        MyGUI::Align::Default);
    g_savedLocationsEmptyText = g_savedLocationsRowsRoot->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(0, 56, kPanelWidth - 40, kSavedLocationEmptyHeight),
        MyGUI::Align::Default);
    g_savedLocationTeleportButton = g_savedLocationsRowsRoot->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(0, 182, 86, 28),
        MyGUI::Align::Default);
    g_savedLocationPinButton = g_savedLocationsRowsRoot->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(92, 182, 64, 28),
        MyGUI::Align::Default);
    g_savedLocationRenameButton = g_savedLocationsRowsRoot->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(162, 182, 70, 28),
        MyGUI::Align::Default);
    g_savedLocationDeleteButton = g_savedLocationsRowsRoot->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(238, 182, 62, 28),
        MyGUI::Align::Default);
    g_inventorySectionText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxPaintedText",
        BuildBodyCoord(14, 236, kPanelWidth - 28, 20),
        MyGUI::Align::Default);
    g_moneyAmountLabelText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 236, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_moneyAmountEdit = bodyParent->createWidget<MyGUI::EditBox>(
        "Kenshi_EditBox",
        BuildBodyCoord(20, 258, 156, 28),
        MyGUI::Align::Default);
    g_addMoneyButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(184, 258, 156, 28),
        MyGUI::Align::Default);
    g_spawnFoodSectionText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxPaintedText",
        BuildBodyCoord(14, 302, kPanelWidth - 28, 20),
        MyGUI::Align::Default);
    g_itemCategoryLabelText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 324, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_itemCategoryDropdown = bodyParent->createWidget<MyGUI::ComboBox>(
        "Kenshi_ComboBox",
        BuildBodyCoord(20, 346, kPanelWidth - 40, 30),
        MyGUI::Align::Default);
    g_itemSearchLabelText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 382, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_itemSearchEdit = bodyParent->createWidget<MyGUI::EditBox>(
        "Kenshi_EditBox",
        BuildBodyCoord(20, 404, kPanelWidth - 40, 28),
        MyGUI::Align::Default);
    g_itemSearchResultsList = bodyParent->createWidget<MyGUI::ListBox>(
        "Kenshi_ListBox",
        BuildBodyCoord(20, 438, kPanelWidth - 40, 86),
        MyGUI::Align::Default);
    g_itemQualityLabelText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 530, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_itemQualityDropdown = bodyParent->createWidget<MyGUI::ComboBox>(
        "Kenshi_ComboBox",
        BuildBodyCoord(20, 552, kPanelWidth - 40, 30),
        MyGUI::Align::Default);
    g_itemQuantityLabelText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 586, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_itemQuantityEdit = bodyParent->createWidget<MyGUI::EditBox>(
        "Kenshi_EditBox",
        BuildBodyCoord(20, 608, 156, 28),
        MyGUI::Align::Default);
    g_spawnItemButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(184, 608, 156, 28),
        MyGUI::Align::Default);
    g_spawnSectionText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxPaintedText",
        BuildBodyCoord(14, 236, kPanelWidth - 28, 20),
        MyGUI::Align::Default);
    g_spawnCategoryLabelText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 258, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_spawnCategoryDropdown = bodyParent->createWidget<MyGUI::ComboBox>(
        "Kenshi_ComboBox",
        BuildBodyCoord(20, 280, kPanelWidth - 40, 30),
        MyGUI::Align::Default);
    g_spawnSearchLabelText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 316, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_spawnSearchEdit = bodyParent->createWidget<MyGUI::EditBox>(
        "Kenshi_EditBox",
        BuildBodyCoord(20, 338, kPanelWidth - 40, 28),
        MyGUI::Align::Default);
    g_spawnResultCountText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 372, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_spawnResultsList = bodyParent->createWidget<MyGUI::ListBox>(
        "Kenshi_ListBox",
        BuildBodyCoord(20, 394, kPanelWidth - 40, 108),
        MyGUI::Align::Default);
    g_spawnSelectedSummaryText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 508, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_spawnQuantityLabelText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 532, 156, 18),
        MyGUI::Align::Default);
    g_spawnQuantityEdit = bodyParent->createWidget<MyGUI::EditBox>(
        "Kenshi_EditBox",
        BuildBodyCoord(20, 554, 156, 28),
        MyGUI::Align::Default);
    g_spawnRadiusLabelText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(184, 532, 156, 18),
        MyGUI::Align::Default);
    g_spawnRadiusDropdown = bodyParent->createWidget<MyGUI::ComboBox>(
        "Kenshi_ComboBox",
        BuildBodyCoord(184, 554, 156, 30),
        MyGUI::Align::Default);
    g_spawnAllegianceLabelText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 588, 156, 18),
        MyGUI::Align::Default);
    g_spawnAllegianceDropdown = bodyParent->createWidget<MyGUI::ComboBox>(
        "Kenshi_ComboBox",
        BuildBodyCoord(20, 610, 156, 30),
        MyGUI::Align::Default);
    g_spawnFactionLabelText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 646, 156, 18),
        MyGUI::Align::Default);
    g_spawnFactionDropdown = bodyParent->createWidget<MyGUI::ComboBox>(
        "Kenshi_ComboBox",
        BuildBodyCoord(20, 668, 156, 30),
        MyGUI::Align::Default);
    g_spawnCreatureAgeLabelText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(184, 646, 156, 18),
        MyGUI::Align::Default);
    g_spawnCreatureAgeDropdown = bodyParent->createWidget<MyGUI::ComboBox>(
        "Kenshi_ComboBox",
        BuildBodyCoord(184, 668, 156, 30),
        MyGUI::Align::Default);
    g_spawnCustomFactionLabelText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 704, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_spawnCustomFactionSearchEdit = bodyParent->createWidget<MyGUI::EditBox>(
        "Kenshi_EditBox",
        BuildBodyCoord(20, 726, kPanelWidth - 40, 28),
        MyGUI::Align::Default);
    g_spawnCustomFactionResultsList = bodyParent->createWidget<MyGUI::ListBox>(
        "Kenshi_ListBox",
        BuildBodyCoord(20, 758, kPanelWidth - 40, 96),
        MyGUI::Align::Default);
    g_spawnModeLabelText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(184, 588, 156, 18),
        MyGUI::Align::Default);
    g_spawnModeDropdown = bodyParent->createWidget<MyGUI::ComboBox>(
        "Kenshi_ComboBox",
        BuildBodyCoord(184, 610, 156, 30),
        MyGUI::Align::Default);
    g_spawnCharactersButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(184, 862, 156, 28),
        MyGUI::Align::Default);
    g_spawnPreviewText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 896, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_constructionSectionText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxPaintedText",
        BuildBodyCoord(14, 236, kPanelWidth - 28, 20),
        MyGUI::Align::Default);
    g_constructionSelectedText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 260, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_constructionStatusText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, 282, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_constructionFinishButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(20, 314, kPanelWidth - 40, 28),
        MyGUI::Align::Default);
    g_dangerousSectionText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxPaintedText",
        BuildBodyCoord(14, 422, kPanelWidth - 28, 20),
        MyGUI::Align::Default);
    g_forceDeadButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(20, 444, kPanelWidth - 40, 28),
        MyGUI::Align::Default);
    g_forceDyingButton = bodyParent->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        BuildBodyCoord(20, 478, kPanelWidth - 40, 28),
        MyGUI::Align::Default);
    g_statusText = bodyParent->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        BuildBodyCoord(20, GetStatusTopInPanelCoords(), kPanelWidth - 40, 18),
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
    LogDebugLine(std::string("event=testkit_panel_created visible=") + (!g_panelHidden ? "true" : "false"));
}

void EnsurePanel(PlayerInterface* thisptr)
{
    g_lastPlayerInterface = thisptr;

    if (!g_panel)
    {
        CreatePanelWidgets();
    }

    TickPanelDrag();
    TickDangerousActionArmTimeout();
    UpdateCollapseButtonCaption();
    UpdateDangerousActionButtonCaptions();
    ApplyPanelLayout();

    if (!g_panelHidden && !g_panelCollapsed)
    {
        UpdateTargetInspection(thisptr);
    }
}

void TogglePanelHidden(const char* source)
{
    g_panelHidden = !g_panelHidden;

    if (g_panelHidden)
    {
        ClearDangerousActionArm("panel_hidden", false);
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
        ClearDangerousActionArm("panel_collapsed", false);
    }

    UpdateCollapseButtonCaption();
    ApplyPanelLayout();
    PersistCollapsedStateSetting();
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
        LogDebugLine(line.str());
        TogglePanelHidden("hotkey");
    }

    g_hotkeyPrevDown = hotkeyDown;
}
}
