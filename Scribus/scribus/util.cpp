/***************************************************************************
                          util.cpp  -  description
                             -------------------
    begin                : Fri Sep 14 2001
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

#include <qstring.h>
#include <qpixmap.h>
#include <qbitmap.h>
#include <qpainter.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qtextstream.h>
#include <qstringlist.h>
#include <qmap.h>
#include <qdom.h>
#include <qimage.h>
#include <qdir.h>
#include <qpointarray.h>
#include <qmessagebox.h>
#include <cstdlib>
#include <cmath>
#include "md5.h"

#if (_MSC_VER >= 1200)
 #include "win-config.h"
#else
 #include "config.h"
#endif

#include "scribusdoc.h"
#include "libpdf/pdflib.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_GLYPH_H
#ifdef HAVE_LIBZ
	#include <zlib.h>
#endif
#ifdef HAVE_TIFF
	#include <tiffio.h>
#endif
#ifdef HAVE_CMS
	#include CMS_INC
extern cmsHPROFILE CMSoutputProf;
extern cmsHPROFILE CMSprinterProf;
extern cmsHTRANSFORM stdTrans;
extern cmsHTRANSFORM stdProof;
extern cmsHTRANSFORM stdTransImg;
extern cmsHTRANSFORM stdProofImg;
extern bool SoftProofing;
extern bool Gamut;
extern bool CMSuse;
extern int IntentMonitor;
extern int IntentPrinter;
#endif
extern ProfilesL InputProfiles;

void WordAndPara(PageItem* b, int *w, int *p, int *c, int *wN, int *pN, int *cN);
void CopyPageItem(struct CLBuf *Buffer, PageItem *b);
bool overwrite(QWidget *parent, QString filename);
FPointArray traceChar(FT_Face face, uint chr, int chs, double *x, double *y);
FPoint GetMaxClipF(FPointArray Clip);
QPixmap FontSample(QString da, int s, QString ts, QColor back);
QString Path2Relative(QString Path);
QPixmap LoadPDF(QString fn, int Seite, int Size, int *w, int *h);
bool GlyNames(QMap<uint, QString> *GList, QString Dat);
bool GlyIndex(QMap<uint, PDFlib::GlNamInd> *GListInd, QString Dat);
QByteArray ComputeMD5Sum(QByteArray *in);
char *toHex( uchar u );
QString String2Hex(QString *in, bool lang = true);
QString CompressStr(QString *in);
QString ImageToTxt(QImage *im);
QString ImageToCMYK(QImage *im);
QString ImageToCMYK_PS(QImage *im, int pl, bool pre);
QString MaskToTxt(QImage *im, bool PDF = true);
void Level2Layer(ScribusDoc *doc, struct Layer *ll, int Level);
void BezierPoints(QPointArray *ar, QPoint n1, QPoint n2, QPoint n3, QPoint n4);
double xy2Deg(double x, double y);
QPointArray FlattenPath(FPointArray ina, QValueList<uint> &Segs);
QPointArray RegularPolygon(double w, double h, uint c, bool star, double factor, double rota);
FPointArray RegularPolygonF(double w, double h, uint c, bool star, double factor, double rota);
QPixmap loadIcon(QString nam);
bool loadText(QString nam, QString *Buffer);
double Cwidth(ScribusDoc *doc, QString name, QString ch, int Siz, QString ch2 = " ");
double QStodouble(QString in);
int QStoInt(QString in);
QString GetAttr(QDomElement *el, QString at, QString def="0");
QImage LoadPict(QString fn);
#ifdef HAVE_CMS
QImage ProofPict(QImage *Im, QString Prof, int Rend, cmsHPROFILE emPr=0);
QImage LoadPictCol(QString fn, QString Prof, bool UseEmbedded, bool *realCMYK);
#else
QImage ProofPict(QImage *Im, QString Prof, int Rend);
#endif
QImage ProofImage(QImage *Im);

#ifdef HAVE_CMS
QImage ProofPict(QImage *Im, QString Prof, int Rend, cmsHPROFILE emPr)
#else
QImage ProofPict(QImage *Im, QString Prof, int Rend)
#endif
{
	bool emp = false;
	if (Prof == "")
		return Im->copy();
#ifdef HAVE_CMS
	QImage out = Im->copy();
	if ((CMSuse) && (SoftProofing))
		{
		cmsHTRANSFORM xform;
		cmsHPROFILE inputProf;
  	if (emPr != 0)
  		inputProf = emPr;
  	else
			{
  		inputProf = cmsOpenProfileFromFile(InputProfiles[Prof], "r");
			emp = true;
			}
		int dcmsFlags = 0;
		if (Gamut)
			dcmsFlags |= cmsFLAGS_GAMUTCHECK;
		else
			dcmsFlags |= cmsFLAGS_SOFTPROOFING;
		xform = cmsCreateProofingTransform(inputProf, TYPE_RGBA_8,
															 				 CMSoutputProf, TYPE_RGBA_8,
															 				 CMSprinterProf,
															 				 IntentPrinter,
															 				 Rend, dcmsFlags);
		for (int i=0; i < out.height(); ++i)
			{
			LPBYTE ptr = out.scanLine(i);
			cmsDoTransform(xform, ptr, ptr, out.width());
			}
		cmsDeleteTransform(xform);
		if (emp)
			cmsCloseProfile(inputProf);
		}
	else
		{
		if (CMSuse)
			{
			cmsHTRANSFORM xform;
			cmsHPROFILE inputProf;
  		if (emPr != 0)
  			inputProf = emPr;
  		else
				{
  			inputProf = cmsOpenProfileFromFile(InputProfiles[Prof], "r");
				emp = true;
				}
			xform = cmsCreateTransform(inputProf, TYPE_RGBA_8,
															 	 CMSoutputProf, TYPE_RGBA_8,
															 	 Rend,
															 	 0);
			for (int i=0; i < out.height(); ++i)
				{
				LPBYTE ptr = out.scanLine(i);
				cmsDoTransform(xform, ptr, ptr, out.width());
				}
			cmsDeleteTransform(xform);
			if (emp)
				cmsCloseProfile(inputProf);
			}
		}
	return out;
#else
	return Im->copy();
#endif
}

QImage ProofImage(QImage *Im)
{
#ifdef HAVE_CMS
	QImage out = Im->copy();
	if ((CMSuse) && (SoftProofing))
		{
		for (int i=0; i < out.height(); ++i)
			{
			LPBYTE ptr = out.scanLine(i);
			cmsDoTransform(stdProofImg, ptr, ptr, out.width());
			}
		}
	else
		{
		if (CMSuse)
			{
			for (int i=0; i < out.height(); ++i)
				{
				LPBYTE ptr = out.scanLine(i);
				cmsDoTransform(stdTransImg, ptr, ptr, out.width());
				}
			}
		}
	return out;
#else
	return Im->copy();
#endif
}

QPixmap LoadPDF(QString fn, int Seite, int Size, int *w, int *h)
{
	QString tmp, cmd1, cmd2;
  QString tmpFile = QString(getenv("HOME"))+"/.scribus/sc.png";
	QPixmap pm;
	int ret = -1;
	tmp.setNum(Seite);
	cmd1 = "gs -q -dNOPAUSE -sDEVICE=png16m -r72 -sOutputFile="+tmpFile+" -dFirstPage="+tmp+" -dLastPage="+tmp+" ";
	cmd2 = " -c showpage -c quit";
	ret = system(cmd1 + "\"" + fn + "\"" + cmd2);
	if (ret == 0)
		{
		QImage image;
		image.load(tmpFile);
		system("rm -f "+tmpFile);
  	QImage im2;
		*h = image.height();
		*w = image.width();
		double sx = image.width() / static_cast<double>(Size);
		double sy = image.height() / static_cast<double>(Size);
		if (sy < sx)
			im2 = image.smoothScale(static_cast<int>(image.width() / sx), static_cast<int>(image.height() / sx));
		else
			im2 = image.smoothScale(static_cast<int>(image.width() / sy), static_cast<int>(image.height() / sy));
		pm.convertFromImage(im2);
		QPainter p;
		p.begin(&pm);
		p.setBrush(Qt::NoBrush);
		p.setPen(Qt::black);
		p.drawRect(0, 0, pm.width(), pm.height());
		p.end();
		im2.detach();
		}
	return pm;
}

QImage LoadPict(QString fn)
{
	QString tmp, dummy, cmd1, cmd2, BBox, tmp2;
	QChar tc;
	QImage Bild;
	double x, y, b, h;
	bool found = false;
	int ret = -1;
  QString tmpFile = QString(getenv("HOME"))+"/.scribus/sc.png";
	QFileInfo fi = QFileInfo(fn);
	QString ext = fi.extension(false).lower();
	if (ext == "pdf")
		{
		cmd1 = "gs -q -dNOPAUSE -sDEVICE=png16m -r72 -sOutputFile="+tmpFile+" -dFirstPage=1 -dLastPage=1 ";
		cmd2 = " -c showpage -c quit";
		ret = system(cmd1 + "\"" + fn + "\"" + cmd2);
		if (ret == 0)
			{
			QImage image;
			image.load(tmpFile);
  		Bild = image.convertDepth(32);
			system("rm -f "+tmpFile);
			}
		}
	if ((ext == "eps") || (ext == "ps"))
		{
		QFile f(fn);
		if (f.open(IO_ReadOnly))
			{
			QTextStream ts(&f);
			while (!ts.atEnd())
				{
				tc = ' ';
				tmp = "";
				while ((tc != '\n') && (tc != '\r'))
					{
					ts >> tc;
					if ((tc != '\n') && (tc != '\r'))
						tmp += tc;
					}
				if (tmp.startsWith("%%BoundingBox"))
					{
					found = true;
					BBox = tmp;
					}
				if (tmp.startsWith("%%EndComments"))
					break;
				}
			f.close();
			if (found)
				{
				QTextStream ts2(&BBox, IO_ReadOnly);
				ts2 >> dummy >> x >> y >> b >> h;
				cmd1 = "gs -q -dNOPAUSE -sDEVICE=png16m -r72 -sOutputFile="+tmpFile+" -g";
				cmd2 = " -c showpage -c quit";
				ret = system(cmd1 + tmp.setNum(qRound(b)) + "x" + tmp2.setNum(qRound(h)) + " \"" + fn + "\"" + cmd2);
				if (ret == 0)
					{
					QImage im4;
					QImage image;
					image.load(tmpFile);
  				image = image.convertDepth(32);
					int wi = image.width();
					int hi = image.height();
					QBitmap bm(tmpFile);
					bm.fill(Qt::color1);
    			QPainter pp;
    			pp.begin(&bm);
    			pp.setPen(Qt::color0);
    			QString tmp2;
    			if ( image.depth() == 8 )
						{
        		for( int yi=0; yi < hi; ++yi )
							{
            	uchar * s = image.scanLine( yi );
            	for( int xi=0; xi < wi; ++xi )
								{
                if(image.color(s[xi]) == 0xffffffff)
                	pp.drawPoint(xi, yi);
            		}
        			}
    				}
					else
						{
        		for( int yi=0; yi < hi; ++yi )
							{
            	QRgb * s = (QRgb*)(image.scanLine( yi ));
              for(int xi=0; xi < wi; ++xi )
								{
                if((*s++) == 0xffffffff)
                	pp.drawPoint(xi, yi);
                }
        			}
    				}
    			pp.end();
					QPixmap pm;
					pm.convertFromImage(image);
					pm.setMask(bm);
					im4.setAlphaBuffer(true);
					im4 = pm.convertToImage();
					Bild = im4.copy(static_cast<int>(x), 0, static_cast<int>(b-x), static_cast<int>(h-y));
					system("rm -f "+tmpFile);
					}
				}
			}
		}
#ifdef HAVE_TIFF
	if (ext == "tif")
		{
		QImage img;
		QImage inI2;
		TIFF* tif = TIFFOpen(fn, "r");
		if(tif)
			{
			unsigned width, height,size;
			TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
			TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
			size=width*height;
			uint32 *bits=(uint32*) _TIFFmalloc(size * sizeof (uint32));
			if(bits)
				{
				if (TIFFReadRGBAImage(tif, width, height, bits, 0))
					{
        	img.create(width,height,32);
					if(TIFFGetR(0x1234567)==qRed  (0x1234567) &&
						 TIFFGetG(0x1234567)==qGreen(0x1234567) &&
						 TIFFGetB(0x1234567)==qBlue (0x1234567))
						{
						for(unsigned y=0; y<height; ++y)
							memcpy(img.scanLine(height-1-y),bits+y*width,width*4);
						}
					else
						{
						uint32 *inp=bits;
						for(unsigned y=0; y<height; ++y)
							{
							QRgb *row=(QRgb*) (img.scanLine(height-1-y));
							for(unsigned x=0; x<width; ++x)
								{
								const uint32 col=*(inp++);
								row[x]=qRgb(TIFFGetR(col), TIFFGetG(col), TIFFGetB(col) ) | (TIFFGetA(col)<<24);
								}
							}
						}
					Bild = img.copy();
					}
				_TIFFfree(bits);
				}
			TIFFClose(tif);
			}
		}
#endif
	else
		{
		Bild.load(fn);
  	Bild = Bild.convertDepth(32);
		}
	return Bild;
}

#ifdef HAVE_CMS
QImage LoadPictCol(QString fn, QString Prof, bool UseEmbedded, bool *realCMYK)
{
	QString tmp, dummy, cmd1, cmd2, BBox, tmp2;
	QChar tc;
	QImage Bild;
	double x, y, b, h;
	bool found = false;
	int ret = -1;
  QString tmpFile = QString(getenv("HOME"))+"/.scribus/sc.png";
	QFileInfo fi = QFileInfo(fn);
	QString ext = fi.extension(false).lower();
	if (ext == "pdf")
		{
		cmd1 = "gs -q -dNOPAUSE -sDEVICE=png16m -r72 -sOutputFile="+tmpFile+" -dFirstPage=1 -dLastPage=1 ";
		cmd2 = " -c showpage -c quit";
		ret = system(cmd1 + "\"" + fn + "\"" + cmd2);
		if (ret == 0)
			{
			QImage image;
			image.load(tmpFile);
  		Bild = image.convertDepth(32);
			system("rm -f "+tmpFile);
			*realCMYK = false;
			}
		}
	if ((ext == "eps") || (ext == "ps"))
		{
		QFile f(fn);
		if (f.open(IO_ReadOnly))
			{
			QTextStream ts(&f);
			while (!ts.atEnd())
				{
				tc = ' ';
				tmp = "";
				while ((tc != '\n') && (tc != '\r'))
					{
					ts >> tc;
					if ((tc != '\n') && (tc != '\r'))
						tmp += tc;
					}
				if (tmp.startsWith("%%BoundingBox"))
					{
					found = true;
					BBox = tmp;
					}
				if (tmp.startsWith("%%EndComments"))
					break;
				}
			f.close();
			if (found)
				{
				QTextStream ts2(&BBox, IO_ReadOnly);
				ts2 >> dummy >> x >> y >> b >> h;
				cmd1 = "gs -q -dNOPAUSE -sDEVICE=png16m -r72 -sOutputFile="+tmpFile+" -g";
				cmd2 = " -c showpage -c quit";
				ret = system(cmd1 + tmp.setNum(qRound(b)) + "x" + tmp2.setNum(qRound(h)) + " \"" + fn + "\"" + cmd2);
				if (ret == 0)
					{
					QImage im4;
					QImage image;
					image.load(tmpFile);
  				image = image.convertDepth(32);
					int wi = image.width();
					int hi = image.height();
					QBitmap bm(tmpFile);
					bm.fill(Qt::color1);
    			QPainter pp;
    			pp.begin(&bm);
    			pp.setPen(Qt::color0);
    			QString tmp2;
    			if ( image.depth() == 8 )
						{
        		for( int yi=0; yi < hi; ++yi )
							{
            	uchar * s = image.scanLine( yi );
            	for( int xi=0; xi < wi; ++xi )
								{
                if(image.color(s[xi]) == 0xffffffff)
                	pp.drawPoint(xi, yi);
            		}
        			}
    				}
					else
						{
        		for( int yi=0; yi < hi; ++yi )
							{
            	QRgb * s = (QRgb*)(image.scanLine( yi ));
              for(int xi=0; xi < wi; ++xi )
								{
                if((*s++) == 0xffffffff)
                	pp.drawPoint(xi, yi);
                }
        			}
    				}
    			pp.end();
					QPixmap pm;
					pm.convertFromImage(image);
					pm.setMask(bm);
					im4.setAlphaBuffer(true);
					im4 = pm.convertToImage();
					Bild = im4.copy(static_cast<int>(x), 0, static_cast<int>(b-x), static_cast<int>(h-y));
					system("rm -f "+tmpFile);
					*realCMYK = false;
					}
				}
			}
		}
#ifdef HAVE_TIFF
	if (ext == "tif")
		{
		QImage img;
		QImage inI2;
		TIFF* tif = TIFFOpen(fn, "r");
		if(tif)
			{
    	DWORD EmbedLen = 0;
    	LPBYTE EmbedBuffer;
			unsigned width, height,size;
			TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
			TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
			size=width*height;
			uint32 *bits=(uint32*) _TIFFmalloc(size * sizeof (uint32));
			if(bits)
				{
				if (TIFFReadRGBAImage(tif, width, height, bits, 0))
					{
        	img.create(width,height,32);
					if(TIFFGetR(0x1234567)==qRed  (0x1234567) &&
						 TIFFGetG(0x1234567)==qGreen(0x1234567) &&
						 TIFFGetB(0x1234567)==qBlue (0x1234567))
						{
						for(unsigned y=0; y<height; ++y)
							memcpy(img.scanLine(height-1-y),bits+y*width,width*4);
						}
					else
						{
						uint32 *inp=bits;
						for(unsigned y=0; y<height; ++y)
							{
							QRgb *row=(QRgb*) (img.scanLine(height-1-y));
							for(unsigned x=0; x<width; ++x)
								{
								const uint32 col=*(inp++);
								row[x]=qRgb(TIFFGetR(col), TIFFGetG(col), TIFFGetB(col) ) | (TIFFGetA(col)<<24);
								}
							}
						}
					Bild = img.copy();
					}
				_TIFFfree(bits);
				cmsHTRANSFORM xform;
				cmsHPROFILE inputProf;
				if((TIFFGetField(tif, TIFFTAG_ICCPROFILE, &EmbedLen, &EmbedBuffer)) && (UseEmbedded))
					{
					inputProf = cmsOpenProfileFromMem(EmbedBuffer, EmbedLen);
					if ((int) cmsGetColorSpace(inputProf) == icSigRgbData)
						{
						switch ((int) cmsGetColorSpace(CMSprinterProf))
							{
							case icSigRgbData:
								*realCMYK = false;
								xform = cmsCreateTransform(inputProf, TYPE_RGBA_8, CMSprinterProf, TYPE_RGBA_8, IntentPrinter, 0);
								break;
							case icSigCmykData:
								*realCMYK = true;
								xform = cmsCreateTransform(inputProf, TYPE_RGBA_8, CMSprinterProf, TYPE_CMYK_8, IntentPrinter, 0);
								break;
							}
						for (int i=0; i < Bild.height(); ++i)
							{
							LPBYTE ptr = Bild.scanLine(i);
							cmsDoTransform(xform, ptr, ptr, Bild.width());
							}
						}
					}
				else
					{
					inputProf = cmsOpenProfileFromFile(InputProfiles[Prof], "r");
					switch ((int) cmsGetColorSpace(CMSprinterProf))
						{
						case icSigRgbData:
							*realCMYK = false;
							xform = cmsCreateTransform(inputProf, TYPE_RGBA_8, CMSprinterProf, TYPE_RGBA_8, IntentPrinter, 0);
							break;
						case icSigCmykData:
							*realCMYK = true;
							xform = cmsCreateTransform(inputProf, TYPE_RGBA_8, CMSprinterProf, TYPE_CMYK_8, IntentPrinter, 0);
							break;
						}
					for (int i=0; i < Bild.height(); ++i)
						{
						LPBYTE ptr = Bild.scanLine(i);
						cmsDoTransform(xform, ptr, ptr, Bild.width());
						}
					}
				cmsDeleteTransform(xform);
				cmsCloseProfile(inputProf);
				}
			TIFFClose(tif);
			}
		}
#endif
	else
		{
		cmsHTRANSFORM xform;
		cmsHPROFILE inputProf;
		Bild.load(fn);
  	Bild = Bild.convertDepth(32);
		inputProf = cmsOpenProfileFromFile(InputProfiles[Prof], "r");
		switch (static_cast<int>(cmsGetColorSpace(CMSprinterProf)))
			{
			case icSigRgbData:
				*realCMYK = false;
				xform = cmsCreateTransform(inputProf, TYPE_RGBA_8, CMSprinterProf, TYPE_RGBA_8, IntentPrinter, 0);
				break;
			case icSigCmykData:
				*realCMYK = true;
				xform = cmsCreateTransform(inputProf, TYPE_RGBA_8, CMSprinterProf, TYPE_CMYK_8, IntentPrinter, 0);
				break;
			}
		for (int i=0; i < Bild.height(); ++i)
			{
			LPBYTE ptr = Bild.scanLine(i);
			cmsDoTransform(xform, ptr, ptr, Bild.width());
			}
		cmsDeleteTransform(xform);
		cmsCloseProfile(inputProf);
		}
	return Bild;
}
#endif

QString GetAttr(QDomElement *el, QString at, QString def)
{
	return el->attribute(at, def);
}

int QStoInt(QString in)
{
	bool ok = false;
	int c = in.toInt(&ok);
	if (ok)
		return c;
	else
		return 0;
}

double QStodouble(QString in)
{
	bool ok = false;
	double c = in.toDouble(&ok);
	if (ok)
		return c;
	else
		return 0.0;
}

QPixmap loadIcon(QString nam)
{
  QString pfad = PREL;
  pfad += "/lib/scribus/icons/"+nam;
	QPixmap pm;
	pm.load(pfad);
	return pm;
}

bool loadText(QString nam, QString *Buffer)
{
	QFile f(nam);
	QFileInfo fi(f);
	if (!fi.exists())
		return false;
	uint posi;
	bool ret = false;
	QByteArray bb(f.size());
	if (f.open(IO_ReadOnly))
		{
		f.readBlock(bb.data(), f.size());
		f.close();
		for (posi = 0; posi < bb.size(); ++posi)
			{
			*Buffer += bb[posi];
			}
		ret = true;
		}
	else
		{
		ret = false;
		}
	return ret;
}

double Cwidth(ScribusDoc *doc, QString name, QString ch, int Siz, QString ch2)
{
	double w;
	FT_Vector  delta;
	uint c1 = ch.at(0).unicode();
	uint c2 = ch2.at(0).unicode();
	Foi* fo = (*doc->AllFonts)[name];
	if (fo->CharWidth.contains(c1))
		{
		w = fo->CharWidth[c1]*(Siz / 10.0);
		if (fo->HasKern)
			{
			uint cl = FT_Get_Char_Index(doc->FFonts[name], c1);
			uint cr = FT_Get_Char_Index(doc->FFonts[name], c2);
			FT_Get_Kerning(doc->FFonts[name], cl, cr, ft_kerning_unscaled, &delta);
			w += delta.x / fo->uniEM * (Siz / 10.0);
			}
		return w;
		}
	else
		return static_cast<double>(Siz / 10.0);
}

QPointArray RegularPolygon(double w, double h, uint c, bool star, double factor, double rota)
{
	uint cx;
	if (star)
		cx = c * 2;
	else
		cx = c;
	double seg = 360.0 / cx;
	double sc = rota + 180.0;
	double di = factor;
	int mx = 0;
	int my = 0;
	QPointArray pts = QPointArray();
	for (uint x = 0; x < cx; ++x)
		{
		sc = seg * x + 180.0 + rota;
		if (star)
			{
			if (x % 2 == 0)
				{
				mx = qRound(sin(sc / 180 * M_PI) * (w/2) + (w/2));
				my = qRound(cos(sc / 180 * M_PI) * (h/2) + (h/2));
				}
			else
				{
				mx = qRound(sin(sc / 180 * M_PI) * (w/2*di) + (w/2));
				my = qRound(cos(sc / 180 * M_PI) * (h/2*di) + (h/2));
				}
			}
		else
			{
			mx = qRound(sin(sc / 180 * M_PI) * (w/2) + (w/2));
			my = qRound(cos(sc / 180 * M_PI) * (h/2) + (h/2));
			}
		pts.resize(x+1);
		pts.setPoint(x, mx, my);
		}
	return pts;
}

FPointArray RegularPolygonF(double w, double h, uint c, bool star, double factor, double rota)
{
	uint cx;
	if (star)
		cx = c * 2;
	else
		cx = c;
	double seg = 360.0 / cx;
	double sc = rota + 180.0;
	double di = factor;
	double mx = 0;
	double my = 0;
	FPointArray pts = FPointArray();
	for (uint x = 0; x < cx; ++x)
		{
		sc = seg * x + 180.0 + rota;
		if (star)
			{
			if (x % 2 == 0)
				{
				mx = sin(sc / 180 * M_PI) * (w/2) + (w/2);
				my = cos(sc / 180 * M_PI) * (h/2) + (h/2);
				}
			else
				{
				mx = sin(sc / 180 * M_PI) * (w/2*di) + (w/2);
				my = cos(sc / 180 * M_PI) * (h/2*di) + (h/2);
				}
			}
		else
			{
			mx = sin(sc / 180 * M_PI) * (w/2) + (w/2);
			my = cos(sc / 180 * M_PI) * (h/2) + (h/2);
			}
		pts.resize(x+1);
		pts.setPoint(x, mx, my);
		}
	return pts;
}

QPointArray FlattenPath(FPointArray ina, QValueList<uint> &Segs)
{
	QPointArray Bez(4);
	QPointArray outa, cli;
	Segs.clear();
	if (ina.size() > 3)
		{
		for (uint poi=0; poi<ina.size()-3; poi += 4)
			{
			if (ina.point(poi).x() > 900000)
				{
				outa.resize(outa.size()+1);
				outa.setPoint(outa.size()-1, cli.point(cli.size()-1));
				Segs.append(outa.size());
				continue;
				}
			BezierPoints(&Bez, ina.pointQ(poi), ina.pointQ(poi+1), ina.pointQ(poi+3), ina.pointQ(poi+2));
			cli = Bez.cubicBezier();
			outa.putPoints(outa.size(), cli.size()-1, cli);
			}
		outa.resize(outa.size()+1);
		outa.setPoint(outa.size()-1, cli.point(cli.size()-1));
		}
	return outa;
}

double xy2Deg(double x, double y)
{
	return (atan2(y,x)*(180.0/M_PI));
}

void BezierPoints(QPointArray *ar, QPoint n1, QPoint n2, QPoint n3, QPoint n4)
{
	ar->setPoint(0, n1);
	ar->setPoint(1, n2);
	ar->setPoint(2, n3);
	ar->setPoint(3, n4);
	return;
}

void Level2Layer(ScribusDoc *doc, struct Layer *ll, int Level)
{
	for (uint la2 = 0; la2 < doc->Layers.count(); ++la2)
		{
		if (doc->Layers[la2].Level == Level)
			{
			ll->Sichtbar = doc->Layers[la2].Sichtbar;
			ll->Drucken = doc->Layers[la2].Drucken;
			ll->LNr = doc->Layers[la2].LNr;
			break;
			}
		}
}

QString ImageToTxt(QImage *im)
{
	int h = im->height();
	int w = im->width();
	QString ImgStr = "";
	for( int yi=0; yi < h; ++yi )
		{
		QRgb * s = (QRgb*)(im->scanLine( yi ));
		for( int xi=0; xi < w; ++xi )
			{
			QRgb r=*s++;
			unsigned char u=qRed(r);
			ImgStr += u;
			u=qGreen(r);
			ImgStr += u;
			u=qBlue(r);
			ImgStr += u;
			}
		}
	return ImgStr;
}

QString ImageToCMYK(QImage *im)
{
	int h = im->height();
	int w = im->width();
	QString ImgStr = "";
	for( int yi=0; yi < h; ++yi )
		{
		QRgb * s = (QRgb*)(im->scanLine( yi ));
		for( int xi=0; xi < w; ++xi )
			{
			QRgb r=*s++;
			int c = 255 - qRed(r);
			int m = 255 - qGreen(r);
			int y = 255 - qBlue(r);
			int k = QMIN(QMIN(c, m), y);
			ImgStr += c - k;
			ImgStr += m - k;
			ImgStr += y - k;
			ImgStr += k;
			}
		}
	return ImgStr;
}

QString ImageToCMYK_PS(QImage *im, int pl, bool pre)
{
	int h = im->height();
	int w = im->width();
	QString ImgStr = "";
	if (pre)
		{
		for( int yi=0; yi < h; ++yi )
			{
			QRgb * s = (QRgb*)(im->scanLine( yi ));
			for( int xi=0; xi < w; ++xi )
				{
				QRgb r=*s++;
				int c = qRed(r);
				int m = qGreen(r);
				int y = qBlue(r);
				int k = qAlpha(r);
				if (pl == -1)
					{
					ImgStr += c;
					ImgStr += m;
					ImgStr += y;
					ImgStr += k;
					}
				else
					{
					if (pl == -2)
						ImgStr += QMIN(255, qRound(0.3 * c + 0.59 * m + 0.11 * y + k));
					if (pl == 1)
						ImgStr += c;
					if (pl == 2)
						ImgStr += m;
					if (pl == 3)
						ImgStr += y;
					if (pl == 0)
						ImgStr += k;
					}
				}
			}
		}
	else
		{
		for( int yi=0; yi < h; ++yi )
			{
			QRgb * s = (QRgb*)(im->scanLine( yi ));
			for( int xi=0; xi < w; ++xi )
				{
				QRgb r=*s++;
				int c = 255 - qRed(r);
				int m = 255 - qGreen(r);
				int y = 255 - qBlue(r);
				int k = QMIN(QMIN(c, m), y);
				if (pl == -1)
					{
					ImgStr += c - k;
					ImgStr += m - k;
					ImgStr += y - k;
					ImgStr += k;
					}
				else
					{
					if (pl == -2)
						ImgStr += QMIN(255, qRound(0.3 * c + 0.59 * m + 0.11 * y + k));
					if (pl == 1)
						ImgStr += c - k;
					if (pl == 2)
						ImgStr += m - k;
					if (pl == 3)
						ImgStr += y - k;
					if (pl == 0)
						ImgStr += k;
					}
				}
			}
		}
	return ImgStr;
}

QString MaskToTxt(QImage *im, bool PDF)
{
	int h = im->height();
	int w = im->width();
  int w2;
  w2 = w / 8;
  if ((w % 8) != 0)
  	w2++;
	QString ImgStr = "";
	for( int yi=0; yi < h; ++yi )
		{
		uchar * s = im->scanLine( yi );
		for( int xi=0; xi < w2; ++xi )
			{
			unsigned char u = *(s+xi);
			if (PDF)
				ImgStr += ~u;
			else
				ImgStr += u;
			}
		}
	return ImgStr;
}

QString CompressStr(QString *in)
{
	QString out = "";
#ifdef HAVE_LIBZ
	QByteArray bb(in->length());
	for (uint ax = 0; ax < in->length(); ++ax)
		{
		bb[ax] = uchar(QChar(in->at(ax)));
		}
	uLong exlen = uint(bb.size() * 0.001 + 16) + bb.size();
	QByteArray bc(exlen);
	compress2((Byte *)bc.data(), &exlen, (Byte *)bb.data(), uLong(bb.size()), 9);
	for (uint cl = 0; cl < exlen; ++cl)
		{
		out += bc[cl];
		}
#else
	out = *in;
#endif
	return out;
}

char *toHex( uchar u )
{
    static char hexVal[3];
    int i = 1;
    while ( i >= 0 )
			{
			ushort hex = (u & 0x000f);
			if ( hex < 0x0a )
	    	hexVal[i] = '0'+hex;
			else
	    	hexVal[i] = 'A'+(hex-0x0a);
			u = u >> 4;
			i--;
    	}
    hexVal[2] = '\0';
    return hexVal;
}

QString String2Hex(QString *in, bool lang)
{
	int i = 0;
	QString out = "";
	for( uint xi = 0; xi < in->length(); ++xi )
		{
		out += toHex(uchar(QChar(in->at(xi))));
		++i;
		if ((i>40) && (lang))
			{
			out += '\n';
			i=0;
			}
		}
	return out;
}

QByteArray ComputeMD5Sum(QByteArray *in)
{
	QByteArray MDsum(16);
	md5_buffer (in->data(), in->size(), reinterpret_cast<void*>(MDsum.data()));
	return MDsum;
}

QString Path2Relative(QString Path)
{
	QString	Ndir = "";
	QStringList Pdir = QStringList::split("/", QDir::currentDirPath());
	QFileInfo Bfi = QFileInfo(Path);
	QStringList Bdir = QStringList::split("/", Bfi.dirPath(true));
	bool ende = true;
	uint dcoun = 0;
	uint dcoun2 = 0;
	while (ende)
		{
		if (Pdir[dcoun] == Bdir[dcoun])
			dcoun++;
		else
			break;
		if (dcoun > Pdir.count())
			break;
		}
	dcoun2 = dcoun;
	for (uint ddx2 = dcoun; ddx2 < Pdir.count(); ddx2++)
		{
		Ndir += "../";
		}
	for (uint ddx = dcoun2; ddx < Bdir.count(); ddx++)
		{
		Ndir += Bdir[ddx]+"/";
		}
	Ndir += Bfi.fileName();
	return Ndir;
}

bool GlyNames(QMap<uint, QString> *GList, QString Dat)
{
	bool error;
	char *buf[50];
	FT_Library library;
	FT_Face face;
	FT_ULong  charcode;
  FT_UInt   gindex;
	error = FT_Init_FreeType(&library);
	error = FT_New_Face(library, Dat, 0, &face);
	gindex = 0;
  charcode = FT_Get_First_Char(face, &gindex );
  while (gindex != 0)
		{
		FT_Get_Glyph_Name(face, gindex, buf, 50);
		GList->insert(charcode, QString(reinterpret_cast<char*>(buf)));
    charcode = FT_Get_Next_Char(face, charcode, &gindex );
		}
	FT_Done_FreeType( library );
	return true;
}

bool GlyIndex(QMap<uint, PDFlib::GlNamInd> *GListInd, QString Dat)
{
	struct PDFlib::GlNamInd gln;
	bool error;
	char *buf[50];
	FT_Library library;
	FT_Face face;
	FT_ULong  charcode;
  FT_UInt   gindex;
  uint counter1 = 32;
  uint counter2 = 0;
	error = FT_Init_FreeType(&library);
	error = FT_New_Face(library, Dat, 0, &face);
	gindex = 0;
  charcode = FT_Get_First_Char(face, &gindex );
  while (gindex != 0)
		{
		FT_Get_Glyph_Name(face, gindex, buf, 50);
		gln.Code = counter1 + counter2;
		gln.Name = "/"+QString(reinterpret_cast<char*>(buf));
		GListInd->insert(charcode, gln);
    charcode = FT_Get_Next_Char(face, charcode, &gindex );
    counter1++;
    if (counter1 > 255)
			{
			counter1 = 32;
			counter2 += 0x100;
			}
		}
	FT_Done_FreeType( library );
	return true;
}

FPoint firstP;
bool FirstM;

int traceMoveto( FT_Vector *to, FPointArray *composite )
{
	double tox = ( to->x / 64.0 );
	double toy = ( to->y / 64.0 );
	if (!FirstM)
		{
		composite->addPoint(firstP);
		composite->addPoint(firstP);
		composite->setMarker();
		}
	else
		FirstM = false;
	composite->addPoint(FPoint(tox, toy));
	composite->addPoint(FPoint(tox, toy));
	firstP = FPoint(tox, toy);
	return 0;
}

int traceLineto( FT_Vector *to, FPointArray *composite )
{
	double tox = ( to->x / 64.0 );
	double toy = ( to->y / 64.0 );
	if (composite->size() > 4)
		{
		FPoint b1 = composite->point(composite->size()-4);
		FPoint b2 = composite->point(composite->size()-3);
		FPoint b3 = composite->point(composite->size()-2);
		FPoint b4 = composite->point(composite->size()-1);
		FPoint n1 = FPoint(tox, toy);
		FPoint n2 = FPoint(tox, toy);
		FPoint n3 = FPoint(tox, toy);
		FPoint n4 = FPoint(tox, toy);
		if ((b1 == n1) && (b2 == n2) && (b3 == n3) && (b4 == n4))
			return 0;
		}
	composite->addPoint(FPoint(tox, toy));
	composite->addPoint(FPoint(tox, toy));
	composite->addPoint(FPoint(tox, toy));
	composite->addPoint(FPoint(tox, toy));
	return 0;
}

int traceQuadraticBezier( FT_Vector *control, FT_Vector *to, FPointArray *composite )
{
	double x1 = ( control->x / 64.0 );
	double y1 = ( control->y / 64.0 );
	double x2 = ( to->x / 64.0 );
	double y2 = ( to->y / 64.0 );
	if (composite->size() > 4)
		{
		FPoint b1 = composite->point(composite->size()-4);
		FPoint b2 = composite->point(composite->size()-3);
		FPoint b3 = composite->point(composite->size()-2);
		FPoint b4 = composite->point(composite->size()-1);
		FPoint n1 = FPoint(x2, y2);
		FPoint n2 = FPoint(x1, y1);
		FPoint n3 = FPoint(x2, y2);
		FPoint n4 = FPoint(x2, y2);
		if ((b1 == n1) && (b2 == n2) && (b3 == n3) && (b4 == n4))
			return 0;
		}
	composite->addPoint(FPoint(x2, y2));
	composite->addPoint(FPoint(x1, y1));
	composite->addPoint(FPoint(x2, y2));
	composite->addPoint(FPoint(x2, y2));
	return 0;
};

int traceCubicBezier( FT_Vector *p, FT_Vector *q, FT_Vector *to, FPointArray *composite )
{
	double x1 = ( p->x / 64.0 );
	double y1 = ( p->y / 64.0 );
	double x2 = ( q->x / 64.0 );
	double y2 = ( q->y / 64.0 );
	double x3 = ( to->x / 64.0 );
	double y3 = ( to->y / 64.0 );
	if (composite->size() > 4)
		{
		FPoint b1 = composite->point(composite->size()-4);
		FPoint b2 = composite->point(composite->size()-3);
		FPoint b3 = composite->point(composite->size()-2);
		FPoint b4 = composite->point(composite->size()-1);
		FPoint n1 = FPoint(x3, y3);
		FPoint n2 = FPoint(x2, y2);
		FPoint n3 = FPoint(x3, y3);
		FPoint n4 = FPoint(x3, y3);
		if ((b1 == n1) && (b2 == n2) && (b3 == n3) && (b4 == n4))
			return 0;
		}
	composite->setPoint(composite->size()-1, FPoint(x1, y1));
	composite->addPoint(FPoint(x3, y3));
	composite->addPoint(FPoint(x2, y2));
	composite->addPoint(FPoint(x3, y3));
	composite->addPoint(FPoint(x3, y3));
	return 0;
}

FT_Outline_Funcs OutlineMethods =
{
	(FT_Outline_MoveTo_Func) traceMoveto,
	(FT_Outline_LineTo_Func) traceLineto,
	(FT_Outline_ConicTo_Func) traceQuadraticBezier,
	(FT_Outline_CubicTo_Func) traceCubicBezier,
	0,
	0
};

FPointArray traceChar(FT_Face face, uint chr, int chs, double *x, double *y)
{
	FT_UInt glyphIndex;
	FPointArray pts, pts2;
	pts.resize(0);
	firstP = FPoint(0,0);
	FirstM = true;
	FT_Set_Char_Size(	face, 0, chs*64, 72, 72 );
	glyphIndex = FT_Get_Char_Index(face, chr);
	FT_Load_Glyph( face, glyphIndex, FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP );
	FT_Outline_Decompose(&face->glyph->outline, &OutlineMethods, reinterpret_cast<void*>(&pts));
	*x = face->glyph->metrics.horiBearingX / 64.0;
	*y = face->glyph->metrics.horiBearingY / 64.0;
	QWMatrix ma;
	ma.scale(1, -1);
	pts.map(ma);
	pts.translate(0, chs);
	pts2.resize(0);
	pts2.putPoints(0, pts.size()-2, pts, 0);
	return pts2;
}

FPoint GetMaxClipF(FPointArray Clip)
{
	FPoint np, rp;
	double mx = 0;
	double my = 0;
	for (uint c = 0; c < Clip.size(); ++c)
		{
		np = Clip.point(c);
		if (np.x() > 900000)
			continue;
		if (np.x() > mx)
			mx = np.x();
		if (np.y() > my)
			my = np.y();
		}
	rp = FPoint(mx, my);
	return rp;
}

QPixmap FontSample(QString da, int s, QString ts, QColor back)
{
	FT_Face face;
	FT_Library library;
	double x, y, ymax;
	bool error;
	int  pen_x;
	FPoint gp;
	error = FT_Init_FreeType( &library );
	error = FT_New_Face( library, da, 0, &face );
	double uniEM = static_cast<double>(face->units_per_EM);
	int h = qRound(face->height / uniEM) * s + 1;
	double a = static_cast<double>(face->descender) / uniEM * s + 1;
	int w = qRound((face->bbox.xMax - face->bbox.xMin) / uniEM) * s * (ts.length()+1);
	QPixmap pm(w, h);
	pm.fill();
	pen_x = 0;
	ymax = 0.0;
	ScPainter *p = new ScPainter(&pm, pm.width(), pm.height());
	p->setFillMode(1);
	p->setLineWidth(0.0);
	p->setBrush(back);
	p->drawRect(0.0, 0.0, static_cast<double>(w), static_cast<double>(h));
	p->setBrush(Qt::black);
	for (uint n = 0; n < ts.length(); n++)
		{
		uint dv = ts[n].unicode();
		FPointArray gly = traceChar(face, dv, s, &x, &y);
		if (gly.size() > 3)
			{
			gly.translate(static_cast<double>(pen_x) / 64.0, a);
			gp = GetMaxClipF(gly);
			ymax = QMAX(ymax, gp.y());
			p->setupPolygon(&gly);
			p->fillPath();
			}
		pen_x += face->glyph->advance.x;
		}
	p->end();
	pm.resize(QMIN(qRound(gp.x()), w), QMIN(qRound(ymax), h));
	delete p;
	FT_Done_FreeType( library );
	return pm;
}

/***************************************************************************
    begin                : Wed Oct 29 2003
    copyright            : (C) 2003 The Scribus Team
    email                : paul@all-the-johnsons.co.uk
 ***************************************************************************/
