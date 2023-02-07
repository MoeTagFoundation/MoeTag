#include "directoryindexer.h"
#include "qjsonarray.h"
#include "qjsondocument.h"
#include <QDir>
#include <QFileInfoList>
#include <QJsonObject>
#include <QtConcurrent>

DirectoryIndexer::DirectoryIndexer()
{
    setCurrentlyIndexing(false);

    extensionMap.insert(".png", FileType::IMAGE);
    extensionMap.insert(".bmp", FileType::IMAGE);
    extensionMap.insert(".tiff", FileType::IMAGE);
    extensionMap.insert(".jpg", FileType::IMAGE);
    extensionMap.insert(".jpeg", FileType::IMAGE);

    extensionMap.insert(".gif", FileType::GIF);

    extensionMap.insert(".mov", FileType::VIDEO);
    extensionMap.insert(".mp4", FileType::VIDEO);
    extensionMap.insert(".webm", FileType::VIDEO);

    jsonNetworkManager = new QNetworkAccessManager();

    connect(jsonNetworkManager, &QNetworkAccessManager::finished, this, [&](QNetworkReply* reply){
        QList<DirectoryResult> results;

        QNetworkReply::NetworkError error = reply->error();
        if (error == QNetworkReply::NetworkError::NoError) {
            QByteArray jsonBytes = reply->readAll();
            QString jsonString(jsonBytes.toStdString().c_str());
            QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonString.toUtf8());

            if(jsonDocument.isNull()) { qWarning("warning: json null in indexer network"); return; }
            if(!jsonDocument.isArray()) { qWarning("warning: document non-array in indexer network"); return; }

            QJsonArray array = jsonDocument.array();

            if (array.count() == 0)
            {
                qWarning() << "warning: 0 results returned, too many tags? endpoint down? no results?";
            }

            QJsonArray::Iterator it;
            for (it = array.begin(); it != array.end(); it++)
            {
                if(it->isObject())
                {
                    QJsonObject subObject = it->toObject();

                    DirectoryResult result;
                    result.indexType = IndexType::NETWORKAPI;                 
                    result.uuid = QUuid::createUuid();

                    // ADD PREVIEW
                    QJsonValueRef previewObject = subObject["preview_file_url"];
                    if(!previewObject.isNull() && previewObject.isString())
                    {
                        QString previewString = previewObject.toString();
                        if(!previewString.isNull() && !previewString.isEmpty())
                        {
                            result.thumbnailSource = previewString;
                        } else {
                            continue;
                        }
                    } else {
                        continue;
                    }

                    // ADD FULL
                    QJsonValueRef fullObject = subObject["file_url"];
                    if(!fullObject.isNull() && fullObject.isString())
                    {
                        QString fullString = fullObject.toString();
                        if(!fullString.isNull() && !fullString.isEmpty())
                        {
                            if(fullString.endsWith(".zip"))
                            {
                                qWarning() << "warning: .zip files or ugoira files are not supported yet, skipping";
                                continue;
                            }
                            result.contentSource = fullString;
                        }
                    }

                    // ADD TAGS_GENERAL
                    applyTaggingInformation(&result, subObject["tag_string_artist"], TagType::ARTIST);
                    applyTaggingInformation(&result, subObject["tag_string_copyright"], TagType::COPYRIGHT);
                    applyTaggingInformation(&result, subObject["tag_string_character"], TagType::CHARACTER);
                    applyTaggingInformation(&result, subObject["tag_string_general"], TagType::GENERAL);
                    applyTaggingInformation(&result, subObject["tag_string_meta"], TagType::META);

                    applyFileTypeInformation(&result);

                    results.append(result);
                } else {
                    qWarning("warning: non-object in indexer network"); return;
                }
            }

        } else {
            qWarning() << "warning: json network error in indexer: " << error;
        }

        setCurrentlyIndexing(false);
        emit finished(results, IndexType::NETWORKAPI);
    });
}

