#include "gtk_common.h"
#include "dir_defs.h"

#include "x11menu.h"
#include "x11info.h"
#include "x11cood.h"
#include "x11merge.h"
#include "x11lgnd.h"
#include "x11axis.h"
#include "x11file.h"
#include "x11parameter.h"
#include "x11graph.h"
#include "x11print.h"
#include "x11opt.h"
#include "x11view.h"
#include "ox11menu.h"

static void
help_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmHelpHelp();
}

static void
about_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmHelpAbout();
}

static void
demo_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmHelpDemo();
}

static void
quit_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmGraphQuit();
}

static void
GraphNewFrameAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmGraphNewMenu(MenuIdGraphNewFrame);
}

static void
GraphNewSectionAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmGraphNewMenu(MenuIdGraphNewSection);
}

static void
GraphNewCrossAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmGraphNewMenu(MenuIdGraphNewCross);
}

static void
GraphNewClearAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmGraphNewMenu(MenuIdGraphAllClear);
}

static void
GraphLoadAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmGraphLoad();
}

static void
GraphSaveAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmGraphOverWrite();
}

static void
GraphSaveAsAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmGraphSave();
}

static void
GraphExportGRAAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOutputMenu(MenuIdOutputGRAFile);
}

static void
GraphExportPSAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOutputMenu(MenuIdOutputPSFile);
}

static void
GraphExportEPSAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOutputMenu(MenuIdOutputEPSFile);
}

static void
GraphExportPDFAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOutputMenu(MenuIdOutputPDFFile);
}

static void
GraphExportSVGAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOutputMenu(MenuIdOutputSVGFile);
}

static void
GraphExportPNGAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOutputMenu(MenuIdOutputPNGFile);
}

#if WINDOWS
static void
GraphExportEMFAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOutputMenu(MenuIdOutputEMFFile);
}

static void
GraphExportEMFPlusAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOutputMenu(MenuIdOutputEMFPlusFile);
}

static void
GraphExportEMFClipboardAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOutputMenu(MenuIdOutputEMFClipboard);
}

static void
GraphExportEMFPlusClipboardAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOutputMenu(MenuIdOutputEMFPlusClipboard);
}
#endif

static void
GraphDrawOrderAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmGraphSwitch();
}

static void
GraphPageSetupAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmGraphPage(FALSE);
}

static void
GraphPrintPreviewAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOutputViewerB();
}

static void
GraphPrintAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOutputPrinterB();
}

static void
GraphCurrentDirectoryAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmGraphDirectory();
}

static void
GraphShellAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmGraphShell();
}

static void
GraphAddinAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  int i, n;
  struct script *fcur;

  if (Menulock || Globallock) {
    return;
  }

  n = g_variant_get_int32(parameter);
  if (n < 0) {
    return;
  }

  fcur = Menulocal.scriptroot;
  for (i = 0; i < n; i++) {
    if (fcur == NULL) {
      return;
    }
    fcur = fcur->next;
  }

  script_exec(fcur);
}

static void
EditRedoAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(MenuIdEditRedo);
}

static void
EditUndoAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(MenuIdEditUndo);
}

static void
EditCutAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(MenuIdEditCut);
}

static void
EditCopyAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(MenuIdEditCopy);
}

static void
EditPasteAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(MenuIdEditPaste);
}

static void
EditDeleteAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(MenuIdEditDelete);
}

static void
EditDuplicateAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(MenuIdEditDuplicate);
}

static void
EditSelectAllAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(MenuIdEditSelectAll);
}

static void
EditOrderTopAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(MenuIdEditOrderTop);
}

static void
EditOrderUpAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(MenuIdEditOrderUp);
}

static void
EditOrderDownAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(MenuIdEditOrderDown);
}

static void
EditOrderBottomAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(MenuIdEditOrderBottom);
}

static void
EditAlignLeftAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(MenuIdAlignLeft);
}

static void
EditAlignVCenterAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(MenuIdAlignVCenter);
}

static void
EditAlignRightAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(MenuIdAlignRight);
}

