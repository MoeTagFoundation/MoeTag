
#include "directorylistmodel.h"
#include <QVariant>
#include <QListWidgetItem>
#include <QDebug>
#include <QMetaObject>
#include <QPixmap>
#include <QtGlobal>
#include <QFutureWatcher>
#include <QPainter>
#include <QIcon>
#include <QString>
#include <QPixmap>
#include "moeglobals.h"

Q_DECLARE_METATYPE(QListWidgetItem*)
DirectoryListModel::DirectoryListModel(QObject* parent) : QAbstractListModel(parent) {
	defaultPixmap = new QPixmap(g_thumbWidth, g_thumbHeight);
	defaultPixmap->fill(QColor("black"));
	defaultIcon = new QIcon(*defaultPixmap);
}

DirectoryListModel::~DirectoryListModel()
{
	delete defaultPixmap;
	delete defaultIcon;

	this->resetDirectories();
}

void DirectoryListModel::addDirectory(DirectoryResult directory)
{
	QMutexLocker locker(&directoryMutex);

	// Step 1: Generate UUID
	if (directory.uuid.isNull()) {
		directory.uuid = QUuid::createUuid();
	}

	// Step 2: Format
	if (directory.thumbnail.isNull()) {
		directory.thumbnail = *defaultPixmap;
	}
	formatThumbnail(directory.thumbnail);

	// Step 3: Add
	QSharedPointer<DirectoryResult> dr = QSharedPointer<DirectoryResult>::create();
	if (dr != nullptr) {
		*dr = directory;
		directories.append(dr);
	}
}

const QList<QSharedPointer<DirectoryResult>> DirectoryListModel::getDirectories() const
{
	return directories;
}

QSharedPointer<DirectoryResult> DirectoryListModel::getDirectory(int64_t index)
{
	QMutexLocker locker(&directoryMutex);

	if (index < directories.size() && index >= 0) {
		return directories[index];
	}
	else {
		qWarning() << "error: tried to get getDirectory() out of range";
		return nullptr;
	}
}

void DirectoryListModel::resetDirectories()
{
	QMutexLocker locker(&directoryMutex);

	// delete directory data before clear
	for (QSharedPointer<DirectoryResult> result : directories)
	{
		if (!result.isNull()) {
			deleteDirectoryData(*result);
		}
		result.clear();
	}

	directories.clear(); // clear directory data
	revert(); // clear internal tree data
	emit layoutChanged(); // inform layout to prevent invalid model indexes
	iconCache.clear(); // clear icon cache as previews can be confirmed as invalidated
}

void DirectoryListModel::deleteDirectoryData(DirectoryResult result)
{
	// Delete thumbnail data
	if (!result.thumbnail.isNull()) {
		QPixmap map = QPixmap();
		result.thumbnail.swap(map);
	}
	// Delete content image data
	if (!result.contentImage.isNull()) {
		QPixmap map = QPixmap();
		result.contentImage.swap(map);
	}
	// Delete gif buffer data
	if (result.contentGif != nullptr) {
		delete result.contentGif;
		result.contentGif = nullptr;
	}
}

void DirectoryListModel::invalidateCache(QSharedPointer<DirectoryResult> result)
{
	if (result == nullptr)
	{
		qWarning() << "warning: trying to invalidate cache with invalid result";
		return;
	}

	if (iconCache.contains(result->uuid))
	{
		iconCache.remove(result->uuid);
	}
}

void DirectoryListModel::formatThumbnail(QPixmap& image)
{
	// Create base thumbnail
	QPixmap basePixmap(g_thumbWidth, g_thumbHeight);
	basePixmap.fill(QColor(0, 0, 0, 0));
	if (basePixmap.isNull())
	{
		qWarning() << "error: formatThumbnail() basePixmap is null";
		return;
	}

	QPixmap scaled = image.scaled(g_thumbWidth, g_thumbHeight, Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);
	if (scaled.isNull())
	{
		qWarning() << "error: formatThumbnail() scaledImage is null";
		return;
	}

	// Create painter
	QPainter painter(&basePixmap);
	int centerX = std::max(0, (g_thumbWidth - scaled.width())) / 2;
	int centerY = std::max(0, (g_thumbHeight - scaled.height())) / 2;
	painter.drawPixmap(QPointF(centerX, centerY), scaled, scaled.rect()); // Draw to image#
	painter.end();

	image.swap(basePixmap); // Replace data // TODO: Check
}

int DirectoryListModel::rowCount(const QModelIndex& parent) const
{
	QMutexLocker locker(&directoryMutex);
	qsizetype size = directories.size();
	return size;
}

QVariant DirectoryListModel::data(const QModelIndex& index, int role) const
{
	QMutexLocker locker(&directoryMutex);

	int row = index.row();
	if (row > directories.size() || row < 0)
	{
		qWarning() << "warning: data invalid row";
		return QVariant();
	}
	QSharedPointer<DirectoryResult> directory = directories[row];
	if (directory.isNull())
	{
		qWarning() << "warning: directory null in data";
		return QVariant();
	}

	QIcon icon = *defaultIcon;
	QString textRole = QString("#")
		.append(QString::number(index.row()))
		.append(" ")
		.append(DirectoryIndexer::fileTypeToDisplayString(directory->contentFileType));
	switch (role)
	{
	case Qt::ItemDataRole::DecorationRole:
		if (iconCache.contains(directory->uuid)) {
			icon = *iconCache[directory->uuid];
		}
		else {
			if (!directory->thumbnail.isNull()) {
				QPixmap pixmap = directory->thumbnail;

				if (pixmap.isNull()) {
					qWarning() << "error: null pixmap for directory " << directory->uuid;
					return QVariant(*defaultIcon);
				}

				if (!iconCache.insert(directory->uuid, new QIcon(pixmap)))
				{
					icon = QIcon(pixmap);
				}
				else {
					icon = *iconCache[directory->uuid];
				}
			}
			else {
				qWarning() << "error: nullptr pixmap for directory " << directory->uuid;
				return QVariant(*defaultIcon);
			}
		}
		return QVariant(icon);
	case Qt::ItemDataRole::DisplayRole:;
		return QVariant();
	default:
		return QVariant();
	}
}
