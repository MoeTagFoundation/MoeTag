#ifndef CONTENTLISTVIEW_H
#define CONTENTLISTVIEW_H

#include <QListView>

class ContentListView : public QListView
{
public:
    ContentListView();
public slots:
    void itemClicked(const QModelIndex& index);
    void itemDoubleClicked(const QModelIndex& index);
};

#endif // CONTENTLISTVIEW_H
