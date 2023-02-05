#include "moemainwindow.h"
#include <QStyleFactory>
#include <QStandardItemModel>
#include <QListWidgetItem>
#include <QtConcurrent/QtConcurrent>
#include <ffmpegthumbs/ffmpegthumbnailer.h>
#include "Qt-Advanced-Docking-System/examples/dockindock/dockindockmanager.h"
#include "directorylistmodel.h"
#include "directoryindexer.h"
#include "moeglobals.h"
#include <QResizeEvent>
#include "qapplication.h"
#include <QMenu>
#include <QClipboard>
#include <QVBoxLayout>
#include <QDockWidget>
#include <QPushButton>
#include <QScrollBar>
#include <QMovie>
#include <QLineEdit>
#include <QList>
#include "delimitedcompleter.h"
#include <QMediaPlayer>
#include <QMenuBar>
#include <QMessageBox>
#include <QButtonGroup>
#include <QDesktopServices>
#include <Qt-Advanced-Docking-System/src/DockAreaTitleBar.h>
#include <QGroupBox>
#include <QComboBox>
#include <DockAreaWidget.h>
#include "moepreviewlabel.h"
#include "taglistitem.h"

MoeMainWindow::MoeMainWindow(QWidget *parent) : QMainWindow(parent)
{
    // Set properties for the window itself
    if (objectName().isEmpty())
        setObjectName("MoeMainWindow");
    setContextMenuPolicy(Qt::DefaultContextMenu);
    setTabShape(QTabWidget::Triangular);
    resize(1025, 530);
    setMinimumSize(QSize(260, 160));

    setWindowTitle(QString("MoeTag"));

    setAcceptDrops(true);
    setEnabled(true);

    // Contents
    viewingWidgetContents = new QWidget();
    QVBoxLayout* viewingLayout = new QVBoxLayout(viewingWidgetContents);
    viewingWidgetContents->setLayout(viewingLayout);
    QTabWidget* navigationWidgetContents = new QTabWidget();
    QVBoxLayout* navigationLayout = new QVBoxLayout(navigationWidgetContents);
    navigationWidgetContents->setLayout(navigationLayout);

    // Primary list view properties
    directoryListView = new QListView(navigationWidgetContents);
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(directoryListView->sizePolicy().hasHeightForWidth());

    directoryListView->setSizePolicy(sizePolicy);
    directoryListView->setAutoScroll(true);
    directoryListView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    directoryListView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    directoryListView->verticalScrollBar()->setSingleStep(directoryListView->verticalScrollBar()->pageStep() * 2);
    directoryListView->setResizeMode(QListView::ResizeMode::Adjust);
    directoryListView->setLayoutMode(QListView::LayoutMode::Batched);
    directoryListView->setViewMode(QListView::ViewMode::IconMode);
    directoryListView->setUniformItemSizes(true);
    directoryListView->setItemAlignment(Qt::AlignJustify);
    directoryListView->setIconSize(QSize(g_thumbWidth / directoryRenderScale, g_thumbHeight / directoryRenderScale));
    directoryListView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    //directoryButtonLocal = new QPushButton(navigationWidgetContents);
    //directoryButtonLocal->setText(tr("Import Content (FS)"));

    QWidget* groupBox = new QWidget(navigationWidgetContents);

    QHBoxLayout * groupBoxHorizontal = new QHBoxLayout(groupBox);
    QSizePolicy sizePolicyGroupBox(QSizePolicy::Preferred, QSizePolicy::Maximum);
    sizePolicyGroupBox.setHorizontalStretch(0);
    sizePolicyGroupBox.setVerticalStretch(0);
    groupBox->setSizePolicy(sizePolicyGroupBox);

    // DIRECTORY BUTTON
    directoryButtonNetwork = new QPushButton(groupBox);
    directoryButtonNetwork->setText(tr("Search Content (API)"));
    // CONFIG BUTTON
    QComboBox* networkConfiguration = new QComboBox(groupBox);
    //networkConfiguration->setText(tr("Config"));
    networkConfiguration->addItem(tr("Danbooru"));
    networkConfiguration->addItem(tr("Gelbooru"));
    networkConfiguration->addItem(tr("Safebooru"));

    groupBox->setLayout(groupBoxHorizontal);
    groupBoxHorizontal->addWidget(directoryButtonNetwork);
    groupBoxHorizontal->addWidget(networkConfiguration);

    tagWidgetContents = new QListWidget();
    TagsDockWidget = new ads::CDockWidget(tr("Tags"));
    TagsDockWidget->setWidget(tagWidgetContents);

    connect(tagWidgetContents, &QListWidget::itemDoubleClicked, this, [=](QListWidgetItem* item) {
        directorySearchEdit->setText(item->text());
        onNetworkImport();
    });

    directorySearchEdit = new QLineEdit(navigationWidgetContents);

    // Build completer
    completer = new DelimitedCompleter(directorySearchEdit, ' ', getTagsFromFile());
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    directorySearchEdit->setPlaceholderText("enter search query (i.e idol, red_hair)");

    navigationFooter = new QWidget(navigationWidgetContents);
    QHBoxLayout* navigationFooterLayout = new QHBoxLayout(navigationFooter);
    QSizePolicy sizePolicyFooter(QSizePolicy::Preferred, QSizePolicy::Maximum);
    sizePolicyFooter.setHorizontalStretch(0);
    sizePolicyFooter.setVerticalStretch(0);
    navigationFooter->setSizePolicy(sizePolicyFooter);
    navigationFooter->setLayout(navigationFooterLayout);

    progressBar = new QProgressBar(navigationFooter);
    progressBar->setFormat(QString(tr("%p%"))); //  Loaded (#%v of %m)

    directoryButtonNextPage = new QPushButton(navigationFooter);
    directoryButtonNextPage->setText(tr("Next"));
    directoryButtonNextPage->setMinimumWidth(70);
    directoryButtonPreviousPage = new QPushButton(navigationFooter);
    directoryButtonPreviousPage->setText(tr("Previous"));
    directoryButtonPreviousPage->setMinimumWidth(70);
    directoryCurrentPage = new QLabel(navigationFooter);
    setPage(0);

    //navigationLayout->addWidget(directoryButtonLocal);
    navigationLayout->addWidget(groupBox);
    navigationLayout->addWidget(directorySearchEdit);
    navigationLayout->addWidget(directoryListView);
    navigationLayout->addWidget(navigationFooter);

    navigationFooterLayout->addWidget(directoryCurrentPage);
    navigationFooterLayout->addWidget(progressBar);
    navigationFooterLayout->addWidget(directoryButtonPreviousPage);
    navigationFooterLayout->addWidget(directoryButtonNextPage);

    connect(directoryButtonNextPage, &QPushButton::clicked, this, [&]() {
        onNetworkDeltaImport(1);
    });
    connect(directoryButtonPreviousPage, &QPushButton::clicked, this, [&]() {
        onNetworkDeltaImport(-1);
    });

    setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::TabPosition::West);

    //connect(directoryButtonLocal, &QPushButton::released, this, &MoeMainWindow::onFilesystemImport);
    connect(directoryButtonNetwork,  &QPushButton::released, this, &MoeMainWindow::onNetworkImport);

    connect(directoryListView, &QListView::activated, this, &MoeMainWindow::enterView);

    connect(directorySearchEdit, &QLineEdit::returnPressed, this, &MoeMainWindow::onNetworkImport);

    directoryListModel = new DirectoryListModel(this);
    directoryListView->setModel(directoryListModel);

    directoryListView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(directoryListView, &QListView::customContextMenuRequested, this, [=](QPoint pos) {
        QMenu contextMenu(tr("Context menu"), this);

        if(directoryListView->selectionModel()->hasSelection()) {
            QModelIndexList selected = directoryListView->selectionModel()->selectedIndexes();
            auto construct_suffix = [=]() {
                QString constructedMoeTagLabel(""); 
                int selectedSize = selected.size();
                if (selectedSize > 1)
                    constructedMoeTagLabel += QString(" (x ") + QString::number(selected.size()) + QString(")");
                return constructedMoeTagLabel;
            };
            contextMenu.addAction(QString("Open in MoeTag") + construct_suffix(), this, [=]() {
                for(const QModelIndex& index : selected) {
                    selectView(index.row());
                }
			});
            contextMenu.addAction(QString("Open in Browser") + construct_suffix(), this, [=]() {
				for (const QModelIndex& index : selected) {
					QSharedPointer<DirectoryResult> result = directoryListModel->getDirectory(index.row());
					if (result == nullptr)
					{
						qWarning() << "error: openInBrowser() result is nullptr";
						return;
					}
                    openContentLocation(result);
				}
			});
        }

        contextMenu.exec(directoryListView->mapToGlobal(pos));
    });

    connect(&directoryPopulator, &DirectoryPopulator::finishedResult, this, [&](DirectoryResult result, PopulateType type) {
        if(type == PopulateType::PREVIEW) {
            for(const QSharedPointer<DirectoryResult> directory : directoryListModel->getDirectories())
            {
                if(directory->thumbnailSource == result.thumbnailSource) {
                    // delete thumbnail if it exists
                    if(!result.thumbnail.isNull()) {
                        directory->thumbnail = result.thumbnail;
                        directoryListModel->formatThumbnail(directory->thumbnail);
                        directoryListModel->invalidateCache(directory);
                        emit directoryListModel->layoutChanged();
                    }
                    return;
                }
            }
            qWarning() << "warning: failed to match populator on directory-preview";
        }
        if(type == PopulateType::FULL) {
            for(const QSharedPointer<DirectoryResult>& directory : directoryListModel->getDirectories())
            {
                if(directory->contentSource == result.contentSource) {
                    // delete contentImage if it exists
                    directory->contentImage = result.contentImage;
                    directory->contentGif = result.contentGif;

                    refreshDirectoryData(directory);
                    return;
                }
            }
            qWarning() << "warning: failed to match populator on directory-full";
        }
    });

    connect(&directoryIndexer, &DirectoryIndexer::onCurrentlyIndexingChange, [=](bool indexing) {
        directoryButtonNetwork->setEnabled(!indexing);
        directoryButtonNextPage->setEnabled(!indexing);
        directoryButtonPreviousPage->setEnabled(!indexing);
    });

    connect(&directoryPopulator, &DirectoryPopulator::previewProgress, this, [&](int progress, int max) {
        if(progress == max)
        {
            progressBar->reset();
        } else {
            progressBar->setMaximum(max);
            progressBar->setMinimum(0);
            progressBar->setValue(progress);
        }
    });

    connect(&directoryPopulator, &DirectoryPopulator::fullProgress, this, [&](QUuid uuid, qint64 bytes, quint64 max) {
        setDirectoryPopulationProgressBytes(uuid, bytes, max);
    });

    connect(&directoryIndexer, &DirectoryIndexer::finished, this, [=](QList<DirectoryResult> indexDirectories, IndexType type) {
        // Add new directories
        for(DirectoryResult& directory : indexDirectories)
            directoryListModel->addDirectory(directory);

        // Update view
        emit directoryListModel->layoutChanged();

        // Populate directory previews
        directoryPopulator.populateDirectory(indexDirectories, type, PopulateType::PREVIEW);
    });

    increaseRenderScaleShortcut = new QShortcut(QKeySequence(tr("ctrl+q", "Increase Render Scale")), this);
    decreaseRenderScaleShortcut = new QShortcut(QKeySequence(tr("ctrl+e", "Decrease Render Scale")), this);

    QShortcut* previousContentShortcut = new QShortcut(QKeySequence(tr("ctrl+a", "Previous Content")), this);
	QShortcut* nextContentShortcut = new QShortcut(QKeySequence(tr("ctrl+d", "Next Content")), this);
    QShortcut* closeContentShortcut = new QShortcut(QKeySequence(tr("ctrl+x", "Close Content")), this);

    connect(increaseRenderScaleShortcut, &QShortcut::activated, this, &MoeMainWindow::increaseDirectoryRenderScale);
    connect(decreaseRenderScaleShortcut, &QShortcut::activated, this, &MoeMainWindow::decreaseDirectoryRenderScale);
    connect(previousContentShortcut, &QShortcut::activated, this, &MoeMainWindow::previousTab);
    connect(nextContentShortcut, &QShortcut::activated, this, &MoeMainWindow::nextTab);
    connect(closeContentShortcut, &QShortcut::activated, this, &MoeMainWindow::closeTab);

    ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaDynamicTabsMenuButtonVisibility);
    ads::CDockManager::setConfigFlag(ads::CDockManager::DefaultOpaqueConfig);
    ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting);
    ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaCloseButtonClosesTab);
    m_DockManager = new ads::CDockManager(this);
    m_DockManager->setStyleSheet(""); // needed to remove default styling from dockmanager.

    // Create a dock widget with the title Label 1 and set the created label
    // as the dock widget content
    NavigationDockWidget = new ads::CDockWidget("Navigation");
    NavigationDockWidget->setWidget(navigationWidgetContents);
    NavigationDockWidget->setMinimumWidth(450);

    ViewingDockWidget = new ads::CDockWidget("Viewing");

    m_DockManager->setCentralWidget(ViewingDockWidget);
    m_DockManager->addDockWidget(ads::LeftDockWidgetArea, TagsDockWidget);
    m_DockManager->addDockWidget(ads::RightDockWidgetArea, NavigationDockWidget);

    m_DockInDockManager = new QtAdsUtl::DockInDockWidget(this,true,m_perspectivesManager.get());
    m_DockInDockManager->setStyleSheet("");
    m_DockInDockManager->getManager()->setStyleSheet("");
    ViewingDockWidget->setWidget(m_DockInDockManager);

    QtAdsUtl::DockInDockManager* didm = m_DockInDockManager->getManager();
    connect(didm, &QtAdsUtl::DockInDockManager::focusedDockWidgetChanged, this, [&](ads::CDockWidget* old, ads::CDockWidget* now) {
        if (now != nullptr && now->widget() != nullptr) {
            if (tabData.contains(now->widget())) {
                TabData data = tabData[now->widget()];
                focusedTabData = data;
                setTagList(data.directory);

                if (data.type == TabType::QMediaType) {
                    QtConcurrent::run([=] {
                        data.mpvPlayer->play();
                    });
                }

				if (old != nullptr && old->widget() != nullptr)
				{
					if (tabData.contains(old->widget()))
					{
						TabData data = tabData[old->widget()];
						if (data.type == TabType::QMediaType) {
							QtConcurrent::run([=] {data.mpvPlayer->pause(); });
						}
					}
				}
            }
        }
    });

    connect(didm, &QtAdsUtl::DockInDockManager::dockWidgetAboutToBeRemoved, this, [&](ads::CDockWidget* widget) {
        if (widget != nullptr && widget->widget() != nullptr) {

            if (tabData.contains(widget->widget())) {
                TabData data = tabData[widget->widget()];

                if (data.mpvPlayer != nullptr) {
                    data.mpvPlayer->stop();
                    data.mpvPlayer->disconnect();
                    delete data.mpvPlayer;
                    data.mpvPlayer = nullptr;
                }
                if (data.audioOutput != nullptr) {
                    data.audioOutput->disconnect();
                    delete data.audioOutput;
                    data.audioOutput = nullptr;
                }
                if (data.mpvWidget != nullptr) {
                    data.mpvWidget->disconnect();
                    delete data.mpvWidget;
                    data.mpvWidget = nullptr;
                }
                if (data.directory != nullptr) {
                    directoryListModel->deleteDirectoryData(*data.directory);
                    data.directory = nullptr;
                }
            }
        }
    });


    // File Actions
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction("Open File(s)", [=]() { onFilesystemImport(); });

    // View Actions
    QMenu* windowMenu = menuBar()->addMenu(tr("&View"));
    windowMenu->addAction(NavigationDockWidget->toggleViewAction());
    windowMenu->addAction(ViewingDockWidget->toggleViewAction());
    windowMenu->addAction(TagsDockWidget->toggleViewAction());
    windowMenu->addSeparator();
    QAction* renderScaleActionPlus = windowMenu->addAction(tr("Increase Render Scale"), [=]() { this->increaseDirectoryRenderScale(); });
    renderScaleActionPlus->setShortcut(QKeySequence(tr("ctrl+=", "Increase Render Scale")));
    QAction* renderScaleActionMinus = windowMenu->addAction(tr("Decrease Render Scale"), [=]() { this->decreaseDirectoryRenderScale(); });
    renderScaleActionMinus->setShortcut(QKeySequence(tr("ctrl+-", "Decrease Render Scale")));

    // Help Actions
    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
    QAction* aboutMoeTagAction = helpMenu->addAction(tr("About MoeTag"), [=]() {

        QDialog* dialog = new QDialog(this);
        QVBoxLayout* boxLayout = new QVBoxLayout(dialog);
        QLabel* information = new QLabel(dialog);
        dialog->setWindowTitle("About MoeTag");
        // TODO: maybe read from a file?
        information->setText(
            QString("<b>About MoeTag</b><hr><br>") +
            QString("MoeTag is open source and licensed under the ") +
            QString("<a href = \"https://www.gnu.org/licenses/gpl-3.0.en.html\">") +
            QString("The GNU General Public License v3.0.</a><br><br>") +
            QString("<i>@The Qt Company (QT 6.4.2), @FFmpeg, @KDE (FFmpegThumbs, Breeze), @githubuser0xFFFF, @MoeTag</i>"));
        information->setTextFormat(Qt::RichText);
        information->setTextInteractionFlags(Qt::TextBrowserInteraction);
        information->setOpenExternalLinks(true);
        boxLayout->addWidget(information);
        dialog->setLayout(boxLayout);
        dialog->setModal(true);
        dialog->exec();
    });

    menuBar()->show();

    QMessageBox msgBox;
    msgBox.setText("MoeTag is Alpha Software. Bugs and crashes are to be expected.");
    msgBox.exec();
}

