#ifndef MOEMAINWINDOW_H
#define MOEMAINWINDOW_H

#include <QMainWindow>
#include "directorylistmodel.h"
#include "moepreviewlabel.h"
#include "directorypopulator.h"
#include <QAudioOutput>
#include <QCompleter>
#include <QLabel>
#include <QListView>
#include <QListWidget>
#include <QProgressBar>
#include <QPushButton>
#include <QShortcut>
#include <QVideoWidget>

// ADS
#include <DockWidget.h>
#include "dockindock/dockindock.h" // QtAdsUtl::DockInDockWidget
#include "dockindock/perspectives.h"

namespace Ui {
class MoeMainWindow;
}

enum TabType
{
    QImageType,
    QMediaType
};

typedef struct TabData_S
{
    QSharedPointer<DirectoryResult> directory;
    TabType type;

    QVideoWidget* mpvWidget = nullptr;
    QMediaPlayer* mpvPlayer = nullptr;
    QAudioOutput* audioOutput = nullptr;

    MoePreviewLabel* imageWidget = nullptr;
    QMovie* imageMovie = nullptr;

    QProgressBar* progressBar = nullptr;

    ads::CDockWidget* dockWidget = nullptr;
} TabData;

class MoeMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MoeMainWindow(QWidget *parent = nullptr);
    ~MoeMainWindow();


private slots:

    void importDirectories(IndexType indexType);
    void onNetworkDeltaImport(int delta);
    void onNetworkImport();
    void onFilesystemImport();

    void selectView(int64_t index);
    void enterView(const QModelIndex& index);

    void copyContent(QSharedPointer<DirectoryResult> directory);
    void saveContentLocation(QSharedPointer<DirectoryResult> directory);
    void copyContentLocation(QSharedPointer<DirectoryResult> directory);
    void openContentLocation(QSharedPointer<DirectoryResult> directory);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent* event) override;
    void closeTab();
    void nextTab();
    void previousTab();


private:
    // TAB FUNCTIONS
    void createTab(QSharedPointer<DirectoryResult> directory, TabType type);
    std::optional<TabData> currentTab() const;
    std::optional<TabData> getTab(const QUuid uuid) const;
    std::optional<QWidget*> getTabWidget(const QUuid uuid) const;
    bool swapTab(const QUuid uuid);

    void refreshDirectoryData(QSharedPointer<DirectoryResult> directory);
    void setDirectoryPopulationProgressBytes(QUuid uuid, qint64 bytes, quint64 max);
    void setDirectoryPopulationProgress(QUuid uuid, float progress);

    QStringList getTagsFromFile();

    void increaseDirectoryRenderScale();
    void decreaseDirectoryRenderScale();
    void modifyDirectoryRenderScale(const float delta);

    void setPage(int page);

    void setTagList(QSharedPointer<DirectoryResult> directory);

    // The main container for docking
    ads::CDockManager* m_DockManager;
    QtAdsUtl::DockInDockWidget* m_DockInDockManager;
    std::unique_ptr<QtAdsUtl::PerspectivesManager> m_perspectivesManager;

    QPushButton* directoryButtonLocal = nullptr;
    QPushButton* directoryButtonNetwork = nullptr;
    QLineEdit* directorySearchEdit = nullptr;
    QWidget* navigationFooter = nullptr;

    QProgressBar* progressBar = nullptr;
    QListView* directoryListView = nullptr;
    QPushButton* directoryButtonNextPage = nullptr;
    QPushButton* directoryButtonPreviousPage = nullptr;
    QLabel* directoryCurrentPage = nullptr;

    QWidget* viewingWidgetContents = nullptr;

    DirectoryListModel * directoryListModel = nullptr;

    DirectoryIndexer directoryIndexer;
    DirectoryPopulator directoryPopulator;

    ads::CDockWidget* ViewingDockWidget;
    ads::CDockWidget* NavigationDockWidget;
    ads::CDockWidget* TagsDockWidget;

    QListWidget* tagWidgetContents = nullptr;
    QCompleter* completer = nullptr;

    QMap<QWidget*, TabData> tabData;
    std::optional<TabData> focusedTabData;

    int currentPage = 0;
    int64_t selectedDirectoryIndex = -1;
    float directoryRenderScale = 3.0;
    QShortcut* increaseRenderScaleShortcut;
    QShortcut* decreaseRenderScaleShortcut;
};

#endif // MOEMAINWINDOW_H