static void
EditAlignTopAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(MenuIdAlignTop);
}

static void
EditAlignHCenterAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(MenuIdAlignHCenter);
}

static void
EditAlignBottomAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(MenuIdAlignBottom);
}

static void
EditRotateCWAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(MenuIdEditRotateCW);
}

static void
EditRotateCCWAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(MenuIdEditRotateCCW);
}

static void
EditFlipHAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(MenuIdEditFlipHorizontally);
}

static void
EditFlipVAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(MenuIdEditFlipVertically);
}

static void
ViewDrawDirectAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmViewerDraw(FALSE);
}

static void
ViewDrawAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmViewerDraw(TRUE);
}

static void
ViewClearInformationWindowAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  if (Menulock || Globallock) {
    return;
  }
  InfoWinClear();
}

static void
toggle_action(GSimpleAction *action, GVariant *parameter, enum MenuID id)
{
  int state;

  if (Menulock || Globallock) {
    return;
  }
  state = g_variant_get_boolean(parameter);
  if (toggle_view(id, state)) {
    g_simple_action_set_state(action, parameter);
  }
}

static void
ViewSidebarAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  toggle_action(action, parameter, MenuIdToggleSidebar);
}

static void
ViewStatusbarAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  toggle_action(action, parameter, MenuIdToggleStatusbar);
}

static void
ViewRulerAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  toggle_action(action, parameter, MenuIdToggleRuler);
}

static void
ViewScrollbarAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  toggle_action(action, parameter, MenuIdToggleScrollbar);
}

static void
ViewCommandToolbarAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  toggle_action(action, parameter, MenuIdToggleCToolbar);
}

static void
ViewToolboxAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  toggle_action(action, parameter, MenuIdTogglePToolbar);
}

static void
ViewCrossGaugeAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  toggle_action(action, parameter, MenuIdToggleCrossGauge);
}

static void
ViewGridLineAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  toggle_action(action, parameter, MenuIdToggleGridLine);
}

static void
DataAddFileAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmFileOpen(NULL, NULL, app);
}

static void
DataAddRangeAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmRangeAdd(NULL, NULL, app);
}

static void
DataPropertyAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmFileUpdate();
}

static void
DataCloseAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmFileClose();
}

static void
DataEditAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmFileEdit();
}

static void
DataSaveAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmFileSaveData();
}

static void
DataMathAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmFileMath();
}

static void
ParameterAddAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmParameterAdd();
}

static void
ParameterPropertyAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmParameterUpdate();
}

static void
ParameterDeleteAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmParameterDelete();
}

static void
AxisAddFrameAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmAxisAddFrame(NULL, NULL, app);
}

static void
AxisAddSectionAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmAxisAddSection(NULL, NULL, app);
}

static void
AxisAddCrossAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmAxisAddCross(NULL, NULL, app);
}

static void
AxisAddSingleAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmAxisAddSingle(NULL, NULL, app);
}

static void
AxisPropertyAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmAxisUpdate();
}

static void
AxisDeleteAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmAxisDel();
}

static void
AxisScaleZoomAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmAxisZoom();
}

static void
AxisScaleClearAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmAxisClear();
}

static void
AxisScaleUndoAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmAxisScaleUndo();
}

static void
AxisGridNewAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmAxisGridNew();
}

static void
AxisGridPropertyAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmAxisGridUpdate();
}

static void
AxisGridDeleteAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmAxisGridDel();
}

static void
LegendPathPropertyAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmLineUpdate();
}

static void
LegendPathDeleteAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmLineDel();
}

static void
LegendRectanglePropertyAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmRectUpdate();
}

static void
LegendRectangleDeleteAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmRectDel();
}

static void
LegendArcPropertyAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmArcUpdate();
}

static void
LegendArcDeleteAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmArcDel();
}

static void
LegendMarkPropertyAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmMarkUpdate();
}

static void
LegendMarkDeleteAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmMarkDel();
}

static void
LegendTextPropertyAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmTextUpdate();
}

static void
LegendTextDeleteAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmTextDel();
}

