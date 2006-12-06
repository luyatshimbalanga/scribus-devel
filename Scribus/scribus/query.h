/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#ifndef QUERY_H
#define QUERY_H

#include <qdialog.h>
class QLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QString;
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class MSpinBox;
class LinkButton;

#include "scribusapi.h"

class SCRIBUS_API Query : public QDialog
{
	Q_OBJECT

public:
	Query( QWidget* parent=0, const char* name=0, bool modal = FALSE, WFlags fl=0, QString text=0,
	       QString titel=0 );
	~Query() {};

	const QString getEditText();
	void setEditText(QString newText, bool setSelected);

public slots:
	void Leave();

private:
	QVBoxLayout* queryLayout;
	QHBoxLayout* editLayout;
	QHBoxLayout* okCancelLayout;
	QPushButton* okButton;
	QPushButton* cancelButton;
	QLineEdit* answerEdit;
	QLabel* questionLabel;
};

class SCRIBUS_API QuerySize : public QDialog
{
	Q_OBJECT

public:
	QuerySize( QWidget* parent, QString titel, int unitIndex, double defW, double defH );
	~QuerySize() {};
	MSpinBox *spinWidth;
	MSpinBox *spinHeight;
	LinkButton* linkSize;

public slots:
	void ToggleKette();
	void changeSizesH();
	void changeSizesW();

private:
	QVBoxLayout* queryLayout;
	QGridLayout* editLayout;
	QHBoxLayout* okCancelLayout;
	QPushButton* okButton;
	QPushButton* cancelButton;
	QLabel* questionLabel;
	QLabel* questionLabel2;
};

#endif // QUERY_H