void MoeMainWindow::importDirectories(IndexType indexType)
{
    emit directoryListModel->resetDirectories();

    setPage(0);
    directoryIndexer.indexDirectory(this, this->directorySearchEdit->text(), std::nullopt, indexType);
}

void MoeMainWindow::onNetworkDeltaImport(int delta)
{
    emit directoryListModel->resetDirectories();

    setPage(currentPage + delta);
    directoryIndexer.indexDirectory(this, this->directorySearchEdit->text(), currentPage, IndexType::NETWORKAPI);
}

void MoeMainWindow::onNetworkImport()
{
    importDirectories(IndexType::NETWORKAPI);
}

void MoeMainWindow::onFilesystemImport()
{
    importDirectories(IndexType::FILESYSTEM);
}

void MoeMainWindow::selectView(int64_t index)
{
    selectedDirectoryIndex = index;

    QSharedPointer<DirectoryResult> result = directoryListModel->getDirectory(index);
    if(result == nullptr)
    {
        qWarning() << "error: selectView() result is nullptr";
        return;
    }

    TabType type = TabType::QImageType;
    if(result->contentFileType == FileType::VIDEO) {
        type = TabType::QMediaType;
    }

    if(!getTab(result->uuid).has_value()) {
        createTab(result, type);
    } else {
        swapTab(result->uuid);
    }

    if(type == TabType::QImageType) {
        if(result->contentImage.isNull())
        {
            directoryPopulator.populateDirectory(QList<DirectoryResult>{*result}, result->indexType, PopulateType::FULL);
        }
    }

    refreshDirectoryData(result);
}

