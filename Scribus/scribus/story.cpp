/***************************************************************************
                          story.cpp  -  description
                             -------------------
    begin                : Tue Nov 11 2003
    copyright            : (C) 2003 by Franz Schmid
    email                : Franz.Schmid@altmuehlnet.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "story.h"
#include "story.moc"
#include <qlayout.h>
#include <qtooltip.h>
#include <qpixmap.h>
#include <qcombobox.h>
#include <qmessagebox.h>
#include "serializer.h"
extern QPixmap loadIcon(QString nam);


SEditor::SEditor(QWidget* parent) : QTextEdit(parent)
{
	clines = 0;
	setFrameStyle( QFrame::NoFrame | QFrame::Plain );
	setUndoRedoEnabled(true);
	setUndoDepth(50);
//	setTextFormat(Qt::RichText);
}

void SEditor::focusInEvent(QFocusEvent *f)
{
	bool u = isUndoAvailable();
	bool r = isRedoAvailable();
	emit UnRe(u, r);
	QTextEdit::focusInEvent(f);	
}

void SEditor::keyPressEvent(QKeyEvent *k)
{
	int p, i;
	getCursorPosition(&p, &i);
	if ((k->key() == Key_Backspace) && (i == 0))
		{
		emit bsPressed();
		return;
		}
	if ((k->key() == Key_Delete) && (i == static_cast<int>(text().length())))
		{
		emit delPressed();
		return;
		}
	QTextEdit::keyPressEvent(k);
	if (clines != lines())
		emit wrapped();
	clines = lines();
	bool u = isUndoAvailable();
	bool r = isRedoAvailable();
	emit UnRe(u, r);
	ensureCursorVisible();
}


STable::STable(QWidget* parent) : QTable(parent)
{
	setShowGrid(false);
	HomeK = 0;
	EndK = 0;
}

void STable::keyPressEvent(QKeyEvent *k)
{
	QString tmp;
	int p, i, l, r, c, n;
	SEditor *tt;
	n = numRows();
	r = currentRow();
	c = currentColumn();
	if (c == 1)
		{
		tt = (SEditor*)cellWidget(r, 1);
		if ((k->key() == Key_Prior) || (k->key() == Key_Next))
			{
			HomeK = 0;
			EndK = 0;
			return;
			}
		if ((k->key() == Key_Left)
				|| (k->key() == Key_Right)
				|| (k->key() == Key_Down)
				|| (k->key() == Key_Up)
				|| (k->key() == Key_Home)
				|| (k->key() == Key_End))
			{
			QTextEdit::CursorAction move;
			tt->getCursorPosition(&p, &i);
			switch (k->key())
				{
				case Key_Home:
					EndK = 0;
					HomeK++;
					switch (HomeK)
						{
						case 1:
							move = QTextEdit::MoveLineStart;
							break;
						case 2:
							move = QTextEdit::MoveHome;
							break;
						case 3:
							tt = (SEditor*)cellWidget(0, 1);
							setCurrentCell(0, 1);
							move = QTextEdit::MoveHome;
							HomeK = 0;
							break;
						}
					break;
				case Key_End:
					HomeK = 0;
					EndK++;
					switch (EndK)
						{
						case 1:
							move = QTextEdit::MoveLineEnd;
							break;
						case 2:
							move = QTextEdit::MoveEnd;
							break;
						case 3:
							tt = (SEditor*)cellWidget(n-1, 1);
							setCurrentCell(n-1, 1);
							move = QTextEdit::MoveEnd;
							EndK = 0;
							break;
						}
					break;
				case Key_Left:
					if ((i == 0) && (r > 0))
						{
						tt = (SEditor*)cellWidget(r-1, 1);
						setCurrentCell(r-1, 1);
						move = QTextEdit::MoveEnd;
						}
					else
						move = QTextEdit::MoveBackward;
					break;
				case Key_Right:
					if ((i == static_cast<int>(tt->text().length())) && (r < n-1))
						{
						tt = (SEditor*)cellWidget(r+1, 1);
						setCurrentCell(r+1, 1);
						move = QTextEdit::MoveLineStart;
						}
					else
						move = QTextEdit::MoveForward;
					break;
				case Key_Up:
					l = tt->lineOfChar(0, i);
					if ((l == 0) && (r > 0))
						{
						tt = (SEditor*)cellWidget(r-1, 1);
						setCurrentCell(r-1, 1);
						move = QTextEdit::MoveEnd;
						}
					else
						move = QTextEdit::MoveUp;
					break;
				case Key_Down:
					l = tt->lineOfChar(0, i);
					if ((l == tt->lines()-1) && (r < n-1))
						{
						tt = (SEditor*)cellWidget(r+1, 1);
						setCurrentCell(r+1, 1);
						move = QTextEdit::MoveLineStart;
						}
					else
						move = QTextEdit::MoveDown;
					break;
				}
			if (k->key() != Key_Home)
				HomeK = 0;
			if (k->key() != Key_End)
				EndK = 0;
			tt->moveCursor(move, false);
			tt->setFocus();
			updateHeaderStates(); 
			return;
			}
		}
	HomeK = 0;
	EndK = 0;
	QTable::keyPressEvent(k);
}

void STable::adjHeight(int r)
{
	SEditor *cp = (SEditor*)cellWidget(r, 1);
	cp->sync();
	QFontMetrics fm2(cp->currentFont());
	setRowHeight(r, QMAX((fm2.lineSpacing() * (cp->lines()+1)), 24));
	updateHeaderStates();;
}

StoryEditor::StoryEditor(QWidget* parent, ScribusDoc *docc, PageItem *ite) : QDialog(parent, "StoryEditor", true, 0)
{
  uint a;
	int para = 0;
	int pstyle;
	doc = docc;
  QString Dat = "";
	setCaption( tr( "Story Editor" ) );
	setIcon(loadIcon("AppIcon.png"));
	Form1Layout = new QHBoxLayout( this, 5, 5, "Form1Layout"); 
	edList.clear();
	stList.clear();
	style.append( tr("Left"));
	style.append( tr("Center"));
	style.append( tr("Right"));
	style.append( tr("Block"));
	style.append( tr("Forced"));
	if (doc->Vorlagen.count() > 5)
		{
		for (uint a = 5; a < doc->Vorlagen.count(); ++a)
			{
			style.append(doc->Vorlagen[a].Vname);
			}
		}
	CurrItem = ite;

 	fmenu = new QPopupMenu();
 	fmenu->insertItem(loadIcon("DateiNeu16.png"), tr("New"), this, SLOT(Do_new()), CTRL+Key_N);
/*  	fmenu->insertItem( tr("Save as..."), this, SLOT(SaveAs()));
  	fmenu->insertItem(loadIcon("DateiOpen16.png"), tr("Load..."), this, SLOT(LoadScript()));
		fmenu->insertSeparator();      */
	fmenu->insertItem( tr("Save and Exit"), this, SLOT(accept()));
	fmenu->insertItem( tr("Exit without Saving"), this, SLOT(Do_leave()));
 	emenu = new QPopupMenu();
 	Mundo = emenu->insertItem( tr("Undo"), this, SLOT(Do_undo()), CTRL+Key_Z);
 	Mredo = emenu->insertItem( tr("Redo"), this, SLOT(Do_redo()));
	emenu->insertSeparator();
	Mcopy = emenu->insertItem(loadIcon("editcut.png"), tr("Cut"), this, SLOT(Do_cut()), CTRL+Key_X);
	Mcut = emenu->insertItem(loadIcon("editcopy.png"), tr("Copy"), this, SLOT(Do_copy()), CTRL+Key_C);
	Mpaste = emenu->insertItem(loadIcon("editpaste.png"), tr("Paste"), this, SLOT(Do_paste()), CTRL+Key_V);
	Mdel = emenu->insertItem(loadIcon("editdelete.png"), tr("Clear"), this, SLOT(Do_del()), CTRL+Key_V);
	emenu->insertSeparator();
	emenu->insertItem( tr("Edit Styles..."), this , SLOT(slotEditStyles()));
	Mupdt = emenu->insertItem( tr("Update Textframe"), this, SLOT(updateTextFrame()));
 	menuBar = new QMenuBar(this);
	menuBar->insertItem( tr("File"), fmenu);
	menuBar->insertItem( tr("Edit"), emenu);
	Form1Layout->setMenuBar( menuBar );
	table1 = new STable( this );
	table1->setNumCols( 2 );
	table1->horizontalHeader()->setLabel( 0, tr( "Style" ) );
	table1->horizontalHeader()->setLabel( 1, tr( "Text" ) );
	table1->setSelectionMode( QTable::Single );
	table1->setColumnStretchable(0, false);
	table1->setColumnStretchable(1, true);
	Form1Layout->addWidget( table1 );
	resize( QSize(509, 326).expandedTo(minimumSizeHint()) );
	show();
	QPtrList<Pti> y;
	PageItem *nb = ite;
	while (nb != 0)
		{
		if (nb->BackBox != 0)
			nb = nb->BackBox;
		else
			break;
		}
	while (nb != 0)
		{
		y = nb->Ptext;
  	for (a = 0; a < y.count(); ++a)
  		{
			QString b = y.at(a)->ch;
			pstyle = y.at(a)->cab;
			if (b == QChar(13))
				{
				addPar(para, Dat, pstyle);
				Dat = "";
				para++;
				}
			else
    		Dat += b;
    	}
		nb = nb->NextBox;
		}
	if (Dat != "")
		addPar(para, Dat, pstyle);
	if (table1->numRows() == 0)
		addPar(0, "", doc->CurrentABStil);
	TextChanged = false;
	table1->setCurrentCell(0, 1);
	table1->ensureVisible(0, 1);
	SEditor *cp = (SEditor*)table1->cellWidget(0, 1);
	cp->setFocus();
	cp->setCursorPosition(0, 0);
	for (uint a = 0; a < edList.count(); ++a)
		{
		SEditor *tt = edList.at(a);
		tt->setUndoRedoEnabled(false);
		tt->setUndoRedoEnabled(true);
		}
	emenu->setItemEnabled(Mundo, 0);
	emenu->setItemEnabled(Mredo, 0);
	emenu->setItemEnabled(Mcopy, 0);
	emenu->setItemEnabled(Mcut, 0);
	emenu->setItemEnabled(Mdel, 0);
	emenu->setItemEnabled(Mupdt, 0);
}

