#include "contentlistview.h"
#include <QDebug>

ContentListView::ContentListView()
{

    bool successClick = connect(this, &ContentListView::doubleClicked, this, &ContentListView::itemDoubleClicked);
    Q_ASSERT(successClick);

    bool successClick2 = connect(this, &ContentListView::clicked, this, &ContentListView::itemClicked);
    Q_ASSERT(successClick2);
}

void ContentListView::itemClicked(const QModelIndex &index)
{
    qDebug() << index;
}

void ContentListView::itemDoubleClicked(const QModelIndex &index)
{
    qDebug() << index;
}