void MoeMainWindow::enterView(const QModelIndex &index)
{
    selectView((int64_t)index.row());
}

void MoeMainWindow::copyContent(QSharedPointer<DirectoryResult> directory)
{
    if(directory.isNull()) {
        qWarning() << "warning: could not copy null directory to clipboard";
        return;
    }
    if (directory->contentImage.isNull()) {
        qWarning() << "warning: could not copy null image to clipboard";
    }

	if (directory->contentFileType == FileType::IMAGE) {
		QApplication::clipboard()->setPixmap(directory->contentImage, QClipboard::Mode::Clipboard);
    }
    else if (directory->contentFileType == FileType::GIF) {
		QMimeData* mimeData = new QMimeData;
        QString rawHtml = QString("<img src=\"") + directory->contentSource + QString("\" alt=\"") + directory->contentSource + QString("\" class=\"transparent\">");
        mimeData->setData("text/html", rawHtml.toUtf8());
        mimeData->setImageData(directory->contentImage);
        mimeData->setText(directory->contentSource);
		QApplication::clipboard()->setMimeData(mimeData, QClipboard::Mode::Clipboard);
        qWarning() << "warning: tried to copy GIF; this is not fully supported as it requires OS-dependent temp files e.g. CF_HDROP on windows";
    } else {
        qWarning() << "warning: tried to copy unsupported format";
    }

}