void StoryEditor::closeEvent(QCloseEvent *)
{
	Do_leave();
}

void StoryEditor::Do_leave()
{
	if (TextChanged)
		{
		int t = QMessageBox::warning(this, tr("Warning"),
  														 	tr("Do you really want to lose all your Changes?"),
                         			 	QMessageBox::No, QMessageBox::Yes, QMessageBox::NoButton);
  	if (t == QMessageBox::No)
			return;
		}
	reject();
}

void StoryEditor::Do_new()
{
	int t = QMessageBox::warning(this, tr("Warning"),
  														 tr("Do you really want to clear all your Text?"),
                         			 QMessageBox::No, QMessageBox::Yes, QMessageBox::NoButton);
  if (t == QMessageBox::No)
		return;
	table1->setNumCols( 2 );
	table1->setNumRows( 0 );
	stList.clear();
	edList.clear();
	addPar(0, "", doc->CurrentABStil);
	SEditor *cp = (SEditor*)table1->cellWidget(0, 1);
	cp->setUndoRedoEnabled(false);
	cp->setUndoRedoEnabled(true);
	cp->setFocus();
	cp->setCursorPosition(0, 0);
	emenu->setItemEnabled(Mundo, 0);
	emenu->setItemEnabled(Mredo, 0);
	emenu->setItemEnabled(Mcopy, 0);
	emenu->setItemEnabled(Mcut, 0);
	emenu->setItemEnabled(Mdel, 0);
}

