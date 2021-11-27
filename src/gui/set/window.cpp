//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <gui/set/window.hpp>
#include <gui/set/panel.hpp>
#include <gui/set/cards_panel.hpp>
#include <gui/set/set_info_panel.hpp>
#include <gui/set/style_panel.hpp>
#include <gui/set/keywords_panel.hpp>
#include <gui/set/stats_panel.hpp>
#include <gui/set/random_pack_panel.hpp>
#include <gui/set/console_panel.hpp>
#include <gui/control/card_list.hpp>
#include <gui/control/card_viewer.hpp>
#include <gui/control/gallery_list.hpp>
#include <gui/about_window.hpp>
#include <gui/update_checker.hpp>
#include <gui/packages_window.hpp>
#include <gui/new_window.hpp>
#include <gui/preferences_window.hpp>
#include <gui/print_window.hpp>
#include <gui/images_export_window.hpp>
#include <gui/html_export_window.hpp>
#include <gui/auto_replace_window.hpp>
#include <gui/util.hpp>
#include <util/io/package_manager.hpp>
#include <util/window_id.hpp>
#include <data/game.hpp>
#include <data/set.hpp>
#include <data/card.hpp>
#include <data/settings.hpp>
#include <data/format/formats.hpp>
#include <data/action/value.hpp>
#include <data/action/set.hpp>

// ----------------------------------------------------------------------------- : Constructor

