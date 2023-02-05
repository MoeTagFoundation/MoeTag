#include "moepreviewlabel.h"
#include "qevent.h"

MoePreviewLabel::MoePreviewLabel(QWidget *parent)
    : QLabel{parent} { }

MoePreviewLabel::~MoePreviewLabel() {}

void MoePreviewLabel::setInternalPixmap(QPixmap map)
{
    internalPixmap = map;
    updateScale();
}

void MoePreviewLabel::resizeEvent(QResizeEvent *event)
{
    updateScale();
    QLabel::resizeEvent(event);
}

void MoePreviewLabel::updateScale()
{
    if(internalPixmap.isNull())
        return;
    setPixmapScaled(internalPixmap);
}

void MoePreviewLabel::setPixmapScaled(QPixmap image)
{
    QWidget *parent = parentWidget();
    if(parent != nullptr) {
        int w = parent->width();
        int h = parent->height();
        QPixmap scaled = image.scaled(w, h, Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);
        setPixmap(scaled);
    }
}