void StoryEditor::Do_undo()
{
	SEditor *cp = (SEditor*)table1->cellWidget(table1->currentRow(), 1);
	cp->undo();
	table1->adjHeight(table1->currentRow());
}

void StoryEditor::Do_redo()
{
	SEditor *cp = (SEditor*)table1->cellWidget(table1->currentRow(), 1);
	cp->redo();
	table1->adjHeight(table1->currentRow());
}

void StoryEditor::Do_copy()
{
	SEditor *cp = (SEditor*)table1->cellWidget(table1->currentRow(), 1);
	cp->copy();
	table1->adjHeight(table1->currentRow());
}

void StoryEditor::Do_paste()
{
	SEditor *cp = (SEditor*)table1->cellWidget(table1->currentRow(), 1);
	cp->paste();
	table1->adjHeight(table1->currentRow());
}

void StoryEditor::Do_cut()
{
	SEditor *cp = (SEditor*)table1->cellWidget(table1->currentRow(), 1);
	cp->cut();
	table1->adjHeight(table1->currentRow());
}

void StoryEditor::Do_del()
{
	SEditor *cp = (SEditor*)table1->cellWidget(table1->currentRow(), 1);
	cp->del();
	table1->adjHeight(table1->currentRow());
}

void StoryEditor::UnReMenu(bool u, bool r)
{
	emenu->setItemEnabled(Mundo, u);
	emenu->setItemEnabled(Mredo, r);
}