void MoeMainWindow::saveContentLocation(QSharedPointer<DirectoryResult> directory)
{
	if (directory.isNull()) {
		qWarning() << "warning: could not save null directory";
		return;
	}
	if (directory->contentImage.isNull()) {
		qWarning() << "warning: could not save null image";
	}

    if (directory->contentFileType == FileType::IMAGE) {
        const QUrl url = QFileDialog::getSaveFileUrl(this, tr("Save Image"), QUrl(directory->contentSource).fileName());
        if (url.isValid()) {
            directory->contentImage.save(url.toLocalFile());
        } else {
            qWarning() << "warning: was given invalid file url for save content location";
        }
    }
    else {
        qWarning() << "warning: tried to save unsupported format";
    }
}

void MoeMainWindow::copyContentLocation(QSharedPointer<DirectoryResult> directory)
{
	if (directory.isNull()) {
        qWarning() << "warning: could not copy content location due to null directory";
		return;
	}
    if (QApplication::clipboard() != nullptr)
        QApplication::clipboard()->setText(directory->contentSource);
}

void MoeMainWindow::openContentLocation(QSharedPointer<DirectoryResult> directory)
{
	if (directory.isNull()) {
		qWarning() << "warning: could not open content location due to null directory";
		return;
	}

    QUrl directoryUrl(directory->contentSource);
    if (directoryUrl.isValid()) {
        QDesktopServices::openUrl(directoryUrl);
    } else {
        qWarning() << "warning: invalid directory url for open content location";
    }
}

void MoeMainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if(event->mimeData()->hasUrls())
        event->acceptProposedAction();
    if(event->mimeData()->hasText())
        event->acceptProposedAction();
    else event->ignore();
}

void MoeMainWindow::dropEvent(QDropEvent *event)
{
    if(event->mimeData() != nullptr) {
        if(event->mimeData()->hasUrls())
        {
            QList<QUrl> urls = event->mimeData()->urls();

            QList<DirectoryResult> results;

            for(QUrl& url : urls)
            {
                DirectoryResult result;
                result.contentSource = url.toString();
                result.thumbnailSource = result.contentSource;
                directoryIndexer.applyFileTypeInformation(&result);
                result.uuid = QUuid::createUuid();
                directoryListModel->addDirectory(result);
                results.append(result);
            }

            // Update view
            emit directoryListModel->layoutChanged();

            // Populate directory previews
            directoryPopulator.populateDirectory(results, IndexType::NETWORKAPI, PopulateType::PREVIEW);
            // Populate directory full

            //directoryPopulator.populateDirectory(results, IndexType::NETWORKAPI, PopulateType::FULL);
        }
        else if(event->mimeData()->hasText())
        {
            QString searchText = event->mimeData()->text();
            this->directorySearchEdit->setText(searchText);
            this->onNetworkImport();
        }

		QList<DirectoryResult> directoryResults;
		// Add some DirectoryResult objects to the list...

		DirectoryResult search;
		// Set the values of the search object...
    }
}