void DirectoryIndexer::applyTaggingInformation(DirectoryResult *result, QJsonValueRef property, TagType type)
{
    if(!property.isNull() && property.isString())
    {
        QString tagString = property.toString();
        QVector<QString> splitTags = tagString.split(' ');
        for(const QString& st : splitTags) {
            if(!st.isNull() && !st.isEmpty()) {
                result->tags.append(TagResult{ st, type });
            }
        }
    }
}

void DirectoryIndexer::setCurrentlyIndexing(bool currentlyIndexing)
{
    this->currentlyIndexing = currentlyIndexing;
    emit onCurrentlyIndexingChange(currentlyIndexing);
}

DirectoryIndexer::~DirectoryIndexer()
{
    jsonNetworkManager->deleteLater();
    jsonNetworkManager = nullptr;
}

void DirectoryIndexer::indexDirectory(QWidget* parent, QString query, std::optional<int> page, IndexType type)
{
    int p = 0;
    if(page.has_value())
        p = page.value();
    if (type == IndexType::NETWORKAPI) {
        setCurrentlyIndexing(true);
        indexDirectoryNetwork(query, p);
    }
    else if(type == IndexType::FILESYSTEM)
    {
        QFuture<void> future = QtConcurrent::run(&DirectoryIndexer::indexDirectoryFilesystem, this, openFileSelector(parent));
        future.onFailed([]{
           qWarning() << "warning: failed to index directory on filesystem";
        });
    }
}

QList<QUrl> DirectoryIndexer::openFileSelector(QWidget* parent) const
{
    QStringList filter;
    auto placeholder = extensionMap.keys();
    for (const QString &extension : placeholder) {
        filter << QString("*").append(extension).toLatin1(); // tested
    }

    QString filterConcat;
    for(QString& string : filter)
    {
        filterConcat.append(string);
        filterConcat.append(';');
    }

    const QList<QUrl> folderPath = QFileDialog::getOpenFileUrls(parent, QString(tr("Select Files for Import")), QUrl(), filterConcat);
    return folderPath;
}

void DirectoryIndexer::indexDirectoryNetwork(QString query, int page) const
{
    // TODO: Add API key options
    QString endpoint("https://danbooru.donmai.us/posts.json?tags=[search]&page=[page]&limit=100");
    endpoint = endpoint.replace(QString("[search]"), query);
    endpoint = endpoint.replace(QString("[page]"), QString::number(page));

    jsonNetworkManager->get(QNetworkRequest(QUrl(endpoint)));
}

void DirectoryIndexer::indexDirectoryFilesystem(QPromise<void>& promise, QList<QUrl> urls)
{
    setCurrentlyIndexing(true);

    promise.start();
    QList<DirectoryResult> results;
    for (const QUrl& url : urls) {
        QString fileResolvedDirectory = url.toString();

        DirectoryResult result;
        {
            result.indexType = IndexType::FILESYSTEM;
            result.thumbnailSource = fileResolvedDirectory;
            result.contentSource = fileResolvedDirectory;
            result.uuid = QUuid::createUuid();
        }

        applyFileTypeInformation(&result);

        results.append(result);
    }
    promise.finish();

    setCurrentlyIndexing(false);
    emit finished(results, IndexType::FILESYSTEM);
}

void DirectoryIndexer::applyFileTypeInformation(DirectoryResult* result) const
{
    if(result != nullptr) {
        auto placeholder = extensionMap.keys();
        for (const QString &extension : placeholder) {
            if (result->contentSource.endsWith(extension, Qt::CaseInsensitive)) {
                result->contentFileType = extensionMap[extension];
                break;
            }
        }
        for (const QString &extension : placeholder) {
            if (result->thumbnailSource.endsWith(extension, Qt::CaseInsensitive)) {
                result->thumbnailFileType = extensionMap[extension];
                break;
            }
        }
    }
}

QString DirectoryIndexer::fileTypeToDisplayString(const FileType type)
{
    switch(type)
    {
        case FileType::GIF:
            return QString("GIF");
        case FileType::IMAGE:
            return QString("IMAGE");
        case FileType::VIDEO:
            return QString("VIDEO");
        case FileType::UNKNOWNFILE:
            return QString("UNKNOWN");
        default:
            return QString("ERROR");
    }
}
