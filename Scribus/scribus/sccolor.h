/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
/***************************************************************************
                          sccolor.h  -  description
                             -------------------
    begin                : Sun Sep 9 2001
    copyright            : (C) 2001 by Franz Schmid
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

#ifndef SCCOLOR_H
#define SCCOLOR_H

#include "qstring.h"
#include <qcolor.h>
#include "qmap.h"

#include "scribusapi.h"


/**
  *@author Franz Schmid
  * \brief This Class adds support for CMYK-Colors to QT.
  * its API is based on the API of QColor
  */

/** Scribus color models */
enum colorModel
{
	colorModelRGB,
	colorModelCMYK
};

class SCRIBUS_API ScColor 
{
public:
	ScColor();
/** \brief Constructs a ScColor with 4 Components
 * in the range from 0 - 255 */
	ScColor(int c, int m, int y, int k);
/** \brief Constructs a RGBColor with 3 Components
 * in the range from 0 - 255 */
	ScColor(int r, int g, int b);
	~ScColor() {};
	
	bool operator==(const ScColor& rhs) const;
	
	/** \brief flag to enable and disable use of color management (default: true) */
	static bool UseProf;

	/** \brief Same as the Constructor but for an existing Color */
	void setColor(int c, int m, int y, int k);

	/** \brief Computes a ScColor from an RGB-Color
	* Applies Gray-Component-Removal to the resulting ScColor
	* or if color management is enabled, an approriate transform */
	void setColorRGB(int r, int g, int b);

	/** \brief set the color model */
	void setColorModel (colorModel cm);

	/** \brief get the color model */
	colorModel getColorModel ();

	/** \brief Computes a ScColor for a QColor */
	void fromQColor(QColor color);

	/** \brief Returns the transformed/proofed RGB color  */
	QColor getRGBColor();
	void getRawRGBColor(int *r, int *g, int *b);
	QColor getRawRGBColor();

	/** \brief Returns the 4 Values that form an ScColor */
	void getCMYK(int *c, int *m, int *y, int *k);

	/** \brief Returns the 3 Values that form an RGBColor */
	void getRGB(int *r, int *g, int *b);

	/** \brief get CMYK values of a specified shade */
	void getShadeColorCMYK(int *c, int *m, int *y, int *k, int level);

	/** \brief get RGB values of a specified shade */
	void getShadeColorRGB(int *r, int *g, int *b, int level);

	/** \brief Return a proofed QColor with the specified shade */
	QColor getShadeColorProof(int level);

	/** \brief Applys Gray-Component-Removal to an ScColor */
	void applyGCR();

/** \brief Returns the ScColor as an Hex-String in the Form #CCYYMMKK for
 * a CMYK color or ##RRGGBB for a RGB color*/
	QString name();
	/** \brief Returns the ScColor as an Hex-String in the Form #RRGGBB */
	QString nameRGB();
	/** \brief Returns the ScColor as an Hex-String in the Form #CCYYMMKK */
	QString nameCMYK();

/** \brief Sets the Values of a color from an Hex-String in the Form #CCMMYYKK
 * or #RRGGBB */
	void setNamedColor(QString nam);
	void RecalcRGB();
	bool isOutOfGamut();
	void checkGamut();
	bool isSpotColor();
	void setSpotColor(bool s);
	bool isRegistrationColor();
	void setRegistrationColor(bool s);

private:
	/** \brief Cyan-Component of Color */
	int C;
	
	/** \brief Magenta-Component of Color */
	int M;

	/** \brief Yellow-Component of Color */
	int Y;

	/** \brief Black-Component of Color */
	int K;

	/** \brief Red-Component of Color */
	int R;

	/** \brief Green-Component of Color */
	int G;

	/** \brief Blue-Component of Color */
	int B;

	/** \brief RGB-Equivalent including color transforms */
	QColor RGB;

	/** \brief Flag, true if the Color is a Spotcolor */
	bool Spot;

	/** \brief Flag, true if the Color is a Registration color */
	bool Regist;

	/** \brief Color model of the current color */
	colorModel Model;

	/** \brief Flag, true if out of Gamut */
	bool outOfGamutFlag;
};

/** \brief Definition of the Color list */
typedef QMap<QString,ScColor> ColorList;

#endif