void MoeMainWindow::nextTab()
{
	auto widgets = m_DockInDockManager->getManager()->getWidgetsInGUIOrder();
	bool f{};
	for (auto widget : widgets)
	{
		if (f)
		{
			widget->setAsCurrentTab();
			break;
		}
		else if (widget->isCurrentTab())
		{
			f = true;
		}
	}
}

void MoeMainWindow::closeTab()
{
    ads::CDockWidget* widget = m_DockInDockManager->getManager()->focusedDockWidget();
    if (widget != nullptr)
    {
        m_DockInDockManager->getManager()->removeDockWidget(widget);
    }
}

void MoeMainWindow::previousTab()
{
	auto widgets = m_DockInDockManager->getManager()->getWidgetsInGUIOrder();
	std::reverse(std::begin(widgets), std::end(widgets));
	bool f{};
	for (auto widget : widgets)
	{
		if (f)
		{
			widget->setAsCurrentTab();
			break;
		}
		else if (widget->isCurrentTab())
		{
			f = true;
		}
	}
}

void MoeMainWindow::createTab(QSharedPointer<DirectoryResult> directory, TabType type)
{
    QWidget* tabWidget = new QWidget(viewingWidgetContents);
    QVBoxLayout* tabWidgetLayout = new QVBoxLayout(tabWidget);
    tabWidget->setLayout(tabWidgetLayout);

    QProgressBar* progressBar = new QProgressBar(viewingWidgetContents);
    progressBar->setFormat(QString(tr("%p%")));

    TabData data;
    data.directory = directory;
    data.type = type;
    data.progressBar = progressBar;

    if(type == TabType::QMediaType)
    {
        // TODO: Attempted to call isFormatSupported() without a window set
        QMediaPlayer* player = new QMediaPlayer(tabWidget);
        player->setSource(QUrl(data.directory->contentSource));
        player->setLoops(QMediaPlayer::Infinite);

		connect(player, &QMediaPlayer::bufferProgressChanged, [=](float progress) {
			if (data.directory != nullptr) {
				qDebug() << "setting video buffer to: " << progress;
				setDirectoryPopulationProgress(data.directory->uuid, progress);
			}
		});

        QAudioOutput* audioOutput = new QAudioOutput(tabWidget);
        player->setAudioOutput(audioOutput);

        data.mpvWidget = new QVideoWidget(tabWidget);
        player->setVideoOutput(data.mpvWidget);

        data.mpvWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(data.mpvWidget, &QVideoWidget::customContextMenuRequested, this, [=](QPoint pos) {
            QMenu contextMenu(tr("Context menu"), this);

            contextMenu.addAction(tr("Copy Video Location"), this, [=]() { copyContentLocation(directory); });
            contextMenu.addAction(tr("Open Video Location"), this, [=]() { openContentLocation(directory); });

            contextMenu.exec(data.mpvWidget->mapToGlobal(pos));
        });

        tabWidgetLayout->addWidget(data.mpvWidget, 2);

        data.mpvWidget->show();
        data.mpvPlayer = player;
        data.mpvPlayer->play();
    }
    else if(type == TabType::QImageType)
    {
        data.imageWidget = new MoePreviewLabel(tabWidget);
        QSizePolicy sizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(data.imageWidget->sizePolicy().hasHeightForWidth());
        data.imageWidget->setSizePolicy(sizePolicy);
        data.imageWidget->setMinimumSize(QSize(1, 1));
        data.imageWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        data.imageWidget->setScaledContents(false);
        data.imageWidget->setAlignment(Qt::AlignCenter);

        connect(data.imageWidget, &MoePreviewLabel::customContextMenuRequested, this, [=](QPoint pos) {
            QMenu contextMenu(tr("Context menu"), this);

            contextMenu.addAction(tr("Copy Image"), this, [=]() { copyContent(directory); });
            contextMenu.addAction(tr("Save Image"), this, [=]() { saveContentLocation(directory); });
			contextMenu.addAction(tr("Copy Image Location"), this, [=]() { copyContentLocation(directory); });
			contextMenu.addAction(tr("Open Image Location"), this, [=]() { openContentLocation(directory); });

            contextMenu.exec(data.imageWidget->mapToGlobal(pos));
        });

        tabWidgetLayout->addWidget(data.imageWidget);
    }

    tabWidgetLayout->addWidget(progressBar);

    ads::CDockAreaWidget* cdaw =  m_DockInDockManager->addTabWidget(tabWidget, data.directory->contentSource, nullptr);
    data.dockWidget = cdaw->currentDockWidget();
    data.dockWidget->setFeature(ads::CDockWidget::DockWidgetDeleteOnClose, true);
    data.dockWidget->setFeature(ads::CDockWidget::DockWidgetForceCloseWithArea, true);

    // Tab data
    focusedTabData = data;
    setTagList(directory);
    tabData.insert(tabWidget, data);
}

