import sys
import os
import json
import markdown
import shutil
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QPlainTextEdit, QFileDialog, QMessageBox,
    QSplitter, QTreeView, QStackedWidget, QTextBrowser, QToolBar,
    QWidget, QVBoxLayout, QSizePolicy, QStyle, QMenu, QInputDialog,
    QLineEdit, QLabel, QDialog, QListWidget, QListWidgetItem, QHBoxLayout,
    QPushButton, QAbstractItemView
)
from PyQt6.QtGui import QAction, QFont, QFileSystemModel, QIcon, QPdfWriter, QTextDocument, QColor
from PyQt6.QtCore import Qt, QDir, QSettings, QSortFilterProxyModel, QRegularExpression, QByteArray

class FileFilterProxyModel(QSortFilterProxyModel):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._dir_match_cache = {}
        self.hide_empty_folders = False
        self._all_md_files = []
        
    def update_workspace_index(self, root_path):
        self._all_md_files = []
        try:
            for root, dirs, files in os.walk(root_path):
                for f in files:
                    if f.endswith('.md'):
                        self._all_md_files.append(os.path.join(root, f))
        except Exception:
            pass
        self._dir_match_cache.clear()
        self.invalidateFilter()

    def add_to_index(self, filepath):
        if filepath not in self._all_md_files:
            self._all_md_files.append(filepath)
            self._dir_match_cache.clear()
            self.invalidateFilter()
            
    def remove_from_index(self, filepath):
        if filepath in self._all_md_files:
            self._all_md_files.remove(filepath)
            self._dir_match_cache.clear()
            self.invalidateFilter()
            
    def rename_in_index(self, old_path, new_path):
        if old_path in self._all_md_files:
            self._all_md_files.remove(old_path)
            self._all_md_files.append(new_path)
            self._dir_match_cache.clear()
            self.invalidateFilter()

    def setHideEmptyFolders(self, hide):
        self.hide_empty_folders = hide
        self.invalidateFilter()
        
    def setFilterRegularExpression(self, regex):
        self._dir_match_cache.clear()
        super().setFilterRegularExpression(regex)
        
    def setSourceModel(self, model):
        super().setSourceModel(model)
        try:
            # Drop cache on file system changes or background finishes
            model.directoryLoaded.connect(lambda: self._dir_match_cache.clear())
            model.fileRenamed.connect(lambda *args: self._dir_match_cache.clear())
            model.rowsInserted.connect(lambda *args: self._dir_match_cache.clear())
            model.rowsRemoved.connect(lambda *args: self._dir_match_cache.clear())
        except AttributeError:
            pass

    def _has_matching_file(self, path, regex):
        if path in self._dir_match_cache:
            return self._dir_match_cache[path]
            
        has_match = False
        path_prefix = path if path.endswith(os.sep) else path + os.sep
        
        for md_file in self._all_md_files:
            if md_file == path or md_file.startswith(path_prefix):
                name = os.path.basename(md_file)
                if regex.pattern() == "" or regex.match(name).hasMatch():
                    has_match = True
                    break
            
        self._dir_match_cache[path] = has_match
        return has_match

    def data(self, index, role=Qt.ItemDataRole.DisplayRole):
        if role == Qt.ItemDataRole.ForegroundRole:
            source_index = self.mapToSource(index)
            model = self.sourceModel()
            if model and model.isDir(source_index):
                path = model.filePath(source_index)
                regex = self.filterRegularExpression()
                if not self._has_matching_file(path, regex):
                    return QColor("gray")
                    
        return super().data(index, role)
    def filterAcceptsRow(self, source_row, source_parent):
        model = self.sourceModel()
        index = model.index(source_row, 0, source_parent)
        
        # Always accept directories to preserve tree expandability
        if model.isDir(index):
            if self.hide_empty_folders:
                path = model.filePath(index)
                regex = self.filterRegularExpression()
                return self._has_matching_file(path, regex)
            return True
            
        # Otherwise, check file name against our regex filter
        return super().filterAcceptsRow(source_row, source_parent)

