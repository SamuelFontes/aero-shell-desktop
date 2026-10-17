/* SPDX-License-Identifier: MIT */
#include "explorer.h"
#include <KIO/CopyJob>
#include <KIO/FileUndoManager>
#include <KIO/DeleteJob>
#include <KIO/JobUiDelegateFactory>
#include <KPropertiesDialog>
#include <KJob>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QToolButton>
#include <QMimeDatabase>
#include <QFileIconProvider>
#include <QScreen>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QShortcut>
#include <QSplitter>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStorageInfo>
#include <QToolBar>
#include <QTreeView>
#include <QTreeWidget>

namespace {
class AeroFileIcons final : public QFileIconProvider {
public:
    QIcon icon(const QFileInfo &file) const override {
        const QString name = file.isDir() ? QStringLiteral("folder") : QMimeDatabase().mimeTypeForFile(file).iconName();
        return QIcon::fromTheme(name, QFileIconProvider::icon(file));
    }
};
}

Explorer::Explorer(const QString &path)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName(QStringLiteral("explorer"));
    const QSize screenSize = QGuiApplication::primaryScreen() ? QGuiApplication::primaryScreen()->availableGeometry().size() : QSize(1280, 720);
    resize(qMin(1000, screenSize.width() - 80), qMin(620, screenSize.height() - 110));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("system-file-manager")));
    setStyleSheet(QStringLiteral(
        "QMainWindow {background:#edf4fc;} QToolBar {border:0; spacing:5px; padding:5px; background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #f9fcff,stop:1 #dce7f5);}"
        "QToolButton {padding:4px 8px; border:1px solid transparent; border-radius:3px;} QToolButton:hover {border-color:#9ebbd8; background:#e6f2ff;}"
        "QLineEdit {background:white; border:1px solid #8cabc8; padding:4px; color:#172b40;}"
        "QTreeView,QListView {background:white; border:0; color:#1c2938; selection-background-color:#cce8ff; selection-color:#162c44;}"
        "QTreeView::item,QListView::item {padding:3px;} QHeaderView::section {background:#fff; color:#4b6583; border:0; border-right:1px solid #e5e5e5; border-bottom:1px solid #ddd; padding:5px;}"
        "QStatusBar {background:#f1f5fa; border-top:1px solid #a8bed5;} QMenuBar {background:#f4f7fc;}"));

    auto *navigation = addToolBar(tr("Navigation"));
    navigation->setMovable(false);
    m_back = navigation->addAction(QIcon::fromTheme(QStringLiteral("go-previous")), tr("Back"), this, [this] {
        if (m_historyIndex > 0) navigate(m_history.at(--m_historyIndex), false);
    });
    m_forward = navigation->addAction(QIcon::fromTheme(QStringLiteral("go-next")), tr("Forward"), this, [this] {
        if (m_historyIndex + 1 < m_history.size()) navigate(m_history.at(++m_historyIndex), false);
    });
    navigation->addAction(QIcon::fromTheme(QStringLiteral("go-up")), tr("Up"), this, [this] { navigate(QFileInfo(m_path).absolutePath()); });
    m_address = new QLineEdit;
    m_address->setObjectName(QStringLiteral("address"));
    m_address->setAccessibleName(tr("Address"));
    m_address->setMinimumWidth(250);
    m_addressViews = new QStackedWidget;
    m_addressViews->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_addressViews->setMinimumWidth(250);
    auto *crumbs = new QWidget;
    crumbs->setStyleSheet(QStringLiteral("background:white; border:1px solid #8cabc8;"));
    m_breadcrumbs = new QHBoxLayout(crumbs);
    m_breadcrumbs->setContentsMargins(3, 0, 3, 0);
    m_breadcrumbs->setSpacing(0);
    m_addressViews->addWidget(crumbs);
    m_addressViews->addWidget(m_address);
    navigation->addWidget(m_addressViews);
    connect(m_address, &QLineEdit::returnPressed, this, [this] { navigate(m_address->text()); });
    connect(new QShortcut(QKeySequence(Qt::Key_Escape), m_address), &QShortcut::activated, this, [this] { m_addressViews->setCurrentIndex(0); view()->setFocus(); });
    navigation->addAction(QIcon::fromTheme(QStringLiteral("view-refresh")), tr("Refresh"), this, [this] { navigate(m_path, false); });
    m_search = new QLineEdit;
    m_search->setObjectName(QStringLiteral("search"));
    m_search->setAccessibleName(tr("Search current folder"));
    m_search->setMaximumWidth(230);
    navigation->addWidget(m_search);

    addToolBarBreak();
    auto *commands = addToolBar(tr("Commands"));
    commands->setMovable(false);
    auto *organizeButton = commands->addAction(tr("Organize"));
    auto *organize = new QMenu(this);
    organizeButton->setMenu(organize);
    qobject_cast<QToolButton *>(commands->widgetForAction(organizeButton))->setPopupMode(QToolButton::InstantPopup);
    auto action = [this](QMenu *menu, const QString &name, const QKeySequence &key, auto callback) {
        auto *a = menu->addAction(name, this, callback);
        a->setShortcut(key);
        addAction(a);
        return a;
    };
    action(organize, tr("Cut"), QKeySequence::Cut, [this] { copy(true); });
    action(organize, tr("Copy"), QKeySequence::Copy, [this] { copy(false); });
    action(organize, tr("Paste"), QKeySequence::Paste, [this] { paste(); });
    organize->addSeparator();
    action(organize, tr("Select all"), QKeySequence::SelectAll, [this] { view()->selectAll(); });
    action(organize, tr("Rename"), QKeySequence(Qt::Key_F2), [this] { view()->edit(view()->currentIndex()); });
    action(organize, tr("Delete"), QKeySequence::Delete, [this] { remove(false); });
    action(organize, tr("Permanently delete…"), QKeySequence(Qt::SHIFT | Qt::Key_Delete), [this] { remove(true); });
    organize->addSeparator();
    auto *undo = action(organize, tr("Undo last file operation"), QKeySequence::Undo, [this] {
        KIO::FileUndoManager::self()->uiInterface()->setParentWidget(this);
        KIO::FileUndoManager::self()->undo();
    });
    undo->setEnabled(KIO::FileUndoManager::self()->isUndoAvailable());
    connect(KIO::FileUndoManager::self(), &KIO::FileUndoManager::undoAvailable, undo, &QAction::setEnabled);
    auto *redo = action(organize, tr("Redo last file operation"), QKeySequence::Redo, [this] {
        KIO::FileUndoManager::self()->uiInterface()->setParentWidget(this);
        KIO::FileUndoManager::self()->redo();
    });
    redo->setEnabled(KIO::FileUndoManager::self()->isRedoAvailable());
    connect(KIO::FileUndoManager::self(), &KIO::FileUndoManager::redoAvailable, redo, &QAction::setEnabled);
    action(organize, tr("Properties"), QKeySequence(Qt::ALT | Qt::Key_Return), [this] { properties(); });
    commands->addAction(tr("Open"), this, &Explorer::openSelection);
    commands->addAction(tr("New folder"), this, &Explorer::newFolder);
    commands->addSeparator();
    commands->addAction(tr("Details"), this, [this] { m_views->setCurrentWidget(m_details); updateStatus(); });
    commands->addAction(tr("Large icons"), this, [this] { m_views->setCurrentWidget(m_icons); updateStatus(); });
    auto *hidden = commands->addAction(tr("Hidden items"));
    hidden->setCheckable(true);

    auto *split = new QSplitter;
    auto *places = new QTreeWidget;
    places->setObjectName(QStringLiteral("places"));
    places->setHeaderHidden(true);
    places->setMinimumWidth(155);
    places->setMaximumWidth(320);
    places->setIndentation(16);
    auto group = [places](const QString &name, const QString &icon) {
        auto *item = new QTreeWidgetItem(places, {name});
        item->setIcon(0, QIcon::fromTheme(icon));
        item->setExpanded(true);
        item->setFlags(Qt::ItemIsEnabled);
        return item;
    };
    auto place = [](QTreeWidgetItem *parent, const QString &name, const QString &icon, const QString &location) {
        auto *item = new QTreeWidgetItem(parent, {name});
        item->setIcon(0, QIcon::fromTheme(icon));
        item->setData(0, Qt::UserRole, location);
    };
    auto *favorites = group(tr("Favorites"), QStringLiteral("bookmarks"));
    place(favorites, tr("Desktop"), QStringLiteral("user-desktop"), QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    place(favorites, tr("Downloads"), QStringLiteral("folder-downloads"), QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    place(favorites, tr("Home"), QStringLiteral("user-home"), QDir::homePath());
    auto *libraries = group(tr("Libraries"), QStringLiteral("folder-documents"));
    for (const auto &[name, icon, location] : QList<std::tuple<QString, QString, QStandardPaths::StandardLocation>>{
             {tr("Documents"), QStringLiteral("folder-documents"), QStandardPaths::DocumentsLocation},
             {tr("Music"), QStringLiteral("folder-music"), QStandardPaths::MusicLocation},
             {tr("Pictures"), QStringLiteral("folder-pictures"), QStandardPaths::PicturesLocation},
             {tr("Videos"), QStringLiteral("folder-videos"), QStandardPaths::MoviesLocation}}) {
        place(libraries, name, icon, QStandardPaths::writableLocation(location));
    }
    auto *computer = group(tr("Computer"), QStringLiteral("computer"));
    place(computer, tr("Local Disk (/)"), QStringLiteral("drive-harddisk"), QStringLiteral("/"));
    for (const auto &volume : QStorageInfo::mountedVolumes()) {
        if (volume.isValid() && volume.isReady() && volume.device().startsWith("/dev/") && !volume.isRoot() && !volume.rootPath().startsWith(QStringLiteral("/proc"))
            && !volume.rootPath().startsWith(QStringLiteral("/sys")) && !volume.rootPath().startsWith(QStringLiteral("/dev"))) {
            place(computer, volume.displayName(), QStringLiteral("drive-removable-media"), volume.rootPath());
        }
    }
    connect(places, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item) {
        const auto location = item->data(0, Qt::UserRole).toString();
        if (!location.isEmpty()) navigate(location);
    });

    m_files = new QFileSystemModel(this);
    static AeroFileIcons fileIcons;
    m_files->setIconProvider(&fileIcons);
    m_files->setReadOnly(false);
    m_files->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs);
    m_files->setNameFilterDisables(false);
    m_files->setRootPath(QStringLiteral("/"));
    m_details = new QTreeView;
    m_details->setObjectName(QStringLiteral("details"));
    m_details->setRootIsDecorated(false);
    m_details->setItemsExpandable(false);
    m_details->setSortingEnabled(true);
    m_details->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_details->sortByColumn(0, Qt::AscendingOrder);
    m_icons = new QListView;
    m_icons->setViewMode(QListView::IconMode);
    m_icons->setIconSize(QSize(48, 48));
    m_icons->setGridSize(QSize(112, 92));
    m_icons->setResizeMode(QListView::Adjust);
    m_icons->setMovement(QListView::Static);
    m_icons->setWordWrap(true);
    m_views = new QStackedWidget;
    for (QAbstractItemView *v : {static_cast<QAbstractItemView *>(m_details), static_cast<QAbstractItemView *>(m_icons)}) {
        v->setModel(m_files);
        v->setSelectionMode(QAbstractItemView::ExtendedSelection);
        v->setEditTriggers(QAbstractItemView::EditKeyPressed);
        v->setDragEnabled(true);
        v->setAcceptDrops(true);
        v->setDropIndicatorShown(true);
        v->setDragDropMode(QAbstractItemView::DragDrop);
        v->setDefaultDropAction(Qt::CopyAction);
        v->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(v, &QAbstractItemView::doubleClicked, this, [this](const QModelIndex &index) {
            const QString path = m_files->filePath(index);
            if (m_files->isDir(index)) navigate(path);
            else QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        });
        connect(v->selectionModel(), &QItemSelectionModel::selectionChanged, this, &Explorer::updateStatus);
        connect(v, &QWidget::customContextMenuRequested, this, [this, v, organize](const QPoint &point) {
            QMenu menu;
            menu.addAction(tr("Open"), this, &Explorer::openSelection);
            menu.addAction(tr("New folder"), this, &Explorer::newFolder);
            menu.addSeparator();
            menu.addActions(organize->actions());
            menu.exec(v->viewport()->mapToGlobal(point));
        });
        m_views->addWidget(v);
    }
    m_details->setColumnWidth(0, 320);
    m_details->setColumnWidth(1, 100);
    m_details->setColumnWidth(2, 150);
    split->addWidget(places);
    split->addWidget(m_views);
    split->setStretchFactor(1, 1);
    split->setSizes({185, 815});
    setCentralWidget(split);
    m_status = new QLabel;
    statusBar()->addWidget(m_status, 1);
    connect(m_files, &QFileSystemModel::directoryLoaded, this, &Explorer::updateStatus);
    connect(m_search, &QLineEdit::textChanged, this, [this](const QString &text) {
        m_files->setFilter(text.isEmpty() ? m_files->filter() | QDir::AllDirs : m_files->filter() & ~QDir::AllDirs);
        m_files->setNameFilters(text.isEmpty() ? QStringList{} : QStringList{QStringLiteral("*") + text + QStringLiteral("*")});
        updateStatus();
    });
    connect(hidden, &QAction::toggled, this, [this](bool show) {
        m_files->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs | (show ? QDir::Hidden : QDir::Filters{}));
    });
    auto key = [this](const QKeySequence &sequence, auto callback) { connect(new QShortcut(sequence, this), &QShortcut::activated, this, callback); };
    key(QKeySequence(Qt::ALT | Qt::Key_Left), [this] { m_back->trigger(); });
    key(QKeySequence(Qt::ALT | Qt::Key_Right), [this] { m_forward->trigger(); });
    key(QKeySequence(Qt::ALT | Qt::Key_Up), [this] { navigate(QFileInfo(m_path).absolutePath()); });
    key(QKeySequence(Qt::CTRL | Qt::Key_L), [this] { showAddress(); });
    key(QKeySequence(Qt::ALT | Qt::Key_D), [this] { showAddress(); });
    key(QKeySequence(Qt::CTRL | Qt::Key_E), [this] { m_search->setFocus(); m_search->selectAll(); });
    key(QKeySequence(Qt::Key_F3), [this] { m_search->setFocus(); m_search->selectAll(); });
    key(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N), [this] { newFolder(); });
    key(QKeySequence(Qt::CTRL | Qt::Key_N), [this] { (new Explorer(m_path))->show(); });
    key(QKeySequence(Qt::CTRL | Qt::Key_W), [this] { close(); });
    key(QKeySequence(Qt::Key_F5), [this] { navigate(m_path, false); });
    key(QKeySequence(Qt::Key_Return), [this] { if (view()->hasFocus()) openSelection(); });
    navigate(path.isEmpty() ? QDir::homePath() : path);
}

