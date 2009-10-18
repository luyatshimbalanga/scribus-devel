/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
 
#ifndef SCCOLORTRANSFORMDATA_H
#define SCCOLORTRANSFORMDATA_H

#include <QByteArray>
#include <QString>

#include "icc34.h" //part of lcms1
#include "sccolormngtelem.h"
#include "sccolormngtstructs.h"

class ScColorTransformData  : public ScColorMngtElem
{
protected:
	ScColorTransformInfo m_transformInfo;

public:
	inline const ScColorTransformInfo& transformInfo() const { return m_transformInfo; }
	inline void  setTransformInfo(const ScColorTransformInfo& info) { m_transformInfo = info; }
	
	virtual bool isNull() const = 0;

	virtual bool apply(void* input, void* output, uint numElem) = 0;
	virtual bool apply(QByteArray& input, QByteArray& output, uint numElem) = 0;

	virtual void changeBufferFormat(eColorFormat input, eColorFormat output) = 0;
};

#endif
