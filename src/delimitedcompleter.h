#ifndef DELIMITEDCOMPLETER_H
#define DELIMITEDCOMPLETER_H

// Based off code provided for a delimited completer in this thread
// https://stackoverflow.com/questions/21773348/how-to-force-qcompleter-to-check-second-word-in-qlineedit

#include <QCompleter>
#include <QStringList>
#include <QLineEdit>
#include <QString>


class DelimitedCompleter : public QCompleter {
	Q_OBJECT

public:
	DelimitedCompleter(QLineEdit* parent, char delimiter);
	DelimitedCompleter(QLineEdit* parent, char delimiter, QAbstractItemModel* model);
	DelimitedCompleter(QLineEdit* parent, char delimiter, const QStringList& list);
	QString pathFromIndex(const QModelIndex& index) const;
	QStringList splitPath(const QString& path) const;

private:
	char delimiter;
	mutable int cursor_pos = -1;

	void connectSignals();

private slots:
	void onActivated(const QString& text);
	void onCursorPositionChanged(int old_pos, int new_pos);
};

#endif