class CustomizeToolbarDialog(QDialog):
    def __init__(self, catalog, current_layout, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Customize Toolbar")
        self.catalog = catalog
        self.current_layout = list(current_layout)
        
        self.resize(500, 400)
        
        main_layout = QHBoxLayout(self)
        
        left_layout = QVBoxLayout()
        left_layout.addWidget(QLabel("Available Actions:"))
        self.avail_list = QListWidget()
        left_layout.addWidget(self.avail_list)
        main_layout.addLayout(left_layout)
        
        btn_layout = QVBoxLayout()
        btn_layout.addStretch()
        self.btn_add = QPushButton("Add ->")
        self.btn_remove = QPushButton("<- Remove")
        btn_layout.addWidget(self.btn_add)
        btn_layout.addWidget(self.btn_remove)
        btn_layout.addStretch()
        main_layout.addLayout(btn_layout)
        
        right_layout = QVBoxLayout()
        right_layout.addWidget(QLabel("Current Toolbar:"))
        self.curr_list = QListWidget()
        right_layout.addWidget(self.curr_list)
        
        ud_layout = QHBoxLayout()
        self.btn_up = QPushButton("Up")
        self.btn_down = QPushButton("Down")
        ud_layout.addWidget(self.btn_up)
        ud_layout.addWidget(self.btn_down)
        right_layout.addLayout(ud_layout)
        
        ac_layout = QHBoxLayout()
        self.btn_apply = QPushButton("Apply")
        self.btn_cancel = QPushButton("Cancel")
        ac_layout.addWidget(self.btn_apply)
        ac_layout.addWidget(self.btn_cancel)
        right_layout.addLayout(ac_layout)
        
        main_layout.addLayout(right_layout)
        
        self.populate_lists()
        
        self.btn_add.clicked.connect(self.add_item)
        self.btn_remove.clicked.connect(self.remove_item)
        self.btn_up.clicked.connect(self.move_up)
        self.btn_down.clicked.connect(self.move_down)
        self.btn_apply.clicked.connect(self.accept)
        self.btn_cancel.clicked.connect(self.reject)

    def populate_lists(self):
        for item_id in self.current_layout:
            name = self.catalog.get(item_id, {}).get("name", item_id)
            item = QListWidgetItem(name)
            item.setData(Qt.ItemDataRole.UserRole, item_id)
            self.curr_list.addItem(item)
            
        for item_id, info in self.catalog.items():
            item = QListWidgetItem(info["name"])
            item.setData(Qt.ItemDataRole.UserRole, item_id)
            self.avail_list.addItem(item)

    def add_item(self):
        row = self.avail_list.currentRow()
        if row >= 0:
            item = self.avail_list.item(row)
            item_id = item.data(Qt.ItemDataRole.UserRole)
            new_item = QListWidgetItem(item.text())
            new_item.setData(Qt.ItemDataRole.UserRole, item_id)
            self.curr_list.addItem(new_item)

    def remove_item(self):
        row = self.curr_list.currentRow()
        if row >= 0:
            self.curr_list.takeItem(row)

    def move_up(self):
        row = self.curr_list.currentRow()
        if row > 0:
            item = self.curr_list.takeItem(row)
            self.curr_list.insertItem(row - 1, item)
            self.curr_list.setCurrentRow(row - 1)

    def move_down(self):
        row = self.curr_list.currentRow()
        if row >= 0 and row < self.curr_list.count() - 1:
            item = self.curr_list.takeItem(row)
            self.curr_list.insertItem(row + 1, item)
            self.curr_list.setCurrentRow(row + 1)
            
    def get_layout(self):
        new_layout = []
        for i in range(self.curr_list.count()):
            new_layout.append(self.curr_list.item(i).data(Qt.ItemDataRole.UserRole))
        return new_layout

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        
        self.current_file = None
        self.current_folder = None
        self.custom_font_size = 14
        
        # Load global settings
        self.settings = QSettings("Felsic", "FelsicNotes")
        
        self.init_ui()
        
        geom = self.settings.value("geometry")
        if geom is not None:
            self.restoreGeometry(geom)
            
        state = self.settings.value("windowState")
        if state is not None:
            self.restoreState(state)
            
        splitter_state = self.settings.value("splitterState")
        if splitter_state is not None:
            self.splitter.restoreState(splitter_state)
        
        # Restore last opened workspace if it exists
        last_workspace = self.settings.value("last_workspace", "")
        if last_workspace and os.path.isdir(last_workspace):
            self.open_workspace(last_workspace)
        else:
            self.update_title()

    def init_ui(self):
        # Main Splitter
        self.splitter = QSplitter(Qt.Orientation.Horizontal)
        
        # Left Panel: File Tree
        left_container = QWidget()
        left_layout = QVBoxLayout(left_container)
        left_layout.setContentsMargins(0, 0, 0, 0)
        
        self.search_box = QLineEdit()
        self.search_box.setPlaceholderText("Search notes...")
        self.search_box.setClearButtonEnabled(True)
        self.search_box.textChanged.connect(self.on_search_changed)
        self.search_box.setStyleSheet("QLineEdit { padding: 5px; border-radius: 4px; border: 1px solid #ccc; margin: 4px; }")
        
        self.tree_view = QTreeView()
        self.file_model = QFileSystemModel()
        # Filter for directories and markdown files (AllDirs keeps directories regardless of name filters)
        self.file_model.setFilter(QDir.Filter.AllDirs | QDir.Filter.Files | QDir.Filter.NoDotAndDotDot)
        self.file_model.setNameFilters(["*.md"])
        self.file_model.setNameFilterDisables(False) # Hide files that don't match, instead of disabling
        
        self.file_model.setRootPath("")
        
        self.proxy_model = FileFilterProxyModel()
        self.proxy_model.setSourceModel(self.file_model)
        self.proxy_model.setFilterCaseSensitivity(Qt.CaseSensitivity.CaseInsensitive)
        
        self.tree_view.setModel(self.proxy_model)
        
        left_layout.addWidget(self.search_box)
        left_layout.addWidget(self.tree_view)
        
        # Hide standard file system columns except Name
        for col in range(1, 4):
            self.tree_view.hideColumn(col)
            
        self.tree_view.setHeaderHidden(True)
        self.tree_view.setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        self.tree_view.customContextMenuRequested.connect(self.show_tree_context_menu)
        self.tree_view.clicked.connect(self.on_tree_clicked)
        self.file_model.directoryLoaded.connect(self.sync_tree_selection)
        
        # Central Editor & Previewer mechanism
        self.stacked_widget = QStackedWidget()
        
        # Page 0: Pure Text Editor
        self.editor = QPlainTextEdit()
        font = QFont()
        # Explicit priority list for Linux/Cross-platform to avoid "LastResort" script 18 errors
        font.setFamilies(["DejaVu Sans Mono", "Noto Sans Mono", "Liberation Mono", "Monospace"])
        font.setStyleHint(QFont.StyleHint.TypeWriter)
        self.editor.setFont(font)
        
        # Page 1: Rendered HTML Browser
        self.previewer = QTextBrowser()
        self.previewer.setOpenExternalLinks(True)
        
        self.stacked_widget.addWidget(self.editor)
        self.stacked_widget.addWidget(self.previewer)
        
        # Right Side Container (Toolbar + Title + Editor)
        right_container = QWidget()
        right_layout = QVBoxLayout()
        right_layout.setContentsMargins(0, 0, 0, 0)
        right_layout.setSpacing(0)
        right_container.setLayout(right_layout)
        
        # Local Tool Bar for Preview Toggle
        self.view_toolbar = QToolBar("View")
        self.view_toolbar.setToolButtonStyle(Qt.ToolButtonStyle.ToolButtonIconOnly)
        
        save_icon = QIcon.fromTheme("document-save", self.style().standardIcon(QStyle.StandardPixmap.SP_DialogSaveButton))
        self.save_button_action = QAction(save_icon, "Save", self)
        self.save_button_action.setShortcut("Ctrl+S")
        self.save_button_action.triggered.connect(self.save_file)
        
        save_as_icon = QIcon.fromTheme("document-save-as", self.style().standardIcon(QStyle.StandardPixmap.SP_DialogSaveButton))
        self.save_as_button_action = QAction(save_as_icon, "Save As...", self)
        self.save_as_button_action.setShortcut("Ctrl+Shift+S")
        self.save_as_button_action.triggered.connect(self.save_file_as)
        
        new_icon = QIcon.fromTheme("document-new", self.style().standardIcon(QStyle.StandardPixmap.SP_FileIcon))
        self.new_action = QAction(new_icon, "New", self)
        self.new_action.setShortcut("Ctrl+N")
        self.new_action.triggered.connect(self.new_file)
        
        open_icon = QIcon.fromTheme("document-open", self.style().standardIcon(QStyle.StandardPixmap.SP_DialogOpenButton))
        self.open_action = QAction(open_icon, "Open File...", self)
        self.open_action.setShortcut("Ctrl+O")
        self.open_action.triggered.connect(self.open_file)
        
        folder_icon = QIcon.fromTheme("folder-open", self.style().standardIcon(QStyle.StandardPixmap.SP_DirOpenIcon))
        self.open_folder_action = QAction(folder_icon, "Open Folder...", self)
        self.open_folder_action.setShortcut("Ctrl+Shift+O")
        self.open_folder_action.triggered.connect(self.open_folder)
        
        exit_icon = QIcon.fromTheme("application-exit", self.style().standardIcon(QStyle.StandardPixmap.SP_DialogCloseButton))
        self.exit_action = QAction(exit_icon, "Quit", self)
        self.exit_action.setShortcut("Ctrl+Q")
        self.exit_action.triggered.connect(self.close)
        
        pdf_icon = QIcon.fromTheme("application-pdf-symbolic", QIcon.fromTheme("application-pdf", self.style().standardIcon(QStyle.StandardPixmap.SP_DriveFDIcon)))
        self.export_pdf_action = QAction(pdf_icon, "Export to PDF", self)
        self.export_pdf_action.triggered.connect(self.export_pdf)
        
        zoom_in_icon = QIcon.fromTheme("zoom-in")
        self.zoom_in_action = QAction(zoom_in_icon, "Zoom In", self)
        self.zoom_in_action.triggered.connect(self.zoom_in)
        
        zoom_out_icon = QIcon.fromTheme("zoom-out")
        self.zoom_out_action = QAction(zoom_out_icon, "Zoom Out", self)
        self.zoom_out_action.triggered.connect(self.zoom_out)
        
        bold_icon = QIcon.fromTheme("format-text-bold")
        self.bold_action = QAction(bold_icon, "Bold", self)
        self.bold_action.triggered.connect(lambda: self.toggle_markdown("**"))
        
        italic_icon = QIcon.fromTheme("format-text-italic")
        self.italic_action = QAction(italic_icon, "Italic", self)
        self.italic_action.triggered.connect(lambda: self.toggle_markdown("*"))
        
        link_icon = QIcon.fromTheme("insert-link", self.style().standardIcon(QStyle.StandardPixmap.SP_ArrowRight))
        self.link_action = QAction(link_icon, "Link", self)
        self.link_action.triggered.connect(self.insert_link)
        
        code_icon = QIcon.fromTheme("format-text-code")
        if code_icon.isNull(): # fallback gracefully
             code_icon = QIcon.fromTheme("text-x-script", self.style().standardIcon(QStyle.StandardPixmap.SP_FileIcon))
        self.code_action = QAction(code_icon, "Code", self)
        self.code_action.triggered.connect(lambda: self.toggle_markdown("`"))
        
        preview_icon = QIcon.fromTheme("view-preview", self.style().standardIcon(QStyle.StandardPixmap.SP_DesktopIcon))
        self.toggle_preview_action = QAction(preview_icon, "Toggle Preview", self)
        self.toggle_preview_action.setShortcut("Ctrl+P")
        self.toggle_preview_action.setCheckable(True)
        self.toggle_preview_action.triggered.connect(self.toggle_preview)
        
        wrap_icon = QIcon.fromTheme("format-text-wrap", self.style().standardIcon(QStyle.StandardPixmap.SP_FileDialogDetailedView))
        self.toggle_wrap_action = QAction(wrap_icon, "Wrap Text", self)
        self.toggle_wrap_action.setCheckable(True)
        self.toggle_wrap_action.setChecked(True)
        self.toggle_wrap_action.triggered.connect(self.toggle_wrapping)
        
        self.upper_action = QAction("UPPER CASE", self)
        self.upper_action.triggered.connect(lambda: self.change_case("upper"))
        
        self.lower_action = QAction("lower case", self)
        self.lower_action.triggered.connect(lambda: self.change_case("lower"))
        
        self.title_action = QAction("Title Case", self)
        self.title_action.triggered.connect(lambda: self.change_case("title"))
        
        self.sentence_action = QAction("Sentence case", self)
        self.sentence_action.triggered.connect(lambda: self.change_case("sentence"))
        
        self.toggle_hide_empty_action = QAction("Hide Empty Folders", self)
        self.toggle_hide_empty_action.setCheckable(True)
        self.toggle_hide_empty_action.triggered.connect(self.toggle_hide_empty_folders)
        
        self.about_action = QAction("About", self)
        self.about_action.triggered.connect(self.show_about)
        
        self.available_tools_catalog = {
            "new_file": {"name": "New File", "action": self.new_action},
            "open_file": {"name": "Open File...", "action": self.open_action},
            "open_folder": {"name": "Open Folder...", "action": self.open_folder_action},
            "exit": {"name": "Quit", "action": self.exit_action},
            "save_file": {"name": "Save", "action": self.save_button_action},
            "save_as": {"name": "Save As...", "action": self.save_as_button_action},
            "export_pdf": {"name": "Export to PDF", "action": self.export_pdf_action},
            "zoom_in": {"name": "Zoom In", "action": self.zoom_in_action},
            "zoom_out": {"name": "Zoom Out", "action": self.zoom_out_action},
            "bold": {"name": "Bold", "action": self.bold_action},
            "italic": {"name": "Italic", "action": self.italic_action},
            "link": {"name": "Link", "action": self.link_action},
            "code": {"name": "Code", "action": self.code_action},
            "upper_case": {"name": "UPPER CASE", "action": self.upper_action},
            "lower_case": {"name": "lower case", "action": self.lower_action},
            "title_case": {"name": "Title Case", "action": self.title_action},
            "sentence_case": {"name": "Sentence case", "action": self.sentence_action},
            "preview": {"name": "Toggle Preview", "action": self.toggle_preview_action},
            "wrap_text": {"name": "Toggle Wrap", "action": self.toggle_wrap_action},
            "hide_empty_folders": {"name": "Hide Empty Folders", "action": self.toggle_hide_empty_action},
            "about": {"name": "About", "action": self.about_action},
            "spacer": {"name": "Space (Align Right)", "action": None},
            "separator": {"name": "Vertical Separator", "action": None}
        }
        
        self.default_toolbar_layout = [
            "save_file", "save_as", "export_pdf", "spacer",
            "zoom_in", "zoom_out", "separator", "bold", "italic", "link", "code", "spacer",
            "preview", "wrap_text"
        ]
        
        self.current_toolbar_layout = list(self.default_toolbar_layout)
            
        self.build_toolbar()
        
        # Title Box
        self.title_box = QLineEdit()
        self.title_box.setPlaceholderText("Untitled Note")
        title_font = QFont("Sans Serif", 16, QFont.Weight.Bold)
        self.title_box.setFont(title_font)
        self.title_box.setStyleSheet("QLineEdit { border: none; padding: 10px; }")
        self.title_box.textChanged.connect(self.on_title_changed)
        self.title_box.returnPressed.connect(self.save_file)
        
        right_layout.addWidget(self.view_toolbar)
        right_layout.addWidget(self.title_box)
        right_layout.addWidget(self.stacked_widget)
        
        self.splitter.addWidget(left_container)
        self.splitter.addWidget(right_container)
        
        # Set proportions
        self.splitter.setSizes([200, 600])
        
        self.setCentralWidget(self.splitter)
        
        # Menu Bar
        menu_bar = self.menuBar()
        file_menu = menu_bar.addMenu("File")
        
        file_menu.addAction(self.new_action)
        file_menu.addAction(self.open_action)
        file_menu.addAction(self.open_folder_action)
        file_menu.addSeparator()
        file_menu.addAction(self.save_button_action)
        file_menu.addAction(self.save_as_button_action)
        file_menu.addAction(self.export_pdf_action)
        file_menu.addSeparator()
        file_menu.addAction(self.exit_action)
        
        edit_menu = menu_bar.addMenu("Edit")
        edit_menu.addAction(self.bold_action)
        edit_menu.addAction(self.italic_action)
        edit_menu.addAction(self.link_action)
        edit_menu.addAction(self.code_action)
        edit_menu.addSeparator()
        edit_menu.addAction(self.upper_action)
        edit_menu.addAction(self.lower_action)
        edit_menu.addAction(self.title_action)
        edit_menu.addAction(self.sentence_action)
        
        # View Menu
        view_menu = menu_bar.addMenu("View")
        view_menu.addAction(self.toggle_preview_action)
        view_menu.addAction(self.toggle_wrap_action)
        view_menu.addAction(self.toggle_hide_empty_action)
        view_menu.addSeparator()
        view_menu.addAction(self.zoom_in_action)
        view_menu.addAction(self.zoom_out_action)
        view_menu.addSeparator()
        
        customize_action = QAction("Customize Toolbar...", self)
        customize_action.triggered.connect(self.customize_toolbar)
        view_menu.addAction(customize_action)
        
        # Help Menu
        help_menu = menu_bar.addMenu("Help")
        help_menu.addAction(self.about_action)
        
        # Window attributes
        self.resize(800, 600)
        
        # Status Bar
        self.status_bar = self.statusBar()
        self.stats_label = QLabel()
        self.status_bar.addPermanentWidget(self.stats_label)
        
        # Listen for content changes
        self.editor.document().modificationChanged.connect(self.update_title)
        self.editor.textChanged.connect(self.update_stats)
        
        # Apply initial fallback zoom if not restored
        self.apply_font_size()

    def build_toolbar(self):
        self.view_toolbar.clear()
        for item_id in self.current_toolbar_layout:
            if item_id == "spacer":
                spacer = QWidget()
                spacer.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
                self.view_toolbar.addWidget(spacer)
            elif item_id == "separator":
                self.view_toolbar.addSeparator()
            else:
                info = self.available_tools_catalog.get(item_id)
                if info and info["action"]:
                    self.view_toolbar.addAction(info["action"])

    def customize_toolbar(self):
        dialog = CustomizeToolbarDialog(self.available_tools_catalog, self.current_toolbar_layout, self)
        if dialog.exec():
            # Dialog passed the accept signal
            self.current_toolbar_layout = dialog.get_layout()
            self.build_toolbar()
            self._save_workspace_config()

    def update_stats(self):
        text = self.editor.toPlainText()
        chars = len(text)
        words = len(text.split())
        
        size_str = "Unsaved"
        if self.current_file and os.path.exists(self.current_file):
            size_bytes = os.path.getsize(self.current_file)
            if size_bytes < 1024:
                size_str = f"{size_bytes} B"
            elif size_bytes < 1024 * 1024:
                size_str = f"{size_bytes / 1024:.1f} KB"
            else:
                size_str = f"{size_bytes / (1024 * 1024):.2f} MB"
        else:
            # Estimate size
            size_bytes = len(text.encode('utf-8'))
            if size_bytes < 1024:
                size_str = f"~{size_bytes} B"
            else:
                size_str = f"~{size_bytes / 1024:.1f} KB"
            
        self.stats_label.setText(f"{words} Words  |  {chars} Characters  |  Size: {size_str}")

    def on_title_changed(self):
        if not self.editor.document().isModified():
             self.editor.document().setModified(True)
             self.update_title()

    def toggle_preview(self, checked=False):
        if checked:
            # Switch to Rendered HTML
            text = self.editor.toPlainText()
            # Compile markdown
            html = markdown.markdown(text, extensions=['extra', 'nl2br'])
            # We add a bit of basic styling so it doesn't look totally raw
            styled_html = f"""
            <style>
                body {{ font-family: sans-serif; font-size: 14px; line-height: 1.6; }}
                code {{ background-color: #f4f4f4; padding: 2px 4px; border-radius: 4px; font-family: monospace; }}
                pre {{ background-color: #f4f4f4; padding: 10px; border-radius: 4px; overflow-x: auto; }}
                blockquote {{ border-left: 4px solid #ccc; margin-left: 0; padding-left: 16px; color: #666; }}
                table {{ border-collapse: collapse; width: 100%; }}
                th, td {{ border: 1px solid #ddd; padding: 8px; }}
            </style>
            {html}
            """
            self.previewer.setHtml(styled_html)
            self.stacked_widget.setCurrentIndex(1)
        else:
            # Switch back to plaintext editor
            self.stacked_widget.setCurrentIndex(0)

    def insert_link(self):
        cursor = self.editor.textCursor()
        url = ""
        if cursor.hasSelection():
            # Strips eventual broken newlines to preserve link logic
            url = cursor.selectedText().replace('\u2029', '').strip()
            
        text_to_insert = f"[]({url})"
        cursor.insertText(text_to_insert)
        
        # O cursor termina no fim da string ex: [](url) |
        # Recuamos exatamente o trecho equivalente a "]" seguido de "({url})"
        # Para que o cursor repouse magicamente entre os colchetes!
        steps_back = len(url) + 3
        cursor.movePosition(cursor.MoveOperation.Left, cursor.MoveMode.MoveAnchor, steps_back)
        self.editor.setTextCursor(cursor)

    def change_case(self, case_type):
        cursor = self.editor.textCursor()
        if not cursor.hasSelection():
            return
            
        text = cursor.selectedText().replace('\u2029', '\n')
        
        if case_type == "upper":
            new_text = text.upper()
        elif case_type == "lower":
            new_text = text.lower()
        elif case_type == "title":
            new_text = text.title()
        elif case_type == "sentence":
            new_text = text.capitalize()
            
        cursor.insertText(new_text)

    def toggle_markdown(self, marker):
        cursor = self.editor.textCursor()
        if not cursor.hasSelection():
            # If nothing is selected, surround with marker and put cursor in the middle
            cursor.insertText(f"{marker}{marker}")
            cursor.movePosition(cursor.MoveOperation.Left, cursor.MoveMode.MoveAnchor, len(marker))
            self.editor.setTextCursor(cursor)
            return
            
        text = cursor.selectedText().replace('\u2029', '\n')
        mlen = len(marker)
        
        # Verify if text is completely surrounded by the marker to unwrap it
        if text.startswith(marker) and text.endswith(marker) and len(text) >= mlen * 2:
            # Do an exact unwrap
            # Exception for '*' against '**': if italic marker, make sure it isn't bold
            if marker == '*' and text.startswith('**') and text.endswith('**'):
                cursor.insertText(f"{marker}{text}{marker}")
            else:
                cursor.insertText(text[mlen:-mlen])
        else:
            # Wrap the text
            cursor.insertText(f"{marker}{text}{marker}")

    def toggle_wrapping(self, checked):
        if checked:
            self.editor.setLineWrapMode(QPlainTextEdit.LineWrapMode.WidgetWidth)
        else:
            self.editor.setLineWrapMode(QPlainTextEdit.LineWrapMode.NoWrap)

    def zoom_in(self):
        if self.custom_font_size < 48:
            self.custom_font_size += 2
            self.apply_font_size()
            self._save_workspace_config()

    def zoom_out(self):
        if self.custom_font_size > 8:
            self.custom_font_size -= 2
            self.apply_font_size()
            self._save_workspace_config()

    def apply_font_size(self):
        font = self.editor.font()
        font.setPointSize(self.custom_font_size)
        self.editor.setFont(font)
        if self.stacked_widget.currentIndex() == 1:
            self.toggle_preview(True)

    def on_search_changed(self, text):
        regex = QRegularExpression(text, QRegularExpression.PatternOption.CaseInsensitiveOption)
        self.proxy_model.setFilterRegularExpression(regex)

    def toggle_hide_empty_folders(self, checked):
        self.proxy_model.setHideEmptyFolders(checked)
        self._save_workspace_config()

    def show_about(self):
        text = """<h3>Felsic Notes</h3>
        <p>A fast, portable, and lightweight Markdown note-taking app.</p>
        <p>Built with PyQt6 and copious amounts of AI.</p>
        <p><a href="https://github.com/deomkds/felsic-notes">GitHub Repository</a></p>"""
        QMessageBox.about(self, "About Felsic Notes", text)

    def sync_tree_selection(self, path=None):
        if self.current_file:
            source_index = self.file_model.index(self.current_file)
            if source_index.isValid():
                proxy_index = self.proxy_model.mapFromSource(source_index)
                if proxy_index.isValid():
                    self.tree_view.setCurrentIndex(proxy_index)
                    self.tree_view.scrollTo(proxy_index)

    def update_title(self):
        title = "Felsic Notes"
        if self.current_file:
            filename = os.path.basename(self.current_file)
            title = f"{filename} - {title}"
        else:
            title = f"Untitled - {title}"
            
        if self.editor.document().isModified():
            title = f"*{title}"
            
        if self.current_folder:
            folder_name = os.path.basename(self.current_folder)
            title += f" ({folder_name})"
            
        self.setWindowTitle(title)

    def set_current_document(self, filename):
        self.current_file = filename
        
        # Update the UI Title Box
        self.title_box.blockSignals(True)
        if filename:
            name = os.path.splitext(os.path.basename(filename))[0]
            self.title_box.setText(name)
        else:
            self.title_box.setText("")
        self.title_box.blockSignals(False)
        
        self.editor.document().setModified(False)
        self._save_workspace_config()
        # Se estivermos no modo Preview, atualizamos a renderização para a nova nota
        if self.stacked_widget.currentIndex() == 1:
            self.toggle_preview(True)
        self.update_title()
        self.sync_tree_selection()
        self.update_stats()

    def _get_workspace_config_path(self):
        if not self.current_folder: return None
        return os.path.join(self.current_folder, ".felsic", "config.json")

    def _load_workspace_config(self):
        config_path = self._get_workspace_config_path()
        if not config_path or not os.path.exists(config_path):
            self.editor.clear()
            self.set_current_document(None)
            return
            
        try:
            with open(config_path, "r", encoding="utf-8") as f:
                data = json.load(f)
                
            self.custom_font_size = data.get("font_size", 14)
            self.apply_font_size()
            
            raw_layout = data.get("toolbar_layout", self.default_toolbar_layout)
            if isinstance(raw_layout, list):
                self.current_toolbar_layout = [str(x) for x in raw_layout]
            else:
                self.current_toolbar_layout = list(self.default_toolbar_layout)
            self.build_toolbar()
            
            hide_empty = data.get("hide_empty_folders", False)
            self.toggle_hide_empty_action.setChecked(hide_empty)
            self.proxy_model.setHideEmptyFolders(hide_empty)
                
            window_geometry = data.get("window_geometry")
            if window_geometry:
                self.restoreGeometry(QByteArray.fromHex(window_geometry.encode('utf-8')))

            splitter_state = data.get("splitter_state")
            if splitter_state:
                self.splitter.restoreState(QByteArray.fromHex(splitter_state.encode('utf-8')))
                
            last_file = data.get("last_file")
            # Only reopen if file resides strictly inside the workspace and still exists
            if last_file and os.path.exists(last_file) and last_file.startswith(self.current_folder):
                self.load_file(last_file)
            else:
                self.editor.clear()
                self.set_current_document(None)
        except Exception:
            self.editor.clear()
            self.set_current_document(None)

    def _save_workspace_config(self):
        if not self.current_folder: return
        
        felsic_dir = os.path.join(self.current_folder, ".felsic")
        if not os.path.exists(felsic_dir):
            try:
                os.makedirs(felsic_dir)
            except Exception:
                return # Fail gracefully if read-only
                
        config_path = os.path.join(felsic_dir, "config.json")
        data = {}
        if self.current_file:
            data["last_file"] = self.current_file
        data["font_size"] = self.custom_font_size
        
        if hasattr(self, 'current_toolbar_layout'):
            data["toolbar_layout"] = self.current_toolbar_layout
            
        data["hide_empty_folders"] = self.toggle_hide_empty_action.isChecked()
        data["window_geometry"] = self.saveGeometry().toHex().data().decode('utf-8')
        data["splitter_state"] = self.splitter.saveState().toHex().data().decode('utf-8')
            
        try:
            with open(config_path, "w", encoding="utf-8") as f:
                json.dump(data, f)
        except Exception:
            pass

    def new_file(self):
        if self.maybe_save():
            self.editor.clear()
            self.set_current_document(None)

    def open_file(self):
        if self.maybe_save():
            filename, _ = QFileDialog.getOpenFileName(
                self, "Open Document", "", "Markdown Files (*.md);;All Files (*)"
            )
            
            if filename:
                self.load_file(filename)

    def open_folder(self):
        if self.maybe_save():
            folder = QFileDialog.getExistingDirectory(self, "Open Folder")
            if folder:
                self.open_workspace(folder)

    def open_workspace(self, folder):
        if self.current_folder:
            self._save_workspace_config() # persist state of previous folder
            
        self.current_folder = folder
        self.settings.setValue("last_workspace", folder)
        self.file_model.setRootPath(folder)
        self.proxy_model.update_workspace_index(folder)
        
        source_index = self.file_model.index(folder)
        self.tree_view.setRootIndex(self.proxy_model.mapFromSource(source_index))
        
        self._load_workspace_config()
        self.update_title()

    def show_tree_context_menu(self, position):
        if not self.current_folder:
            return  # No workspace is opened
            
        proxy_index = self.tree_view.indexAt(position)
        source_index = self.proxy_model.mapToSource(proxy_index) if proxy_index.isValid() else proxy_index
        
        menu = QMenu()
        
        if source_index.isValid() and not self.file_model.isDir(source_index):
            # Clicou com botão direito num arquivo
            file_path = self.file_model.filePath(source_index)
            
            rename_action = QAction("Rename...", self)
            rename_action.triggered.connect(lambda checked=False, p=file_path: self.rename_note(p))
            menu.addAction(rename_action)
            
            move_action = QAction("Move To...", self)
            move_action.triggered.connect(lambda checked=False, p=file_path: self.move_note(p))
            menu.addAction(move_action)
            
            dup_action = QAction("Duplicate", self)
            dup_action.triggered.connect(lambda checked=False, p=file_path: self.duplicate_note(p))
            menu.addAction(dup_action)
            
            menu.addSeparator()
            
            delete_action = QAction("Delete", self)
            delete_action.triggered.connect(lambda checked=False, p=file_path: self.delete_note(p))
            menu.addAction(delete_action)
            
        else:
            # Clicou nas pastas ou vazio
            if source_index.isValid() and self.file_model.isDir(source_index):
                base_dir = self.file_model.filePath(source_index)
            else:
                base_dir = self.current_folder

            new_note_action = QAction("New Note...", self)
            new_note_action.triggered.connect(lambda checked=False, d=base_dir: self.create_new_note(d))
            menu.addAction(new_note_action)
        
        menu.exec(self.tree_view.viewport().mapToGlobal(position))

    def rename_note(self, source_path):
        current_name = os.path.basename(source_path)
        new_name, ok = QInputDialog.getText(self, "Rename Note", "New Name:", text=current_name)
        
        if ok and new_name.strip() and new_name != current_name:
            new_name = new_name.strip()
            if not new_name.endswith(".md") and '.' not in new_name:
                new_name += ".md"
            
            dest_path = os.path.join(os.path.dirname(source_path), new_name)
            if os.path.exists(dest_path):
                QMessageBox.warning(self, "Error", "A file with this name already exists.")
                return
                
            try:
                os.rename(source_path, dest_path)
                if self.current_file == source_path:
                    self.set_current_document(dest_path)
            except Exception as e:
                QMessageBox.warning(self, "Error", f"Failed to rename note:\n{e}")

    def move_note(self, source_path):
        dest_folder = QFileDialog.getExistingDirectory(self, "Select Destination Folder", self.current_folder)
        if dest_folder:
            filename = os.path.basename(source_path)
            dest_path = os.path.join(dest_folder, filename)
            
            if os.path.exists(dest_path):
                if dest_path == source_path:
                    return
                QMessageBox.warning(self, "Error", "A file with this name already exists in the destination.")
                return
                
            try:
                shutil.move(source_path, dest_path)
                if self.current_file == source_path:
                    self.set_current_document(dest_path)
            except Exception as e:
                QMessageBox.warning(self, "Error", f"Failed to move note:\n{e}")

    def duplicate_note(self, source_path):
        base_dir = os.path.dirname(source_path)
        base_name, ext = os.path.splitext(os.path.basename(source_path))
        
        counter = 1
        suffix = " (copy)"
        while True:
            new_name = f"{base_name}{suffix}{ext}"
            dest_path = os.path.join(base_dir, new_name)
            if not os.path.exists(dest_path):
                break
            counter += 1
            suffix = f" (copy {counter})"
            
        try:
            shutil.copy2(source_path, dest_path)
        except Exception as e:
            QMessageBox.warning(self, "Error", f"Failed to duplicate note:\n{e}")

    def delete_note(self, source_path):
        answer = QMessageBox.warning(
            self, "Confirm Delete", 
            f"Are you sure you want to permanently delete:\n{os.path.basename(source_path)}?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No
        )
        
        if answer == QMessageBox.StandardButton.Yes:
            try:
                os.remove(source_path)
                if self.current_file == source_path:
                    self.editor.clear()
                    self.title_box.clear()
                    self.set_current_document(None)
            except Exception as e:
                QMessageBox.warning(self, "Error", f"Failed to delete note:\n{e}")

    def create_new_note(self, base_dir):
        text, ok = QInputDialog.getText(self, "New Note", "Note Name:")
        if ok and text.strip():
            filename = text.strip()
            if not filename.endswith(".md") and '.' not in filename:
                filename += ".md"
                
            filepath = os.path.join(base_dir, filename)
            
            if os.path.exists(filepath):
                QMessageBox.warning(self, "Error", "A file with this name already exists in the selected folder.")
                return
                
            try:
                # Create an empty file
                with open(filepath, 'w', encoding='utf-8') as f:
                    f.write('')
                    
                # Open the new note directly
                if self.maybe_save():
                    self.load_file(filepath)
            except Exception as e:
                QMessageBox.warning(self, "Error", f"Failed to create note:\n{e}")

    def on_tree_clicked(self, proxy_index):
        source_index = self.proxy_model.mapToSource(proxy_index)
        if not self.file_model.isDir(source_index):
            file_path = self.file_model.filePath(source_index)
            if file_path == self.current_file:
                return
            if self.maybe_save():
                self.load_file(file_path)

    def load_file(self, filename):
        try:
            with open(filename, 'r', encoding='utf-8') as f:
                content = f.read()
            self.editor.setPlainText(content)
            self.set_current_document(filename)
        except Exception as e:
            QMessageBox.warning(self, "Error", f"Could not read file:\n{e}")

    def save_file(self):
        new_title = self.title_box.text().strip()
        if not new_title:
            new_title = "Untitled"
            
        if not self.current_file:
            # Not working on a file yet. Target the workspace folder or standalone "Save As" behavior.
            if self.current_folder:
                dest_path = os.path.join(self.current_folder, new_title + '.md')
                if os.path.exists(dest_path):
                     QMessageBox.warning(self, "Error", "A note with this name already exists in the workspace.")
                     return False
                return self._save_to_path(dest_path)
            else:
                return self.save_file_as()
        else:
            # Working on an existing file
            current_basename = os.path.splitext(os.path.basename(self.current_file))[0]
            if new_title == current_basename:
                # Name unchanged, standard save
                return self._save_to_path(self.current_file)
            else:
                # Name changed visually
                dest_path = os.path.join(os.path.dirname(self.current_file), new_title + '.md')
                if os.path.exists(dest_path):
                     QMessageBox.warning(self, "Error", "A note with this name already exists. Choose another title.")
                     return False
                     
                old_path = self.current_file
                success = self._save_to_path(dest_path)
                if success:
                     # Attempt to clean up the legacy older-named file
                     try:
                         os.remove(old_path)
                     except Exception:
                         pass
                return success

    def save_file_as(self):
        filename, _ = QFileDialog.getSaveFileName(
            self, "Save Document", "", "Markdown Files (*.md);;All Files (*)"
        )
        if filename:
            if not filename.endswith('.md') and '.' not in os.path.basename(filename):
                filename += '.md'
            return self._save_to_path(filename)
        return False

    def export_pdf(self):
        filename, _ = QFileDialog.getSaveFileName(
            self, "Export PDF", "", "PDF Files (*.pdf)"
        )
        if filename:
            if not filename.endswith('.pdf') and '.' not in os.path.basename(filename):
                filename += '.pdf'
                
            title = self.title_box.text().strip()
            if not title:
                title = "Untitled Note"
                
            text = self.editor.toPlainText()
            html = markdown.markdown(text, extensions=['extra', 'nl2br'])
            styled_html = f"""
            <style>
                body {{ font-family: sans-serif; font-size: {self.custom_font_size}px; line-height: 1.6; color: black; }}
                code {{ background-color: #f4f4f4; padding: 2px 4px; border-radius: 4px; font-family: monospace; }}
                pre {{ background-color: #f4f4f4; padding: 10px; border-radius: 4px; }}
                blockquote {{ border-left: 4px solid #ccc; margin-left: 0; padding-left: 16px; color: #666; }}
                table {{ border-collapse: collapse; width: 100%; }}
                th, td {{ border: 1px solid #ddd; padding: 8px; }}
                h1.pdf-title {{ border-bottom: 2px solid #ccc; font-size: 24px; padding-bottom: 10px; margin-bottom: 20px; }}
            </style>
            <h1 class="pdf-title">{title}</h1>
            {html}
            """
            
            doc = QTextDocument()
            doc.setHtml(styled_html)
            
            try:
                writer = QPdfWriter(filename)
                doc.print(writer)
                QMessageBox.information(self, "Export PDF", "PDF successfully exported.")
            except Exception as e:
                QMessageBox.warning(self, "Error", f"Could not export PDF:\n{e}")

    def _save_to_path(self, path):
        try:
            with open(path, 'w', encoding='utf-8') as f:
                f.write(self.editor.toPlainText())
            self.proxy_model.add_to_index(path)
            self.set_current_document(path)
            return True
        except Exception as e:
            QMessageBox.warning(self, "Error", f"Could not write file:\n{e}")
            return False

    def maybe_save(self):
        if not self.editor.document().isModified():
            return True
            
        ret = QMessageBox.warning(
            self, "Application",
            "The document has been modified.\nDo you want to save your changes?",
            QMessageBox.StandardButton.Save | QMessageBox.StandardButton.Discard | QMessageBox.StandardButton.Cancel
        )
        
        if ret == QMessageBox.StandardButton.Save:
            return self.save_file()
        elif ret == QMessageBox.StandardButton.Cancel:
            return False
            
        return True

    def closeEvent(self, event):
        if self.maybe_save():
            self._save_workspace_config()
            self.settings.setValue("geometry", self.saveGeometry())
            self.settings.setValue("windowState", self.saveState())
            self.settings.setValue("splitterState", self.splitter.saveState())
            event.accept()
        else:
            event.ignore()

def main():
    app = QApplication(sys.argv)
    
    icon_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "icon.png")
    if os.path.exists(icon_path):
        app.setWindowIcon(QIcon(icon_path))
        
    window = MainWindow()
    window.show()
    sys.exit(app.exec())

if __name__ == "__main__":
    main()