std::optional<TabData> MoeMainWindow::currentTab() const
{
    return focusedTabData;
}

std::optional<TabData> MoeMainWindow::getTab(const QUuid uuid) const
{
    for(int i = 0; i < tabData.count(); i++) {
        QWidget* widget = tabData.keys().at(i);
        TabData data = tabData[widget];
        QSharedPointer<DirectoryResult> directory = data.directory;
        if(directory.isNull())
        {
            return std::nullopt;
        }

        if(directory->uuid == uuid)
        {
            return data;
        }
    }
    return std::nullopt;
}

std::optional<QWidget*> MoeMainWindow::getTabWidget(const QUuid uuid) const
{
    for(int i = 0; i < tabData.count(); i++) {
        QWidget* widget = tabData.keys().at(i);
        TabData data = tabData[widget];
        QSharedPointer<DirectoryResult> directory = data.directory;
        if(directory->uuid == uuid)
        {
            return widget;
        }
    }
    return std::nullopt;
}

bool MoeMainWindow::swapTab(const QUuid uuid)
{
    std::optional<TabData> widgetOptional = getTab(uuid);
    if(widgetOptional.has_value()) {
        if(ViewingDockWidget->isTabbed()) {
            ViewingDockWidget->setAsCurrentTab();
        }
        if(widgetOptional->dockWidget != nullptr) {
            widgetOptional->dockWidget->setAsCurrentTab();
            widgetOptional->dockWidget->focusWidget();
            focusedTabData = widgetOptional;
            setTagList(widgetOptional->directory);
        }
        return true;
    }
    return false;
}

void MoeMainWindow::refreshDirectoryData(QSharedPointer<DirectoryResult> directory)
{
    if(directory != nullptr && !directory->contentImage.isNull())
    {
        std::optional<TabData> data = getTab(directory->uuid);
        if(data.has_value()) {
            if(data.value().type == TabType::QImageType) {
                if(directory->contentFileType == FileType::IMAGE) {
                    data.value().imageWidget->setInternalPixmap(directory->contentImage);
                    data.value().imageWidget->repaint();
                } else if(directory->contentFileType == FileType::GIF) {
                    directory->contentGif->open(QBuffer::ReadOnly);
                    data.value().imageMovie = new QMovie(directory->contentGif);
                    if(data.value().imageMovie->isValid())
                    {
                        connect(data.value().imageMovie, &QMovie::frameChanged, this, [=](int frameNumber) {
                            if(data.has_value()) {
                                data.value().imageWidget->setPixmapScaled(data.value().imageMovie->currentPixmap());
                            }
                        });
                    } else {
                        qWarning() << "Invalid QMovie Error: " << data.value().imageMovie->lastErrorString();
                    }
                    data.value().imageWidget->setMovie(data.value().imageMovie);
                    data.value().imageMovie->start();
                }
            }
        }
    }
    else {
        qWarning() << "warning: refreshDirectoryData() failed due to nullptr directory";
    }
}