// check if the file exists, if it does, ask if they're sure
// return true if they're sure, else return false;

bool overwrite(QWidget *parent, QString filename)
{
  bool retval = true;
  QFileInfo fi(filename);
  if (fi.exists())
  	{
    int t = QMessageBox::warning(parent,	QObject::tr("Warning"),
  																QObject::tr("Do you really want to overwrite the File:\n%1 ?").arg(filename),
                         					QMessageBox::No, QMessageBox::Yes, QMessageBox::NoButton);
    if (t == QMessageBox::No)
			retval = false;
  	}
  return retval;
}

void CopyPageItem(struct CLBuf *Buffer, PageItem *b)
{
	uint a;
	Buffer->PType = b->PType;
	Buffer->Xpos = b->Xpos;
	Buffer->Ypos = b->Ypos;
	Buffer->Width = b->Width;
	Buffer->Height = b->Height;
	Buffer->RadRect = b->RadRect;
	Buffer->FrameType = b->FrameType;
	Buffer->ClipEdited = b->ClipEdited;
	Buffer->Pwidth = b->Pwidth;
	Buffer->Pcolor = b->Pcolor;
	Buffer->Pcolor2 = b->Pcolor2;
	Buffer->Shade = b->Shade;
	Buffer->Shade2 = b->Shade2;
	Buffer->GrColor = b->GrColor;
	Buffer->GrColor2 = b->GrColor2;
	Buffer->GrShade = b->GrShade;
	Buffer->GrShade2 = b->GrShade2;
	Buffer->GrType = b->GrType;
	Buffer->TxtStroke = b->TxtStroke;
	Buffer->TxtFill = b->TxtFill;
	Buffer->ShTxtStroke = b->ShTxtStroke;
	Buffer->ShTxtFill = b->ShTxtFill;
	Buffer->TxtScale = b->TxtScale;
	Buffer->TxTStyle = b->TxTStyle;
	Buffer->Rot = b->Rot;
	Buffer->PLineArt = b->PLineArt;
	Buffer->PLineEnd = b->PLineEnd;
	Buffer->PLineJoin = b->PLineJoin;
	Buffer->LineSp = b->LineSp;
	Buffer->LocalScX = b->LocalScX;
	Buffer->LocalScY = b->LocalScY;
	Buffer->LocalX = b->LocalX;
	Buffer->LocalY = b->LocalY;
	Buffer->PicArt = b->PicArt;
	Buffer->flippedH = b->flippedH;
	Buffer->flippedV = b->flippedV;
	Buffer->BBoxX = b->BBoxX;
	Buffer->BBoxH = b->BBoxH;
	Buffer->isPrintable = b->isPrintable;
	Buffer->isBookmark = b->isBookmark;
	Buffer->BMnr = b->BMnr;
	Buffer->isAnnotation = b->isAnnotation;
	Buffer->AnType = b->AnType;
	Buffer->AnAction = b->AnAction;
	Buffer->An_E_act = b->An_E_act;
	Buffer->An_X_act = b->An_X_act;
	Buffer->An_D_act = b->An_D_act;
	Buffer->An_Fo_act = b->An_Fo_act;
	Buffer->An_Bl_act = b->An_Bl_act;
	Buffer->An_K_act = b->An_K_act;
	Buffer->An_F_act = b->An_F_act;
	Buffer->An_V_act = b->An_V_act;
	Buffer->An_C_act = b->An_C_act;
	Buffer->An_Extern = b->An_Extern;
	Buffer->AnZiel = b->AnZiel;
	Buffer->AnName = b->AnName;
	Buffer->AnActType = b->AnActType;
	Buffer->AnToolTip = b->AnToolTip;
	Buffer->AnBwid = b->AnBwid;
	Buffer->AnBsty = b->AnBsty;
	Buffer->AnFeed = b->AnFeed;
	Buffer->AnFlag = b->AnFlag;
	Buffer->AnFont = b->AnFont;
	Buffer->AnRollOver = b->AnRollOver;
	Buffer->AnDown = b->AnDown;
	Buffer->AnFormat = b->AnFormat;
	Buffer->AnVis = b->AnVis;
	Buffer->AnMaxChar = b->AnMaxChar;
	Buffer->AnChkStil = b->AnChkStil;
	Buffer->AnIsChk = b->AnIsChk;
	Buffer->AnAAact = b->AnAAact;
	Buffer->AnBColor = b->AnBColor;
	Buffer->AnHTML = b->AnHTML;
	Buffer->AnUseIcons = b->AnUseIcons;
	Buffer->AnIPlace = b->AnIPlace;
	Buffer->AnScaleW = b->AnScaleW;
	Buffer->Extra = b->Extra;
	Buffer->TExtra = b->TExtra;
	Buffer->BExtra = b->BExtra;
	Buffer->RExtra = b->RExtra;
	Buffer->Pfile = b->Pfile;
	Buffer->Pfile2 = b->Pfile2;
	Buffer->Pfile3 = b->Pfile3;
	QString Text = "";
	if (b->Ptext.count() != 0)
		{
		for (a=0; a<b->Ptext.count(); ++a)
			{
			if( (b->Ptext.at(a)->ch == "\n") || (b->Ptext.at(a)->ch == "\r"))
				Text += QString(QChar(5))+"\t";
			else if(b->Ptext.at(a)->ch == "\t")
				Text += QString(QChar(4))+"\t";
			else
				Text += b->Ptext.at(a)->ch+"\t";
			Text += b->Ptext.at(a)->cfont+"\t";
			Text += QString::number(b->Ptext.at(a)->csize / 10.0)+"\t";
			Text += b->Ptext.at(a)->ccolor+"\t";
			Text += QString::number(b->Ptext.at(a)->cextra)+"\t";
			Text += QString::number(b->Ptext.at(a)->cshade)+'\t';
			Text += QString::number(b->Ptext.at(a)->cstyle)+'\t';
			Text += QString::number(b->Ptext.at(a)->cab)+'\t';
			Text += b->Ptext.at(a)->cstroke+"\t";
			Text += QString::number(b->Ptext.at(a)->cshade2)+'\t';
			Text += QString::number(b->Ptext.at(a)->cscale)+'\n';
			}
		}
	Buffer->Ptext = Text;
	Buffer->Clip = b->Clip.copy();
	Buffer->PoLine = b->PoLine.copy();
	Buffer->TabValues = b->TabValues;
	Buffer->DashValues = b->DashValues;
	Buffer->DashOffset = b->DashOffset;
	Buffer->PoShow = b->PoShow;
	Buffer->BaseOffs = b->BaseOffs;
	Buffer->Textflow = b->Textflow;
	Buffer->Textflow2 = b->Textflow2;
	Buffer->Ausrich = b->Ausrich;
	Buffer->IFont = b->IFont;
	Buffer->ISize = b->ISize;
	Buffer->ExtraV = b->ExtraV;
	Buffer->Groups = b->Groups;
	Buffer->IProfile = b->IProfile;
	Buffer->IRender = b->IRender;
	Buffer->UseEmbedded = b->UseEmbedded;
	Buffer->EmProfile = b->EmProfile;
	Buffer->LayerNr = b->LayerNr;
	Buffer->ScaleType = b->ScaleType;
	Buffer->AspectRatio = b->AspectRatio;
	Buffer->Locked = b->Locked;
	Buffer->Transparency = b->Transparency;
	Buffer->TranspStroke = b->TranspStroke;
	Buffer->Reverse = b->Reverse;
	Buffer->InvPict = b->InvPict;
	Buffer->NamedLStyle = b->NamedLStyle;
	Buffer->Language = b->Language;
	Buffer->Cols = b->Cols;
	Buffer->ColGap = b->ColGap;
}

