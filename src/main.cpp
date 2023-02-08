#include "moemainwindow.h"
#include <QApplication>
#include <QStyleFactory>
#include <QtDebug>

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
#ifndef DISABLE_C_LC_NUMERIC
	setlocale(LC_NUMERIC, "C");
#endif
	qDebug() << "styledebug: available styles: " << QStyleFactory::keys();
	if (QStyleFactory::keys().contains(QString("Fusion")))
	{
		qDebug() << "styledebug: found fusion, using fusion style";
		a.setStyle(QStyleFactory::create("Fusion"));
	}
	else {
		qDebug() << "styledebug: fusion not found, using default style";
	}

	QFile file(":/dark/stylesheet.qss");
	file.open(QFile::ReadOnly | QFile::Text);
	QTextStream stream(&file);
	a.setStyleSheet(stream.readAll());

	MoeMainWindow w;
	w.show();
	return a.exec();
}