void StoryEditor::CopyAvail(bool u)
{
	emenu->setItemEnabled(Mcopy, u);
	emenu->setItemEnabled(Mcut, u);
	emenu->setItemEnabled(Mdel, u);
}

void StoryEditor::updateTextFrame()
{
	bool first = false;
	PageItem *nb = CurrItem;
	while (nb != 0)
		{
		if (nb->BackBox != 0)
			nb = nb->BackBox;
		else
			break;
		}
	for (uint a = 0; a < edList.count(); ++a)
		{
		Serializer *ss = new Serializer("");
		SEditor *tt = edList.at(a);
		QComboBox *cp = stList.at(a);
		ss->Objekt = tt->text();
		if (a < edList.count()-1)
			ss->Objekt += QChar(10);
		int st = cp->currentItem();
		ss->GetText(nb, st, doc->Vorlagen[st].Font, doc->Vorlagen[st].FontSize, first);
		delete ss;
		first = true;
		}
	while (nb != 0)
		{
		if (doc->Trenner->AutoCheck)
			{
			if (doc->Trenner->Language != nb->Language)
				doc->Trenner->slotNewDict(nb->Language);
			doc->Trenner->slotHyphenate(nb);
			}
		else
			nb->OwnPage->RefreshItem(nb);
		nb = nb->NextBox;
		}
	TextChanged = false;
	emenu->setItemEnabled(Mupdt, 0);
	emit DocChanged();
}

void StoryEditor::slotEditStyles()
{
	int sty;
	QComboBox *ct;
	emit EditSt();
	style.clear();
	style.append( tr("Left"));
	style.append( tr("Center"));
	style.append( tr("Right"));
	style.append( tr("Block"));
	style.append( tr("Forced"));
	if (doc->Vorlagen.count() > 5)
		{
		for (uint a = 5; a < doc->Vorlagen.count(); ++a)
			{
			style.append(doc->Vorlagen[a].Vname);
			}
		}
	for (uint b = 0; b < stList.count(); ++b)
		{
		ct = stList.at(b);
		sty = ct->currentItem();
		if (sty > static_cast<int>(doc->Vorlagen.count()-1))
			sty = 0;
		disconnect(ct, SIGNAL(activated(int)), this, SLOT(styleChange(int)));
		ct->clear();
		ct->insertStringList(style);
		connect(ct, SIGNAL(highlighted(int)), this, SLOT(styleChange(int)));
		ct->setCurrentItem(sty);
		disconnect(ct, SIGNAL(highlighted(int)), this, SLOT(styleChange(int)));
		connect(ct, SIGNAL(activated(int)), this, SLOT(styleChange(int)));
		}
}

void StoryEditor::styleChange(int st)
{
	int r = stList.findRef((QComboBox*)sender());
	int align;
	if (st > 4)
		align = doc->Vorlagen[st].Ausri;
	else
		align = st;
	if (r != -1)
		{
		SEditor *tt = (SEditor*)table1->cellWidget(r, 1);
		switch (align)
			{
			case 0:
				tt->setAlignment(Qt::AlignLeft);
				break;
			case 1:
				tt->setAlignment(Qt::AlignCenter);
				break;
			case 2:
				tt->setAlignment(Qt::AlignRight);
				break;
			case 3:
			case 4:
				tt->setAlignment(Qt::AlignJustify);
				break;
			default:
				break;
			}
		modifiedText();
		}
}

int StoryEditor::getStyle(int where)
{
	QComboBox *cp = stList.at(where);
	return cp->currentItem();
}

