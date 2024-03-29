/*
    SPDX-FileCopyrightText: 2010 Dirk Vanden Boer <dirk.vdb@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KFFMPEG_THUMBNAILER_H
#define KFFMPEG_THUMBNAILER_H

#include <QObject>
#include <QCache>
// KF

#include "ffmpegthumbnailer/videothumbnailer.h"

class QCheckBox;
class QLineEdit;
class QSpinBox;

class FFMpegThumbnailer : public QObject
{
    Q_OBJECT

private:
    typedef QCache<QString, QImage> ThumbCache;

public:
    FFMpegThumbnailer();
    ~FFMpegThumbnailer() override;
    bool create(const QString& path, int width, int height, QImage& img);

private:
    void updateSequenceIndexWraparoundPoint(float offset);

private:
    ffmpegthumbnailer::VideoThumbnailer m_Thumbnailer;
    ThumbCache                          m_thumbCache;
};

#endif
