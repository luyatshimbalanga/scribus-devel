/***************************************************************************
 *   Copyright (C) 2004 by Riku Leino                                      *
 *   tsoots@welho.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef GTSTYLE_H
#define GTSTYLE_H

#include <qstring.h>
#include "gtfont.h"

class gtStyle 
{
protected:
	QString name;
	gtFont font;
public:
	gtStyle(QString name);
	gtStyle(const gtStyle& s);
	void setFont(gtFont newFont);
	gtFont* getFont();
	QString getName();
	void setName(QString newName);
	virtual ~gtStyle();
	virtual QString target();
};

#endif // STYLE_H
