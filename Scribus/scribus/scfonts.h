#ifndef SCFONTS_H
#define SCFONTS_H

#include <qstring.h>
#include <qstrlist.h>
#include <qdict.h>
#include <qfont.h>
#include "config.h"

#include "scfonts_encoding.h"

/*  Base Class Foi : This is subclassed by a class to handle Type1 fonts, a class
		to handle TrueType fonts, and potentially any other type that becomes appropriate in
		the future.
		Note the virtual destructor, needed to ensure that the correct destructor is called
		for subclasses

		The RealName field has been changed from a data member to a member function.
		This is because the only place the PostScript real name of a font is required is
		the printing code, so it's cheaper to extract this information only when it is
		required, for just the used fonts, than for every one of potentially hundreds at
		application startup!  This also allows for the fact that truetype fonts will require
		a different method of extracting their names.

		One implication of using a base class/subclass model for fonts:  It is no longer
		possible to store the Foi structures in a QMap.  This is because QMap allocates
		its own structures, and copies the supplied data to them.  A QMap<QString,Foi>
		would demote all subclasses to Foi classes, and hence break the polymorphism.
		QDict can be used instead, with very little change to the rest of the code, since
		it stores references to the data instead of copying the data.  With AutoDelete set
		to true, it will automatically dispose of all data when its destructor is called,
		so there are no extra cleaning-up chores to take care of.
*/

class Foi
{
	public:
		Foi(QString scname, QString path, bool embedps, SCFonts_Encoding &encoding);
		virtual ~Foi() {};
		virtual QString RealName();
		virtual bool EmbedFont(QString &str);
		virtual bool ReadMetrics();
		virtual bool GlNames(QMap<uint, QString> *GList);
		QString SCName;
		QString Datei;
		QFont Font;
		bool EmbedPS;
		SCFonts_Encoding &Encoding;
		static SCFonts_Encoding StdEncoding;
		static SCFonts_Encoding TTFEncoding;
		bool HasMetrics;
#ifdef HAVE_FREETYPE
		QMap<uint,float> CharWidth;
#else
		float CharWidth[256];
#endif
		QString Ascent;
		QString CapHeight;
		QString Descender;
		QString ItalicAngle;
		QString StdVW;
		QString FontEnc;
		bool IsFixedPitch;
		QString FontBBox;
		bool UseFont;
		bool HasKern;
		float uniEM;
};


/* Main class SCFonts.
   Subclass of QDict<Foi>.
   This class replaces the previous SCFonts typedef, and is nearly as convenient.
   The chief difference from the application point of view is that while data can
   still be retrieved with SCFonts[fontname], this cannot be used to add members.
   Since the only piece of code that will generally add members is scfonts.h, this
   is not a major problem.
*/

class SCFonts : public QDict<Foi>
{
	public:
		SCFonts() : QDict<Foi>(), FontPath(true), Encoding("ISOenc.txt")
		{
			setAutoDelete(true);
		}
		~SCFonts();
		void GetFonts(QString pf);
		void FreeExtraFonts();
	private:
		void AddPath(QString p);
		void AddScalableFonts(const QString &path);
		void AddUserPath(QString pf);
		void AddXFontServerPath();
		void AddXFontPath();
		QStrList FontPath;
		QString ExtraPath;
		SCFonts_Encoding Encoding;
};

typedef QDictIterator<Foi> SCFontsIterator;

#endif
