#ifndef DIRECTORYINDEXER_H
#define DIRECTORYINDEXER_H

#include <QImage>
#include <QString>
#include <QUuid>
#include <QPromise>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFileDialog>
#include <QBuffer>
#include <QJsonValueRef>

enum IndexType
{
	FILESYSTEM,
	NETWORKAPI,
	UNKNOWNINDEX,
};
enum FileType
{
	VIDEO,
	IMAGE,
	GIF,
	UNKNOWNFILE,
};
enum TagType
{
	GENERAL,
	CHARACTER,
	COPYRIGHT,
	ARTIST,
	META
};
enum DirectoryEndpointType
{
	JsonApi,
	Unknown,
};

typedef struct DirectoryTagResult_S
{
	QString tag;
	TagType type;
} TagResult;

typedef struct DirectoryPropertiesTags_S
{
	std::optional<QString> artist;
	std::optional<QString> copyright;
	std::optional<QString> character;
	std::optional<QString> general;
	std::optional<QString> meta;
} DirectoryPropertiesTags;

typedef struct DirectoryEndpointProperties_S
{
	QUrl url;
	DirectoryEndpointType type;
} DirectoryEndpointProperties;
typedef struct DirectoryProperties_S
{
	QString preview;
	QString full;
	std::optional<QString> root;
	DirectoryPropertiesTags tags;
} DirectoryProperties;
typedef struct DirectoryEndpoint_S
{
	QString name;
	DirectoryEndpointProperties endpoint;
	DirectoryProperties properties;
} DirectoryEndpoint;

typedef struct DirectoryResult_S
{
	// General Data
	QUuid uuid;
	IndexType indexType = UNKNOWNINDEX;

	// Metadata
	QVector<TagResult> tags;

	// Thumbnail Data
	QPixmap thumbnail;
	QString thumbnailSource = QString("/");
	FileType thumbnailFileType = UNKNOWNFILE;

	// Content Data
	QPixmap contentImage; // image data
	QBuffer* contentGif = nullptr; // gif data (additional space for qmovie)
	QString contentSource = QString("/");
	FileType contentFileType = UNKNOWNFILE;
} DirectoryResult;


class MoeMainWindow;

class DirectoryIndexer : public QObject
{
	Q_OBJECT
public:
	explicit DirectoryIndexer();
	~DirectoryIndexer();

	void indexDirectory(QWidget* parent, QString query, std::optional<int> page, IndexType type);
	QList<QUrl> openFileSelector(QWidget* parent) const;
	static QString fileTypeToDisplayString(const FileType type);

	void applyFileTypeInformation(DirectoryResult* result) const;

	void setEndpoint(QString endpoint);
	QMap<QString, DirectoryEndpoint> getEndpointMap();

	bool currentlyIndexing;
private:
	void indexDirectoryNetwork(QString query, int page) const;
	void indexDirectoryFilesystem(QPromise<void>& promise, QList<QUrl> urls);

	void applyTaggingInformation(DirectoryResult* result, QString tags, TagType type);

	void setCurrentlyIndexing(bool currentlyIndexing);

	void decodeEndpoints();
	QString endpointPathToValue(QJsonObject object, QString path);

	QNetworkAccessManager* jsonNetworkManager;

	QMap<QString, FileType> extensionMap;
	QMap<QString, DirectoryEndpoint> endpointMap;

	QString currentEndpoint;
signals:
	void finished(QList<DirectoryResult> results, IndexType type);
	void onCurrentlyIndexingChange(bool currentlyIndexing);
};

#endif // DIRECTORYINDEXER_H
