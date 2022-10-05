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
  CmHelpHelp(NULL, NULL);
}

static void
about_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmHelpAbout(NULL, NULL);
}

static void
demo_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmHelpDemo(NULL, NULL);
}

static void
quit_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmGraphQuit(NULL, NULL);
}

static void
GraphNewFrameAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmGraphNewMenu(NULL, GINT_TO_POINTER(MenuIdGraphNewFrame));
}

static void
GraphNewSectionAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmGraphNewMenu(NULL, GINT_TO_POINTER(MenuIdGraphNewSection));
}

static void
GraphNewCrossAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmGraphNewMenu(NULL, GINT_TO_POINTER(MenuIdGraphNewCross));
}

static void
GraphNewClearAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmGraphNewMenu(NULL, GINT_TO_POINTER(MenuIdGraphAllClear));
}

static void
GraphLoadAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmGraphLoad(NULL, NULL);
}

static void
GraphSaveAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmGraphOverWrite(NULL, NULL);
}

static void
GraphSaveAsAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmGraphSave(NULL, NULL);
}

static void
GraphExportGRAAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOutputMenu(NULL, GINT_TO_POINTER(MenuIdOutputGRAFile));
}

static void
GraphExportPSAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOutputMenu(NULL, GINT_TO_POINTER(MenuIdOutputPSFile));
}

static void
GraphExportEPSAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOutputMenu(NULL, GINT_TO_POINTER(MenuIdOutputEPSFile));
}

static void
GraphExportPDFAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOutputMenu(NULL, GINT_TO_POINTER(MenuIdOutputPDFFile));
}

static void
GraphExportSVGAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOutputMenu(NULL, GINT_TO_POINTER(MenuIdOutputSVGFile));
}

static void
GraphExportPNGAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOutputMenu(NULL, GINT_TO_POINTER(MenuIdOutputPNGFile));
}

#if WINDOWS
static void
GraphExportEMFAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOutputMenu(NULL, GINT_TO_POINTER(MenuIdOutputEMFFile));
}

static void
GraphExportEMFClipboardAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOutputMenu(NULL, GINT_TO_POINTER(MenuIdOutputEMFClipboard));
}
#endif

static void
GraphDrawOrderAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmGraphSwitch(NULL, NULL);
}

static void
GraphPageSetupAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmGraphPage(NULL, GINT_TO_POINTER(FALSE));
}

static void
GraphPrintPreviewAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOutputViewerB(NULL, NULL);
}

static void
GraphPrintAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOutputPrinterB(NULL, NULL);
}

static void
GraphCurrentDirectoryAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmGraphDirectory(NULL, NULL);
}

static void
GraphShellAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmGraphShell(NULL, NULL);
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

  script_exec(NULL, fcur);
}

#if GTK_CHECK_VERSION(4, 0, 0)
static void
RecentGraphAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  char *fname;
  if (Menulock || Globallock) {
    return;
  }
  fname = g_strdup(g_variant_get_string(parameter, NULL));
  if (fname) {
    graph_dropped(fname);
  }
}
static void
RecentDataAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  const char *fname;
  fname = g_variant_get_string(parameter, NULL);
  if (fname) {
    load_data(fname);
  }
}
#endif

static void
EditRedoAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(NULL, GINT_TO_POINTER(MenuIdEditRedo));
}

static void
EditUndoAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(NULL, GINT_TO_POINTER(MenuIdEditUndo));
}

static void
EditCutAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(NULL, GINT_TO_POINTER(MenuIdEditCut));
}

static void
EditCopyAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(NULL, GINT_TO_POINTER(MenuIdEditCopy));
}

static void
EditPasteAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(NULL, GINT_TO_POINTER(MenuIdEditPaste));
}

static void
EditDeleteAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(NULL, GINT_TO_POINTER(MenuIdEditDelete));
}

static void
EditDuplicateAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(NULL, GINT_TO_POINTER(MenuIdEditDuplicate));
}

static void
EditSelectAllAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(NULL, GINT_TO_POINTER(MenuIdEditSelectAll));
}

static void
EditOrderTopAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(NULL, GINT_TO_POINTER(MenuIdEditOrderTop));
}

static void
EditOrderUpAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(NULL, GINT_TO_POINTER(MenuIdEditOrderUp));
}

static void
EditOrderDownAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(NULL, GINT_TO_POINTER(MenuIdEditOrderDown));
}

static void
EditOrderBottomAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(NULL, GINT_TO_POINTER(MenuIdEditOrderBottom));}

static void
EditAlignLeftAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(NULL, GINT_TO_POINTER(MenuIdAlignLeft));
}

static void
EditAlignVCenterAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(NULL, GINT_TO_POINTER(MenuIdAlignVCenter));
}

static void
EditAlignRightAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(NULL, GINT_TO_POINTER(MenuIdAlignRight));
}

static void
EditAlignTopAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(NULL, GINT_TO_POINTER(MenuIdAlignTop));
}

static void
EditAlignHCenterAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(NULL, GINT_TO_POINTER(MenuIdAlignHCenter));
}

static void
EditAlignBottomAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(NULL, GINT_TO_POINTER(MenuIdAlignBottom));
}

static void
EditRotateCWAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(NULL, GINT_TO_POINTER(MenuIdEditRotateCW));
}

static void
EditRotateCCWAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(NULL, GINT_TO_POINTER(MenuIdEditRotateCCW));
}

static void
EditFlipHAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(NULL, GINT_TO_POINTER(MenuIdEditFlipHorizontally));
}

