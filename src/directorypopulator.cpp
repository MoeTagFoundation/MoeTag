#include "directorylistmodel.h"
#include "directorypopulator.h"
#include "qtconcurrentrun.h"

#include <QBuffer>
#include <QFutureSynchronizer>
#include <QFutureWatcher>
#include <QIODevice>
#include <QImageReader>
#include <QMovie>
#include "moeglobals.h"

QImage *DirectoryPopulator::replyToImage(QNetworkReply *reply, QByteArray imageBytes)
{
    QNetworkReply::NetworkError error = reply->error();
    if (error == QNetworkReply::NetworkError::NoError) {
        QImage* image = new QImage();
        image->loadFromData(imageBytes);
        if(image->isNull())
        {
            if(image != nullptr) {
                delete image;
            }
            qWarning() << "warning: null image returned from preview populator network: " << reply->url().toString();
            return nullptr;
        }
        return image;
    } else {
        qWarning() << "warning: error returned from populator network: " << error << " with details: " << reply->errorString();
        return nullptr;
    }
}

QPixmap DirectoryPopulator::replyToPixmap(QNetworkReply *reply, QByteArray pixmapBytes)
{
    QNetworkReply::NetworkError error = reply->error();
    if (error == QNetworkReply::NetworkError::NoError) {
        QPixmap pixmap = QPixmap();
        pixmap.loadFromData(pixmapBytes);
        if(pixmap.isNull())
        {
            qWarning() << "warning: null image returned from preview populator network: " << reply->url().toString();
        }
        return pixmap;
    } else {
        qWarning() << "warning: ==== for url: " << reply->url();
        qWarning() << "warning: error returned from populator network: " << error << " with details: " << reply->errorString();
        return QPixmap();
    }
}

DirectoryPopulator::DirectoryPopulator()
{
    QImageReader::setAllocationLimit(1000);

    thumbnailer = new FFMpegThumbnailer();
    thumbnailNetworkManager = new QNetworkAccessManager();

    indexes = new QMap<QNetworkReply*, DirectoryResult>();

    connect(thumbnailNetworkManager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply) {
        QByteArray bytes = reply->readAll();
        QPixmap pixmap = replyToPixmap(reply, bytes);
        if(!pixmap.isNull()) {

            DirectoryResult result = indexes->value(reply);
			if (!result.thumbnail.isNull())
			{
				qWarning() << "warning: thumbnail exists when iterating index in thumbnailNetworkManager";
			}
			result.thumbnail = pixmap;

            // Update state
			emit previewProgress(target - (indexes->size() - 1), target);
			emit finishedResult(result, PopulateType::PREVIEW);

        } else {
            // Update state
            emit previewProgress(target - (indexes->size() - 1), target);
        }
        if (indexes->contains(reply))
            indexes->remove(reply);
    });

    imageNetworkManager = new QNetworkAccessManager();
    connect(imageNetworkManager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply) {
        QByteArray bytes = reply->readAll();
        QPixmap pixmap = replyToPixmap(reply, bytes);

        if(!pixmap.isNull()) {
            DirectoryResult result = indexes->value(reply);
            result.contentImage = pixmap;

            if(result.contentFileType == FileType::GIF)
            {
                QBuffer* buffer = new QBuffer(); // needs parent?
                buffer->setData(bytes);
                result.contentGif = buffer;
            }

            emit finishedResult(result, PopulateType::FULL);
        }

		if (indexes->contains(reply))
			indexes->remove(reply);
    });
}

DirectoryPopulator::~DirectoryPopulator()
{
    delete thumbnailNetworkManager;
    delete imageNetworkManager;
    delete thumbnailer;
}

void DirectoryPopulator::populateDirectory(QList<DirectoryResult> ind, IndexType indexType, PopulateType type)
{
    qDebug() << "debug: current ssl library build version: " << QSslSocket::sslLibraryBuildVersionString();
    qDebug() << "debug: is tlsv1.2 or later supported: " << QSslSocket::isProtocolSupported(QSsl::TlsV1_2OrLater);

    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    if(!config.isNull()) {
        config.setProtocol(QSsl::SslProtocol::TlsV1_2OrLater);
    }

    if(type == PopulateType::PREVIEW)
    {
        for(const DirectoryResult &directory : ind) {
            if(directory.thumbnailFileType == FileType::VIDEO)
            {
                // TODO: async generation of thumbnails
				QImage image;
				thumbnailer->create(QUrl(directory.contentSource).toLocalFile(), g_thumbWidth, g_thumbHeight, image);
				DirectoryResult updated = directory;
				updated.thumbnail = QPixmap::fromImage(image);
				emit finishedResult(updated, PopulateType::PREVIEW);
            } else if (directory.thumbnailFileType == FileType::IMAGE || directory.thumbnailFileType == FileType::GIF) {          
                QNetworkRequest request(QUrl(directory.thumbnailSource));
                request.setSslConfiguration(config);
                request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; en-US; rv:1.9.0.20) Gecko/20150609 Firefox/35.0");
                indexes->insert(thumbnailNetworkManager->get(request), directory);
            } else {
                qWarning() << "warning: unsupported thumbnailFileType, cannot generate preview";
            }
        }

        target = indexes->size();
        emit previewProgress(0, indexes->size());
    }
    else if(type == PopulateType::FULL)
    {
        for(const DirectoryResult &directory : ind) {
            if(directory.contentFileType == FileType::VIDEO)
            {
                qWarning() << "warning: cannot populate type 'FULL' in videos, these are streamed via QMediaPlayer";
                continue;
            }

            QNetworkRequest request(QUrl(directory.contentSource));
            request.setSslConfiguration(config);
            request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; en-US; rv:1.9.0.20) Gecko/20150609 Firefox/35.0");
            QNetworkReply* reply = imageNetworkManager->get(request);
            connect(reply, &QNetworkReply::downloadProgress, this, [=](qint64 b1, qint64 b2){
                emit fullProgress(directory.uuid, b1, b2);
            });
            indexes->insert(reply, directory);
        }
    }
}