SetWindow::SetWindow(Window* parent, const SetP& set)
  : wxFrame(parent, wxID_ANY, _TITLE_("magic set editor"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxNO_FULL_REPAINT_ON_RESIZE)
  , current_panel(nullptr)
  , find_data(wxFR_DOWN)
  , number_of_recent_sets(0)
{
  SetIcon(load_resource_icon(_("app")));

  // avoid flicker
  SetBackgroundStyle(wxBG_STYLE_PAINT);

  // initialize menu bar
  auto menuBar = new wxMenuBar();
  auto menuFile = new wxMenu();
    add_menu_item_tr(menuFile, ID_FILE_NEW, "new", "new_set");
    add_menu_item_tr(menuFile, ID_FILE_OPEN, "open", "open_set");
    add_menu_item_tr(menuFile, ID_FILE_SAVE, "save", "save_set");
    add_menu_item_tr(menuFile, ID_FILE_SAVE_AS, nullptr, "save_set_as");
    add_menu_item_tr(menuFile, ID_FILE_SAVE_AS_DIRECTORY, nullptr, "save_set_as_directory");
    add_menu_item_tr(menuFile, wxID_ANY, "export", "export", wxITEM_NORMAL, makeExportMenu());
    menuFile->AppendSeparator();
    #if USE_UPDATE_CHECKER
    add_menu_item_tr(menuFile, ID_FILE_CHECK_UPDATES, nullptr, "check_updates");
    #endif
    #if USE_SCRIPT_PROFILING
    add_menu_item_tr(menuFile, ID_FILE_PROFILER, nullptr, "show_profiler");
    #endif
//    menuFile->Append(ID_FILE_INSPECT,          _("Inspect Internal Data..."),  _("Shows a the data in the set using a tree structure"));
//    menuFile->AppendSeparator();
    add_menu_item_tr(menuFile, ID_FILE_RELOAD, nullptr, "reload_data");
    menuFile->AppendSeparator();
    add_menu_item_tr(menuFile, ID_FILE_PRINT_PREVIEW, "print_preview", "print_preview");
    add_menu_item_tr(menuFile, ID_FILE_PRINT, "print", "print");
    menuFile->AppendSeparator();
    // recent files go here
    menuFile->AppendSeparator();
    add_menu_item_tr(menuFile, ID_FILE_EXIT, nullptr, "exit");
  menuBar->Append(menuFile, _MENU_("file"));
  
  auto menuEdit = new wxMenu();
    add_menu_item(menuEdit, ID_EDIT_UNDO, "undo", _MENU_1_("undo",wxEmptyString),  _HELP_("undo"));
    add_menu_item(menuEdit, ID_EDIT_REDO, "redo", _MENU_1_("redo",wxEmptyString),  _HELP_("redo"));
    menuEdit->AppendSeparator();
    add_menu_item_tr(menuEdit, ID_EDIT_CUT, "cut", "cut");
    add_menu_item_tr(menuEdit, ID_EDIT_COPY, "copy", "copy");
    add_menu_item_tr(menuEdit, ID_EDIT_PASTE, "paste", "paste");
    menuEdit->AppendSeparator();
    add_menu_item_tr(menuEdit, ID_EDIT_SELECT_ALL, nullptr, "select_all");
    menuEdit->AppendSeparator();
    add_menu_item_tr(menuEdit, ID_EDIT_FIND, "find", "find");
    add_menu_item_tr(menuEdit, ID_EDIT_FIND_NEXT, nullptr, "find_next");
    add_menu_item_tr(menuEdit, ID_EDIT_REPLACE, nullptr, "replace");
    add_menu_item_tr(menuEdit, ID_EDIT_AUTO_REPLACE, nullptr, "auto_replace");
    menuEdit->AppendSeparator();
    add_menu_item_tr(menuEdit, ID_EDIT_PREFERENCES, nullptr, "preferences");
  menuBar->Append(menuEdit, _MENU_("edit"));
  
  auto menuWindow = new wxMenu();
    add_menu_item_tr(menuWindow, ID_WINDOW_NEW, nullptr, "new_window");
    menuWindow->AppendSeparator();
  menuBar->Append(menuWindow, _MENU_("window"));
  
  auto menuHelp = new wxMenu();
    add_menu_item_tr(menuHelp, ID_HELP_INDEX, "help", "index");
    add_menu_item_tr(menuHelp, ID_HELP_WEBSITE, nullptr, "website");
    add_menu_item_tr(menuHelp, ID_HELP_ABOUT, nullptr, "about");
  menuBar->Append(menuHelp, _MENU_("help"));
  
  SetMenuBar(menuBar);
  
  // status bar
  CreateStatusBar();
  SetStatusText(_HELP_("welcome"));
  
  // tool bar
  wxToolBar* tb = CreateToolBar(wxTB_FLAT | wxNO_BORDER | wxTB_HORIZONTAL);
  tb->SetToolBitmapSize(wxSize(20,20));
  add_tool_tr(tb, ID_FILE_NEW, "new", "new_set");
  add_tool_tr(tb, ID_FILE_OPEN, "open", "open_set");
  add_tool_tr(tb, ID_FILE_SAVE, "save", "save_set");
  tb->AddSeparator();
  add_tool_tr(tb, ID_FILE_EXPORT, "export", "export");
  tb->AddSeparator();
  add_tool_tr(tb, ID_EDIT_CUT, "cut", "cut");
  add_tool_tr(tb, ID_EDIT_COPY, "copy", "copy");
  add_tool_tr(tb, ID_EDIT_PASTE, "paste", "paste");
  tb->AddSeparator();
  add_tool(tb, ID_EDIT_UNDO, "undo", {}, _TOOLTIP_1_("undo", {}), _HELP_("undo"));
  add_tool(tb, ID_EDIT_REDO, "redo", {}, _TOOLTIP_1_("redo", {}), _HELP_("redo"));
  tb->AddSeparator();
  tb->Realize();
  
  // tab bar, sizer
  wxToolBar* tabBar = new wxToolBar(this, ID_TAB_BAR, wxDefaultPosition, wxDefaultSize, wxTB_FLAT | wxNO_BORDER | wxTB_HORIZONTAL | wxTB_HORZ_TEXT | wxTB_NOALIGN);
  tabBar->SetToolBitmapSize(wxSize(18,18));
  wxSizer* s = new wxBoxSizer(wxVERTICAL);
  s->Add(tabBar, 0, wxEXPAND | wxBOTTOM, 0);
  SetSizer(s);
  #if defined(__WXMSW__) && defined(TBSTYLE_EX_DOUBLEBUFFER)
    // Use double buffering
    HWND hWND = (HWND)tabBar->GetHWND();
    int style = ::SendMessage(hWND, TB_GETEXTENDEDSTYLE, 0, 0);
    ::SendMessage(hWND, TB_SETEXTENDEDSTYLE, style | LVS_EX_DOUBLEBUFFER, 0);
  #endif
  
  // panels
  addPanel(menuWindow, tabBar, new CardsPanel     (this, wxID_ANY), 0, _("window_cards"),      _("cards tab"));
  addPanel(menuWindow, tabBar, new StylePanel     (this, wxID_ANY), 1, _("window_style"),      _("style tab"));
  addPanel(menuWindow, tabBar, new SetInfoPanel   (this, wxID_ANY), 2, _("window_set_info"),   _("set info tab"));
  if (set->game->has_keywords) {
    addPanel(menuWindow, tabBar, new KeywordsPanel  (this, wxID_ANY), 3, _("window_keywords"),   _("keywords tab"));
  }
  addPanel(menuWindow, tabBar, new StatsPanel     (this, wxID_ANY), 4, _("window_statistics"), _("stats tab"));
  addPanel(menuWindow, tabBar, new RandomPackPanel(this, wxID_ANY), 5, _("window_random_pack"),_("random pack tab"));
  addPanel(menuWindow, tabBar, new ConsolePanel   (this, wxID_ANY), 6, _("window_console"),    _("console tab"));
  selectPanel(ID_WINDOW_CARDS); // select cards panel
  
  // loose ends
  tabBar->Realize();
  SetSize(settings.set_window_width, settings.set_window_height);
  if (settings.set_window_maximized) {
    Maximize();
  }
  set_windows.push_back(this); // register this window
  // don't send update ui events to children
  // note: this still sends events for menu and toolbar items!
  wxUpdateUIEvent::SetMode(wxUPDATE_UI_PROCESS_SPECIFIED);
  SetExtraStyle(wxWS_EX_PROCESS_UI_UPDATES);
  tabBar->SetExtraStyle(wxWS_EX_PROCESS_UI_UPDATES);
  
  try {
    setSet(set);
  } catch (...) {
    // clean up!
    // if we don't destroy the panel we could crash in ~CardsPanel, since it expected
    // the insertSymbolMenu to be removed by destroyUI but not deleted.
    current_panel->destroyUI(GetToolBar(), GetMenuBar());
    set_windows.erase(remove(set_windows.begin(), set_windows.end(), this));
    throw;
  }
  current_panel->Layout();
}

wxMenu* SetWindow::makeExportMenu() {
  auto menuExport = new wxMenu();
  add_menu_item_tr(menuExport, ID_FILE_EXPORT_HTML, "export_html", "export_html");
  add_menu_item_tr(menuExport, ID_FILE_EXPORT_IMAGE, "export_image", "export_image");
  add_menu_item_tr(menuExport, ID_FILE_EXPORT_IMAGES, "export_images", "export_images");
  #if false // these only make sense for mtg
  add_menu_item_tr(menuExport, ID_FILE_EXPORT_APPR, "export_apprentice", "export_apprentice");
  add_menu_item_tr(menuExport, ID_FILE_EXPORT_MWS, "export_mws", "export_mws");
  #endif
  return menuExport;
}

SetWindow::~SetWindow() {
  // store window size in settings
  wxSize s = GetSize();
  settings.set_window_maximized = IsMaximized();
  if (!IsMaximized()) {
    settings.set_window_width  = s.GetWidth();
    settings.set_window_height = s.GetHeight();
  }
  // destroy ui of selected panel
  current_panel->destroyUI(GetToolBar(), GetMenuBar());
  // cleanup (see find stuff)
  delete export_menu;
  // remove from list of set windows
  set_windows.erase(remove(set_windows.begin(), set_windows.end(), this));
}

// ----------------------------------------------------------------------------- : Panel managment

void SetWindow::addPanel(wxMenu* windowMenu, wxToolBar* tabBar, SetWindowPanel* panel, UInt pos, const String& image_name, const String& name) {
  // insert in list
  panels.push_back(panel);
  // names
  String menu_name   = tr(LOCALE_CAT_MENU,    name);
  String description = tr(LOCALE_CAT_HELP,    name);
  String tab_name    = tr(LOCALE_CAT_TOOL,    name);
  String tab_help    = tr(LOCALE_CAT_TOOLTIP, name);
  // add to tab bar
  int id = ID_WINDOW_MIN + pos;
  panel->tool_window_id = id;
  tabBar->AddTool(id, tab_name + _("   "), load_resource_tool_image(image_name), wxNullBitmap, wxITEM_CHECK, tab_help, description);
  tabBar->AddSeparator();
  // add to menu bar
  auto menu_item = windowMenu->Append(id, menu_name, description, wxITEM_CHECK);
  set_menu_item_image(menu_item, image_name);
  // add to sizer
  GetSizer()->Add(panel, 1, wxEXPAND);
}

void SetWindow::selectPanel(int id) {
  // find which panel to select
  SetWindowPanel* toSelect = nullptr;
  for (auto p : panels) {
    if (p->tool_window_id == id) toSelect = p;
  }
  if (!toSelect) return;
  if (current_panel == toSelect) {
    // don't change, but fix tab bar
    wxToolBar* tabBar  = (wxToolBar*)FindWindow(ID_TAB_BAR);
    FOR_EACH(p, panels) {
      tabBar->ToggleTool(p->tool_window_id, p == current_panel);
    }
    return;
  }
  // destroy & create menus
  if (current_panel) current_panel->destroyUI(GetToolBar(), GetMenuBar());
  current_panel = toSelect;
  current_panel->initUI(GetToolBar(), GetMenuBar());
  // show/hide panels and select tabs
  wxSizer*   sizer   = GetSizer();
  wxToolBar* tabBar  = (wxToolBar*)FindWindow(ID_TAB_BAR);
  wxMenuBar* menuBar = GetMenuBar();
  FOR_EACH(p, panels) {
    sizer->Show       (p, p == current_panel);
    tabBar->ToggleTool(p->tool_window_id, p == current_panel);
    menuBar->Check    (p->tool_window_id, p == current_panel);
  }
  // fix sizer stuff
  fixMinWindowSize();
  // select something
  current_panel->SetFocus();
}

void toolbar_SetToolNormalBitmap(wxToolBar* toolbar, int id, wxBitmap const& bitmap) {
  #if wxVERSION_NUMBER < 2800
    // copied from wx2.8.11, tbar95.cpp
    wxToolBarToolBase* tool = wx_static_cast(wxToolBarToolBase*, toolbar->FindById(id));
    if (tool) {
      tool->SetNormalBitmap(bitmap);
      toolbar->Realize();
    }
  #else
    toolbar->SetToolNormalBitmap(id, bitmap);
  #endif
}

void SetWindow::setPanelIcon(SetWindowPanel* panel, wxBitmap const& icon) {
  wxToolBar* tabBar = (wxToolBar*)FindWindow(ID_TAB_BAR);
  toolbar_SetToolNormalBitmap(tabBar, panel->tool_window_id, icon);
  #if wxVERSION_NUMBER < 2800
    // This is needed at least in wx2.6, other versions seem not to need it, but maybe they do
    Layout();
  #endif
  // TODO: this could be done better, wx requires a full new Realize of the toolbar, but on win32 a single message would do
  // if only we could set up the imagelist correctly.
}

// ----------------------------------------------------------------------------- : Window managment

vector<SetWindow*> SetWindow::set_windows;

bool SetWindow::isOnlyWithSet() {
  FOR_EACH(w, set_windows) {
    if (w != this && w->set == set) return false;
  }
  return true;
}

// ----------------------------------------------------------------------------- : Set actions

void SetWindow::onChangeSet() {
  // window title
  updateTitle();
  // make sure there is always at least one card
  // some things need this
  if (set->cards.empty()) set->cards.push_back(make_intrusive<Card>(*set->game));
  // all panels view the same set
  FOR_EACH(p, panels) {
    p->setSet(set);
  }
  // only after setSet select a card
  FOR_EACH(p, panels) {
    p->selectFirstCard();
  }
  fixMinWindowSize();
}

void SetWindow::onAction(const Action& action, bool undone) {
  TYPE_CASE(action, ValueAction) {
    if (!action.card) {
      if (set->data.contains(action.valueP) && action.valueP->fieldP->identifying) {
        updateTitle();
      }
    }
  }
/*  TYPE_CASE_(action, DisplayChangeAction) {
    // The style changed, maybe also the size of card viewers
    if (current_panel) current_panel->Layout();
    fixMinWindowSize();
  }
*/
}

void SetWindow::updateTitle() {
  if (!set) {
    SetTitle(_TITLE_("magic set editor"));
  } else {
    String identification = set->identification();
    if (identification.empty()) identification = set->name();
    if (identification.empty()) identification = _TITLE_("untitled");
    set->short_name = identification;
    SetTitle(_TITLE_1_("%s - magic set editor",identification));
  }
}

void SetWindow::fixMinWindowSize() {
  current_panel->SetMinSize(current_panel->GetSizer()->GetMinSize());
  Layout();
  current_panel->Layout();
  wxSize s  = GetSizer()->GetMinSize();
  wxSize ws = GetSize();
  wxSize cs = GetClientSize();
  wxSize minSize = wxSize(s.x + ws.x - cs.x, s.y + ws.y - cs.y);
  if (ws.x < minSize.x) ws.x = minSize.x;
  if (ws.y < minSize.y) ws.y = minSize.y;
  SetSize(ws);
  SetMinSize(minSize);
}

void SetWindow::onSizeChange(wxCommandEvent&) {
  FOR_EACH(p, panels) {
    p->Layout();
  }
  fixMinWindowSize();
}


// ----------------------------------------------------------------------------- : Cards

void SetWindow::onCardSelect(CardSelectEvent& ev) {
  FOR_EACH(p, panels) {
    p->selectCard(ev.getCard());
  }
}
void SetWindow::onCardActivate(CardSelectEvent& ev) {
  selectPanel(ID_WINDOW_CARDS);
}

void SetWindow::selectionChoices(ExportCardSelectionChoices& out) {
  out.push_back(make_intrusive<ExportCardSelectionChoice>(*set)); // entire set
  FOR_EACH(p, panels) {
    p->selectionChoices(out);
  }
  out.push_back(make_intrusive<ExportCardSelectionChoice>()); // custom
}

vector<CardListBase*> SetWindow::getCardLists() {
  vector<CardListBase*> out;
  FOR_EACH(p, panels) {
    p->getCardLists(out);
  }
  return out;
}

// ----------------------------------------------------------------------------- : Window events - close

void SetWindow::onClose(wxCloseEvent& ev) {
  // only ask if we want to save is this is the only window that has the current set opened
  if (!isOnlyWithSet() || askSaveAndContinue()) {
    Destroy();
  } else {
    ev.Veto();
  }
}


int ask_save_changes_impl(wxWindow* parent, String const& message, String const& title) {
  #if defined(__WXMSW__) && defined(UNICODE) && defined(TD_WARNING_ICON) // the last one is a hack to test for precense of TASKDIALOG stuff
    // Do we have the TaskDialogIndirect function?
    HMODULE h = ::LoadLibrary(L"comctl32.dll");
    if (!h) return 0;
    typedef HRESULT (WINAPI *type_TaskDialogIndirect)(const TASKDIALOGCONFIG *pTaskConfig, int *pnButton, int *pnRadioButton, BOOL *pfVerificationFlagChecked);
    type_TaskDialogIndirect func_TaskDialogIndirect = !h ? nullptr : (type_TaskDialogIndirect)::GetProcAddress(h, "TaskDialogIndirect" );
    if (!func_TaskDialogIndirect) return 0;
    
    int nButtonPressed                  = 0;
    TASKDIALOGCONFIG config             = {0};
    const TASKDIALOG_BUTTON buttons[]   = { { IDYES, L"&Save" }, { IDNO, L"Do&n't Save" } };
    config.cbSize                       = sizeof(config);
    config.dwCommonButtons              = TDCBF_CANCEL_BUTTON;
    config.pszWindowTitle               = title.c_str();
    config.pszMainInstruction           = message.c_str();
    config.pButtons                     = buttons;
    config.cButtons                     = ARRAYSIZE(buttons);
    config.hwndParent                   = (HWND)(parent->GetHWND()); // without this the dialog is not modal
    
    func_TaskDialogIndirect(&config, &nButtonPressed, NULL, NULL);
    
    FreeLibrary(h);
    
    switch (nButtonPressed) {
      case IDYES: return wxYES;
      case IDNO:  return wxNO;
      default:    return wxCANCEL;
    }
  #else
    return 0;
  #endif
}
// Fancy save dialog
int ask_save_changes(wxWindow* parent, String const& message, String const& title) {
  int result = ask_save_changes_impl(parent, message,title);
  if (result) return result;
  // Use a normal message box
  return wxMessageBox(message, title, wxYES_NO | wxCANCEL | wxICON_EXCLAMATION, parent);
}


bool SetWindow::askSaveAndContinue() {
  if (set->actions.atSavePoint()) return true;
  int save = ask_save_changes(this, _LABEL_1_("save changes", set->short_name), _TITLE_("save changes"));
  if (save == wxYES) {
    // save the set
    try {
      if (set->needSaveAs()) {
        // need save as
        wxFileDialog dlg(this, _TITLE_("save_set"), settings.default_set_dir, clean_filename(set->short_name), export_formats(*set->game), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (dlg.ShowModal() == wxID_OK) {
          settings.default_set_dir = dlg.GetDirectory();
          export_set(*set, dlg.GetPath(), dlg.GetFilterIndex());
          return true;
        } else {
          return false;
        }
      } else {
        set->save();
        set->actions.setSavePoint();
        return true;
      }
    } catch (Error const& e) {
      // something went wrong with saving, don't proceed
      handle_error(e);
      return false;
    }
  } else if (save == wxNO) {
    return true;
  } else { // wxCANCEL
    return false;
  }
}

void SetWindow::switchSet(const SetP& new_set) {
  if (new_set) {
    if (settings.open_sets_in_new_window) {
      (new SetWindow(nullptr, new_set))->Show();
    } else {
      setSet(new_set);
    }
  }
}

// ----------------------------------------------------------------------------- : Window events - update UI

void SetWindow::onUpdateUI(wxUpdateUIEvent& ev) {
  switch (ev.GetId()) {
    // file menu
    case ID_FILE_SAVE:         ev.Enable(!set->actions.atSavePoint() || set->needSaveAs());  break;
    case ID_FILE_EXPORT_IMAGE: ev.Enable(!!current_panel->selectedCard());          break;
    case ID_FILE_EXPORT_APPR:  ev.Enable(set->game->isMagic());                break;
    case ID_FILE_EXPORT_MWS:   ev.Enable(set->game->isMagic());                break;
    case ID_FILE_EXIT:
      // update for ID_FILE_RECENT done for a different id, because ID_FILE_RECENT may not be in the menu yet
      updateRecentSets();
      break;
    // undo/redo
    case ID_EDIT_UNDO: {
      ev.Enable(set->actions.canUndo());
      String label = set->actions.undoName();
      ev.SetText(_MENU_1_("undo", label));
      GetToolBar()->SetToolShortHelp(ID_EDIT_UNDO, _TOOLTIP_1_("undo", label));
      break;
    } case ID_EDIT_REDO: {
      ev.Enable(set->actions.canRedo());
      String label = set->actions.redoName();
      ev.SetText(_MENU_1_("redo", label));
      GetToolBar()->SetToolShortHelp(ID_EDIT_REDO, _TOOLTIP_1_("redo", label));
      break;
    }
    // copy & paste & find
    case ID_EDIT_CUT       : ev.Enable(current_panel->canCut());  break;
    case ID_EDIT_COPY      : ev.Enable(current_panel->canCopy());  break;
    case ID_EDIT_PASTE     : ev.Enable(current_panel->canPaste());  break;
    case ID_EDIT_SELECT_ALL: ev.Enable(current_panel->canSelectAll()); break;
    case ID_EDIT_FIND      : ev.Enable(current_panel->canFind());  break;
    case ID_EDIT_FIND_NEXT : ev.Enable(current_panel->canFind());  break;
    case ID_EDIT_REPLACE   : ev.Enable(current_panel->canReplace());break;
    // windows
    case ID_WINDOW_KEYWORDS: ev.Enable(set->game->has_keywords);  break;
    case ID_WINDOW_RANDOM_PACK: ev.Enable(!set->game->pack_types.empty());  break;
    // help
    case ID_HELP_INDEX     : ev.Enable(false);            break; // not implemented
    // other
    default:
      // items created by the panel, and cut/copy/paste and find/replace
      if(current_panel) current_panel->onUpdateUI(ev);
  }
}

static const int FILE_MENU_SIZE_BEFORE_RECENT_SETS = 12; // HACK; we should calculate the position to insert!
void SetWindow::updateRecentSets() {
  wxMenuBar* mb = GetMenuBar();
  assert(number_of_recent_sets <= (UInt)settings.recent_sets.size()); // the number of recent sets should only increase
  UInt i = 0;
  FOR_EACH(file, settings.recent_sets) {
    if (i >= settings.max_recent_sets) break;
    if (i < number_of_recent_sets) {
      // menu item already exists, update it
      mb->SetLabel(ID_FILE_RECENT + i, String(_("&")) << (i+1) << _(" ") << file);
    } else {
      // add new item
      wxMenu* file_menu = mb->GetMenu(0);
      size_t pos = file_menu->GetMenuItemCount() - 2; // last two items are separator and exit
      file_menu->Insert(pos, ID_FILE_RECENT + i, String(_("&")) << (i+1) << _(" ") << file, wxEmptyString);
    }
    i++;
  }
  number_of_recent_sets = (UInt)settings.recent_sets.size();
}

// ----------------------------------------------------------------------------- : Window events - menu - file


void SetWindow::onFileNew(wxCommandEvent&) {
  if (!settings.open_sets_in_new_window && isOnlyWithSet() && !askSaveAndContinue()) return;
  // new set?
  SetP new_set = new_set_window(this);
  switchSet(new_set);
}

void SetWindow::onFileOpen(wxCommandEvent&) {
  if (!settings.open_sets_in_new_window && isOnlyWithSet() && !askSaveAndContinue()) return;
  wxFileDialog dlg(this, _TITLE_("open_set"), settings.default_set_dir, _(""), import_formats(), wxFD_OPEN);
  if (dlg.ShowModal() == wxID_OK) {
    settings.default_set_dir = dlg.GetDirectory();
    wxBusyCursor busy;
    SetP new_set = import_set(dlg.GetPath());
    switchSet(new_set);
  }
}

void SetWindow::onFileSave(wxCommandEvent& ev) {
  if (set->needSaveAs()) {
    onFileSaveAs(ev);
  } else {
    wxBusyCursor busy;
    settings.addRecentFile(set->absoluteFilename());
    set->save();
    set->actions.setSavePoint();
  }
}

void SetWindow::onFileSaveAs(wxCommandEvent&) {
  wxFileDialog dlg(this, _TITLE_("save_set"), settings.default_set_dir, clean_filename(set->short_name), export_formats(*set->game), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (dlg.ShowModal() == wxID_OK) {
    settings.default_set_dir = dlg.GetDirectory();
    export_set(*set, dlg.GetPath(), dlg.GetFilterIndex());
    updateTitle(); // title may depend on filename
  }
}

void SetWindow::onFileSaveAsDirectory(wxCommandEvent&) {
  wxFileDialog dlg(this, _TITLE_("save_set_as_directory"), settings.default_set_dir, clean_filename(set->short_name), "Magic Set Editor sets (*.mse-set)|*.mse-set", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (dlg.ShowModal() == wxID_OK) {
    String filename = dlg.GetPath();
    settings.default_set_dir = dlg.GetDirectory();
    set->saveAs(filename, true, true);
    settings.addRecentFile(filename);
    set->actions.setSavePoint();
    updateTitle(); // title may depend on filename
  }
}

/*
void SetWindow::onFileInspect(wxCommandEvent&) {
  var wnd = new TreeGridWindow(&this, set);
  wnd.show();
}*/

#if defined(__WXMSW__)
#include "wx/msw/wrapcctl.h"
#endif

void SetWindow::onFileExportMenu(wxCommandEvent& ev) {
  // find position of export tool bar button
  wxToolBar* tb = GetToolBar();
  int pos = tb->GetToolPos(ev.GetId());
  wxRect tool_rect;
  #if defined(__WXMSW__)
    RECT r = {0,0,0,0};
    ::SendMessage((HWND)tb->GetHWND(), TB_GETITEMRECT, pos, (LPARAM)&r);
    tool_rect.x      = r.left;
    tool_rect.y      = r.top;
    tool_rect.width  = r.right - r.left;
    tool_rect.height = r.bottom - r.top;
  #else
    // TODO: This is probably not correct!
    wxSize tool_size = tb->GetToolSize();
    wxSize tool_margin = tb->GetToolMargins();
    tool_rect.SetSize(tool_size);
    tool_rect.y = 0;
    tool_rect.x = tool_size.x * pos;
  #endif
  // tool down
  tb->ToggleTool(ev.GetId(), true);
  // pop up a menu of export options
  if (!export_menu) export_menu = makeExportMenu();
  tb->PopupMenu(export_menu, tool_rect.x, tool_rect.GetBottom());
  // tool up
  tb->ToggleTool(ev.GetId(), false);
}

void SetWindow::onFileExportImage(wxCommandEvent&) {
  CardP card = current_panel->selectedCard();
  if (!card)  return; // no card selected
  String name = wxFileSelector(_TITLE_("save image"), settings.default_export_dir, clean_filename(card->identification()), _(""),
                             _("PNG images (*.png)|*.png|JPEG images (*.jpg)|*.jpg|Windows bitmaps (*.bmp)|*.bmp|TIFF images (*.tif)|*.tif"),
                             wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
  if (!name.empty()) {
    settings.default_export_dir = wxPathOnly(name);
    export_image(set, card, name);
  }
}

void SetWindow::onFileExportImages(wxCommandEvent&) {
  ExportCardSelectionChoices choices;
  selectionChoices(choices);
  ImagesExportWindow wnd(this, set, choices);
  wnd.ShowModal();
}

void SetWindow::onFileExportHTML(wxCommandEvent&) {
  ExportCardSelectionChoices choices;
  selectionChoices(choices);
  HtmlExportWindow wnd(this, set, choices);
  wnd.ShowModal();
}

void SetWindow::onFileExportApprentice(wxCommandEvent&) {
  export_apprentice(this, set);
}

void SetWindow::onFileExportMWS(wxCommandEvent&) {
  export_mws(this, set);
}

#if USE_UPDATE_CHECKER
void SetWindow::onFileCheckUpdates(wxCommandEvent&) {
  if (!askSaveAndContinue()) return;
  (new PackagesWindow(this))->Show();
  //Destroy();
}
#endif

#if USE_SCRIPT_PROFILING
  void show_profiler_window(wxWindow* parent);
  void SetWindow::onFileProfiler(wxCommandEvent&) {
    show_profiler_window(this);
  }
#endif

void SetWindow::onFilePrint(wxCommandEvent&) {
  ExportCardSelectionChoices choices;
  selectionChoices(choices);
  print_set(this, set, choices);
}

void SetWindow::onFilePrintPreview(wxCommandEvent&) {
  ExportCardSelectionChoices choices;
  selectionChoices(choices);
  print_preview(this, set, choices);
}

void SetWindow::onFileReload(wxCommandEvent&) {
  if (!askSaveAndContinue()) return;
  String filename = set->absoluteFilename();
  if (filename.empty()) return;
  wxBusyCursor busy;
  settings.write();   // save settings
  // current card
  size_t card_pos = 0;
  {
    vector<CardP>::const_iterator card_it = find(set->cards.begin(), set->cards.end(), current_panel->selectedCard());
    if (card_it != set->cards.end()) card_pos = card_it - set->cards.begin();
  }
  package_manager.reset(); // unload all packages
  settings.read();         // reload settings
  setSet(import_set(filename));
  // reselect card
  if (card_pos < set->cards.size()) {
    FOR_EACH(p, panels) {
      p->selectCard(set->cards[card_pos]);
    }
  }
}

void SetWindow::onFileRecent(wxCommandEvent& ev) {
  wxBusyCursor busy;
  switchSet(import_set(settings.recent_sets.at(ev.GetId() - ID_FILE_RECENT)));
}

void SetWindow::onFileExit(wxCommandEvent&) {
  Close();
}

// ----------------------------------------------------------------------------- : Window events - menu - edit

void SetWindow::onEditUndo(wxCommandEvent&) {
  set->actions.undo();
}
void SetWindow::onEditRedo(wxCommandEvent&) {
  set->actions.redo();
}

void SetWindow::onEditCut  (wxCommandEvent&) {
  current_panel->doCut();
}
void SetWindow::onEditCopy (wxCommandEvent&) {
  current_panel->doCopy();
}
void SetWindow::onEditPaste(wxCommandEvent&) {
  current_panel->doPaste();
}

void SetWindow::onEditSelectAll(wxCommandEvent&) {
  current_panel->doSelectAll();
}

void SetWindow::onEditFind(wxCommandEvent&) {
  find_dialog = make_unique<wxFindReplaceDialog>(this, &find_data, _("Find"));
  find_dialog->Show();
  find_dialog->SetPosition(this->GetPosition());
}
void SetWindow::onEditFindNext(wxCommandEvent&) {
  current_panel->doFind(find_data);
}
void SetWindow::onEditReplace(wxCommandEvent&) {
  find_dialog = make_unique<wxFindReplaceDialog>(this, &find_data, _("Replace"), wxFR_REPLACEDIALOG);
  find_dialog->Show();
}

// find events
void SetWindow::onFind      (wxFindDialogEvent&) {
  current_panel->doFind(find_data);
}
void SetWindow::onFindNext  (wxFindDialogEvent&) {
  current_panel->doFind(find_data);
}
void SetWindow::onReplace   (wxFindDialogEvent&) {
  current_panel->doReplace(find_data);
}
void SetWindow::onReplaceAll(wxFindDialogEvent&) {
  current_panel->doReplaceAll(find_data);
}

void SetWindow::onEditAutoReplace(wxCommandEvent&) {
  (new AutoReplaceWindow(this, *set->game))->Show();
}

void SetWindow::onEditPreferences(wxCommandEvent&) {
  PreferencesWindow wnd(this);
  if (wnd.ShowModal() == wxID_OK) {
    // render settings may have changed, notify all windows
    set->actions.tellListeners(DisplayChangeAction(),true);
  }
}


// ----------------------------------------------------------------------------- : Window events - menu - window

void SetWindow::onWindowNewWindow(wxCommandEvent&) {
  SetWindow* newWindow = new SetWindow(nullptr, set);
  newWindow->Show();
}

void SetWindow::onWindowSelect(wxCommandEvent& ev) {
  selectPanel(ev.GetId());
}


// ----------------------------------------------------------------------------- : Window events - menu - help

void SetWindow::onHelpIndex(wxCommandEvent&) {
  // TODO : context sensitive
//  HelpWindow* wnd = new HelpWindow(this);
//  wnd->Show();
}

void SetWindow::onHelpWebsite(wxCommandEvent&) {
  wxLaunchDefaultBrowser(_("https://github.com/twanvl/HeroesOfThargosSetEditor"));
}

void SetWindow::onHelpAbout(wxCommandEvent&) {
  AboutWindow wnd(this);
  wnd.ShowModal();
}

// ----------------------------------------------------------------------------- : Window events - other

void SetWindow::onChildMenu(wxCommandEvent& ev) {
  current_panel->onCommand(ev.GetId());
}

void SetWindow::onMenuOpen(wxMenuEvent& ev) {
  current_panel->onMenuOpen(ev);
  wxFrame::OnMenuOpen(ev);
}

void SetWindow::onIdle(wxIdleEvent& ev) {
  // Stuff that must be done in the main thread
#if USE_UPDATE_CHECKER
  show_update_dialog(this);
#endif
}

// ----------------------------------------------------------------------------- : Event table

BEGIN_EVENT_TABLE(SetWindow, wxFrame)
  EVT_MENU      (ID_FILE_NEW,      SetWindow::onFileNew)
  EVT_MENU      (ID_FILE_OPEN,      SetWindow::onFileOpen)
  EVT_MENU      (ID_FILE_SAVE,      SetWindow::onFileSave)
  EVT_MENU      (ID_FILE_SAVE_AS,    SetWindow::onFileSaveAs)
  EVT_MENU      (ID_FILE_SAVE_AS_DIRECTORY, SetWindow::onFileSaveAsDirectory)
  EVT_MENU      (ID_FILE_EXPORT,    SetWindow::onFileExportMenu)
  EVT_MENU      (ID_FILE_EXPORT_IMAGE,  SetWindow::onFileExportImage)
  EVT_MENU      (ID_FILE_EXPORT_IMAGES,  SetWindow::onFileExportImages)
  EVT_MENU      (ID_FILE_EXPORT_HTML,  SetWindow::onFileExportHTML)
  EVT_MENU      (ID_FILE_EXPORT_APPR,  SetWindow::onFileExportApprentice)
  EVT_MENU      (ID_FILE_EXPORT_MWS,  SetWindow::onFileExportMWS)
#if USE_UPDATE_CHECKER
  EVT_MENU      (ID_FILE_CHECK_UPDATES,  SetWindow::onFileCheckUpdates)
#endif
#if USE_SCRIPT_PROFILING
  EVT_MENU      (ID_FILE_PROFILER,    SetWindow::onFileProfiler)
#endif
//  EVT_MENU      (ID_FILE_INSPECT,    SetWindow::onFileInspect)
  EVT_MENU      (ID_FILE_PRINT,      SetWindow::onFilePrint)
  EVT_MENU      (ID_FILE_PRINT_PREVIEW,  SetWindow::onFilePrintPreview)
  EVT_MENU      (ID_FILE_RELOAD,    SetWindow::onFileReload)
  EVT_MENU_RANGE    (ID_FILE_RECENT, ID_FILE_RECENT_MAX, SetWindow::onFileRecent)
  EVT_MENU      (ID_FILE_EXIT,      SetWindow::onFileExit)
  EVT_MENU      (ID_EDIT_UNDO,      SetWindow::onEditUndo)
  EVT_MENU      (ID_EDIT_REDO,      SetWindow::onEditRedo)
  EVT_MENU      (ID_EDIT_CUT,      SetWindow::onEditCut)
  EVT_MENU      (ID_EDIT_COPY,      SetWindow::onEditCopy)
  EVT_MENU      (ID_EDIT_PASTE,      SetWindow::onEditPaste)
  EVT_MENU      (ID_EDIT_SELECT_ALL, SetWindow::onEditSelectAll)
  EVT_MENU      (ID_EDIT_FIND,      SetWindow::onEditFind)
  EVT_MENU      (ID_EDIT_FIND_NEXT,    SetWindow::onEditFindNext)
  EVT_MENU      (ID_EDIT_REPLACE,    SetWindow::onEditReplace)
  EVT_MENU      (ID_EDIT_AUTO_REPLACE,  SetWindow::onEditAutoReplace)
  EVT_MENU      (ID_EDIT_PREFERENCES,  SetWindow::onEditPreferences)
  EVT_MENU      (ID_WINDOW_NEW,      SetWindow::onWindowNewWindow)
  EVT_TOOL_RANGE    (ID_WINDOW_MIN, ID_WINDOW_MAX, SetWindow::onWindowSelect)
  EVT_MENU      (ID_HELP_INDEX,      SetWindow::onHelpIndex)
  EVT_MENU      (ID_HELP_WEBSITE,    SetWindow::onHelpWebsite)
  EVT_MENU      (ID_HELP_ABOUT,      SetWindow::onHelpAbout)
  EVT_MENU_OPEN    (            SetWindow::onMenuOpen)
  EVT_TOOL_RANGE    (ID_CHILD_MIN, ID_CHILD_MAX,   SetWindow::onChildMenu)
  EVT_COMMAND_RANGE  (ID_CHILD_MIN, ID_CHILD_MAX, wxEVT_COMMAND_BUTTON_CLICKED, SetWindow::onChildMenu)
  EVT_COMMAND_RANGE  (ID_CHILD_MIN, ID_CHILD_MAX, wxEVT_COMMAND_SPINCTRL_UPDATED, SetWindow::onChildMenu)
  EVT_COMMAND_RANGE  (ID_CHILD_MIN, ID_CHILD_MAX, wxEVT_COMMAND_RADIOBUTTON_SELECTED, SetWindow::onChildMenu)
  EVT_COMMAND_RANGE  (ID_CHILD_MIN, ID_CHILD_MAX, wxEVT_COMMAND_TEXT_UPDATED, SetWindow::onChildMenu)
  EVT_GALLERY_SELECT  (ID_FIELD_LIST,                SetWindow::onChildMenu) // for StatsPanel, because it is not a EVT_TOOL
  
  EVT_UPDATE_UI    (wxID_ANY,        SetWindow::onUpdateUI)
  EVT_FIND      (wxID_ANY,        SetWindow::onFind)
  EVT_FIND_NEXT    (wxID_ANY,        SetWindow::onFindNext)
  EVT_FIND_REPLACE  (wxID_ANY,        SetWindow::onReplace)
  EVT_FIND_REPLACE_ALL(wxID_ANY,        SetWindow::onReplaceAll)
  EVT_CLOSE      (            SetWindow::onClose)
  EVT_IDLE      (            SetWindow::onIdle)
  EVT_CARD_SELECT    (wxID_ANY,        SetWindow::onCardSelect)
  EVT_CARD_ACTIVATE  (wxID_ANY,        SetWindow::onCardActivate)
  EVT_SIZE_CHANGE    (wxID_ANY,        SetWindow::onSizeChange)
END_EVENT_TABLE  ()
