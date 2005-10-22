/***************************************************************************
                          fontcombo.cpp  -  description
                             -------------------
    begin                : Die Jun 17 2003
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
#include <qstringlist.h>
#include <qcombobox.h>
#include <qlistbox.h>
#include <qfont.h>
#include <qpainter.h>

#include "sccombobox.h"

#include "scribusdoc.h"
#include "fontcombo.h"
#include "fontcombo.moc"
#include "page.h"
#include "prefsmanager.h"

FontListItem::FontListItem(QComboBox* parent, QString f, QFont fo) : QListBoxItem(parent->listBox())
{
	fontName = f;
	iFont = fo;
	setText(fontName);
}

const int FontListItem::width(const QListBox *listbox)
{
	return listbox->fontMetrics().width(text()) + 2;
}

const int FontListItem::height(const QListBox *listbox)
{
	QFontMetrics fontMetrics(listbox->fontMetrics());
	return fontMetrics.lineSpacing() + 2;
}

void FontListItem::paint(QPainter *painter)
{
	painter->setFont(iFont);
	QFontMetrics fontMetrics(painter->fontMetrics());
	painter->drawText(3, fontMetrics.ascent() + fontMetrics.leading() / 2, fontName);
}

FontCombo::FontCombo(QWidget* pa) : QComboBox(true, pa)
{
	prefsManager = PrefsManager::instance();
	setEditable(false);
	QFont f(font());
	f.setPointSize(f.pointSize()-1);
	setFont(f);
	QStringList rlist;
	rlist.clear();
	SCFontsIterator it(prefsManager->appPrefs.AvailFonts);
	for ( ; it.current(); ++it)
	{
		if (it.current()->UseFont)
			rlist.append(it.currentKey());
	}
	rlist.sort();
	clear();
	for (QStringList::ConstIterator it2 = rlist.begin(); it2 != rlist.end(); ++it2)
		insertItem(*it2);
	listBox()->setMinimumWidth(listBox()->maxItemWidth()+24);
}

void FontCombo::RebuildList(ScribusDoc *currentDoc)
{
	QStringList rlist;
	clear();
	rlist.clear();
	SCFontsIterator it(prefsManager->appPrefs.AvailFonts);
	for ( ; it.current(); ++it)
	{
		if (it.current()->UseFont)
		{
			if (currentDoc != NULL)
			{
				if ((currentDoc->DocName == it.current()->PrivateFont) || (it.current()->PrivateFont.isEmpty()))
					rlist.append(it.currentKey());
			}
			else
				rlist.append(it.currentKey());
			}
	}
	rlist.sort();
	for (QStringList::ConstIterator it2 = rlist.begin(); it2 != rlist.end(); ++it2)
		insertItem(*it2);
	listBox()->setMinimumWidth(listBox()->maxItemWidth()+24);
}

FontComboH::FontComboH(QWidget* parent) : QWidget(parent, "FontComboH")
{
	prefsManager = PrefsManager::instance();
	currDoc = 0;
	fontComboLayout = new QVBoxLayout( this, 0, 0, "fontComboLayout");
	fontFamily = new ScComboBox( false, this, "fontFamily" );
	fontComboLayout->addWidget(fontFamily);
	fontStyle = new ScComboBox( false, this, "fontStyle" );
	fontComboLayout->addWidget(fontStyle);
	QStringList flist = prefsManager->appPrefs.AvailFonts.fontMap.keys();
	fontFamily->insertStringList(flist);
	fontStyle->clear();
	QStringList slist = prefsManager->appPrefs.AvailFonts.fontMap[fontFamily->currentText()];
	slist.sort();
	fontStyle->insertStringList(slist);
	connect(fontFamily, SIGNAL(activated(int)), this, SLOT(familySelected(int)));
	connect(fontStyle, SIGNAL(activated(int)), this, SLOT(styleSelected(int)));
}

void FontComboH::familySelected(int id)
{
	disconnect(fontStyle, SIGNAL(activated(int)), this, SLOT(styleSelected(int)));
	QString curr = fontStyle->currentText();
	fontStyle->clear();
	QStringList slist = prefsManager->appPrefs.AvailFonts.fontMap[fontFamily->text(id)];
	slist.sort();
	fontStyle->insertStringList(slist);
	if (slist.contains(curr))
		fontStyle->setCurrentText(curr);
	emit fontSelected(fontFamily->text(id) + " " + fontStyle->currentText());
	connect(fontStyle, SIGNAL(activated(int)), this, SLOT(styleSelected(int)));
}

void FontComboH::styleSelected(int id)
{
	emit fontSelected(fontFamily->currentText() + " " + fontStyle->text(id));
}

QString FontComboH::currentFont()
{
	return fontFamily->currentText() + " " + fontStyle->currentText();
}

void FontComboH::setCurrentFont(QString f)
{
	disconnect(fontFamily, SIGNAL(activated(int)), this, SLOT(familySelected(int)));
	disconnect(fontStyle, SIGNAL(activated(int)), this, SLOT(styleSelected(int)));
	QString family = prefsManager->appPrefs.AvailFonts[f]->family();
	QString style = prefsManager->appPrefs.AvailFonts[f]->style();
	fontFamily->setCurrentText(family);
	fontStyle->clear();
	QStringList slist = prefsManager->appPrefs.AvailFonts.fontMap[family];
	slist.sort();
	QStringList ilist;
	ilist.clear();
	if (currDoc != NULL)
	{
		for (QStringList::ConstIterator it3 = slist.begin(); it3 != slist.end(); ++it3)
		{
			if ((currDoc->DocName == prefsManager->appPrefs.AvailFonts[family + " " + *it3]->PrivateFont) || (prefsManager->appPrefs.AvailFonts[family + " " + *it3]->PrivateFont.isEmpty()))
				ilist.append(*it3);
		}
		fontStyle->insertStringList(ilist);
	}
	else
		fontStyle->insertStringList(slist);
	fontStyle->setCurrentText(style);
	connect(fontFamily, SIGNAL(activated(int)), this, SLOT(familySelected(int)));
	connect(fontStyle, SIGNAL(activated(int)), this, SLOT(styleSelected(int)));
}

void FontComboH::RebuildList(ScribusDoc *currentDoc)
{
	currDoc = currentDoc;
	disconnect(fontFamily, SIGNAL(activated(int)), this, SLOT(familySelected(int)));
	disconnect(fontStyle, SIGNAL(activated(int)), this, SLOT(styleSelected(int)));
	fontFamily->clear();
	fontStyle->clear();
	QStringList rlist = prefsManager->appPrefs.AvailFonts.fontMap.keys();
	QStringList flist;
	flist.clear();
	for (QStringList::ConstIterator it2 = rlist.begin(); it2 != rlist.end(); ++it2)
	{
		if (currentDoc != NULL)
		{
			QStringList slist = prefsManager->appPrefs.AvailFonts.fontMap[*it2];
			slist.sort();
			QStringList ilist;
			ilist.clear();
			for (QStringList::ConstIterator it3 = slist.begin(); it3 != slist.end(); ++it3)
			{
				if ((currentDoc->DocName == prefsManager->appPrefs.AvailFonts[*it2 + " " + *it3]->PrivateFont) || (prefsManager->appPrefs.AvailFonts[*it2 + " " + *it3]->PrivateFont.isEmpty()))
					ilist.append(*it3);
			}
			if (!ilist.isEmpty())
				flist.append(*it2);
		}
		else
			flist.append(*it2);
	}
	fontFamily->insertStringList(flist);
	QString family = fontFamily->currentText();
	QStringList slist = prefsManager->appPrefs.AvailFonts.fontMap[family];
	slist.sort();
	QStringList ilist;
	ilist.clear();
	if (currentDoc != NULL)
	{
		for (QStringList::ConstIterator it3 = slist.begin(); it3 != slist.end(); ++it3)
		{
			if ((currentDoc->DocName == prefsManager->appPrefs.AvailFonts[family + " " + *it3]->PrivateFont) || (prefsManager->appPrefs.AvailFonts[family + " " + *it3]->PrivateFont.isEmpty()))
				ilist.append(*it3);
		}
		fontStyle->insertStringList(ilist);
	}
	else
		fontStyle->insertStringList(slist);
	connect(fontFamily, SIGNAL(activated(int)), this, SLOT(familySelected(int)));
	connect(fontStyle, SIGNAL(activated(int)), this, SLOT(styleSelected(int)));
}
