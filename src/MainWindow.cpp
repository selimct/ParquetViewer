#include "MainWindow.h"

#include "FileTab.h"

#include <QAction>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QMessageBox>
#include <QSplitter>
#include <QStyle>
#include <QTabWidget>
#include <QToolBar>
#include <QTreeView>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("Parquet Viewer"));
    resize(1280, 800);

    auto *splitter = new QSplitter(this);
    setCentralWidget(splitter);

    auto *explorer = new QWidget(splitter);
    auto *explorerLayout = new QVBoxLayout(explorer);
    explorerLayout->setContentsMargins(0, 0, 0, 0);
    explorerLayout->setSpacing(0);

    auto *explorerToolbar = new QToolBar(explorer);
    explorerToolbar->setIconSize({18, 18});
    const auto openFileAction = explorerToolbar->addAction(style()->standardIcon(QStyle::SP_DialogOpenButton), tr("Open file"));
    connect(openFileAction, &QAction::triggered, this, &MainWindow::openFile);
    const auto openFolderAction = explorerToolbar->addAction(style()->standardIcon(QStyle::SP_DirOpenIcon), tr("Open folder"));
    connect(openFolderAction, &QAction::triggered, this, &MainWindow::openDirectory);
    explorerLayout->addWidget(explorerToolbar);

    fileModel_ = new QFileSystemModel(this);
    fileModel_->setRootPath(QDir::homePath());
    fileModel_->setNameFilters({QStringLiteral("*.parquet"), QStringLiteral("*.parq")});
    fileModel_->setNameFilterDisables(false);

    fileTree_ = new QTreeView(explorer);
    fileTree_->setModel(fileModel_);
    fileTree_->setRootIndex(fileModel_->index(QDir::homePath()));
    fileTree_->setHeaderHidden(false);
    fileTree_->setAnimated(true);
    fileTree_->setSortingEnabled(true);
    for (int column = 1; column < fileModel_->columnCount(); ++column) {
        fileTree_->hideColumn(column);
    }
    connect(fileTree_, &QTreeView::doubleClicked, this, &MainWindow::handleTreeActivated);
    explorerLayout->addWidget(fileTree_, 1);

    tabs_ = new QTabWidget(splitter);
    tabs_->setTabsClosable(true);
    tabs_->setMovable(true);
    connect(tabs_, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);

    splitter->addWidget(explorer);
    splitter->addWidget(tabs_);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({260, 1020});

    auto *mainToolbar = addToolBar(tr("Main"));
    mainToolbar->setIconSize({18, 18});
    mainToolbar->addAction(openFileAction);
    mainToolbar->addAction(openFolderAction);
}

void MainWindow::openFiles(const QStringList &paths)
{
    for (const QString &path : paths) {
        openFilePath(path);
    }
}

void MainWindow::openFile()
{
    const QStringList paths = QFileDialog::getOpenFileNames(
        this,
        tr("Open Parquet files"),
        QDir::homePath(),
        tr("Parquet files (*.parquet *.parq);;All files (*)"));
    openFiles(paths);
}

void MainWindow::openDirectory()
{
    const QString path = QFileDialog::getExistingDirectory(this, tr("Open folder"), QDir::homePath());
    if (path.isEmpty()) {
        return;
    }

    fileTree_->setRootIndex(fileModel_->index(path));
}

void MainWindow::openFilePath(const QString &path)
{
    QFileInfo info(path);
    if (!info.exists() || !info.isFile()) {
        return;
    }

    for (int index = 0; index < tabs_->count(); ++index) {
        auto *tab = qobject_cast<FileTab *>(tabs_->widget(index));
        if (tab != nullptr && tab->filePath() == info.absoluteFilePath()) {
            tabs_->setCurrentIndex(index);
            return;
        }
    }

    auto *tab = new FileTab(info.absoluteFilePath(), tabs_);
    QString error;
    if (!tab->load(&error)) {
        tab->deleteLater();
        QMessageBox::critical(this, tr("Could not open file"), error);
        return;
    }

    const int index = tabs_->addTab(tab, tab->displayName());
    tabs_->setTabToolTip(index, info.absoluteFilePath());
    tabs_->setCurrentIndex(index);
}

void MainWindow::handleTreeActivated(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    const QString path = fileModel_->filePath(index);
    if (QFileInfo(path).isFile()) {
        openFilePath(path);
    }
}

void MainWindow::closeTab(int index)
{
    QWidget *tab = tabs_->widget(index);
    tabs_->removeTab(index);
    tab->deleteLater();
}