static void
MergeAddAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmMergeOpen(NULL, NULL, app);
}

static void
MergePropertyAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmMergeUpdate();
}

static void
MergeCloseAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmMergeClose();
}

static void
PreferenceViewerAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOptionViewer();
}

static void
PreferenceExternalViewerAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOptionExtViewer();
}

static void
PreferenceFontAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOptionPrefFont();
}

static void
PreferenceAddinAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOptionScript();
}

static void
PreferenceMiscAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOptionMisc();
}

static void
PreferenceSaveSettingAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOptionSaveDefault();
}

static void
PreferenceSaveGraphAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOptionSaveNgp();
}

static void
PreferenceDataDefaultAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOptionFileDef();
}

static void
PreferenceTextDefaultAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOptionTextDef();
}

static void
PreferenceGridDefaultAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOptionGridDef();
}

static void
PopupUpdateAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  ViewerUpdateCB();
}

static GActionEntry AppEntries[] =
{
  { "help", help_activated, NULL, NULL, NULL },
  { "about", about_activated, NULL, NULL, NULL },
  { "demo", demo_activated, NULL, NULL, NULL },
  { "quit", quit_activated, NULL, NULL, NULL },
  { "preferences", PreferenceMiscAction_activated, NULL, NULL, NULL },
  { "GraphNewFrameAction", GraphNewFrameAction_activated, NULL, NULL, NULL },
  { "GraphNewSectionAction", GraphNewSectionAction_activated, NULL, NULL, NULL },
  { "GraphNewCrossAction", GraphNewCrossAction_activated, NULL, NULL, NULL },
  { "GraphNewClearAction", GraphNewClearAction_activated, NULL, NULL, NULL },
  { "GraphLoadAction", GraphLoadAction_activated, NULL, NULL, NULL },
  { "GraphSaveAction", GraphSaveAction_activated, NULL, NULL, NULL },
  { "GraphSaveAsAction", GraphSaveAsAction_activated, NULL, NULL, NULL },
  { "GraphExportGRAAction", GraphExportGRAAction_activated, NULL, NULL, NULL },
  { "GraphExportPSAction", GraphExportPSAction_activated, NULL, NULL, NULL },
  { "GraphExportEPSAction", GraphExportEPSAction_activated, NULL, NULL, NULL },
  { "GraphExportPDFAction", GraphExportPDFAction_activated, NULL, NULL, NULL },
  { "GraphExportSVGAction", GraphExportSVGAction_activated, NULL, NULL, NULL },
  { "GraphExportPNGAction", GraphExportPNGAction_activated, NULL, NULL, NULL },
#if WINDOWS
  { "GraphExportEMFAction", GraphExportEMFAction_activated, NULL, NULL, NULL },
  { "GraphExportEMFPlusAction", GraphExportEMFPlusAction_activated, NULL, NULL, NULL },
  { "GraphExportEMFClipboardAction", GraphExportEMFClipboardAction_activated, NULL, NULL, NULL },
  { "GraphExportEMFPlusClipboardAction", GraphExportEMFPlusClipboardAction_activated, NULL, NULL, NULL },
#endif
  { "GraphDrawOrderAction", GraphDrawOrderAction_activated, NULL, NULL, NULL },
  { "GraphPageSetupAction", GraphPageSetupAction_activated, NULL, NULL, NULL },
  { "GraphPrintPreviewAction", GraphPrintPreviewAction_activated, NULL, NULL, NULL },
  { "GraphPrintAction", GraphPrintAction_activated, NULL, NULL, NULL },
  { "GraphCurrentDirectoryAction", GraphCurrentDirectoryAction_activated, NULL, NULL, NULL },
  { "GraphAddinAction", GraphAddinAction_activated, "i", NULL, NULL },
  { "GraphShellAction", GraphShellAction_activated, NULL, NULL, NULL },
  { "EditRedoAction", EditRedoAction_activated, NULL, NULL, NULL },
  { "EditUndoAction", EditUndoAction_activated, NULL, NULL, NULL },
  { "EditCutAction", EditCutAction_activated, NULL, NULL, NULL },
  { "EditCopyAction", EditCopyAction_activated, NULL, NULL, NULL },
  { "EditPasteAction", EditPasteAction_activated, NULL, NULL, NULL },
  { "EditDeleteAction", EditDeleteAction_activated, NULL, NULL, NULL },
  { "EditDuplicateAction", EditDuplicateAction_activated, NULL, NULL, NULL },
  { "EditSelectAllAction", EditSelectAllAction_activated, NULL, NULL, NULL },
  { "EditOrderTopAction", EditOrderTopAction_activated, NULL, NULL, NULL },
  { "EditOrderUpAction", EditOrderUpAction_activated, NULL, NULL, NULL },
  { "EditOrderDownAction", EditOrderDownAction_activated, NULL, NULL, NULL },
  { "EditOrderBottomAction", EditOrderBottomAction_activated, NULL, NULL, NULL },
  { "EditAlignLeftAction", EditAlignLeftAction_activated, NULL, NULL, NULL },
  { "EditAlignHCenterAction", EditAlignHCenterAction_activated, NULL, NULL, NULL },
  { "EditAlignRightAction", EditAlignRightAction_activated, NULL, NULL, NULL },
  { "EditAlignTopAction", EditAlignTopAction_activated, NULL, NULL, NULL },
  { "EditAlignVCenterAction", EditAlignVCenterAction_activated, NULL, NULL, NULL },
  { "EditAlignBottomAction", EditAlignBottomAction_activated, NULL, NULL, NULL },
  { "EditRotateCWAction", EditRotateCWAction_activated, NULL, NULL, NULL },
  { "EditRotateCCWAction", EditRotateCCWAction_activated, NULL, NULL, NULL },
  { "EditFlipHAction", EditFlipHAction_activated, NULL, NULL, NULL },
  { "EditFlipVAction", EditFlipVAction_activated, NULL, NULL, NULL },
  { "ViewDrawDirectAction", ViewDrawDirectAction_activated, NULL, NULL, NULL },
  { "ViewDrawAction", ViewDrawAction_activated, NULL, NULL, NULL },
  { "ViewClearInformationWindowAction", ViewClearInformationWindowAction_activated, NULL, NULL, NULL },
  { "ViewSidebarAction", NULL, NULL, "true", ViewSidebarAction_activated },
  { "ViewStatusbarAction", NULL, NULL, "true", ViewStatusbarAction_activated },
  { "ViewRulerAction", NULL, NULL, "true", ViewRulerAction_activated },
  { "ViewScrollbarAction", NULL, NULL, "true", ViewScrollbarAction_activated },
  { "ViewCommandToolbarAction", NULL, NULL, "true", ViewCommandToolbarAction_activated },
  { "ViewToolboxAction", NULL, NULL, "true", ViewToolboxAction_activated },
  { "ViewCrossGaugeAction", NULL, NULL, "true", ViewCrossGaugeAction_activated },
  { "ViewGridLineAction", NULL, NULL, "true", ViewGridLineAction_activated },
  { "DataAddFileAction", DataAddFileAction_activated, NULL, NULL, NULL },
  { "DataAddRangeAction", DataAddRangeAction_activated, NULL, NULL, NULL },
  { "DataPropertyAction", DataPropertyAction_activated, NULL, NULL, NULL },
  { "DataCloseAction", DataCloseAction_activated, NULL, NULL, NULL },
  { "DataEditAction", DataEditAction_activated, NULL, NULL, NULL },
  { "DataSaveAction", DataSaveAction_activated, NULL, NULL, NULL },
  { "DataMathAction", DataMathAction_activated, NULL, NULL, NULL },
  { "ParameterAddAction", ParameterAddAction_activated, NULL, NULL, NULL },
  { "ParameterPropertyAction", ParameterPropertyAction_activated, NULL, NULL, NULL },
  { "ParameterDeleteAction", ParameterDeleteAction_activated, NULL, NULL, NULL },
  { "AxisAddFrameAction", AxisAddFrameAction_activated, NULL, NULL, NULL },
  { "AxisAddSectionAction", AxisAddSectionAction_activated, NULL, NULL, NULL },
  { "AxisAddCrossAction", AxisAddCrossAction_activated, NULL, NULL, NULL },
  { "AxisAddSingleAction", AxisAddSingleAction_activated, NULL, NULL, NULL },
  { "AxisPropertyAction", AxisPropertyAction_activated, NULL, NULL, NULL },
  { "AxisDeleteAction", AxisDeleteAction_activated, NULL, NULL, NULL },
  { "AxisScaleZoomAction", AxisScaleZoomAction_activated, NULL, NULL, NULL },
  { "AxisScaleClearAction", AxisScaleClearAction_activated, NULL, NULL, NULL },
  { "AxisScaleUndoAction", AxisScaleUndoAction_activated, NULL, NULL, NULL },
  { "AxisGridNewAction", AxisGridNewAction_activated, NULL, NULL, NULL },
  { "AxisGridPropertyAction", AxisGridPropertyAction_activated, NULL, NULL, NULL },
  { "AxisGridDeleteAction", AxisGridDeleteAction_activated, NULL, NULL, NULL },
  { "LegendPathPropertyAction", LegendPathPropertyAction_activated, NULL, NULL, NULL },
  { "LegendPathDeleteAction", LegendPathDeleteAction_activated, NULL, NULL, NULL },
  { "LegendRectanglePropertyAction", LegendRectanglePropertyAction_activated, NULL, NULL, NULL },
  { "LegendRectangleDeleteAction", LegendRectangleDeleteAction_activated, NULL, NULL, NULL },
  { "LegendArcPropertyAction", LegendArcPropertyAction_activated, NULL, NULL, NULL },
  { "LegendArcDeleteAction", LegendArcDeleteAction_activated, NULL, NULL, NULL },
  { "LegendMarkPropertyAction", LegendMarkPropertyAction_activated, NULL, NULL, NULL },
  { "LegendMarkDeleteAction", LegendMarkDeleteAction_activated, NULL, NULL, NULL },
  { "LegendTextPropertyAction", LegendTextPropertyAction_activated, NULL, NULL, NULL },
  { "LegendTextDeleteAction", LegendTextDeleteAction_activated, NULL, NULL, NULL },
  { "MergeAddAction", MergeAddAction_activated, NULL, NULL, NULL },
  { "MergePropertyAction", MergePropertyAction_activated, NULL, NULL, NULL },
  { "MergeCloseAction", MergeCloseAction_activated, NULL, NULL, NULL },
  { "PreferenceViewerAction", PreferenceViewerAction_activated, NULL, NULL, NULL },
  { "PreferenceExternalViewerAction", PreferenceExternalViewerAction_activated, NULL, NULL, NULL },
  { "PreferenceFontAction", PreferenceFontAction_activated, NULL, NULL, NULL },
  { "PreferenceAddinAction", PreferenceAddinAction_activated, NULL, NULL, NULL },
  { "PreferenceMiscAction", PreferenceMiscAction_activated, NULL, NULL, NULL },
  { "PreferenceSaveSettingAction", PreferenceSaveSettingAction_activated, NULL, NULL, NULL },
  { "PreferenceSaveGraphAction", PreferenceSaveGraphAction_activated, NULL, NULL, NULL },
  { "PreferenceDataDefaultAction", PreferenceDataDefaultAction_activated, NULL, NULL, NULL },
  { "PreferenceTextDefaultAction", PreferenceTextDefaultAction_activated, NULL, NULL, NULL },
  { "PreferenceGridDefaultAction", PreferenceGridDefaultAction_activated, NULL, NULL, NULL },
  { "PopupUpdateAction", PopupUpdateAction_activated, NULL, NULL, NULL },
};

static int Initialized = FALSE;

void
setup_actions(GtkApplication *app)
{
  if (Initialized) {
    return;
  }
  g_action_map_add_action_entries(G_ACTION_MAP(app), AppEntries, G_N_ELEMENTS(AppEntries), app);
  Initialized = TRUE;
}
