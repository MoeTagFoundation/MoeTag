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

typedef struct TagResult_S
{
    QString tag;
    TagType type;
} TagResult;

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

    bool currentlyIndexing;
private:
    void indexDirectoryNetwork(QString query, int page) const;
    void indexDirectoryFilesystem(QPromise<void>& promise, QList<QUrl> urls);

    void applyTaggingInformation(DirectoryResult* result, QJsonValueRef property, TagType type);

    void setCurrentlyIndexing(bool currentlyIndexing);

    QNetworkAccessManager* jsonNetworkManager;

    QMap<QString, FileType> extensionMap;

signals:
    void finished(QList<DirectoryResult> results, IndexType type);
    void onCurrentlyIndexingChange(bool currentlyIndexing);
};

#endif // DIRECTORYINDEXER_H
