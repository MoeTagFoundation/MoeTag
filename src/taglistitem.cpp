#include "taglistitem.h"

void TagListItem::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	QString tag = index.data(Qt::DisplayRole).toString();
	QColor color = QColor::fromString(index.data(Qt::DecorationRole).toString());
	color = color.darker();

	if (option.state & QStyle::State_Selected) {
		color = color.lighter(160);
	}
	else if (option.state & QStyle::State_MouseOver) {
		color = color.lighter(130);
	}

	painter->fillRect(option.rect, color);

	//r = option.rect.adjusted(50, 0, 0, -50);
	painter->drawText(option.rect.left(), option.rect.top(), option.rect.width(), option.rect.height(), Qt::AlignCenter | Qt::TextWordWrap, tag);
}

