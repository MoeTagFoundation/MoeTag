#ifndef DIRECTORYPOPULATOR_H
#define DIRECTORYPOPULATOR_H

#include "directoryindexer.h"
#include <QNetworkAccessManager>
#include <QObject>
#include <ffmpegthumbs/ffmpegthumbnailer.h>

enum PopulateType
{
	PREVIEW,
	FULL,
};

class DirectoryPopulator : public QObject
{
	Q_OBJECT
public:
	explicit DirectoryPopulator();
	~DirectoryPopulator();
	void populateDirectory(QList<DirectoryResult> indexes, IndexType indexType, PopulateType type);
private:
	QImage* replyToImage(QNetworkReply* reply, QByteArray imageBytes);
	QPixmap replyToPixmap(QNetworkReply* reply, QByteArray pixmapBytes);

	QNetworkAccessManager* thumbnailNetworkManager;
	QNetworkAccessManager* imageNetworkManager;
	QMap<QNetworkReply*, DirectoryResult>* indexes;
	FFMpegThumbnailer* thumbnailer;

	int target = 0;
signals:
	void finishedResult(DirectoryResult result, PopulateType type);

	void previewProgress(int current, int maximum);
	void fullProgress(QUuid uuid, qint64 bytes, qint64 bytesTotal);
};

#endif // DIRECTORYPOPULATOR_H