void StoryEditor::addPar(int where, QString text, int sty)
{
	table1->insertRows(where);
	QComboBox *ct = new QComboBox( this );
	ct->insertStringList(style);
	ct->setMaximumSize(200, 24);
	ct->setEditable(false);
  table1->setCellWidget(where, 0, ct);
	SEditor *cp = new SEditor(this);
	stList.insert(where, ct);
	edList.insert(where, cp);
 	table1->setCellWidget(where, 1, cp);
	table1->setCurrentCell(where, 1);
  cp->setText(text);
	table1->adjHeight(where);
	connect(ct, SIGNAL(highlighted(int)), this, SLOT(styleChange(int)));
	ct->setCurrentItem(sty);
	disconnect(ct, SIGNAL(highlighted(int)), this, SLOT(styleChange(int)));
	cp->setFocus();
	cp->setCursorPosition(0, 0);
	connect(cp, SIGNAL(copyAvailable(bool)), this, SLOT(CopyAvail(bool)));
	connect(cp, SIGNAL(UnRe(bool, bool)), this, SLOT(UnReMenu(bool, bool)));
	connect(cp, SIGNAL(wrapped()), this, SLOT(WrapHandler()));
	connect(cp, SIGNAL(delPressed()), this, SLOT(KeyDel()));
	connect(cp, SIGNAL(bsPressed()), this, SLOT(KeyBS()));
	connect(cp, SIGNAL(returnPressed()), this, SLOT(KeyRet()));
	connect(cp, SIGNAL(clicked(int, int)), this, SLOT(clickAt(int, int)));
	connect(cp, SIGNAL(textChanged()), this, SLOT(modifiedText()));
	connect(ct, SIGNAL(activated(int)), this, SLOT(styleChange(int)));
}

void StoryEditor::modifiedText()
{
	TextChanged = true;
	table1->HomeK = 0;
	table1->EndK = 0;
	emenu->setItemEnabled(Mupdt, 1);
}

void StoryEditor::WrapHandler()
{
	int r = edList.findRef((SEditor*)sender());
	if (r != -1)
		table1->adjHeight(r);
}

void StoryEditor::clickAt( int row, int col)
{
	int r = edList.findRef((SEditor*)sender());
	if (r != -1)
		{
		table1->setCurrentCell(r, 1);
		table1->cellWidget(r, 1)->setFocus();
		table1->updateHeaderStates();
		}
}

void StoryEditor::KeyDel()
{
	QString tmp, tmp2;
	int r = table1->currentRow();
	SEditor *tt = (SEditor*)sender();
	tmp = tt->text();
	if (r < table1->numRows()-1)
		{
		int al = tt->alignment();
		SEditor *bt = edList.at(r+1);
		tmp2 = bt->text();
		tt->setText(tmp + tmp2);
		tt->setAlignment(al);
		edList.remove(r+1);
		stList.remove(r+1);
		table1->removeRow(r+1);
		table1->setCurrentCell(r, 1);
		table1->adjHeight(r);
		tt->setFocus();
		tt->setCursorPosition(0, tmp.length());
		}
}

void StoryEditor::KeyBS()
{
	QString tmp, tmp2;
	int r = table1->currentRow();
	SEditor *tt = (SEditor*)sender();
	tmp = tt->text();
	if (r > 0)
		{
		SEditor *bt = edList.at(r-1);
		int al = bt->alignment();
		tmp2 = bt->text();
		bt->setText(tmp2 + tmp);
		bt->setAlignment(al);
		edList.remove(r);
		stList.remove(r);
		table1->removeRow(r);
		table1->setCurrentCell(r-1, 1);
		table1->adjHeight(r-1);
		bt->setFocus();
		bt->setCursorPosition(0, tmp2.length());
		}
}

void StoryEditor::KeyRet()
{
	QString tmp, tmp2;
	QString tmp3 = "";
	SEditor *tt = (SEditor*)sender();
	tmp = tt->text();
	int al = tt->alignment();
	int st = getStyle(table1->currentRow());
	int rPos = tmp.find("\n");
	if (CurrItem->PType == 8)
		{
		tmp.remove("\n");
		tt->setText(tmp);
		tt->setAlignment(al);
		return;
		}
	if (rPos < static_cast<int>(tmp.length()))
		{
		tmp2 = tmp.left(rPos);
		tmp3 = tmp.mid(rPos+1);
		tt->setText(tmp2);
		}
	else
		{
		tmp.remove("\n");
		tt->setText(tmp);
		}
	tt->setAlignment(al);
	table1->adjHeight(table1->currentRow());
	addPar(table1->currentRow()+1, tmp3, st);
}

