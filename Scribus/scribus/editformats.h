#ifndef STILFORMATE_H
#define STILFORMATE_H

#include <qdialog.h>
#include <qlistbox.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qtooltip.h>
#include "scribusdoc.h"

class StilFormate : public QDialog
{ 
    Q_OBJECT

public:
    StilFormate( QWidget* parent, ScribusDoc *doc, SCFonts *avail);
    ~StilFormate() {};

    QListBox* ListBox1;
    QPushButton* NewB;
    QPushButton* EditB;
    QPushButton* DublicateB;
    QPushButton* DeleteB;
    QPushButton* SaveB;
    QPushButton* CancelB;
    int sFnumber;
  	QValueList<StVorL> TempVorl;
    SCFonts *fon;
		ScribusDoc *Docu;
  	void UpdateFList();

public slots:
		void selFormat(QListBoxItem *c);
		void editFormat();
		void neuesFormat();
		void dupFormat();
		void deleteFormat();

protected:
    QHBoxLayout* StilFormateLayout;
    QVBoxLayout* Layout15;
};

#endif // STILFORMATE_H
