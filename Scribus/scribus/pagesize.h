/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
/***************************************************************************
    begin                : Feb 2005
    copyright            : (C) 2005 by Craig Bradney
    email                : cbradney@zip.com.au
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 
#ifndef PAGESIZE_H
#define PAGESIZE_H

#include <qmap.h>
#include <qstring.h>
#include "scribusapi.h"

struct PageSizeInfo
{
	double width;
	double height;
	QString trSizeName;
	int pageUnitIndex;
};

typedef QMap<QString, PageSizeInfo > PageSizeInfoMap;

class SCRIBUS_API PageSize
{
public:
	PageSize(const QString);
	PageSize(const double, const double);
	
	QString name();
	QString nameTR();
	double width();
	double height();
	double originalWidth();
	double originalHeight();
	QStringList sizeList();
	QStringList sizeTRList();
	void generateSizeList();
	void printSizeList();

private:
	QMap<QString, PageSizeInfo > pageSizeList;
	double m_Width;
	double m_Height;
	int pageUnitIndex;
	QString pageSizeName;
	QString trPageSizeName;
};

#endif