static void
EditFlipVAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(NULL, GINT_TO_POINTER(MenuIdEditFlipVertically));
}

static void
ViewDrawDirectAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmViewerDraw(NULL, GINT_TO_POINTER(FALSE));
}

static void
ViewDrawAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmViewerDraw(NULL, GINT_TO_POINTER(TRUE));
}

static void
ViewClearInformationWindowAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  InfoWinClear();
}

static void
toggle_action(GSimpleAction *action, GVariant *parameter, enum MenuID id)
{
  int state;

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
#if GTK_CHECK_VERSION(4, 0, 0)
  CmFileOpen(NULL, NULL, app);
#else
  CmFileOpen(NULL, NULL);
#endif
}

static void
DataAddRangeAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
#if GTK_CHECK_VERSION(4, 0, 0)
  CmRangeAdd(NULL, NULL, app);
#else
  CmRangeAdd(NULL, NULL);
#endif
}

static void
DataPropertyAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmFileUpdate(NULL, NULL);
}

static void
DataCloseAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmFileClose(NULL, NULL);
}

static void
DataEditAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmFileEdit(NULL, NULL);
}

static void
DataSaveAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmFileSaveData(NULL, NULL);
}

static void
DataMathAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmFileMath(NULL, NULL);
}

static void
ParameterAddAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmParameterAdd(NULL, NULL);
}

static void
ParameterPropertyAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmParameterUpdate(NULL, NULL);
}

static void
ParameterDeleteAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmParameterDelete(NULL, NULL);
}

static void
AxisAddFrameAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
#if GTK_CHECK_VERSION(4, 0, 0)
  CmAxisAddFrame(NULL, NULL, app);
#else
  CmAxisAddFrame(NULL, NULL);
#endif
}

static void
AxisAddSectionAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
#if GTK_CHECK_VERSION(4, 0, 0)
  CmAxisAddSection(NULL, NULL, app);
#else
  CmAxisAddSection(NULL, NULL);
#endif
}

static void
AxisAddCrossAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
#if GTK_CHECK_VERSION(4, 0, 0)
  CmAxisAddCross(NULL, NULL, app);
#else
  CmAxisAddCross(NULL, NULL);
#endif
}

static void
AxisAddSingleAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
#if GTK_CHECK_VERSION(4, 0, 0)
  CmAxisAddSingle(NULL, NULL, app);
#else
  CmAxisAddSingle(NULL, NULL);
#endif
}

static void
AxisPropertyAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmAxisUpdate(NULL, NULL);
}

static void
AxisDeleteAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmAxisDel(NULL, NULL);
}

static void
AxisScaleZoomAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmAxisZoom(NULL, NULL);
}

static void
AxisScaleClearAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmAxisClear(NULL, NULL);
}

static void
AxisScaleUndoAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmAxisScaleUndo(NULL, NULL);
}

static void
AxisGridNewAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmAxisGridNew(NULL, NULL);
}

static void
AxisGridPropertyAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmAxisGridUpdate(NULL, NULL);
}

static void
AxisGridDeleteAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmAxisGridDel(NULL, NULL);
}

static void
LegendPathPropertyAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmLineUpdate(NULL, NULL);
}

static void
LegendPathDeleteAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmLineDel(NULL, NULL);
}

static void
LegendRectanglePropertyAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmRectUpdate(NULL, NULL);
}

static void
LegendRectangleDeleteAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmRectDel(NULL, NULL);
}

static void
LegendArcPropertyAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmArcUpdate(NULL, NULL);
}

static void
LegendArcDeleteAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmArcDel(NULL, NULL);
}

static void
LegendMarkPropertyAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmMarkUpdate(NULL, NULL);
}

static void
LegendMarkDeleteAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmMarkDel(NULL, NULL);
}

static void
LegendTextPropertyAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmTextUpdate(NULL, NULL);
}

static void
LegendTextDeleteAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmTextDel(NULL, NULL);
}

static void
MergeAddAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
#if GTK_CHECK_VERSION(4, 0, 0)
  CmMergeOpen(NULL, NULL, app);
#else
  CmMergeOpen(NULL, NULL);
#endif
}

static void
MergePropertyAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmMergeUpdate(NULL, NULL);
}

static void
MergeCloseAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmMergeClose(NULL, NULL);
}

static void
PreferenceViewerAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOptionViewer(NULL, NULL);
}

static void
PreferenceExternalViewerAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOptionExtViewer(NULL, NULL);
}

static void
PreferenceFontAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOptionPrefFont(NULL, NULL);
}

static void
PreferenceAddinAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOptionScript(NULL, NULL);
}

static void
PreferenceMiscAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOptionMisc(NULL, NULL);
}

static void
PreferenceSaveSettingAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOptionSaveDefault(NULL, NULL);
}

static void
PreferenceSaveGraphAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOptionSaveNgp(NULL, NULL);
}

static void
PreferenceDataDefaultAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOptionFileDef(NULL, NULL);
}

static void
PreferenceTextDefaultAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOptionTextDef(NULL, NULL);
}

static void
PreferenceGridDefaultAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmOptionGridDef(NULL, NULL);
}

static void
PopupUpdateAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  ViewerUpdateCB(NULL, NULL);
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
  { "GraphExportEMFClipboardAction", GraphExportEMFClipboardAction_activated, NULL, NULL, NULL },
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
#if GTK_CHECK_VERSION(4, 0, 0)
  { "RecentGraphAction", RecentGraphAction_activated, "s", NULL, NULL },
  { "RecentDataAction", RecentDataAction_activated, "s", NULL, NULL },
#endif
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