void MoeMainWindow::setDirectoryPopulationProgressBytes(QUuid uuid, qint64 bytes, quint64 max)
{
    float progress = 0.0f;
    if (max > 0.0f) {
        progress = bytes / static_cast<float>(max);
    }
    setDirectoryPopulationProgress(uuid, progress);
}

void MoeMainWindow::setDirectoryPopulationProgress(QUuid uuid, float progress)
{
	std::optional<TabData> tabDataOptional = getTab(uuid);
	if (tabDataOptional.has_value()) {
		TabData tabData = tabDataOptional.value();
        if (tabData.directory != nullptr && tabData.progressBar != nullptr) {
            if (progress >= 1.0f)
            {
                tabData.progressBar->hide();
                tabData.progressBar->reset();
            }
            else {
                tabData.progressBar->show();
                tabData.progressBar->setMaximum(100);
                tabData.progressBar->setMinimum(0);
                tabData.progressBar->setValue(progress * 100);
            }
        } else {
            qWarning() << "warning: failed to set progress bar in full progress";
        }
	}
	else {
		qWarning() << "warning: failed to fetch progress bar in full progress";
	}
}

QStringList MoeMainWindow::getTagsFromFile()
{
	QFile tagFile("resources/tags.json");
    if (tagFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray tagFileArray = tagFile.readAll();
        tagFile.close();
        QJsonDocument tagJsonArray = QJsonDocument::fromJson(tagFileArray);
        QJsonArray tagJsonArrayObject = tagJsonArray.object().value("tags").toArray();
        QStringList tagStringList;
        for (QJsonValue tag : tagJsonArrayObject)
        {
            QString tagString = tag.toString();
            tagStringList.append(tagString);
        }
        return tagStringList;
    }
    else {
        qWarning() << "warning: failed to open resources/tags.json";
        return QStringList();
    }
}

void MoeMainWindow::increaseDirectoryRenderScale()
{
    modifyDirectoryRenderScale(0.3f);
}

void MoeMainWindow::decreaseDirectoryRenderScale()
{
    modifyDirectoryRenderScale(-0.3f);
}

void MoeMainWindow::modifyDirectoryRenderScale(const float delta)
{
    directoryRenderScale += delta;
    directoryRenderScale = std::clamp(directoryRenderScale, 0.1f, 20.0f);
    directoryListView->setIconSize(QSize(g_thumbWidth / directoryRenderScale, g_thumbHeight / directoryRenderScale));
    emit directoryListModel->layoutChanged();
}

void MoeMainWindow::setPage(int page)
{
	currentPage = page;
	directoryCurrentPage->setText("Page: " + QString::number(currentPage));
	if (currentPage > 0) {
		directoryButtonPreviousPage->setDisabled(false);
	}
	else {
		directoryButtonPreviousPage->setDisabled(true);
	}
}

void MoeMainWindow::setTagList(QSharedPointer<DirectoryResult> directory)
{
    if(directory.isNull())
    {
        return;
    }

    tagWidgetContents->clear();
    int index = 0;

    tagWidgetContents->setStyleSheet("QListWidget:item { }");

    for(const TagResult& tag : directory->tags)
    {
        tagWidgetContents->addItem(tag.tag);
        QColor tagColor = QColor::fromRgb(66, 152, 217);
        switch(tag.type)
        {
            case TagType::ARTIST:
                tagColor = QColor::fromRgb(255, 121, 92);
                break;
            case TagType::CHARACTER:
                tagColor = QColor::fromRgb(53, 198, 63);
                break;
            case TagType::COPYRIGHT:
                tagColor = QColor::fromRgb(174, 151, 225);
                break;
            case TagType::GENERAL:
                tagColor = QColor::fromRgb(66, 152, 217);
                break;
            case TagType::META:
                tagColor = QColor::fromRgb(255, 201, 14);
                break;
        }
        tagWidgetContents->item(index)->setData(Qt::DisplayRole, tag.tag);
        tagWidgetContents->item(index)->setData(Qt::DecorationRole, tagColor.name());
        tagWidgetContents->item(index)->setForeground(tagColor);
        index++;
    }

    tagWidgetContents->setItemDelegate(new TagListItem(tagWidgetContents));
}

MoeMainWindow::~MoeMainWindow()
{
    if(directoryListModel != nullptr)
        delete directoryListModel;
    //if(directoryButtonLocal != nullptr)
    //    delete directoryButtonLocal;
    if(directoryButtonNetwork != nullptr)
        delete directoryButtonNetwork;
    if(directorySearchEdit != nullptr)
        delete directorySearchEdit;
    if(completer != nullptr)
        delete completer;
}