QAbstractItemView *Explorer::view() const { return qobject_cast<QAbstractItemView *>(m_views->currentWidget()); }
void Explorer::navigate(const QString &input, bool record)
{
    QString path = input;
    if (path.startsWith(QStringLiteral("file:"))) path = QUrl(path).toLocalFile();
    if (path == QStringLiteral("~")) path = QDir::homePath();
    else if (path.startsWith(QStringLiteral("~/"))) path = QDir::homePath() + path.mid(1);
    if (QDir::isRelativePath(path)) path = QDir(m_path).absoluteFilePath(path);
    path = QDir::cleanPath(path);
    if (!QFileInfo(path).isDir() || !QFileInfo(path).isReadable()) {
        QMessageBox::warning(this, tr("Location unavailable"), tr("This folder does not exist or cannot be read:\n%1").arg(path));
        m_address->setText(m_path);
        return;
    }
    m_path = path;
    if (record && (m_historyIndex < 0 || m_history.value(m_historyIndex) != path)) {
        m_history = m_history.mid(0, m_historyIndex + 1);
        m_history.append(path);
        m_historyIndex = m_history.size() - 1;
    }
    m_search->clear();
    const auto root = m_files->index(path);
    m_details->setRootIndex(root);
    m_icons->setRootIndex(root);
    m_address->setText(path);
    while (QLayoutItem *item = m_breadcrumbs->takeAt(0)) { delete item->widget(); delete item; }
    QStringList components = path.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    components.prepend(QStringLiteral("Computer"));
    QString accumulated;
    for (int i = 0; i < components.size(); ++i) {
        accumulated = i == 0 ? QStringLiteral("/") : QDir(accumulated).filePath(components.at(i));
        auto *button = new QToolButton;
        button->setText(components.at(i));
        button->setAutoRaise(true);
        button->setStyleSheet(QStringLiteral("QToolButton {border:0; background:transparent; padding:3px;} QToolButton:hover {background:#e0efff;}"));
        const QString location = accumulated;
        connect(button, &QToolButton::clicked, this, [this, location] { navigate(location); });
        m_breadcrumbs->addWidget(button);
        if (i + 1 < components.size()) {
            auto *arrow = new QLabel(QStringLiteral("›"));
            arrow->setStyleSheet(QStringLiteral("border:0; color:#67819d;"));
            m_breadcrumbs->addWidget(arrow);
        }
    }
    auto *editAddress = new QToolButton;
    editAddress->setText(QStringLiteral("▾"));
    editAddress->setToolTip(tr("Edit address (Alt+D)"));
    editAddress->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    editAddress->setStyleSheet(QStringLiteral("border:0; text-align:right;"));
    connect(editAddress, &QToolButton::clicked, this, &Explorer::showAddress);
    m_breadcrumbs->addWidget(editAddress, 1);
    m_addressViews->setCurrentIndex(0);
    m_search->setPlaceholderText(tr("Search %1").arg(QFileInfo(path).fileName().isEmpty() ? tr("Computer") : QFileInfo(path).fileName()));
    setWindowTitle(tr("%1 — Windows Explorer").arg(path == QStringLiteral("/") ? tr("Computer") : QDir(path).dirName()));
    m_back->setEnabled(m_historyIndex > 0);
    m_forward->setEnabled(m_historyIndex + 1 < m_history.size());
    updateStatus();
}
QList<QUrl> Explorer::selection() const
{
    QList<QUrl> result;
    for (const auto &index : view()->selectionModel()->selectedIndexes()) {
        if (index.column() == 0) result.append(QUrl::fromLocalFile(m_files->filePath(index)));
    }
    // QListView only has column zero; selectedRows also works there.
    return result;
}
void Explorer::copy(bool cut)
{
    const auto urls = selection();
    if (urls.isEmpty()) return;
    auto *mime = new QMimeData;
    mime->setUrls(urls);
    mime->setData(QStringLiteral("application/x-kde-cutselection"), cut ? "1" : "0");
    QApplication::clipboard()->setMimeData(mime);
}
void Explorer::watchJob(KJob *job)
{
    if (auto *copy = qobject_cast<KIO::CopyJob *>(job)) KIO::FileUndoManager::self()->recordCopyJob(copy);
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
    connect(job, &KJob::result, this, [this, job] { if (!job->error()) updateStatus(); });
}
void Explorer::paste()
{
    const auto *mime = QApplication::clipboard()->mimeData();
    if (!mime->hasUrls()) return;
    const auto urls = mime->urls();
    const bool cut = mime->data(QStringLiteral("application/x-kde-cutselection")) == "1";
    auto *job = cut ? KIO::move(urls, QUrl::fromLocalFile(m_path)) : KIO::copy(urls, QUrl::fromLocalFile(m_path));
    watchJob(job);
    if (cut) connect(job, &KJob::result, this, [job, urls] {
        const auto *current = QApplication::clipboard()->mimeData();
        if (!job->error() && current->urls() == urls && current->data(QStringLiteral("application/x-kde-cutselection")) == "1") QApplication::clipboard()->clear();
    });
}
void Explorer::remove(bool permanent)
{
    const auto urls = selection();
    if (urls.isEmpty()) return;
    if (QMessageBox::question(this, permanent ? tr("Permanently delete") : tr("Move to Recycle Bin"),
        permanent ? tr("Permanently delete %1 selected item(s)? This cannot be undone.").arg(urls.size())
                  : tr("Move %1 selected item(s) to the Recycle Bin?").arg(urls.size()), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) return;
    watchJob(permanent ? static_cast<KJob *>(KIO::del(urls)) : static_cast<KJob *>(KIO::trash(urls)));
}
void Explorer::newFolder()
{
    bool ok = false;
    const auto name = QInputDialog::getText(this, tr("New folder"), tr("Folder name:"), QLineEdit::Normal, tr("New folder"), &ok);
    if (!ok || name.isEmpty()) return;
    if (name == QStringLiteral(".") || name == QStringLiteral("..") || name.contains(QLatin1Char('/')) || !QDir(m_path).mkdir(name)) {
        QMessageBox::warning(this, tr("Could not create folder"), tr("Choose an unused folder name without a slash."));
    }
}
void Explorer::properties()
{
    const auto urls = selection();
    auto *dialog = new KPropertiesDialog(urls.isEmpty() ? QUrl::fromLocalFile(m_path) : urls.first(), this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}
void Explorer::openSelection()
{
    for (const auto &url : selection()) {
        if (QFileInfo(url.toLocalFile()).isDir()) { navigate(url.toLocalFile()); break; }
        QDesktopServices::openUrl(url);
    }
}
void Explorer::updateStatus()
{
    const int selected = selection().size();
    m_status->setText(selected ? tr("%1 item(s) selected").arg(selected) : tr("%1 items").arg(m_files->rowCount(view()->rootIndex())));
}

void Explorer::showAddress()
{
    m_addressViews->setCurrentIndex(1);
    m_address->setFocus();
    m_address->selectAll();
}
