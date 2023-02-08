#ifndef DIRECTORYLISTMODEL_H
#define DIRECTORYLISTMODEL_H

#include "directoryindexer.h"
#include <QAbstractListModel>
#include <QCache>
#include <QMutex>
#include <QPromise>
#include <QThread>
#include <QtConcurrent/QtConcurrent>
#include <QCache>

class MoeMainWindow;

class DirectoryListModel : public QAbstractListModel {
	Q_OBJECT
public:
	explicit DirectoryListModel(QObject* parent = nullptr);
	~DirectoryListModel();

	// public interface
	void addDirectory(DirectoryResult directory);
	const QList<QSharedPointer<DirectoryResult>> getDirectories() const;
	QSharedPointer<DirectoryResult> getDirectory(int64_t index);

	void invalidateCache(QSharedPointer<DirectoryResult> result);
	void formatThumbnail(QPixmap& image);

	static void deleteDirectoryData(DirectoryResult result);

public slots:
	void resetDirectories();
private:
	// private interface
	int rowCount(const QModelIndex& parent = QModelIndex()) const;
	QVariant data(const QModelIndex& index, int role) const;
	// storage
	QVector<QSharedPointer<DirectoryResult>> directories;

	QIcon* defaultIcon;
	QPixmap* defaultPixmap;

	mutable QCache<QUuid, QIcon> iconCache;
	mutable QMutex directoryMutex;
};

#endif // DIRECTORY#LISTMODEL_H
