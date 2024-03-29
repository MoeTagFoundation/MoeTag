/*
    SPDX-FileCopyrightText: 2010 Dirk Vanden Boer <dirk.vdb@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "moviedecoder.h"

#include <QDebug>
#include <QFileInfo>

extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/display.h>
#include <libavutil/imgutils.h>
}

using namespace std;

namespace ffmpegthumbnailer
{

MovieDecoder::MovieDecoder(const QString& filename, AVFormatContext* pavContext)
        : m_VideoStream(-1)
        , m_pFormatContext(pavContext)
        , m_pVideoCodecContext(nullptr)
        , m_pVideoCodec(nullptr)
        , m_pVideoStream(nullptr)
        , m_pFrame(nullptr)
        , m_pFrameBuffer(nullptr)
        , m_pPacket(nullptr)
        , m_FormatContextWasGiven(pavContext != nullptr)
        , m_AllowSeek(true)
        , m_initialized(false)
        , m_bufferSinkContext(nullptr)
        , m_bufferSourceContext(nullptr)
        , m_filterGraph(nullptr)
        , m_filterFrame(nullptr)
{
    initialize(filename);
}

MovieDecoder::~MovieDecoder()
{
    destroy();
}

void MovieDecoder::initialize(const QString& filename)
{
    m_lastWidth = -1;
    m_lastHeight = -1;
    m_lastPixfmt = AV_PIX_FMT_NONE;

#if (LIBAVFORMAT_VERSION_MAJOR < 58)
    av_register_all();
#endif

    QFileInfo fileInfo(filename);

    if ((!m_FormatContextWasGiven) && avformat_open_input(&m_pFormatContext, fileInfo.absoluteFilePath().toLocal8Bit().data(), nullptr, nullptr) != 0) {
        qDebug() <<  "Could not open input file: " << fileInfo.absoluteFilePath();
        return;
    }

    if (avformat_find_stream_info(m_pFormatContext, nullptr) < 0) {
        qDebug() << "Could not find stream information";
        return;
    }

    if (!initializeVideo()) {
        // It already printed a message
        return;
    }
    m_pFrame = av_frame_alloc();

    if (m_pFrame) {
        m_initialized=true;
    }
}

bool MovieDecoder::getInitialized()
{
    return m_initialized;
}


void MovieDecoder::destroy()
{
    deleteFilterGraph();
    if (m_pVideoCodecContext) {
        avcodec_close(m_pVideoCodecContext);
        m_pVideoCodecContext = nullptr;
    }
    m_pVideoStream = nullptr;

    if ((!m_FormatContextWasGiven) && m_pFormatContext) {
        avformat_close_input(&m_pFormatContext);
        m_pFormatContext = nullptr;
    }

    if (m_pPacket) {
        av_packet_unref(m_pPacket);
        delete m_pPacket;
        m_pPacket = nullptr;
    }

    if (m_pFrame) {
        av_frame_free(&m_pFrame);
        m_pFrame = nullptr;
    }

    if (m_pFrameBuffer) {
        av_free(m_pFrameBuffer);
        m_pFrameBuffer = nullptr;
    }
}

QString MovieDecoder::getCodec()
{
    QString codecName;
    if (m_pVideoCodec) {
        codecName=QString::fromLatin1(m_pVideoCodec->name);
    }
    return codecName;
}

bool MovieDecoder::initializeVideo()
{
    m_VideoStream = av_find_best_stream(m_pFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &m_pVideoCodec, 0);
    if (m_VideoStream < 0) {
        qDebug() << "Could not find video stream";
        return false;
    }

    m_pVideoCodecContext = avcodec_alloc_context3(m_pVideoCodec);
    m_pVideoStream = m_pFormatContext->streams[m_VideoStream];
    avcodec_parameters_to_context(m_pVideoCodecContext, m_pVideoStream->codecpar);

    if (m_pVideoCodec == nullptr) {
        // set to nullptr, otherwise avcodec_close(m_pVideoCodecContext) crashes
        m_pVideoCodecContext = nullptr;
        qDebug() << "Video Codec not found";
        return false;
    }

    m_pVideoCodecContext->workaround_bugs = 1;

    if (avcodec_open2(m_pVideoCodecContext, m_pVideoCodec, nullptr) < 0) {
        qDebug() << "Could not open video codec";
        return false;
    }

    return true;
}

int MovieDecoder::getWidth()
{
    if (m_pVideoCodecContext) {
        return m_pVideoCodecContext->width;
    }

    return -1;
}

int MovieDecoder::getHeight()
{
    if (m_pVideoCodecContext) {
        return m_pVideoCodecContext->height;
    }

    return -1;
}

int MovieDecoder::getDuration()
{
    if (m_pFormatContext) {
        return static_cast<int>(m_pFormatContext->duration / AV_TIME_BASE);
    }

    return 0;
}

void MovieDecoder::seek(int timeInSeconds)
{
    if (!m_AllowSeek) {
        return;
    }

    qint64 timestamp = AV_TIME_BASE * static_cast<qint64>(timeInSeconds);

    if (timestamp < 0) {
        timestamp = 0;
    }

    int ret = av_seek_frame(m_pFormatContext, -1, timestamp, 0);
    if (ret >= 0) {
        avcodec_flush_buffers(m_pVideoCodecContext);
    } else {
        qDebug() << "Seeking in video failed";
        return;
    }

    int keyFrameAttempts = 0;
    bool gotFrame = 0;

    do {
        int count = 0;
        gotFrame = 0;

        while (!gotFrame && count < 20) {
            getVideoPacket();
            gotFrame = decodeVideoPacket();
            ++count;
        }

        ++keyFrameAttempts;
    } while ((!gotFrame || !m_pFrame->key_frame) && keyFrameAttempts < 200);

    if (gotFrame == 0) {
        qDebug() << "Seeking in video failed";
    }
}


bool MovieDecoder::decodeVideoFrame()
{
    bool frameFinished = false;

    while (!frameFinished && getVideoPacket()) {
        frameFinished = decodeVideoPacket();
    }

    if (!frameFinished) {
        qDebug() << "decodeVideoFrame() failed: frame not finished";
    }

    return frameFinished;
}

bool MovieDecoder::decodeVideoPacket()
{
    if (m_pPacket->stream_index != m_VideoStream) {
        return false;
    }

    av_frame_unref(m_pFrame);

    avcodec_send_packet(m_pVideoCodecContext, m_pPacket);
    int ret = avcodec_receive_frame(m_pVideoCodecContext, m_pFrame);
    if (ret == AVERROR(EAGAIN)) {
        return false;
    }

    return true;
}

QImageIOHandler::Transformations MovieDecoder::transformations()
{
    QImageIOHandler::Transformations ret = QImageIOHandler::TransformationNone;
    if (!m_pVideoStream) {
        qWarning() << "No video stream!";
        return ret;
    }

    for (int i=0; i<m_pVideoStream->nb_side_data; i++) {
        if (m_pVideoStream->side_data[i].type != AV_PKT_DATA_DISPLAYMATRIX) {
            continue;
        }
        if (m_pVideoStream->side_data[i].size != sizeof(int32_t) * 9) {
            qWarning() << "Invalid display matrix size" << m_pVideoStream->side_data[i].size << "expected" << sizeof(int32_t) * 9;
            continue;
        }
        int32_t *matrix = reinterpret_cast<int32_t*>(m_pVideoStream->side_data[i].data);
        double rotation = av_display_rotation_get(matrix);
        if (qFuzzyCompare(rotation, 0.)) {
            ret |= QImageIOHandler::TransformationNone;
        } else if (qFuzzyCompare(rotation, 90.)) {
            ret |= QImageIOHandler::TransformationRotate270;
        } else if (qFuzzyCompare(rotation, 180.) || qFuzzyCompare(rotation, -180.)) {
            ret |= QImageIOHandler::TransformationRotate180;
        } else if (qFuzzyCompare(rotation, -90.)) {
            ret |= QImageIOHandler::TransformationRotate90;
        } else {
            qWarning() << "Unhandled rotation" << rotation;
            continue;
        }
    }
    return ret;
}


bool MovieDecoder::getVideoPacket()
{
    bool framesAvailable = true;
    bool frameDecoded = false;

    int attempts = 0;

    if (m_pPacket) {
        av_packet_unref(m_pPacket);
        delete m_pPacket;
    }

    m_pPacket = new AVPacket();

    while (framesAvailable && !frameDecoded && (attempts++ < 1000)) {
        framesAvailable = av_read_frame(m_pFormatContext, m_pPacket) >= 0;
        if (framesAvailable) {
            frameDecoded = m_pPacket->stream_index == m_VideoStream;
            if (!frameDecoded) {
                av_packet_unref(m_pPacket);
            }
        }
    }

    return frameDecoded;
}

void MovieDecoder::deleteFilterGraph()
{
    if (m_filterGraph) {
        av_frame_free(&m_filterFrame);
        avfilter_graph_free(&m_filterGraph);
        m_filterGraph = nullptr;
    }
}

bool MovieDecoder::initFilterGraph(enum AVPixelFormat pixfmt, int width, int height)
{
    AVFilterInOut *inputs = nullptr, *outputs = nullptr;

    deleteFilterGraph();
    m_filterGraph = avfilter_graph_alloc();

    QByteArray arguments("buffer=");
    arguments += "video_size=" + QByteArray::number(width) + 'x' + QByteArray::number(height) + ':';
    arguments += "pix_fmt=" + QByteArray::number(pixfmt) + ':';
    arguments += "time_base=1/1:pixel_aspect=0/1[in];";
    arguments += "[in]yadif[out];";
    arguments += "[out]buffersink";

    int ret = avfilter_graph_parse2(m_filterGraph, arguments.constData(), &inputs, &outputs);
    if (ret < 0) {
        qWarning() << "Unable to parse filter graph";
        return false;
    }

    if(inputs || outputs)
        return false;

    ret = avfilter_graph_config(m_filterGraph, nullptr);
    if (ret < 0) {
        qWarning() << "Unable to validate filter graph";
        return false;
    }

    m_bufferSourceContext = avfilter_graph_get_filter(m_filterGraph, "Parsed_buffer_0");
    m_bufferSinkContext = avfilter_graph_get_filter(m_filterGraph, "Parsed_buffersink_2");
    if (!m_bufferSourceContext || !m_bufferSinkContext) {
        qWarning() << "Unable to get source or sink";
        return false;
    }
    m_filterFrame = av_frame_alloc();
    m_lastWidth = width;
    m_lastHeight = height;
    m_lastPixfmt = pixfmt;

    return true;
}

bool MovieDecoder::processFilterGraph(AVFrame *dst, const AVFrame *src,
                                enum AVPixelFormat pixfmt, int width, int height)
{
    if (!m_filterGraph || width != m_lastWidth ||
        height != m_lastHeight || pixfmt != m_lastPixfmt) {

        if (!initFilterGraph(pixfmt, width, height)) {
            return false;
        }
    }

    memcpy(m_filterFrame->data, src->data, sizeof(src->data));
    memcpy(m_filterFrame->linesize, src->linesize, sizeof(src->linesize));
    m_filterFrame->width = width;
    m_filterFrame->height = height;
    m_filterFrame->format = pixfmt;

    int ret = av_buffersrc_add_frame(m_bufferSourceContext, m_filterFrame);
    if (ret < 0) {
        return false;
    }

    ret = av_buffersink_get_frame(m_bufferSinkContext, m_filterFrame);
    if (ret < 0) {
        return false;
    }

    av_image_copy(dst->data, dst->linesize, (const uint8_t **)m_filterFrame->data, m_filterFrame->linesize, pixfmt, width, height);
    av_frame_unref(m_filterFrame);

    return true;
}

void MovieDecoder::getScaledVideoFrame(int scaledSize, bool maintainAspectRatio, VideoFrame& videoFrame)
{
    if (m_pFrame->interlaced_frame) {
        processFilterGraph((AVFrame*) m_pFrame, (AVFrame*) m_pFrame, m_pVideoCodecContext->pix_fmt,
                              m_pVideoCodecContext->width, m_pVideoCodecContext->height);
    }

    int scaledWidth, scaledHeight;
    convertAndScaleFrame(AV_PIX_FMT_RGB24, scaledSize, maintainAspectRatio, scaledWidth, scaledHeight);

    videoFrame.width = scaledWidth;
    videoFrame.height = scaledHeight;
    videoFrame.lineSize = m_pFrame->linesize[0];

    videoFrame.frameData.clear();
    const auto frameSize = videoFrame.lineSize * videoFrame.height;
    if (frameSize < 1) {
        return; // calling front() on an empty vector is undefined behavior; return empty frame data instead
    }
    videoFrame.frameData.resize(frameSize);
    memcpy((&(videoFrame.frameData.front())), m_pFrame->data[0], frameSize);
}

void MovieDecoder::convertAndScaleFrame(AVPixelFormat format, int scaledSize, bool maintainAspectRatio, int& scaledWidth, int& scaledHeight)
{
    calculateDimensions(scaledSize, maintainAspectRatio, scaledWidth, scaledHeight);
    SwsContext* scaleContext = sws_getContext(m_pVideoCodecContext->width, m_pVideoCodecContext->height,
                               m_pVideoCodecContext->pix_fmt, scaledWidth, scaledHeight,
                               format, SWS_BICUBIC, nullptr, nullptr, nullptr);

    if (nullptr == scaleContext) {
        qDebug() << "Failed to create resize context";
        return;
    }

    AVFrame* convertedFrame = nullptr;
    uint8_t* convertedFrameBuffer = nullptr;

    createAVFrame(&convertedFrame, &convertedFrameBuffer, scaledWidth, scaledHeight, format);

    sws_scale(scaleContext, m_pFrame->data, m_pFrame->linesize, 0, m_pVideoCodecContext->height,
              convertedFrame->data, convertedFrame->linesize);
    sws_freeContext(scaleContext);

    av_frame_free(&m_pFrame);
    av_free(m_pFrameBuffer);

    m_pFrame        = convertedFrame;
    m_pFrameBuffer  = convertedFrameBuffer;
}

void MovieDecoder::calculateDimensions(int squareSize, bool maintainAspectRatio, int& destWidth, int& destHeight)
{
    if (!maintainAspectRatio) {
        destWidth = squareSize;
        destHeight = squareSize;
    } else {
        int srcWidth            = m_pVideoCodecContext->width;
        int srcHeight           = m_pVideoCodecContext->height;
        int ascpectNominator    = m_pVideoCodecContext->sample_aspect_ratio.num;
        int ascpectDenominator  = m_pVideoCodecContext->sample_aspect_ratio.den;

        if (ascpectNominator != 0 && ascpectDenominator != 0) {
            srcWidth = srcWidth * ascpectNominator / ascpectDenominator;
        }

        if (srcWidth > srcHeight) {
            destWidth  = squareSize;
            destHeight = static_cast<int>(static_cast<float>(squareSize) / srcWidth * srcHeight);
        } else {
            destWidth  = static_cast<int>(static_cast<float>(squareSize) / srcHeight * srcWidth);
            destHeight = squareSize;
        }
    }
}

void MovieDecoder::createAVFrame(AVFrame** avFrame, quint8** frameBuffer, int width, int height, AVPixelFormat format)
{
    *avFrame = av_frame_alloc();

    int numBytes = av_image_get_buffer_size (format, width + 1, height + 1, 16);
    *frameBuffer = reinterpret_cast<quint8*>(av_malloc(numBytes));
    av_image_fill_arrays ((*avFrame)->data, (*avFrame)->linesize, *frameBuffer, format, width, height, 1);
}

}
