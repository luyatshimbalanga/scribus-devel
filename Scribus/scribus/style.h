/*
 For general Scribus (>=1.3.2) copyright and licensing information please refer
 to the COPYING file provided with the program. Following this notice may exist
 a copyright and/or license notice that predates the release of Scribus 1.3.2
 for which a new license (GPL+exception) is in place.
 */
/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/


#ifndef STYLE_H
#define STYLE_H

#include <cassert>
#include <qstring.h>
#include <qvaluelist.h>
#include "scfonts.h"
#include "scribusapi.h"



class Style;


/**
  Style inheritance works with names. A StyleBase finds the Style to a given name.
  If there are changes to the Styles contained in a StyleBase, you have to call
  invalidate() to increase the version info. Styles using this StyleBase will then
  update their cached values the next time they are used.
*/
class StyleBase {
public:
	static const int DOC_LEVEL = 1;
	static const int STORY_LEVEL = 1024;
	static const int PAR_LEVEL = 1024 * 1024;

	StyleBase(int level = DOC_LEVEL) 
		: m_version(0), m_level(level) 
	{
		qDebug(QString("constr. %1 /%2").arg(reinterpret_cast<uint>(this),16).arg(m_level));
	}
		
	StyleBase(const StyleBase& o) 
		: m_version(o.m_version), m_level(o.m_level)
	{
		qDebug(QString("constr. cp %1 /%2").arg(reinterpret_cast<uint>(this),16).arg(m_level));
	}
	

	StyleBase& operator= (const StyleBase& o)
	{
		m_version = o.m_version;
		m_level = o.m_level;
		return *this;
	}
	
	
	virtual int version() const  { return m_version; }
	void invalidate()    { m_version += m_level; }
	
	virtual const Style* resolve(QString name) const = 0;
	virtual ~StyleBase() 
	{
		qDebug(QString("destr. %1").arg(reinterpret_cast<uint>(this),16));
	}
	

protected:
	int m_version;
	int m_level;
};



/**
 *  This as a virtual base class for all style-like objects: CharStyles, 
 *  ParagraphStyles(indirectly), LineStyles, FrameStyles, CellStyles,
 *  FlowStyles,...
 *  It provides a name and an inheritance mechanism which uses a StyleBase.
 *  Before any attribute is queried, you have to call validate(), which checks
 *  the stored m_baseversion against the StyleBase's version and updates all
 *  attributes if they are different.
 */
class Style {
protected:
	QString m_name;
	const StyleBase* m_base;
	int m_baseversion;
	QString m_parent;
public:
//	static const short NOVALUE = -16000;

	Style(): m_name(""), m_base(NULL), m_baseversion(-1), m_parent()  {}

    Style(StyleBase* b, QString n): m_name(n), m_base(b), m_baseversion(-1), m_parent()  {}
	
	Style& operator=(const Style& o) 
	{ //assert(typeinfo() == o.typeinfo()); 
		m_name = o.m_name; 
		m_base = o.m_base; 
		m_baseversion = o.m_baseversion; 
		m_parent = o.m_parent; 
		return *this;
	}
	
	Style(const Style& o) : m_name(o.m_name), 
		m_base(o.m_base), m_baseversion(o.m_baseversion), m_parent(o.m_parent) {} 
	
	virtual ~Style()                 {}

	QString name() const             { return m_name; }
	void setName(QString n)          { m_name = n; }
	bool hasName() const             { return ! m_name.isEmpty(); }

	virtual QString displayName() const { 	
		if ( hasName() || !hasParent() || !m_base)
			return name();
		//	else if ( inheritsAll() )
		//		return parent()->displayName();
		else 
			return parentStyle()->displayName();
	}
	
	QString parent() const           { return m_parent; }
	void setParent(QString p)        { m_parent = p; }
	bool hasParent() const           { return ! m_parent.isEmpty(); }
	const Style* parentStyle() const { qDebug(QString("follow %1").arg(reinterpret_cast<uint>(m_base),16));
		return m_base ? m_base->resolve(m_parent) : NULL; }
	
	
	void setBase(const StyleBase* base)  { m_base = base; m_baseversion = -1;
	 qDebug(QString("setBase of %2 base %1").arg(reinterpret_cast<uint>(m_base),16).arg(reinterpret_cast<uint>(this),16));
	}
	const StyleBase* base() const        { return m_base; }
	
	/**
		sets a new StyleBase if b is not NULL and then uses the StyleBase
		to set all inherited attributes to their valid value.
	 */
	virtual void update(const StyleBase* b = NULL) 
	{
		qDebug(QString("update %2 base %1").arg(reinterpret_cast<uint>(m_base),16).arg(reinterpret_cast<uint>(this),16));
		if (b)
			m_base = b;
		if (m_base)
			m_baseversion = m_base->version(); 
	}
	
	/**
		Checks if this Style needs an update
	 */
	void validate() const
	{ 
		qDebug(QString("validate %2 base %1").arg(reinterpret_cast<uint>(m_base),16).arg(reinterpret_cast<uint>(this),16));
		if (m_base && m_baseversion != m_base->version()) 
			const_cast<Style*>(this)->update(m_base); 
	}

	
	/**
		returns true if both Styles are of the same type, inherit the same attributes,
	    have the same parent, and agree on all attributes which are not inherited.
	    The StyleBase, the name and any inherited attrinutes may be different.
	 */
	virtual bool equiv(const Style& other) const = 0;
	/**
		returns true if both Styles are equivalent and have the same name.
	    Since the base is not tested, this does *not* ensure they will return
	    the same values for all attributes.
	 */
	virtual bool operator==(const Style& other) const { return name() == other.name() && equiv(other); }
	virtual bool operator!=(const Style& other) const { return ! ( (*this) == other ); }

	/**
		resets all attributes to their defaults and makes them inherited.
	    name and parent are not affected.
	 */
	virtual void erase() = 0;
	/**
		if other has a parent, replace this parent with the other ones
	 */
	void applyStyle(const Style& other) { 
		if (other.hasParent())
			setParent(other.parent());
	}
	/** 
		if other has the same parent, remove this parent 
	 */
	void eraseStyle(const Style& other) {
		if (other.parent() == parent())
			setParent("");
	}
};


/** This class turns a style into a new stylebase:
 *  it maps the empty name "" to the given default style and uses
 *  the style's base to resolve all other names.
 */
class StyleBaseProxy: public StyleBase 
{
public:
	const Style* resolve(QString name) const {
		if (name.isEmpty() || ! m_default->base())
			return m_default;
		else
			return m_default->base()->resolve(name);
	}
	
	StyleBaseProxy(int level, const Style* style) 
	: StyleBase(level), m_default(style) {}
	
	StyleBaseProxy(const StyleBaseProxy& other)
	: StyleBase(other), m_default(other.m_default) {}
	
	StyleBaseProxy& operator= (const StyleBaseProxy& other)
	{
		static_cast<StyleBase&>(*this) = static_cast<const StyleBase&>(other); 
		m_default = other.m_default;
		return *this;
	}
	
	int version() const  { 
		assert (this != m_default->base());
		qDebug(QString("version? %1 default %2 base %3").arg(reinterpret_cast<int>(this)).arg(reinterpret_cast<int>(m_default)).arg(reinterpret_cast<int>(m_default->base())));
		return m_default->base() ? m_version ^ m_default->base()->version() : m_version; 
	}
		
private:
	const Style* m_default;
};



#include "styles/charstyle.h"
#include "styles/paragraphstyle.h"


#endif
