#ifndef MOEPREVIEWLABEL_H
#define MOEPREVIEWLABEL_H

#include <QLabel>
#include <QWidget>

class MoePreviewLabel : public QLabel
{
	Q_OBJECT
public:
	explicit MoePreviewLabel(QWidget* parent = nullptr);
	~MoePreviewLabel();

	void resizeEvent(QResizeEvent* event) override;
	void setInternalPixmap(QPixmap map);

	void setPixmapScaled(QPixmap image);
private:
	QPixmap internalPixmap;

	void updateScale();
signals:

};

#endif // MOEPREVIEWLABEL_H
