#ifndef TAGLISTITEM_H
#define TAGLISTITEM_H

#include "qpainter.h"
#include <QStyledItemDelegate>

class TagListItem : public QStyledItemDelegate
{
    Q_OBJECT
public:
    TagListItem(QObject *parent = nullptr) : QStyledItemDelegate (parent){}

    void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;

    QSize sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const{
        return QSize(200, 20);
    }
};

#endif // TAGLISTITEM_H
