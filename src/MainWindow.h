#pragma once

#include <QMainWindow>

class QFileSystemModel;
class QTabWidget;
class QTreeView;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    void openFiles(const QStringList &paths);

private:
    void openFile();
    void openDirectory();
    void openFilePath(const QString &path);
    void handleTreeActivated(const QModelIndex &index);
    void closeTab(int index);

    QFileSystemModel *fileModel_ = nullptr;
    QTreeView *fileTree_ = nullptr;
    QTabWidget *tabs_ = nullptr;
};
