/***************************************************************************
	begin                : 2005
	copyright            : (C) 2005 by Craig Bradney
	email                : cbradney@zip.com.au
***************************************************************************/

/***************************************************************************
*                                                                         *
*   ScApp program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#ifndef TOCINDEXPREFS_H
#define TOCINDEXPREFS_H

#include <qstringlist.h>

#include "tocindexprefsbase.h"
#include "page.h"
#include "pagestructs.h"
#include "scribusdoc.h"

class TOCIndexPrefs : public TOCIndexPrefsBase
{
    Q_OBJECT

public:
    TOCIndexPrefs( QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
    ~TOCIndexPrefs();

    virtual void enableGUIWidgets();
    virtual ToCSetupVector * getNewToCs();

public slots:
    virtual void setup( ToCSetupVector * tocsetups, ScribusDoc * doc );
    virtual void generatePageItemList();
    virtual void setupItemAttrs( QStringList newNames );
    virtual void selectToC( int numberSelected );
    virtual void addToC();
    virtual void updateToCListBox();
    virtual void updateParagraphStyleComboBox();
    virtual void deleteToC();
    virtual void itemAttributeSelected( const QString & itemAttributeName );
    virtual void itemFrameSelected( const QString & frameName );
    virtual void itemPageNumberPlacedSelected( const QString & pageLocation );
    virtual void itemParagraphStyleSelected( const QString & itemStyle );
    virtual void setToCName( const QString & newName );
    virtual void nonPrintingFramesSelected( bool showNonPrinting );

protected:
    uint numSelected;
    QString strPNNotShown;
    QString strPNEnd;
    QString strPNBeginning;
    QString trStrPNNotShown;
    QString trStrPNEnd;
    QString trStrPNBeginning;
    ToCSetupVector localToCSetupVector;
    QString trStrNone;
    ScribusDoc *currDoc;
    QString selectedTOCAttrName;
    QStringList paragraphStyleList;
    QString strNone;

protected slots:
    virtual void languageChange();

private:
    void init();
    void destroy();

};

#endif // TOCINDEXPREFS_H
