#include "gtk_common.h"
#include "dir_defs.h"

#include "x11menu.h"
#include "x11info.h"
#include "x11cood.h"
#include "x11merge.h"
#include "x11lgnd.h"
#include "x11axis.h"
#include "x11file.h"
#include "x11graph.h"
#include "x11print.h"
#include "x11opt.h"
#include "x11view.h"
#include "ox11menu.h"

#define UI_FILE "menus.ui"

#if USE_APP_MENU
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
GraphRecentAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  show_recent_dialog(RECENT_TYPE_GRAPH);
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

static void
GraphDrawOrderAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmGraphSwitch(NULL, NULL);
}

static void
GraphPageSetupAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmGraphPage(NULL, NULL);
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

static void
EditCutAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(NULL, GINT_TO_POINTER(MenuIdEditCut));
}

static void
EditCopyAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmEditMenuCB(NULL, GINT_TO_POINTER(MenuIdEditCut));
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
EditFlipVActiopn_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
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
ViewClearAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmViewerClear(NULL, NULL);
}

static void
ViewClearInformationWindowAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  InfoWinClear();
}

static void
ViewSidebarAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  int state;

  state = g_variant_get_boolean(parameter);
  if (toggle_view(MenuIdToggleSidebar, state)) {
    g_simple_action_set_state(action, parameter);
  }
}

static void
ViewStatusbarAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  int state;

  state = g_variant_get_boolean(parameter);
  if (toggle_view(MenuIdToggleStatusbar, state)){
    g_simple_action_set_state(action, parameter);
  }
}

static void
ViewRulerAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  int state;

  state = g_variant_get_boolean(parameter);
  if (toggle_view(MenuIdToggleRuler, state)) {
    g_simple_action_set_state(action, parameter);
  }
}

static void
ViewScrollbarAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  int state;

  state = g_variant_get_boolean(parameter);
  if (toggle_view(MenuIdToggleScrollbar, state)) {
    g_simple_action_set_state(action, parameter);
  }
}

static void
ViewCommandToolbarAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  int state;

  state = g_variant_get_boolean(parameter);
  if (toggle_view(MenuIdToggleCToolbar, state)) {
    g_simple_action_set_state(action, parameter);
  }
}

static void
ViewToolboxAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  int state;

  state = g_variant_get_boolean(parameter);
  if (toggle_view(MenuIdTogglePToolbar, state)) {
    g_simple_action_set_state(action, parameter);
  }
}

static void
ViewCrossGaugeAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  int state;

  state = g_variant_get_boolean(parameter);
  if (toggle_view(MenuIdToggleCrossGauge, state)) {
    g_simple_action_set_state(action, parameter);
  }
}

static void
DataAddFileAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmFileOpen(NULL, NULL);
}

static void
DataAddRangeAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmRangeAdd(NULL, NULL);
}

static void
DataAddRecentFileAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  show_recent_dialog(RECENT_TYPE_DATA);
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
AxisAddFrameAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmAxisNewFrame(NULL, NULL);
}

static void
AxisAddSectionAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmAxisNewSection(NULL, NULL);
}

static void
AxisAddCrossAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmAxisNewCross(NULL, NULL);
}

static void
AxisAddSingleAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  CmAxisNewSingle(NULL, NULL);
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
  CmMergeOpen(NULL, NULL);
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
PopupUpdateAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  ViewerUpdateCB(NULL, NULL);
}