void WordAndPara(PageItem* b, int *w, int *p, int *c, int *wN, int *pN, int *cN)
{
	QChar Dat = QChar(32);
	int para = 0;
	int ww = 0;
	int cc = 0;
	int paraN = 0;
	int wwN = 0;
	int ccN = 0;
	bool first = true;
	PageItem *nb = b;
	PageItem *nbl = b;
	while (nb != 0)
		{
		if (nb->BackBox != 0)
			nb = nb->BackBox;
		else
			break;
		}
	while (nb != 0)
		{
  	for (uint a = 0; a < nb->Ptext.count(); ++a)
  		{
			QChar b = nb->Ptext.at(a)->ch[0];
			if (b == QChar(13))
				{
				if (a >= nb->MaxChars)
					paraN++;
				else
					para++;
				}
			if ((!b.isLetterOrNumber()) && (Dat.isLetterOrNumber()) && (!first))
				{
				if (a >= nb->MaxChars)
					wwN++;
				else
					ww++;
				}
			if (a >= nb->MaxChars)
				ccN++;
			else
				cc++;
    	Dat = b;
			first = false;
    	}
		nbl = nb;
		nb = nb->NextBox;
		}
	if (nbl->MaxChars < nbl->Ptext.count())
		paraN++;
	else
		para++;
	if (Dat.isLetterOrNumber())
		{
		if (nbl->MaxChars < nbl->Ptext.count())
			wwN++;
		else
			ww++;
		}
	*w = ww;
	*p = para;
	*c = cc;
	*wN = wwN;
	*pN = paraN;
	*cN = ccN;
}