static GActionEntry AppEntries[] =
{
  { "help", help_activated, NULL, NULL, NULL },
  { "about", about_activated, NULL, NULL, NULL },
  { "quit", quit_activated, NULL, NULL, NULL },
  { "GraphNewFrameAction", GraphNewFrameAction_activated, NULL, NULL, NULL },
  { "GraphNewSectionAction", GraphNewSectionAction_activated, NULL, NULL, NULL },
  { "GraphNewCrossAction", GraphNewCrossAction_activated, NULL, NULL, NULL },
  { "GraphNewClearAction", GraphNewClearAction_activated, NULL, NULL, NULL },
  { "GraphRecentAction", GraphRecentAction_activated, NULL, NULL, NULL },
  { "GraphLoadAction", GraphLoadAction_activated, NULL, NULL, NULL },
  { "GraphSaveAction", GraphSaveAction_activated, NULL, NULL, NULL },
  { "GraphSaveAsAction", GraphSaveAsAction_activated, NULL, NULL, NULL },
  { "GraphExportGRAAction", GraphExportGRAAction_activated, NULL, NULL, NULL },
  { "GraphExportPSAction", GraphExportPSAction_activated, NULL, NULL, NULL },
  { "GraphExportEPSAction", GraphExportEPSAction_activated, NULL, NULL, NULL },
  { "GraphExportPDFAction", GraphExportPDFAction_activated, NULL, NULL, NULL },
  { "GraphExportSVGAction", GraphExportSVGAction_activated, NULL, NULL, NULL },
  { "GraphExportPNGAction", GraphExportPNGAction_activated, NULL, NULL, NULL },
  { "GraphDrawOrderAction", GraphDrawOrderAction_activated, NULL, NULL, NULL },
  { "GraphPageSetupAction", GraphPageSetupAction_activated, NULL, NULL, NULL },
  { "GraphPrintPreviewAction", GraphPrintPreviewAction_activated, NULL, NULL, NULL },
  { "GraphPrintAction", GraphPrintAction_activated, NULL, NULL, NULL },
  { "GraphCurrentDirectoryAction", GraphCurrentDirectoryAction_activated, NULL, NULL, NULL },
  { "GraphAddinAction", GraphAddinAction_activated, "i", NULL, NULL },
  { "GraphShellAction", GraphShellAction_activated, NULL, NULL, NULL },
  { "EditCutAction", EditCutAction_activated, NULL, NULL, NULL },
  { "EditCopyAction", EditCopyAction_activated, NULL, NULL, NULL },
  { "EditPasteAction", EditPasteAction_activated, NULL, NULL, NULL },
  { "EditDeleteAction", EditDeleteAction_activated, NULL, NULL, NULL },
  { "EditDuplicateAction", EditDuplicateAction_activated, NULL, NULL, NULL },
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
  { "EditFlipVActiopn", EditFlipVActiopn_activated, NULL, NULL, NULL },
  { "ViewDrawDirectAction", ViewDrawDirectAction_activated, NULL, NULL, NULL },
  { "ViewDrawAction", ViewDrawAction_activated, NULL, NULL, NULL },
  { "ViewClearAction", ViewClearAction_activated, NULL, NULL, NULL },
  { "ViewClearInformationWindowAction", ViewClearInformationWindowAction_activated, NULL, NULL, NULL },
  { "ViewSidebarAction", NULL, NULL, "true", ViewSidebarAction_activated },
  { "ViewStatusbarAction", NULL, NULL, "true", ViewStatusbarAction_activated },
  { "ViewRulerAction", NULL, NULL, "true", ViewRulerAction_activated },
  { "ViewScrollbarAction", NULL, NULL, "true", ViewScrollbarAction_activated },
  { "ViewCommandToolbarAction", NULL, NULL, "true", ViewCommandToolbarAction_activated },
  { "ViewToolboxAction", NULL, NULL, "true", ViewToolboxAction_activated },
  { "ViewCrossGaugeAction", NULL, NULL, "true", ViewCrossGaugeAction_activated },
  { "DataAddFileAction", DataAddFileAction_activated, NULL, NULL, NULL },
  { "DataAddRangeAction", DataAddRangeAction_activated, NULL, NULL, NULL },
  { "DataAddRecentFileAction", DataAddRecentFileAction_activated, NULL, NULL, NULL },
  { "DataPropertyAction", DataPropertyAction_activated, NULL, NULL, NULL },
  { "DataCloseAction", DataCloseAction_activated, NULL, NULL, NULL },
  { "DataEditAction", DataEditAction_activated, NULL, NULL, NULL },
  { "DataSaveAction", DataSaveAction_activated, NULL, NULL, NULL },
  { "DataMathAction", DataMathAction_activated, NULL, NULL, NULL },
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
  { "PopupUpdateAction", PopupUpdateAction_activated, NULL, NULL, NULL }
};

GtkApplication *
create_application_window(GtkWidget **popup)
{
  GtkApplication *app;
  GtkBuilder *builder;
  GObject *menu;
  char *filename;

  app = gtk_application_new(NULL, G_APPLICATION_FLAGS_NONE);
  g_application_register(G_APPLICATION(app), NULL, NULL);

  g_action_map_add_action_entries(G_ACTION_MAP(app), AppEntries, G_N_ELEMENTS(AppEntries), app);

  filename = g_strdup_printf("%s/gtk/%s", CONFDIR, UI_FILE);
  builder = gtk_builder_new_from_file(filename);
  g_free(filename);

  menu = gtk_builder_get_object(builder, "app-menu");
  gtk_application_set_app_menu(app, G_MENU_MODEL(menu));

#if USE_GTK_BUILDER
  menu = gtk_builder_get_object(builder, "menubar");
  gtk_application_set_menubar(app, G_MENU_MODEL(menu));

  menu = gtk_builder_get_object(builder, "popup-menu");
  *popup = gtk_menu_new_from_model(G_MENU_MODEL(menu));
#endif

  g_object_unref(builder);

  return app;
}
#endif	/* USE_APP_MENU*/
