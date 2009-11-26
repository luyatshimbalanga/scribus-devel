/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#include <QByteArray>
#include <QCursor>
#include <QDrag>
#include <QFile>
#include <QMimeData>
#include <QRegExp>
#include <QTextCodec>
#include <QDebug>
#include "qtiocompressor.h"

#include <cstdlib>

#include "ui/customfdialog.h"
#include "importxar.h"
#include "loadsaveplugin.h"
#include "ui/missing.h"
#include "ui/multiprogressdialog.h"
#include "pageitem_imageframe.h"
#include "pageitem_polygon.h"
#include "pageitem_polyline.h"
#include "pagesize.h"
#include "prefscontext.h"
#include "prefsfile.h"
#include "prefsmanager.h"
#include "prefstable.h"
#include "ui/propertiespalette.h"
#include "rawimage.h"
#include "scclocale.h"
#include "sccolorengine.h"
#include "scconfig.h"
#include "scmimedata.h"
#include "scpaths.h"
#include "scpattern.h"
#include "scribus.h"
#include "scribusXml.h"
#include "scribuscore.h"
#include "sctextstream.h"
#include "selection.h"
#include "undomanager.h"
#include "util.h"
#include "util_formats.h"
#include "util_icon.h"
#include "util_math.h"

extern SCRIBUS_API ScribusQApp * ScQApp;

XarPlug::XarPlug(ScribusDoc* doc, int flags)
{
	tmpSel=new Selection(this, false);
	m_Doc=doc;
	interactive = (flags & LoadSavePlugin::lfInteractive);
}

bool XarPlug::import(QString fNameIn, const TransactionSettings& trSettings, int flags, bool showProgress)
{
	QString fName = fNameIn;
	bool success = false;
	interactive = (flags & LoadSavePlugin::lfInteractive);
	importerFlags = flags;
	cancel = false;
	double x, y, b, h;
	bool ret = false;
	QFileInfo fi = QFileInfo(fName);
	if ( !ScCore->usingGUI() )
	{
		interactive = false;
		showProgress = false;
	}
	if ( showProgress )
	{
		ScribusMainWindow* mw=(m_Doc==0) ? ScCore->primaryMainWindow() : m_Doc->scMW();
		progressDialog = new MultiProgressDialog( tr("Importing: %1").arg(fi.fileName()), CommonStrings::tr_Cancel, mw );
		QStringList barNames, barTexts;
		barNames << "GI";
		barTexts << tr("Analyzing File:");
		QList<bool> barsNumeric;
		barsNumeric << false;
		progressDialog->addExtraProgressBars(barNames, barTexts, barsNumeric);
		progressDialog->setOverallTotalSteps(3);
		progressDialog->setOverallProgress(0);
		progressDialog->setProgress("GI", 0);
		progressDialog->show();
		connect(progressDialog, SIGNAL(canceled()), this, SLOT(cancelRequested()));
		qApp->processEvents();
	}
	else
		progressDialog = NULL;
/* Set default Page to size defined in Preferences */
	x = 0.0;
	y = 0.0;
	b = 0.0;
	h = 0.0;
	if (progressDialog)
	{
		progressDialog->setOverallProgress(1);
		qApp->processEvents();
	}
//	parseHeader(fName, x, y, b, h);
	if (b == 0.0)
		b = PrefsManager::instance()->appPrefs.docSetupPrefs.pageWidth;
	if (h == 0.0)
		h = PrefsManager::instance()->appPrefs.docSetupPrefs.pageHeight;
	docWidth = b;
	docHeight = h;
	baseX = 0;
	baseY = 0;
	if (!interactive || (flags & LoadSavePlugin::lfInsertPage))
	{
		m_Doc->setPage(docWidth, docHeight, 0, 0, 0, 0, 0, 0, false, false);
		m_Doc->addPage(0);
		m_Doc->view()->addPage(0, true);
		baseX = -x;
		baseY = -y;
	}
	else
	{
		if (!m_Doc || (flags & LoadSavePlugin::lfCreateDoc))
		{
			m_Doc=ScCore->primaryMainWindow()->doFileNew(docWidth, docHeight, 0, 0, 0, 0, 0, 0, false, false, 0, false, 0, 1, "Custom", true);
			ScCore->primaryMainWindow()->HaveNewDoc();
			ret = true;
			baseX = m_Doc->currentPage()->xOffset() - x;
			baseY = m_Doc->currentPage()->yOffset() - y;
		}
	}
	if ((!ret) && (interactive))
	{
		baseX = m_Doc->currentPage()->xOffset() - x;
		baseY = m_Doc->currentPage()->yOffset() - y;
	}
	if ((ret) || (!interactive))
	{
		if (docWidth > docHeight)
			m_Doc->PageOri = 1;
		else
			m_Doc->PageOri = 0;
		m_Doc->m_pageSize = "Custom";
	}
	Elements.clear();
	FPoint minSize = m_Doc->minCanvasCoordinate;
	FPoint maxSize = m_Doc->maxCanvasCoordinate;
	FPoint cOrigin = m_Doc->view()->canvasOrigin();
	m_Doc->setLoading(true);
	m_Doc->DoDrawing = false;
	m_Doc->view()->updatesOn(false);
	m_Doc->scMW()->ScriptRunning = true;
	qApp->changeOverrideCursor(QCursor(Qt::WaitCursor));
	QString CurDirP = QDir::currentPath();
	QDir::setCurrent(fi.path());
	if (convert(fName))
	{
		tmpSel->clear();
		QDir::setCurrent(CurDirP);
		if ((Elements.count() > 1) && (!(importerFlags & LoadSavePlugin::lfCreateDoc)))
		{
			bool isGroup = true;
			int firstElem = -1;
			if (Elements.at(0)->Groups.count() != 0)
				firstElem = Elements.at(0)->Groups.top();
			for (int bx = 0; bx < Elements.count(); ++bx)
			{
				PageItem* bxi = Elements.at(bx);
				if (bxi->Groups.count() != 0)
				{
					if (bxi->Groups.top() != firstElem)
						isGroup = false;
				}
				else
					isGroup = false;
			}
			if (!isGroup)
			{
				double minx = 99999.9;
				double miny = 99999.9;
				double maxx = -99999.9;
				double maxy = -99999.9;
				uint lowestItem = 999999;
				uint highestItem = 0;
				for (int a = 0; a < Elements.count(); ++a)
				{
					Elements.at(a)->Groups.push(m_Doc->GroupCounter);
					PageItem* currItem = Elements.at(a);
					lowestItem = qMin(lowestItem, currItem->ItemNr);
					highestItem = qMax(highestItem, currItem->ItemNr);
					double x1, x2, y1, y2;
					currItem->getVisualBoundingRect(&x1, &y1, &x2, &y2);
					minx = qMin(minx, x1);
					miny = qMin(miny, y1);
					maxx = qMax(maxx, x2);
					maxy = qMax(maxy, y2);
				}
				double gx = minx;
				double gy = miny;
				double gw = maxx - minx;
				double gh = maxy - miny;
				PageItem *high = m_Doc->Items->at(highestItem);
				int z = m_Doc->itemAdd(PageItem::Polygon, PageItem::Rectangle, gx, gy, gw, gh, 0, m_Doc->itemToolPrefs.shapeFillColor, m_Doc->itemToolPrefs.shapeLineColor, true);
				PageItem *neu = m_Doc->Items->takeAt(z);
				m_Doc->Items->insert(lowestItem, neu);
				neu->Groups.push(m_Doc->GroupCounter);
				neu->setItemName( tr("Group%1").arg(neu->Groups.top()));
				neu->AutoName = false;
				neu->isGroupControl = true;
				neu->groupsLastItem = high;
				neu->setTextFlowMode(PageItem::TextFlowDisabled);
				for (int a = 0; a < m_Doc->Items->count(); ++a)
				{
					m_Doc->Items->at(a)->ItemNr = a;
				}
				Elements.prepend(neu);
				m_Doc->GroupCounter++;
			}
		}
		m_Doc->DoDrawing = true;
		m_Doc->scMW()->ScriptRunning = false;
		m_Doc->setLoading(false);
		qApp->changeOverrideCursor(QCursor(Qt::ArrowCursor));
		if ((Elements.count() > 0) && (!ret) && (interactive))
		{
			if (flags & LoadSavePlugin::lfScripted)
			{
				bool loadF = m_Doc->isLoading();
				m_Doc->setLoading(false);
				m_Doc->changed();
				m_Doc->setLoading(loadF);
				m_Doc->m_Selection->delaySignalsOn();
				for (int dre=0; dre<Elements.count(); ++dre)
				{
					m_Doc->m_Selection->addItem(Elements.at(dre), true);
				}
				m_Doc->m_Selection->delaySignalsOff();
				m_Doc->m_Selection->setGroupRect();
				m_Doc->view()->updatesOn(true);
			}
			else
			{
				m_Doc->DragP = true;
				m_Doc->DraggedElem = 0;
				m_Doc->DragElements.clear();
				m_Doc->m_Selection->delaySignalsOn();
				for (int dre=0; dre<Elements.count(); ++dre)
				{
					m_Doc->DragElements.append(Elements.at(dre)->ItemNr);
					tmpSel->addItem(Elements.at(dre), true);
				}
				tmpSel->setGroupRect();
				ScriXmlDoc *ss = new ScriXmlDoc();
				ScElemMimeData* md = new ScElemMimeData();
				md->setScribusElem(ss->WriteElem(m_Doc, m_Doc->view(), tmpSel));
				delete ss;
				m_Doc->itemSelection_DeleteItem(tmpSel);
				m_Doc->view()->updatesOn(true);
				m_Doc->m_Selection->delaySignalsOff();
				// We must copy the TransationSettings object as it is owned
				// by handleObjectImport method afterwards
				TransactionSettings* transacSettings = new TransactionSettings(trSettings);
				m_Doc->view()->handleObjectImport(md, transacSettings);
				m_Doc->DragP = false;
				m_Doc->DraggedElem = 0;
				m_Doc->DragElements.clear();
			}
		}
		else
		{
			m_Doc->changed();
			m_Doc->reformPages();
			m_Doc->view()->updatesOn(true);
			if ((importerFlags & LoadSavePlugin::lfCreateDoc) && (!activeLayer.isEmpty()))
				m_Doc->setActiveLayer(activeLayer);
		}
		success = true;
	}
	else
	{
		QDir::setCurrent(CurDirP);
		m_Doc->DoDrawing = true;
		m_Doc->scMW()->ScriptRunning = false;
		m_Doc->view()->updatesOn(true);
		qApp->changeOverrideCursor(QCursor(Qt::ArrowCursor));
	}
	if (interactive)
		m_Doc->setLoading(false);
	//CB If we have a gui we must refresh it if we have used the progressbar
	if ((showProgress) && (!interactive))
		m_Doc->view()->DrawNew();
	return success;
}

XarPlug::~XarPlug()
{
	if (progressDialog)
		delete progressDialog;
	delete tmpSel;
}

void XarPlug::parseHeader(QString fName, double &x, double &y, double &b, double &h)
{
	QFile f(fName);
	if (f.open(QIODevice::ReadOnly))
	{
		QDataStream ts(&f);
		ts.device()->seek(512);
		qint16 pgX, pgY, pgW, pgH, dummy;
		ts >> dummy >> pgX >> pgY >> pgW >> pgH;
		h = pgW - pgX;
		b = pgH - pgY;
		x = pgY;
		y = pgX;
		f.close();
//		qDebug() << "W" << b << "H" << h;
	}
}

bool XarPlug::convert(QString fn)
{
	QString tmp;
	Coords.resize(0);
	Coords.svgInit();
	imageData.resize(0);
	importedColors.clear();
	firstLayer = true;
	activeLayer = "";
	currentLayer = 0;
	XarColor color;
	color.colorType = 0;
	color.colorModel = 2;
	color.colorRef = 0;
	color.component1 = 0;
	color.component2 = 0;
	color.component3 = 0;
	color.component1 = 0;
	color.name = CommonStrings::None;
	XarColorMap.insert(-1, color);
	if (!m_Doc->PageColors.contains("Black"))
	{
		m_Doc->PageColors.insert("Black", ScColor(0, 0, 0, 255));
		importedColors.append("Black");
	}
	color.name = "Black";
	XarColorMap.insert(-2, color);
	if (!m_Doc->PageColors.contains("White"))
	{
		m_Doc->PageColors.insert("White", ScColor(0, 0, 0, 0));
		importedColors.append("White");
	}
	color.name = "White";
	XarColorMap.insert(-3, color);
	if (!m_Doc->PageColors.contains("Red"))
	{
		m_Doc->PageColors.insert("Red", ScColor(255, 0, 0));
		importedColors.append("Red");
	}
	color.name = "Red";
	XarColorMap.insert(-4, color);
	if (!m_Doc->PageColors.contains("Green"))
	{
		m_Doc->PageColors.insert("Green", ScColor(255, 0, 255, 0));
		importedColors.append("Green");
	}
	color.name = "Green";
	XarColorMap.insert(-5, color);
	if (!m_Doc->PageColors.contains("Blue"))
	{
		m_Doc->PageColors.insert("Blue", ScColor(0, 0, 255));
		importedColors.append("Blue");
	}
	color.name = "Blue";
	XarColorMap.insert(-6, color);
	if (!m_Doc->PageColors.contains("Cyan"))
	{
		m_Doc->PageColors.insert("Cyan", ScColor(255, 0, 0, 0));
		importedColors.append("Cyan");
	}
	color.name = "Cyan";
	XarColorMap.insert(-7, color);
	if (!m_Doc->PageColors.contains("Magenta"))
	{
		m_Doc->PageColors.insert("Magenta", ScColor(0, 255, 0, 0));
		importedColors.append("Magenta");
	}
	color.name = "Magenta";
	XarColorMap.insert(-8, color);
	if (!m_Doc->PageColors.contains("Yellow"))
	{
		m_Doc->PageColors.insert("Yellow", ScColor(0, 0, 255, 0));
		importedColors.append("Yellow");
	}
	color.name = "Yellow";
	XarColorMap.insert(-9, color);
	ignoreableTags << 2 << 40 << 41 << 43 << 46 << 47 << 53 << 61 << 62 << 63 << 80 << 90 << 91 << 92 << 93 << 111;
	ignoreableTags << 2150 << 2151 << 2205 << 2900 << 2901;
	ignoreableTags << 4031 << 4087 << 4114 << 4115 << 4116 << 4124;
	if(progressDialog)
	{
		progressDialog->setOverallProgress(2);
		progressDialog->setLabel("GI", tr("Generating Items"));
		qApp->processEvents();
	}
	QFile f(fn);
	if (f.open(QIODevice::ReadOnly))
	{
		int fSize = (int) f.size();
		if (progressDialog)
		{
			progressDialog->setTotalSteps("GI", fSize);
			qApp->processEvents();
		}
		QDataStream ts(&f);
		ts.setByteOrder(QDataStream::LittleEndian);
		parseXar(ts);
		if (Elements.count() == 0)
		{
			if (importedColors.count() != 0)
			{
				for (int cd = 0; cd < importedColors.count(); cd++)
				{
					m_Doc->PageColors.remove(importedColors[cd]);
				}
			}
		}
		f.close();
	}
	if (progressDialog)
		progressDialog->close();
	return true;
}

void XarPlug::parseXar(QDataStream &ts)
{
	XarStyle *gc = new XarStyle;
	m_gc.push( gc );
	quint32 id;
	ts >> id;
	if (id != 0x41524158)
		return;
	ts >> id;
	if (id != 0x0A0DA3A3)
		return;
	recordCounter = 0;
	while (!ts.atEnd())
	{
		quint32 opCode, dataLen;
		ts >> opCode;
		ts >> dataLen;
		recordCounter++;
		if (opCode == 30)
		{
			ts.skipRawData(dataLen);
			quint64 pos = ts.device()->pos();
			QtIOCompressor compressor(ts.device(), 6, 1);
			compressor.setStreamFormat(QtIOCompressor::RawZipFormat);
			compressor.open(QIODevice::ReadOnly);
			QDataStream tsc(&compressor);
			tsc.setByteOrder(QDataStream::LittleEndian);
			tsc.device()->seek(pos);
			while (!tsc.atEnd())
			{
				tsc >> opCode;
				tsc >> dataLen;
				recordCounter++;
				if (opCode == 31)
				{
					tsc.skipRawData(dataLen);
					break;
				}
				handleTags(opCode, dataLen, tsc);
			}
			ts.skipRawData(dataLen+1);
		}
		else
			handleTags(opCode, dataLen, ts);
		if (progressDialog)
		{
			progressDialog->setProgress("GI", ts.device()->pos());
			qApp->processEvents();
		}
	}
}

void XarPlug::handleTags(quint32 tag, quint32 dataLen, QDataStream &ts)
{
	XarStyle *gc = m_gc.top();
	bool closed = false;
	if (ignoreableTags.contains(tag))
	{
		ts.skipRawData(dataLen);
		return;
	}
//	qDebug() << QString("OpCode: %1 Data Len %2").arg(tag).arg(dataLen, 8, 16, QLatin1Char('0'));
	if (tag == 0)
	{
		popGraphicContext();
//		delete( m_gc.pop() );
//		qDebug() << "Stack dropped to" << m_gc.count();
	}
	else if (tag == 1)
	{
		addGraphicContext();
//		qDebug() << "Stack pushed to" << m_gc.count();
	}
	else if (tag == 10)
		addToAtomic(dataLen, ts);
	else if (tag == 45)
		handleSpreadInfo(ts);
	else if (tag == 48)
		handleLayerInfo(ts);
	else if (tag == 50)
		handleColorRGB(ts);
	else if (tag == 51)
		handleComplexColor(ts);
/*	else if ((tag == 61) || (tag == 62) || (tag == 63))
	{
		QImage image;
		QByteArray data;
		data.resize(dataLen);
		ts.readRawData(data.data(), dataLen);
		image.loadFromData(data);
		int z = m_Doc->itemAdd(PageItem::ImageFrame, PageItem::Unspecified, baseX, baseY, image.width(), image.height(), 0, m_Doc->itemToolPrefs.imageFillColor, CommonStrings::None, true);
		PageItem *ite = m_Doc->Items->at(z);
		ite->tempImageFile = new QTemporaryFile(QDir::tempPath() + "/scribus_temp_xar_XXXXXX.png");
		ite->tempImageFile->open();
		QString fileName = getLongPathName(ite->tempImageFile->fileName());
		ite->tempImageFile->close();
		ite->isInlineImage = true;
		image.save(fileName, "PNG");
		ite->moveBy(m_Doc->currentPage()->xOffset(), m_Doc->currentPage()->yOffset());
		m_Doc->LoadPict(fileName, z);
		ite->setImageScalingMode(false, false);
		ite->ClipEdited = true;
		ite->FrameType = 3;
		ite->setFillShade(CurrFillShade);
		ite->setLineShade(CurrStrokeShade);
		FPoint wh = getMaxClipF(&ite->PoLine);
		ite->setWidthHeight(wh.x(),wh.y());
		ite->setTextFlowMode(PageItem::TextFlowDisabled);
		m_Doc->AdjustItemSize(ite);
		ite->OldB2 = ite->width();
		ite->OldH2 = ite->height();
		ite->updateClip();
		Elements.append(ite);
	} */
	else if ((tag == 67) || (tag == 68) || (tag == 71))
		defineBitmap(ts, dataLen, tag);
	else if (tag == 104)
		createGroupItem();
	else if (tag == 114)
	{
		closed = handlePathRel(ts, dataLen);
		if (closed)
			createPolygonItem(1);
		else
			createPolylineItem(1);
	}
	else if (tag == 115)
	{
		closed = handlePathRel(ts, dataLen);
		if (closed)
			createPolygonItem(0);
		else
			createPolylineItem(0);
	}
	else if (tag == 116)
	{
		closed = handlePathRel(ts, dataLen);
		if (closed)
			createPolygonItem(2);
		else
			createPolylineItem(2);
	}
//	else if (tag == 118)
//		createSimilarItem(ts);
	else if (tag == 150)
		handleFlatFill(ts);
	else if (tag == 151)
		handleLineColor(ts);
	else if (tag == 152)
		handleLineWidth(ts);
	else if (tag == 153)
		handleSimpleGradient(ts, dataLen, true);
	else if (tag == 154)
		handleSimpleGradient(ts, dataLen, false);
	else if (tag == 155)
		handleSimpleGradientElliptical(ts, dataLen);
	else if (tag == 157)
		handleBitmapFill(ts, dataLen);
	else if (tag == 158)
		handleContoneBitmapFill(ts, dataLen);
	else if (tag == 166)
		handleFlatFillTransparency(ts);
	else if (tag == 167)
		handleSimpleGradientTransparency(ts, dataLen, true);
	else if (tag == 168)
		handleSimpleGradientTransparency(ts, dataLen, false);
	else if (tag == 169)
		handleEllipticalGradientTransparency(ts, dataLen);
	else if (tag == 171)
		handleBitmapTransparency(ts, dataLen);
	else if (tag == 173)
		handleFlatLineTransparency(ts);
	else if ((tag == 174) || (tag == 175))
		handleLineEnd(ts);
	else if (tag == 176)
		handleLineJoin(ts);
	else if (tag == 178)
		handleFillRule(ts);
	else if (tag == 190)
		gc->FillCol = CommonStrings::None;
	else if (tag == 191)
		gc->FillCol = "Black";
	else if (tag == 192)
		gc->FillCol = "White";
	else if (tag == 193)
		gc->StrokeCol = CommonStrings::None;
	else if (tag == 194)
		gc->StrokeCol = "Black";
	else if (tag == 195)
		gc->StrokeCol = "White";
	else if (tag == 198)
		handleBitmap(ts);
	else if (tag == 1000)
		createRectangleItem(ts, true);
	else if (tag == 1100)
		createRectangleItem(ts);
	else if (tag == 1901)
		handleQuickShapeSimple(ts, dataLen);
	else if ((tag == 2000) || (tag == 2001))
		defineTextFontFace(ts, dataLen);
	else if (tag == 2100)
		startSimpleText(ts, dataLen);
	else if (tag == 2101)
		startComplexText(ts, dataLen);
	else if (tag == 2201)
		handleTextString(ts, dataLen);
	else if (tag == 2202)
		handleTextChar(ts);
	else if (tag == 2203)
		endTextLine();
	else if (tag == 2206)
		handleLineInfo(ts);
	else if ((tag == 2902) || (tag == 2903) || (tag == 2904))
		handleTextAlignment(tag);
	else if (tag == 2906)
		handleTextFontSize(ts);
	else if (tag == 2907)
		handleTextFont(ts);
	else if (tag == 2908)
		gc->FontBold = true;
	else if (tag == 2909)
		gc->FontBold = false;
	else if (tag == 2910)
		gc->FontItalic = true;
	else if (tag == 2911)
		gc->FontItalic = false;
	else if (tag == 2912)
		gc->FontUnderline = true;
	else if (tag == 2913)
		gc->FontUnderline = false;
	else if (tag == 2918)
		handleTextTracking(ts);
	else if (tag == 2919)
		handleTextAspectRatio(ts);
	else if (tag == 2920)
		handleTextBaseline(ts);
	else if (tag == 4075)
		handleMultiGradient(ts, true);
	else if (tag == 4076)
		handleMultiGradient(ts, false);
	else if (tag == 4077)
		handleMultiGradientElliptical(ts);
	else if (tag == 4121)
		handleSimpleGradientSkewed(ts, dataLen);
	else if (tag == 4122)
		handleMultiGradientSkewed(ts);
	else if (tag == 4123)
		handleSimpleGradientTransparencySkewed(ts, dataLen);
	else if (tag == 4084)
		createClipItem();
	else if (tag == 4085)
		finishClip();
	else
	{
//		if (m_gc.count() > 3)
//		{
//			if ((tag > 1999) && (tag < 3000))
//				qDebug() << QString("Unhandled OpCode: %1 Data Len %2").arg(tag).arg(dataLen, 8, 16, QLatin1Char('0'));
//		}
		ts.skipRawData(dataLen);
	}
}

void XarPlug::handleTextFontSize(QDataStream &ts)
{
	quint32 size;
	ts >> size;
	XarStyle *gc = m_gc.top();
	gc->FontSize = size / 1000.0;
//	qDebug() << "Font Size" << gc->FontSize;
}

void XarPlug::defineTextFontFace(QDataStream &ts, quint32 dataLen)
{
	quint32 bytesRead = 0;
	quint16 charC = 0;
	ts >> charC;
	bytesRead += 2;
	QString FullFontName = "";
	while (charC != 0)
	{
		FullFontName += QChar(charC);
		ts >> charC;
		bytesRead += 2;
	}
	charC = 0;
	ts >> charC;
	bytesRead += 2;
	QString TypeFaceName = "";
	while (charC != 0)
	{
		TypeFaceName += QChar(charC);
		ts >> charC;
		bytesRead += 2;
	}
	ts.skipRawData(dataLen - bytesRead);
	fontRef.insert(recordCounter, TypeFaceName);
//	qDebug() << "Define Font" << FullFontName << TypeFaceName;
}

void XarPlug::handleTextFont(QDataStream &ts)
{
	XarStyle *gc = m_gc.top();
	qint32 val;
	ts >> val;
	if (val > -1)
	{
		if (fontRef.contains(val))
			gc->FontFamily = fontRef[val];
	}
//	qDebug() << "Using Font" << gc->FontFamily;
}

void XarPlug::handleTextString(QDataStream &ts, quint32 dataLen)
{
	quint32 l = dataLen / 2;
	quint16 val;
	XarStyle *gc = m_gc.top();
	for (quint32 a = 0; a < l; a++)
	{
		ts >> val;
		gc->itemText += QChar(val);
	}
//	qDebug() << "String" << gc->itemText;
}

void XarPlug::handleTextChar(QDataStream &ts)
{
	quint16 val;
	ts >> val;
	XarStyle *gc = m_gc.top();
	gc->itemText += QChar(val);
//	qDebug() << "Char" << QChar(val);
}

void XarPlug::handleLineInfo(QDataStream &ts)
{
	qint32 width, height, spacing;
	ts >> width >> height >> spacing;
	XarStyle *gc = m_gc.top();
	gc->LineHeight = -spacing / 1000.0;
	gc->LineWidth = width / 1000.0;
//	qDebug() << "Linespacing" << gc->LineHeight;
}

void XarPlug::handleTextAlignment(quint32 tag)
{
	XarStyle *gc = m_gc.top();
	if (tag == 2902)
		gc->TextAlignment = 0;
	else if (tag == 2903)
		gc->TextAlignment = 1;
	else if (tag == 2904)
		gc->TextAlignment = 2;
}

void XarPlug::handleTextTracking(QDataStream &ts)
{
	qint32 val;
	ts >> val;
//	qDebug() << "Tracking" << val;
}

void XarPlug::handleTextAspectRatio(QDataStream &ts)
{
	quint32 val;
	ts >> val;
	double scaleX = decodeFixed16(val);
	XarStyle *gc = m_gc.top();
	gc->FontStretch = scaleX;
//	qDebug() << "Aspect Ratio" << scaleX;
}

void XarPlug::handleTextBaseline(QDataStream &ts)
{
	qint32 val;
	ts >> val;
	TextY += val / 1000.0;
}

void XarPlug::endTextLine()
{
	XarStyle *gc = m_gc.top();
	TextY += gc->LineHeight;
	QPainterPath painterPath;
	QFont textFont = QFont(gc->FontFamily, gc->FontSize);
	if (gc->FontSize >= 1)
		textFont.setPixelSize(gc->FontSize);
	else
		textFont.setPointSizeF(gc->FontSize * 72.0 / 96.0);
	textFont.setBold(gc->FontBold);
	textFont.setItalic(gc->FontItalic);
	textFont.setUnderline(gc->FontUnderline);
	painterPath.addText( 0, 0, textFont, gc->itemText);
	QRectF bound = painterPath.boundingRect();
	Coords.resize(0);
	Coords.fromQPainterPath(painterPath);
	Coords.map(textMatrix);
	Coords.scale(gc->FontStretch, 1.0);
	Coords.translate(TextX, TextY);
	if (gc->TextAlignment == 1)
		Coords.translate(-bound.width() / 2.0, 0);
	else if (gc->TextAlignment == 2)
		Coords.translate(-bound.width(), 0);
	if (Coords.size() > 0)
	{
		int z = m_Doc->itemAdd(PageItem::Polygon, PageItem::Unspecified, baseX, baseY, 10, 10, 0, gc->FillCol, CommonStrings::None, true);
		finishItem(z);
	}
	gc->itemText = "";
//	qDebug() << "End of Line, new Y-Pos" << TextY;
}

void XarPlug::startSimpleText(QDataStream &ts, quint32 dataLen)
{
	quint32 flag;
	double textX, textY;
	readCoords(ts, textX, textY);
	if (dataLen > 8)
		ts >> flag;
	XarStyle *gc = m_gc.top();
	gc->itemText = "";
	TextX = textX;
	TextY = docHeight - textY;
	textMatrix = QTransform();
//	qDebug() << "Simple Text at" << textX << docHeight - textY;
}

void XarPlug::startComplexText(QDataStream &ts, quint32 dataLen)
{
	quint32 flag;
	quint32 scX, skX, skY, scY;
	double transX, transY;
	ts >> scX >> skX >> skY >> scY;
	readCoords(ts, transX, transY);
	double scaleX = decodeFixed16(scX);
	double scaleY = decodeFixed16(scY);
	double skewX = decodeFixed16(skX);
	double skewY = decodeFixed16(skY);
	if (dataLen > 24)
		ts >> flag;
	XarStyle *gc = m_gc.top();
	gc->itemText = "";
	TextX = transX;
	TextY = docHeight - transY;
	textMatrix = QTransform(scaleX, -skewX, -skewY, scaleY, 0, 0);
//	qDebug() << "Complex Text at" << transX << docHeight - transY << "Matrix" << scaleX << skewX << skewY << scaleY;
}

void XarPlug::handleFillRule(QDataStream &ts)
{
	quint8 val;
	ts >> val;
	XarStyle *gc = m_gc.top();
	if (val == 0)
		gc->fillRule = false;
	else
		gc->fillRule = true;
}

void XarPlug::handleLineEnd(QDataStream &ts)
{
	quint8 val;
	ts >> val;
	XarStyle *gc = m_gc.top();
	if (val == 0)
		gc->PLineEnd = Qt::FlatCap;
	else if (val == 1)
		gc->PLineEnd = Qt::RoundCap;
	else if (val == 2)
		gc->PLineEnd = Qt::SquareCap;
}

void XarPlug::handleLineJoin(QDataStream &ts)
{
	quint8 val;
	ts >> val;
	XarStyle *gc = m_gc.top();
	if (val == 0)
		gc->PLineJoin = Qt::MiterJoin;
	else if (val == 1)
		gc->PLineJoin = Qt::RoundJoin;
	else if (val == 2)
		gc->PLineJoin = Qt::BevelJoin;
}

void XarPlug::handleQuickShapeSimple(QDataStream &ts, quint32 dataLen)
{
	XarStyle *gc = m_gc.top();
	quint32 bytesRead = 0;
	double minorAxisX, minorAxisY, majorAxisX, majorAxisY;
	quint16 numSides = 0;
	quint8 flags;
	ts >> flags;
	bytesRead++;
	ts >> numSides;
	bytesRead += 2;
	readCoords(ts, majorAxisX, majorAxisY);
	bytesRead += 8;
	readCoords(ts, minorAxisX, minorAxisY);
	bytesRead += 8;
	quint32 scX, skX, skY, scY;
	double transX, transY;
	ts >> scX >> skX >> skY >> scY;
	readCoords(ts, transX, transY);
	bytesRead += 24;
	double scaleX = decodeFixed16(scX);
	double scaleY = decodeFixed16(scY);
	double skewX = decodeFixed16(skX);
	double skewY = decodeFixed16(skY);
	double r1, r2, r3, r4;
	ts >> r1 >> r2 >> r3 >> r4;
	bytesRead += 32;
//	qDebug() << "Regular Polygon";
//	qDebug() << "Sides" << numSides;
//	qDebug() << "MajorAxis" << majorAxisX << majorAxisY;
//	qDebug() << "MinorAxis" << minorAxisX << minorAxisY;
//	qDebug() << "Matrix" << scaleX << skewX << skewY << scaleY << transX << transY;
//	qDebug() << "Radii" << r1 << r2 << r3 << r4;
//	qDebug() << "Flags" << flags;
//	qDebug() << "Bytes read" << bytesRead << "of" << dataLen;
	ts.skipRawData(dataLen - bytesRead);
	int z = m_Doc->itemAdd(PageItem::Polygon, PageItem::Unspecified, baseX, baseY, 10, 10, gc->LWidth, gc->FillCol, gc->StrokeCol, true);
	double w = distance(minorAxisX, minorAxisY);
	double h = distance(majorAxisX, majorAxisY);
	Coords.resize(0);
	Coords.svgInit();
	QPainterPath path;
	if (flags & 1)
		path.addEllipse(QPointF(0,0), w, h);
	else
		path = RegularPolygon(w * 2, h * 2, numSides, flags & 2, r1, 225, 0);
	Coords.fromQPainterPath(path);
	if (!(flags & 1))
		Coords.translate(-w, -h);
	QTransform matrix(-scaleX, skewX, -skewY, scaleY, 0, 0);
	Coords.map(matrix);
	Coords.translate(transX, -transY);
	Coords.translate(0, docHeight);
	finishItem(z);
//	PageItem *ite = m_Doc->Items->at(z);
//	qDebug() << "Item" << ite->itemName();
}

void XarPlug::handleFlatFillTransparency(QDataStream &ts)
{
	quint8 transVal, transType;
	ts >> transVal >> transType;
	XarStyle *gc = m_gc.top();
	if (transType > 0)
	{
		gc->FillOpacity = transVal / 255.0;
		gc->FillBlend = convertBlendMode(transType);
		gc->GradMask = 0;
	}
}

void XarPlug::handleSimpleGradientTransparency(QDataStream &ts, quint32 dataLen, bool linear)
{
	XarStyle *gc = m_gc.top();
	double blx, bly, brx, bry;
	quint8 transStart, transEnd, transType;
	readCoords(ts, blx, bly);
	readCoords(ts, brx, bry);
	ts >> transStart >> transEnd >> transType;
	gc->FillBlend = convertBlendMode(transType);
	if (dataLen == 35)
	{
		double p, p1;
		ts >> p >> p1;
	}
	if (linear)
	{
		gc->GradMask = 1;
		gc->MaskGradient = VGradient(VGradient::linear);
	}
	else
	{
		gc->GradMask = 2;
		gc->MaskGradient = VGradient(VGradient::radial);
	}
	gc->MaskGradient.clearStops();
	gc->MaskGradient.addStop( ScColorEngine::getRGBColor(m_Doc->PageColors["Black"], m_Doc), 0.0, 0.5, 1.0 - transStart / 255.0, "Black", 100 );
	gc->MaskGradient.addStop( ScColorEngine::getRGBColor(m_Doc->PageColors["Black"], m_Doc), 1.0, 0.5, 1.0 - transEnd / 255.0, "Black", 100 );
	gc->GradMaskX1 = blx + baseX + m_Doc->currentPage()->xOffset();
	gc->GradMaskY1 = (docHeight - bly) + baseY + m_Doc->currentPage()->yOffset();
	gc->GradMaskX2 = brx + baseX + m_Doc->currentPage()->xOffset();
	gc->GradMaskY2 = (docHeight - bry) + baseY + m_Doc->currentPage()->yOffset();
}

void XarPlug::handleSimpleGradientTransparencySkewed(QDataStream &ts, quint32 dataLen)
{
	XarStyle *gc = m_gc.top();
	double blx, bly, brx, bry, tlx, tly;
	quint8 transStart, transEnd, transType;
	readCoords(ts, blx, bly);
	readCoords(ts, brx, bry);
	readCoords(ts, tlx, tly);
	ts >> transStart >> transEnd >> transType;
	gc->FillBlend = convertBlendMode(transType);
	if (dataLen == 43)
	{
		double p, p1;
		ts >> p >> p1;
	}
	gc->MaskGradient = VGradient(VGradient::linear);
	gc->MaskGradient.clearStops();
	gc->MaskGradient.addStop( ScColorEngine::getRGBColor(m_Doc->PageColors["Black"], m_Doc), 0.0, 0.5, 1.0 - transStart / 255.0, "Black", 100 );
	gc->MaskGradient.addStop( ScColorEngine::getRGBColor(m_Doc->PageColors["Black"], m_Doc), 1.0, 0.5, 1.0 - transEnd / 255.0, "Black", 100 );
	double distX = distance(brx - blx, bry - bly);
	double distY = distance(tlx - blx, tly - bly);
	double rotB = xy2Deg(brx - blx, bry - bly);
	double rotS = xy2Deg(tlx - blx, tly - bly);
	gc->GradMaskScale = distY / distX;
	gc->GradMaskSkew = rotS - 90 - rotB;
	gc->GradMaskX1 = blx + baseX + m_Doc->currentPage()->xOffset();
	gc->GradMaskY1 = (docHeight - bly) + baseY + m_Doc->currentPage()->yOffset();
	gc->GradMaskX2 = brx + baseX + m_Doc->currentPage()->xOffset();
	gc->GradMaskY2 = (docHeight - bry) + baseY + m_Doc->currentPage()->yOffset();
	gc->GradMask = 1;
}

void XarPlug::handleEllipticalGradientTransparency(QDataStream &ts, quint32 dataLen)
{
	XarStyle *gc = m_gc.top();
	double blx, bly, brx, bry, tlx, tly;
	quint8 transStart, transEnd, transType;
	readCoords(ts, blx, bly);
	readCoords(ts, tlx, tly);
	readCoords(ts, brx, bry);
	ts >> transStart >> transEnd >> transType;
	gc->FillBlend = convertBlendMode(transType);
	if (dataLen == 43)
	{
		double p, p1;
		ts >> p >> p1;
	}
	gc->MaskGradient = VGradient(VGradient::radial);
	gc->MaskGradient.clearStops();
	gc->MaskGradient.addStop( ScColorEngine::getRGBColor(m_Doc->PageColors["Black"], m_Doc), 0.0, 0.5, 1.0 - transStart / 255.0, "Black", 100 );
	gc->MaskGradient.addStop( ScColorEngine::getRGBColor(m_Doc->PageColors["Black"], m_Doc), 1.0, 0.5, 1.0 - transEnd / 255.0, "Black", 100 );
	double distX = distance(brx - blx, bry - bly);
	double distY = distance(tlx - blx, tly - bly);
	double rotB = xy2Deg(brx - blx, bry - bly);
	double rotS = xy2Deg(tlx - blx, tly - bly);
	gc->GradMaskScale = distY / distX;
	gc->GradMaskSkew = rotS - 90 - rotB;
	gc->GradMaskX1 = blx + baseX + m_Doc->currentPage()->xOffset();
	gc->GradMaskY1 = (docHeight - bly) + baseY + m_Doc->currentPage()->yOffset();
	gc->GradMaskX2 = brx + baseX + m_Doc->currentPage()->xOffset();
	gc->GradMaskY2 = (docHeight - bry) + baseY + m_Doc->currentPage()->yOffset();
	gc->GradMask = 2;
}

void XarPlug::handleBitmapTransparency(QDataStream &ts, quint32 dataLen)
{
	XarStyle *gc = m_gc.top();
	qint32 bref;
	double blx, bly, brx, bry, tlx, tly;
	quint8 transStart, transEnd, transType;
	readCoords(ts, blx, bly);
	readCoords(ts, brx, bry);
	readCoords(ts, tlx, tly);
	ts >> transStart >> transEnd >> transType;
	ts >> bref;
	gc->FillBlend = convertBlendMode(transType);
	if (dataLen == 47)
	{
		double p, p1;
		ts >> p >> p1;
	}
	double distX = distance(brx - blx, bry - bly);
	double distY = distance(tlx - blx, tly - bly);
	double rotB = xy2Deg(brx - blx, bry - bly);
	double rotS = xy2Deg(tlx - blx, tly - bly);
	if (patternRef.contains(bref))
	{
		QString imgNam = m_Doc->docPatterns[patternRef[bref]].items.at(0)->externalFile();
		QImage image;
		image.load(imgNam);
		int h = image.height();
		int w = image.width();
		int k;
		int ts = transStart;
		int te = transEnd;
		QRgb *s;
		QRgb r;
		for( int yi=0; yi < h; ++yi )
		{
			s = (QRgb*)(image.scanLine( yi ));
			for( int xi=0; xi < w; ++xi )
			{
				r = *s;
				k = qMin(qRound(0.3 * qRed(r) + 0.59 * qGreen(r) + 0.11 * qBlue(r)), 255);
				if (qAlpha(r) == 0)
					k = 255;
				k = qBound(ts, k, te);
				*s = qRgba(qRed(r), qGreen(r), qBlue(r), 255 - k);
				s++;
			}
		}
		ScPattern pat = ScPattern();
		pat.setDoc(m_Doc);
		PageItem* newItem = new PageItem_ImageFrame(m_Doc, 0, 0, 1, 1, 0, CommonStrings::None, CommonStrings::None);
		newItem->tempImageFile = new QTemporaryFile(QDir::tempPath() + "/scribus_temp_xar_XXXXXX.png");
		newItem->tempImageFile->open();
		QString fileName = getLongPathName(newItem->tempImageFile->fileName());
		newItem->tempImageFile->close();
		newItem->isInlineImage = true;
		image.save(fileName, "PNG");
		if (newItem->loadImage(fileName, false, 72, false))
		{
			pat.width = image.width();
			pat.height = image.height();
			pat.scaleX = (72.0 / newItem->pixm.imgInfo.xres) * newItem->pixm.imgInfo.lowResScale;
			pat.scaleY = (72.0 / newItem->pixm.imgInfo.xres) * newItem->pixm.imgInfo.lowResScale;
			pat.pattern = newItem->pixm.qImage().copy();
			newItem->setWidth(pat.pattern.width());
			newItem->setHeight(pat.pattern.height());
			newItem->SetRectFrame();
			newItem->gXpos = 0.0;
			newItem->gYpos = 0.0;
			newItem->gWidth = pat.pattern.width();
			newItem->gHeight = pat.pattern.height();
			pat.items.append(newItem);
			newItem->ItemNr = pat.items.count();
		}
		QString patternName = patternRef[bref]+"_"+newItem->itemName();
		patternName = patternName.trimmed().simplified().replace(" ", "_");
		m_Doc->addPattern(patternName, pat);
		gc->maskPattern = patternName;
		gc->patternMaskScaleX = distX / pat.width * 100;
		gc->patternMaskScaleY = distY / pat.height * 100;
		gc->patternMaskOffsetX = 0.0;
		gc->patternMaskOffsetY = 0.0;
		gc->patternMaskRotation = -rotB;
		gc->patternMaskSkewX = rotS - 90 - rotB;
		gc->patternMaskSkewY = 0.0;
		gc->GradMask = 3;
	}
}

int XarPlug::convertBlendMode(int val)
{
	int ret = 0;
	if (val == 2)
		ret = 6;
	else if (val == 3)
		ret = 10;
	else if (val == 5)
		ret = 13;
	else if (val == 7)
		ret = 7;
	else if (val == 9)
		ret = 15;
	else if (val == 10)
		ret = 12;
	else
		ret = 0;
	return ret;
}

void XarPlug::handleSimpleGradientElliptical(QDataStream &ts, quint32 dataLen)
{
	XarStyle *gc = m_gc.top();
	double blx, bly, brx, bry, tlx, tly;
	qint32 colRef1, colRef2;
	readCoords(ts, blx, bly);
	readCoords(ts, tlx, tly);
	readCoords(ts, brx, bry);
	ts >> colRef1 >> colRef2;
	if (dataLen == 48)
	{
		double p, p1;
		ts >> p >> p1;
	}
	gc->FillGradient = VGradient(VGradient::linear);
	gc->FillGradient.clearStops();
	QString gCol1 = "Black";
	QString gCol2 = "Black";
	if (XarColorMap.contains(colRef1))
		gCol1 = XarColorMap[colRef1].name;
	if (XarColorMap.contains(colRef2))
		gCol2 = XarColorMap[colRef2].name;
	if (gCol1 != CommonStrings::None)
	{
		const ScColor& gradC1 = m_Doc->PageColors[gCol1];
		gc->FillGradient.addStop( ScColorEngine::getRGBColor(gradC1, m_Doc), 0.0, 0.5, 1.0, gCol1, 100 );
	}
	else
		gc->FillGradient.addStop( QColor(255, 255, 255, 0), 0.0, 0.5, 0.0, gCol1, 100 );
	if (gCol2 != CommonStrings::None)
	{
		const ScColor& gradC2 = m_Doc->PageColors[gCol2];
		gc->FillGradient.addStop( ScColorEngine::getRGBColor(gradC2, m_Doc), 1.0, 0.5, 1.0, gCol2, 100 );
	}
	else
		gc->FillGradient.addStop( QColor(255, 255, 255, 0), 1.0, 0.5, 0.0, gCol2, 100 );
	gc->FillGradientType = 7;
	double distX = distance(brx - blx, bry - bly);
	double distY = distance(tlx - blx, tly - bly);
	double rotB = xy2Deg(brx - blx, bry - bly);
	double rotS = xy2Deg(tlx - blx, tly - bly);
	gc->GrScale = distY / distX;
	gc->GrSkew = rotS - 90 - rotB;
	gc->GradFillX1 = blx + baseX + m_Doc->currentPage()->xOffset();
	gc->GradFillY1 = (docHeight - bly) + baseY + m_Doc->currentPage()->yOffset();
	gc->GradFillX2 = brx + baseX + m_Doc->currentPage()->xOffset();
	gc->GradFillY2 = (docHeight - bry) + baseY + m_Doc->currentPage()->yOffset();
}

void XarPlug::handleMultiGradientElliptical(QDataStream &ts)
{
	XarStyle *gc = m_gc.top();
	double blx, bly, brx, bry, tlx, tly;
	qint32 colRef1, colRef2;
	readCoords(ts, blx, bly);
	readCoords(ts, tlx, tly);
	readCoords(ts, brx, bry);
	ts >> colRef1 >> colRef2;
	gc->FillGradient = VGradient(VGradient::linear);
	gc->FillGradient.clearStops();
	QString gCol1 = "Black";
	QString gCol2 = "Black";
	if (XarColorMap.contains(colRef1))
		gCol1 = XarColorMap[colRef1].name;
	if (XarColorMap.contains(colRef2))
		gCol2 = XarColorMap[colRef2].name;
	if (gCol1 != CommonStrings::None)
	{
		const ScColor& gradC1 = m_Doc->PageColors[gCol1];
		gc->FillGradient.addStop( ScColorEngine::getRGBColor(gradC1, m_Doc), 0.0, 0.5, 1.0, gCol1, 100 );
	}
	else
		gc->FillGradient.addStop( QColor(255, 255, 255, 0), 0.0, 0.5, 0.0, gCol1, 100 );
	quint32 numCols;
	ts >> numCols;
	for (uint a = 0; a < numCols; a++)
	{
		double cpos;
		qint32 colRef;
		ts >> cpos;
		ts >> colRef;
		QString gCol = "Black";
		if (XarColorMap.contains(colRef))
			gCol = XarColorMap[colRef].name;
		if (gCol != CommonStrings::None)
		{
			const ScColor& gradC = m_Doc->PageColors[gCol];
			gc->FillGradient.addStop( ScColorEngine::getRGBColor(gradC, m_Doc), cpos, 0.5, 1.0, gCol, 100 );
		}
		else
			gc->FillGradient.addStop( QColor(255, 255, 255, 0), cpos, 0.5, 0.0, gCol, 100 );
	}
	if (gCol2 != CommonStrings::None)
	{
		const ScColor& gradC2 = m_Doc->PageColors[gCol2];
		gc->FillGradient.addStop( ScColorEngine::getRGBColor(gradC2, m_Doc), 1.0, 0.5, 1.0, gCol2, 100 );
	}
	else
		gc->FillGradient.addStop( QColor(255, 255, 255, 0), 1.0, 0.5, 0.0, gCol2, 100 );
	gc->FillGradientType = 7;
	double distX = distance(brx - blx, bry - bly);
	double distY = distance(tlx - blx, tly - bly);
	double rotB = xy2Deg(brx - blx, bry - bly);
	double rotS = xy2Deg(tlx - blx, tly - bly);
	gc->GrScale = distY / distX;
	gc->GrSkew = rotS - 90 - rotB;
	gc->GradFillX1 = blx + baseX + m_Doc->currentPage()->xOffset();
	gc->GradFillY1 = (docHeight - bly) + baseY + m_Doc->currentPage()->yOffset();
	gc->GradFillX2 = brx + baseX + m_Doc->currentPage()->xOffset();
	gc->GradFillY2 = (docHeight - bry) + baseY + m_Doc->currentPage()->yOffset();
}

void XarPlug::handleMultiGradientSkewed(QDataStream &ts)
{
	XarStyle *gc = m_gc.top();
	double blx, bly, brx, bry, tlx, tly;
	qint32 colRef1, colRef2;
	readCoords(ts, blx, bly);
	readCoords(ts, brx, bry);
	readCoords(ts, tlx, tly);
	ts >> colRef1 >> colRef2;
	gc->FillGradient = VGradient(VGradient::linear);
	gc->FillGradient.clearStops();
	QString gCol1 = "Black";
	QString gCol2 = "Black";
	if (XarColorMap.contains(colRef1))
		gCol1 = XarColorMap[colRef1].name;
	if (XarColorMap.contains(colRef2))
		gCol2 = XarColorMap[colRef2].name;
	if (gCol1 != CommonStrings::None)
	{
		const ScColor& gradC1 = m_Doc->PageColors[gCol1];
		gc->FillGradient.addStop( ScColorEngine::getRGBColor(gradC1, m_Doc), 0.0, 0.5, 1.0, gCol1, 100 );
	}
	else
		gc->FillGradient.addStop( QColor(255, 255, 255, 0), 0.0, 0.5, 0.0, gCol1, 100 );
	quint32 numCols;
	ts >> numCols;
	for (uint a = 0; a < numCols; a++)
	{
		double cpos;
		qint32 colRef;
		ts >> cpos;
		ts >> colRef;
		QString gCol = "Black";
		if (XarColorMap.contains(colRef1))
			gCol = XarColorMap[colRef].name;
		if (gCol != CommonStrings::None)
		{
			const ScColor& gradC = m_Doc->PageColors[gCol];
			gc->FillGradient.addStop( ScColorEngine::getRGBColor(gradC, m_Doc), cpos, 0.5, 1.0, gCol, 100 );
		}
		else
			gc->FillGradient.addStop( QColor(255, 255, 255, 0), cpos, 0.5, 0.0, gCol, 100 );
	}
	if (gCol2 != CommonStrings::None)
	{
		const ScColor& gradC2 = m_Doc->PageColors[gCol2];
		gc->FillGradient.addStop( ScColorEngine::getRGBColor(gradC2, m_Doc), 1.0, 0.5, 1.0, gCol2, 100 );
	}
	else
		gc->FillGradient.addStop( QColor(255, 255, 255, 0), 1.0, 0.5, 0.0, gCol2, 100 );
	gc->FillGradientType = 6;
	double distX = distance(brx - blx, bry - bly);
	double distY = distance(tlx - blx, tly - bly);
	double rotB = xy2Deg(brx - blx, bry - bly);
	double rotS = xy2Deg(tlx - blx, tly - bly);
	gc->GrScale = distY / distX;
	gc->GrSkew = rotS - 90 - rotB;
	gc->GradFillX1 = blx + baseX + m_Doc->currentPage()->xOffset();
	gc->GradFillY1 = (docHeight - bly) + baseY + m_Doc->currentPage()->yOffset();
	gc->GradFillX2 = brx + baseX + m_Doc->currentPage()->xOffset();
	gc->GradFillY2 = (docHeight - bry) + baseY + m_Doc->currentPage()->yOffset();
}

void XarPlug::handleMultiGradient(QDataStream &ts, bool linear)
{
	XarStyle *gc = m_gc.top();
	double blx, bly, brx, bry;
	qint32 colRef1, colRef2;
	readCoords(ts, blx, bly);
	readCoords(ts, brx, bry);
	ts >> colRef1 >> colRef2;
	gc->FillGradient = VGradient(VGradient::linear);
	gc->FillGradient.clearStops();
	QString gCol1 = "Black";
	QString gCol2 = "Black";
	if (XarColorMap.contains(colRef1))
		gCol1 = XarColorMap[colRef1].name;
	if (XarColorMap.contains(colRef2))
		gCol2 = XarColorMap[colRef2].name;
	if (gCol1 != CommonStrings::None)
	{
		const ScColor& gradC1 = m_Doc->PageColors[gCol1];
		gc->FillGradient.addStop( ScColorEngine::getRGBColor(gradC1, m_Doc), 0.0, 0.5, 1.0, gCol1, 100 );
	}
	else
		gc->FillGradient.addStop( QColor(255, 255, 255, 0), 0.0, 0.5, 0.0, gCol1, 100 );
	quint32 numCols;
	ts >> numCols;
	for (uint a = 0; a < numCols; a++)
	{
		double cpos;
		qint32 colRef;
		ts >> cpos;
		ts >> colRef;
		QString gCol = "Black";
		if (XarColorMap.contains(colRef))
			gCol = XarColorMap[colRef].name;
		if (gCol != CommonStrings::None)
		{
			const ScColor& gradC = m_Doc->PageColors[gCol];
			gc->FillGradient.addStop( ScColorEngine::getRGBColor(gradC, m_Doc), cpos, 0.5, 1.0, gCol, 100 );
		}
		else
			gc->FillGradient.addStop( QColor(255, 255, 255, 0), cpos, 0.5, 0.0, gCol, 100 );
	}
	if (gCol2 != CommonStrings::None)
	{
		const ScColor& gradC2 = m_Doc->PageColors[gCol2];
		gc->FillGradient.addStop( ScColorEngine::getRGBColor(gradC2, m_Doc), 1.0, 0.5, 1.0, gCol2, 100 );
	}
	else
		gc->FillGradient.addStop( QColor(255, 255, 255, 0), 1.0, 0.5, 0.0, gCol2, 100 );
	if (linear)
		gc->FillGradientType = 6;
	else
		gc->FillGradientType = 7;
	gc->GradFillX1 = blx + baseX + m_Doc->currentPage()->xOffset();
	gc->GradFillY1 = (docHeight - bly) + baseY + m_Doc->currentPage()->yOffset();
	gc->GradFillX2 = brx + baseX + m_Doc->currentPage()->xOffset();
	gc->GradFillY2 = (docHeight - bry) + baseY + m_Doc->currentPage()->yOffset();
}

void XarPlug::handleSimpleGradientSkewed(QDataStream &ts, quint32 dataLen)
{
	XarStyle *gc = m_gc.top();
	double blx, bly, brx, bry, tlx, tly;
	qint32 colRef1, colRef2;
	readCoords(ts, blx, bly);
	readCoords(ts, brx, bry);
	readCoords(ts, tlx, tly);
	ts >> colRef1 >> colRef2;
	if (dataLen == 48)
	{
		double p, p1;
		ts >> p >> p1;
	}
	gc->FillGradient = VGradient(VGradient::linear);
	gc->FillGradient.clearStops();
	QString gCol1 = "Black";
	QString gCol2 = "Black";
	if (XarColorMap.contains(colRef1))
		gCol1 = XarColorMap[colRef1].name;
	if (XarColorMap.contains(colRef2))
		gCol2 = XarColorMap[colRef2].name;
	if (gCol1 != CommonStrings::None)
	{
		const ScColor& gradC1 = m_Doc->PageColors[gCol1];
		gc->FillGradient.addStop( ScColorEngine::getRGBColor(gradC1, m_Doc), 0.0, 0.5, 1.0, gCol1, 100 );
	}
	else
		gc->FillGradient.addStop( QColor(255, 255, 255, 0), 0.0, 0.5, 0.0, gCol1, 100 );
	if (gCol2 != CommonStrings::None)
	{
		const ScColor& gradC2 = m_Doc->PageColors[gCol2];
		gc->FillGradient.addStop( ScColorEngine::getRGBColor(gradC2, m_Doc), 1.0, 0.5, 1.0, gCol2, 100 );
	}
	else
		gc->FillGradient.addStop( QColor(255, 255, 255, 0), 1.0, 0.5, 0.0, gCol2, 100 );
	gc->FillGradientType = 6;
	double distX = distance(brx - blx, bry - bly);
	double distY = distance(tlx - blx, tly - bly);
	double rotB = xy2Deg(brx - blx, bry - bly);
	double rotS = xy2Deg(tlx - blx, tly - bly);
	gc->GrScale = distY / distX;
	gc->GrSkew = rotS - 90 - rotB;
	gc->GradFillX1 = blx + baseX + m_Doc->currentPage()->xOffset();
	gc->GradFillY1 = (docHeight - bly) + baseY + m_Doc->currentPage()->yOffset();
	gc->GradFillX2 = brx + baseX + m_Doc->currentPage()->xOffset();
	gc->GradFillY2 = (docHeight - bry) + baseY + m_Doc->currentPage()->yOffset();
}

void XarPlug::handleSimpleGradient(QDataStream &ts, quint32 dataLen, bool linear)
{
	XarStyle *gc = m_gc.top();
	double blx, bly, brx, bry;
	qint32 colRef1, colRef2;
	readCoords(ts, blx, bly);
	readCoords(ts, brx, bry);
	ts >> colRef1 >> colRef2;
	if (dataLen == 40)
	{
		double p, p1;
		ts >> p >> p1;
	}
	gc->FillGradient = VGradient(VGradient::linear);
	gc->FillGradient.clearStops();
	QString gCol1 = "Black";
	QString gCol2 = "Black";
	if (XarColorMap.contains(colRef1))
		gCol1 = XarColorMap[colRef1].name;
	if (XarColorMap.contains(colRef2))
		gCol2 = XarColorMap[colRef2].name;
	if (gCol1 != CommonStrings::None)
	{
		const ScColor& gradC1 = m_Doc->PageColors[gCol1];
		gc->FillGradient.addStop( ScColorEngine::getRGBColor(gradC1, m_Doc), 0.0, 0.5, 1.0, gCol1, 100 );
	}
	else
		gc->FillGradient.addStop( QColor(255, 255, 255, 0), 0.0, 0.5, 0.0, gCol1, 100 );
	if (gCol2 != CommonStrings::None)
	{
		const ScColor& gradC2 = m_Doc->PageColors[gCol2];
		gc->FillGradient.addStop( ScColorEngine::getRGBColor(gradC2, m_Doc), 1.0, 0.5, 1.0, gCol2, 100 );
	}
	else
		gc->FillGradient.addStop( QColor(255, 255, 255, 0), 1.0, 0.5, 0.0, gCol2, 100 );
	if (linear)
		gc->FillGradientType = 6;
	else
		gc->FillGradientType = 7;
	gc->GradFillX1 = blx + baseX + m_Doc->currentPage()->xOffset();
	gc->GradFillY1 = (docHeight - bly) + baseY + m_Doc->currentPage()->yOffset();
	gc->GradFillX2 = brx + baseX + m_Doc->currentPage()->xOffset();
	gc->GradFillY2 = (docHeight - bry) + baseY + m_Doc->currentPage()->yOffset();
}

void XarPlug::handleBitmapFill(QDataStream &ts, quint32 dataLen)
{
	XarStyle *gc = m_gc.top();
	qint32 bref;
	double blx, bly, brx, bry, tlx, tly;
	readCoords(ts, blx, bly);
	readCoords(ts, brx, bry);
	readCoords(ts, tlx, tly);
	ts >> bref;
	if (dataLen == 44)
	{
		double p, p1;
		ts >> p >> p1;
	}
	double distX = distance(brx - blx, bry - bly);
	double distY = distance(tlx - blx, tly - bly);
	double rotB = xy2Deg(brx - blx, bry - bly);
	double rotS = xy2Deg(tlx - blx, tly - bly);
	if (patternRef.contains(bref))
	{
		ScPattern pat = m_Doc->docPatterns[patternRef[bref]];
		gc->fillPattern = patternRef[bref];
		gc->patternScaleX = distX / pat.width * 100;
		gc->patternScaleY = distY / pat.height * 100;
		gc->patternOffsetX = 0.0;
		gc->patternOffsetY = 0.0;
		gc->patternRotation = -rotB;
		gc->patternSkewX = rotS - 90 - rotB;
		gc->patternSkewY = 0.0;
	}
}

void XarPlug::handleContoneBitmapFill(QDataStream &ts, quint32 dataLen)
{
	XarStyle *gc = m_gc.top();
	qint32 bref, colRef1, colRef2;
	double blx, bly, brx, bry, tlx, tly;
	readCoords(ts, blx, bly);
	readCoords(ts, brx, bry);
	readCoords(ts, tlx, tly);
	ts >> colRef1 >> colRef2;
	ts >> bref;
	if (dataLen == 52)
	{
		double p, p1;
		ts >> p >> p1;
	}
	QString gCol1 = "Black";
	QString gCol2 = "Black";
	if (XarColorMap.contains(colRef1))
		gCol1 = XarColorMap[colRef1].name;
	if (XarColorMap.contains(colRef2))
		gCol2 = XarColorMap[colRef2].name;
	if (gCol1 == CommonStrings::None)
		gCol1 = "White";
	if (gCol2 == CommonStrings::None)
		gCol2 = "White";
	const ScColor& gradC1 = m_Doc->PageColors[gCol1];
	QColor startC = ScColorEngine::getRGBColor(gradC1, m_Doc);
	const ScColor& gradC2 = m_Doc->PageColors[gCol2];
	QColor endC = ScColorEngine::getRGBColor(gradC2, m_Doc);
	double distX = distance(brx - blx, bry - bly);
	double distY = distance(tlx - blx, tly - bly);
	double rotB = xy2Deg(brx - blx, bry - bly);
	double rotS = xy2Deg(tlx - blx, tly - bly);
	if (patternRef.contains(bref))
	{
		QString imgNam = m_Doc->docPatterns[patternRef[bref]].items.at(0)->externalFile();
		QImage image;
		image.load(imgNam);
		int h = image.height();
		int w = image.width();
		int k;
		int rS, gS, bS, rE, gE, bE, rR1, gR1, bR1, rR2, gR2, bR2;
		startC.getRgb(&rS, &gS, &bS);
		endC.getRgb(&rE, &gE, &bE);
		QRgb *s;
		QRgb r;
		for( int yi=0; yi < h; ++yi )
		{
			s = (QRgb*)(image.scanLine( yi ));
			for( int xi=0; xi < w; ++xi )
			{
				r = *s;
				k = qMin(qRound(0.3 * qRed(r) + 0.59 * qGreen(r) + 0.11 * qBlue(r)), 255);
				rR1 = qMin((rS * k) >> 8, 255);
				gR1 = qMin((gS * k) >> 8, 255);
				gR1 = qMin((bS * k) >> 8, 255);
				rR2 = qMin((rE * k) >> 8, 255);
				gR2 = qMin((gE * k) >> 8, 255);
				bR2 = qMin((bE * k) >> 8, 255);
				*s = qRgba(qMin(rR1+rR2, 255), qMin(gR1+gR2, 255), qMin(bR1+bR2, 255), qAlpha(r));
				s++;
			}
		}
		ScPattern pat = ScPattern();
		pat.setDoc(m_Doc);
		PageItem* newItem = new PageItem_ImageFrame(m_Doc, 0, 0, 1, 1, 0, CommonStrings::None, CommonStrings::None);
		newItem->tempImageFile = new QTemporaryFile(QDir::tempPath() + "/scribus_temp_xar_XXXXXX.png");
		newItem->tempImageFile->open();
		QString fileName = getLongPathName(newItem->tempImageFile->fileName());
		newItem->tempImageFile->close();
		newItem->isInlineImage = true;
		image.save(fileName, "PNG");
		if (newItem->loadImage(fileName, false, 72, false))
		{
			pat.width = image.width();
			pat.height = image.height();
			pat.scaleX = (72.0 / newItem->pixm.imgInfo.xres) * newItem->pixm.imgInfo.lowResScale;
			pat.scaleY = (72.0 / newItem->pixm.imgInfo.xres) * newItem->pixm.imgInfo.lowResScale;
			pat.pattern = newItem->pixm.qImage().copy();
			newItem->setWidth(pat.pattern.width());
			newItem->setHeight(pat.pattern.height());
			newItem->SetRectFrame();
			newItem->gXpos = 0.0;
			newItem->gYpos = 0.0;
			newItem->gWidth = pat.pattern.width();
			newItem->gHeight = pat.pattern.height();
			pat.items.append(newItem);
			newItem->ItemNr = pat.items.count();
		}
		QString patternName = patternRef[bref]+"_"+newItem->itemName();
		patternName = patternName.trimmed().simplified().replace(" ", "_");
		m_Doc->addPattern(patternName, pat);
		gc->fillPattern = patternName;
		gc->patternScaleX = distX / pat.width * 100;
		gc->patternScaleY = distY / pat.height * 100;
		gc->patternOffsetX = 0.0;
		gc->patternOffsetY = 0.0;
		gc->patternRotation = -rotB;
		gc->patternSkewX = rotS - 90 - rotB;
		gc->patternSkewY = 0.0;
	}
}

void XarPlug::handleBitmap(QDataStream &ts)
{
	XarStyle *gc = m_gc.top();
	qint32 bref;
	double blx, bly, brx, bry, tlx, tly, trix, triy;
	readCoords(ts, blx, bly);
	readCoords(ts, brx, bry);
	readCoords(ts, tlx, tly);
	readCoords(ts, trix, triy);
	ts >> bref;
	Coords.resize(0);
	Coords.svgInit();
	Coords.svgMoveTo(blx, docHeight - bly);
	Coords.svgLineTo(brx, docHeight - bry);
	Coords.svgLineTo(tlx, docHeight - tly);
	Coords.svgLineTo(trix, docHeight - triy);
	Coords.svgClosePath();
	int z = m_Doc->itemAdd(PageItem::ImageFrame, PageItem::Unspecified, baseX, baseY, 10, 10, gc->LWidth, gc->FillCol, gc->StrokeCol, true);
	finishItem(z);
	PageItem *ite = m_Doc->Items->at(z);
	if (patternRef.contains(bref))
	{
		QString imgNam = m_Doc->docPatterns[patternRef[bref]].items.at(0)->externalFile();
		QImage image;
		image.load(imgNam);
		ite->tempImageFile = new QTemporaryFile(QDir::tempPath() + "/scribus_temp_xar_XXXXXX.png");
		ite->tempImageFile->open();
		QString fileName = getLongPathName(ite->tempImageFile->fileName());
		ite->tempImageFile->close();
		ite->isInlineImage = true;
		image.save(fileName, "PNG");
		m_Doc->LoadPict(fileName, z);
		ite->setImageScalingMode(false, false);
	}
}

void XarPlug::defineBitmap(QDataStream &ts, quint32 dataLen, quint32 tag)
{
	quint32 bytesRead = 0;
	quint16 charC = 0;
	ts >> charC;
	bytesRead += 2;
	QString XarName = "";
	while (charC != 0)
	{
		XarName += QChar(charC);
		ts >> charC;
		bytesRead += 2;
	}
	if (tag == 71)
	{
		quint8 palcount, r, g, b;
		ts >> palcount;
		bytesRead++;
		for (int a = 0; a < palcount + 1; a++)
		{
			ts >> r;
			ts >> g;
			ts >> b;
			bytesRead += 3;
		}
	}
	imageData.resize(dataLen - bytesRead);
	ts.readRawData(imageData.data(), dataLen - bytesRead);
	QImage image;
	if (image.loadFromData(imageData))
	{
		bool rawAlpha = image.hasAlphaChannel();
		image = image.convertToFormat(QImage::Format_ARGB32);
		if ((tag == 68) && (rawAlpha))
		{
			int h = image.height();
			int w = image.width();
			QRgb *s;
			QRgb r;
			for( int yi=0; yi < h; ++yi )
			{
				s = (QRgb*)(image.scanLine( yi ));
				for( int xi=0; xi < w; ++xi )
				{
					r = *s;
					*s = qRgba(qRed(r), qGreen(r), qBlue(r), 255 - qAlpha(r));
					s++;
				}
			}
		}
		ScPattern pat = ScPattern();
		pat.setDoc(m_Doc);
		PageItem* newItem = new PageItem_ImageFrame(m_Doc, 0, 0, 1, 1, 0, CommonStrings::None, CommonStrings::None);
		newItem->tempImageFile = new QTemporaryFile(QDir::tempPath() + "/scribus_temp_xar_XXXXXX.png");
		newItem->tempImageFile->open();
		QString fileName = getLongPathName(newItem->tempImageFile->fileName());
		newItem->tempImageFile->close();
		newItem->isInlineImage = true;
		image.save(fileName, "PNG");
		if (newItem->loadImage(fileName, false, 72, false))
		{
			pat.width = image.width();
			pat.height = image.height();
			pat.scaleX = (72.0 / newItem->pixm.imgInfo.xres) * newItem->pixm.imgInfo.lowResScale;
			pat.scaleY = (72.0 / newItem->pixm.imgInfo.xres) * newItem->pixm.imgInfo.lowResScale;
			pat.pattern = newItem->pixm.qImage().copy();
			newItem->setWidth(pat.pattern.width());
			newItem->setHeight(pat.pattern.height());
			newItem->SetRectFrame();
			newItem->gXpos = 0.0;
			newItem->gYpos = 0.0;
			newItem->gWidth = pat.pattern.width();
			newItem->gHeight = pat.pattern.height();
			pat.items.append(newItem);
			newItem->ItemNr = pat.items.count();
		}
		QString patternName = "Pattern_"+newItem->itemName();
		patternName = patternName.trimmed().simplified().replace(" ", "_");
		m_Doc->addPattern(patternName, pat);
		patternRef.insert(recordCounter, patternName);
	}
}

void XarPlug::handleLineColor(QDataStream &ts)
{
	XarStyle *gc = m_gc.top();
	qint32 val;
	ts >> val;
	if (XarColorMap.contains(val))
		gc->StrokeCol = XarColorMap[val].name;
}

void XarPlug::handleLineWidth(QDataStream &ts)
{
	XarStyle *gc = m_gc.top();
	quint32 val;
	ts >> val;
	gc->LWidth = val / 1000.0;
}

void XarPlug::handleFlatLineTransparency(QDataStream &ts)
{
	quint8 transVal, transType;
	ts >> transVal >> transType;
	XarStyle *gc = m_gc.top();
	if (transType > 0)
		gc->StrokeOpacity = transVal / 255.0;
}

void XarPlug::handleFlatFill(QDataStream &ts)
{
	XarStyle *gc = m_gc.top();
	qint32 val;
	ts >> val;
	if (XarColorMap.contains(val))
		gc->FillCol = XarColorMap[val].name;
}

void XarPlug::createRectangleItem(QDataStream &ts, bool ellipse)
{
	XarStyle *gc = m_gc.top();
	double centerX, centerY, majorAxis, minorAxis;
	readCoords(ts, centerX, centerY);
	readCoords(ts, majorAxis, minorAxis);
	int z = m_Doc->itemAdd(PageItem::Polygon, PageItem::Unspecified, baseX, baseY, 10, 10, gc->LWidth, gc->FillCol, gc->StrokeCol, true);
	Coords.resize(0);
	Coords.svgInit();
	QPainterPath path;
	if (ellipse)
		path.addEllipse(QPointF(majorAxis, minorAxis), majorAxis, minorAxis);
	else
		path = RegularPolygon(majorAxis, minorAxis, 4, false, 0, 45, 0);
	Coords.fromQPainterPath(path);
	Coords.translate(-majorAxis / 2.0, -minorAxis / 2.0);
	Coords.translate(centerX, -centerY);
	Coords.translate(0, docHeight);
	finishItem(z);
}

void XarPlug::createSimilarItem(QDataStream &ts)
{
	XarStyle *gc = m_gc.top();
	qint32 val;
	ts >> val;
	quint32 scX, skX, skY, scY;
	double transX, transY;
	ts >> scX >> skX >> skY >> scY;
	readCoords(ts, transX, transY);
	double scaleX = decodeFixed16(scX);
	double scaleY = decodeFixed16(scY);
	double skewX = decodeFixed16(skX);
	double skewY = decodeFixed16(skY);
	if ((pathMap.contains(val)) && (scX != 0) && (scY != 0))
	{
		PageItem* newItem;
		int z = -1;
		PageItem* ite = pathMap[val];
		if (ite->realItemType() == PageItem::ImageFrame)
			z = m_Doc->itemAdd(PageItem::ImageFrame, PageItem::Unspecified, baseX, baseY, 10, 10, gc->LWidth, gc->FillCol, gc->StrokeCol, true);
		else if (ite->realItemType() == PageItem::Polygon)
			z = m_Doc->itemAdd(PageItem::Polygon, PageItem::Unspecified, baseX, baseY, 10, 10, gc->LWidth, gc->FillCol, gc->StrokeCol, true);
		else
			z = m_Doc->itemAdd(PageItem::PolyLine, PageItem::Unspecified, baseX, baseY, 10, 10, gc->LWidth, gc->FillCol, gc->StrokeCol, true);
		if (z > -1)
		{
			newItem = m_Doc->Items->at(z);
			Coords = ite->PoLine.copy();
			QTransform matrix(-scaleX, skewX, -skewY, scaleY, 0, 0);
			Coords.map(matrix);
			Coords.translate(transX, transY);
			Coords.translate(ite->xPos(), ite->yPos());
//			Coords.translate(0, docHeight);
			newItem->PoLine = Coords.copy();
//		QTransform matrix(scaleX, skewX, skewY, scaleY, 0, 0);
//		ite->PoLine.map(matrix);
//		ite->PoLine.translate(transX * scaleX, -transY * scaleY);
			newItem->ClipEdited = true;
			newItem->FrameType = 3;
			FPoint wh = getMaxClipF(&newItem->PoLine);
			newItem->setWidthHeight(wh.x(),wh.y());
			newItem->setTextFlowMode(PageItem::TextFlowDisabled);
			m_Doc->AdjustItemSize(newItem);
			Elements.append(newItem);
			XarStyle *gc = m_gc.top();
			gc->Elements.append(newItem);
		}
//		qDebug() << "Similar Item" << ite->itemName() << " -> " << newItem->itemName() << scX << skewX << skewY << scaleY << transX << transY;
	}
}

void XarPlug::createPolygonItem(int type)
{
	int z;
	XarStyle *gc = m_gc.top();
	if (type == 0)
		z = m_Doc->itemAdd(PageItem::Polygon, PageItem::Unspecified, baseX, baseY, 10, 10, gc->LWidth, CommonStrings::None, gc->StrokeCol, true);
	else if (type == 1)
		z = m_Doc->itemAdd(PageItem::Polygon, PageItem::Unspecified, baseX, baseY, 10, 10, gc->LWidth, gc->FillCol, CommonStrings::None, true);
	else if (type == 2)
		z = m_Doc->itemAdd(PageItem::Polygon, PageItem::Unspecified, baseX, baseY, 10, 10, gc->LWidth, gc->FillCol, gc->StrokeCol, true);
	finishItem(z);
}

void XarPlug::createPolylineItem(int type)
{
	int z;
	XarStyle *gc = m_gc.top();
	if (type == 0)
		z = m_Doc->itemAdd(PageItem::PolyLine, PageItem::Unspecified, baseX, baseY, 10, 10, gc->LWidth, CommonStrings::None, gc->StrokeCol, true);
	else if (type == 1)
		z = m_Doc->itemAdd(PageItem::PolyLine, PageItem::Unspecified, baseX, baseY, 10, 10, gc->LWidth, gc->FillCol, CommonStrings::None, true);
	else if (type == 2)
		z = m_Doc->itemAdd(PageItem::PolyLine, PageItem::Unspecified, baseX, baseY, 10, 10, gc->LWidth, gc->FillCol, gc->StrokeCol, true);
	finishItem(z);
//	PageItem *ite = m_Doc->Items->at(z);
//	qDebug() << "Item" << ite->itemName() << type;
}

void XarPlug::createGroupItem()
{
	XarGroup gg;
	gg.index = Elements.count();
	gg.gcStackDepth = m_gc.count();
	gg.clipping = false;
	int z = m_Doc->itemAdd(PageItem::Polygon, PageItem::Rectangle, baseX, baseY, 10, 10, 0, CommonStrings::None, CommonStrings::None, true);
	PageItem *neu = m_Doc->Items->at(z);
	gg.groupItem = neu;
	Elements.append(neu);
	XarStyle *gc = m_gc.top();
	gc->Elements.append(neu);
	groupStack.push(gg);
}

void XarPlug::createClipItem()
{
	XarGroup gg;
	gg.index = Elements.count();
	gg.gcStackDepth = m_gc.count();
	gg.clipping = true;
	int z = m_Doc->itemAdd(PageItem::Polygon, PageItem::Rectangle, baseX, baseY, 10, 10, 0, CommonStrings::None, CommonStrings::None, true);
	PageItem *neu = m_Doc->Items->at(z);
	gg.groupItem = neu;
	Elements.append(neu);
	XarStyle *gc = m_gc.top();
	gc->Elements.append(neu);
	groupStack.push(gg);
}

void XarPlug::finishClip()
{
	if (groupStack.count() > 0)
		groupStack.top().clipping = false;
}

void XarPlug::finishItem(int z)
{
	XarStyle *gc = m_gc.top();
	PageItem *ite = m_Doc->Items->at(z);
	ite->PoLine = Coords.copy();
	ite->PoLine.translate(m_Doc->currentPage()->xOffset(), m_Doc->currentPage()->yOffset());
	ite->ClipEdited = true;
	ite->FrameType = 3;
	FPoint wh = getMaxClipF(&ite->PoLine);
	ite->setWidthHeight(wh.x(),wh.y());
	ite->setTextFlowMode(PageItem::TextFlowDisabled);
	m_Doc->AdjustItemSize(ite);
	if (groupStack.count() > 0)
	{
		XarGroup gg = groupStack.top();
		if (gg.clipping)
		{
			if (clipCoords.size() == 0)
			{
				gc->clipPath = ite->PoLine.copy();
				gc->clipPath.translate(ite->xPos(), ite->yPos());
			}
			else
			{
				gc->clipPath.setMarker();
				gc->clipPath.putPoints(gc->clipPath.size(), ite->PoLine.size(), ite->PoLine);
				m_Doc->Items->removeLast();
				delete ite;
				return;
			}
		}
	}
	Elements.append(ite);
	gc->Elements.append(ite);
	pathMap.insert(recordCounter, ite);
//	qDebug() << "Item" << ite->itemName();
}

bool XarPlug::handlePathRel(QDataStream &ts, quint32 len)
{
	quint32 count = len / 9;
	qint32 x, y;
	quint8  verb, val;
	double co1, co2, cx1, cy1, cx2, cy2, cx3, cy3;
	FPoint currentPoint, startPoint;
	int bezCount = 0;
	bool closed = false;
	bool wasFirst = true;
	Coords.resize(0);
	Coords.svgInit();
	for (uint a = 0; a < count; a++)
	{
		ts >> verb;
		ts >> val;
		x = val;
		ts >> val;
		y = val;
		ts >> val;
		x = (x << 8) | val;
		ts >> val;
		y = (y << 8) | val;
		ts >> val;
		x = (x << 8) | val;
		ts >> val;
		y = (y << 8) | val;
		ts >> val;
		x = (x << 8) | val;
		ts >> val;
		y = (y << 8) | val;
		co1 = x / 1000.0;
		co2 = y / 1000.0;
		switch (verb)
		{
			case 6:
				if (wasFirst)
				{
					Coords.svgMoveTo(co1, docHeight - co2);
					currentPoint = FPoint(co1, co2);
					wasFirst = false;
				}
				else
				{
					currentPoint = FPoint(currentPoint.x() - co1, currentPoint.y() - co2);
					Coords.svgMoveTo(currentPoint.x(), docHeight - currentPoint.y());
				}
				startPoint = currentPoint;
				break;
			case 2:
			case 3:
				currentPoint = FPoint(currentPoint.x() - co1, currentPoint.y() - co2);
				Coords.svgLineTo(currentPoint.x(), docHeight - currentPoint.y());
				if (verb == 3)
				{
					closed = true;
					Coords.svgClosePath();
					currentPoint = startPoint;
				}
				break;
			case 4:
			case 5:
				if (bezCount == 0)
				{
					cx1 = currentPoint.x() - co1;
					cy1 = currentPoint.y() - co2;
					currentPoint = FPoint(cx1, cy1);
					bezCount++;
				}
				else if (bezCount == 1)
				{
					cx2 = currentPoint.x() - co1;
					cy2 = currentPoint.y() - co2;
					currentPoint = FPoint(cx2, cy2);
					bezCount++;
				}
				else if (bezCount == 2)
				{
					cx3 = currentPoint.x() - co1;
					cy3 = currentPoint.y() - co2;
					currentPoint = FPoint(cx3, cy3);
					Coords.svgCurveToCubic(cx1, docHeight - cy1, cx2, docHeight - cy2, cx3, docHeight - cy3);
					if (verb == 5)
					{
						closed = true;
						Coords.svgClosePath();
						currentPoint = startPoint;
					}
					bezCount = 0;
				}
				break;
		}
	}
	return closed;
}

void XarPlug::handleLayerInfo(QDataStream &ts)
{
	quint16 charC = 0;
	quint8 layerFlags;
	ts >> layerFlags;
	ts >> charC;
	QString XarName = "";
	while (charC != 0)
	{
		XarName += QChar(charC);
		ts >> charC;
	}
	if (importerFlags & LoadSavePlugin::lfCreateDoc)
	{
		if (!firstLayer)
			currentLayer = m_Doc->addLayer(XarName, true);
		else
			m_Doc->changeLayerName(currentLayer, XarName);
		m_Doc->setLayerVisible(currentLayer, layerFlags & 1);
		m_Doc->setLayerLocked(currentLayer, layerFlags & 2);
		m_Doc->setLayerPrintable(currentLayer, layerFlags & 4);
		firstLayer = false;
		if (layerFlags & 8)
			activeLayer = XarName;
	}
}

void XarPlug::handleSpreadInfo(QDataStream &ts)
{
	quint32 pgWidth, pgHeight, margin, bleed;
	quint8 spreadFlags;
	ts >> pgWidth >> pgHeight >> margin >> bleed;
	ts >> spreadFlags;
	double w = pgWidth / 1000.0;
	double h = pgHeight / 1000.0;
//	double m = margin / 1000.0;
//	double b = bleed / 1000.0;
	if (importerFlags & LoadSavePlugin::lfCreateDoc)
	{
		m_Doc->setPage(w, h, 0, 0, 0, 0, 0, 0, false, false);
		if (w > h)
			m_Doc->PageOri = 1;
		else
			m_Doc->PageOri = 0;
		m_Doc->m_pageSize = "Custom";
		m_Doc->changePageMargins(0, 0, 0, 0, h, w, h, w, m_Doc->PageOri, m_Doc->m_pageSize, m_Doc->currentPage()->pageNr(), 0);
	}
	docHeight = h;
	docWidth = w;
}

void XarPlug::handleComplexColor(QDataStream &ts)
{
	QString tmpName = CommonStrings::None;
	ScColor tmp;
	ColorList::Iterator it;
	quint8 Rc, Gc, Bc, colorModel, colorType;
	quint16 charC = 0;
	quint32 EntryIndex, component1, component2, component3, component4;
	qint32 colorRef;
	int hR, hG, hB;
	ts >> Rc >> Gc >> Bc >> colorModel >> colorType;
	ts >> EntryIndex;
	ts >> colorRef;
	ts >> component1 >> component2 >> component3 >> component4;
	ts >> charC;
	QString XarName = "";
	while (charC != 0)
	{
		XarName += QChar(charC);
		ts >> charC;
	}
/*	QString colM = "";
	switch (colorModel)
	{
		case 2:
			colM = "RGB";
			break;
		case 3:
			colM = "CMYK";
			break;
		case 4:
			colM = "HSV";
			break;
		case 5:
			colM = "Gray";
			break;
	}
	QString colT = "";
	switch (colorType)
	{
		case 0:
			colT = "Normal";
			break;
		case 1:
			colT = "Spot";
			break;
		case 2:
			colT = "Tint";
			break;
		case 3:
			colT = "Linked";
			break;
		case 4:
			colT = "Shade";
			break;
	} */
	double c1 = decodeColorComponent(component1);
	double c2 = decodeColorComponent(component2);
	double c3 = decodeColorComponent(component3);
	double c4 = decodeColorComponent(component4);
//	qDebug() << "Record" << recordCounter << "Complex Color" << XarName << colM << colT << colorRef;
//	qDebug() << "\t\tComponents" << c1 << c2 << c3 << c4;
	bool found = false;
	if ((!XarName.isEmpty()) && ((XarName == "White") || (XarName == "Black") || (m_Doc->PageColors.contains(XarName))))
		tmpName = XarName;
	else
	{
		QColor c = QColor(Rc, Gc, Bc);
		if ((colorType == 0) || (colorType == 1))
		{
			if (colorModel == 3)
			{
				int Cc = qRound(c1 * 255);
				int Mc = qRound(c2 * 255);
				int Yc = qRound(c3 * 255);
				int Kc = qRound(c4 * 255);
				int hC, hM, hY, hK;
				tmp.setColor(Cc, Mc, Yc, Kc);
				for (it = m_Doc->PageColors.begin(); it != m_Doc->PageColors.end(); ++it)
				{
					if (it.value().getColorModel() == colorModelCMYK)
					{
						it.value().getCMYK(&hC, &hM, &hY, &hK);
						if ((Cc == hC) && (Mc == hM) && (Yc == hY) && (Kc == hK))
						{
							tmpName = it.key();
							found = true;
							break;
						}
					}
				}
				if (!found)
				{
					if (colorType == 1)
						tmp.setSpotColor(true);
					else
						tmp.setSpotColor(false);
					tmp.setRegistrationColor(false);
					if (XarName.isEmpty())
						tmpName = "FromXara"+c.name();
					else
						tmpName = XarName;
					m_Doc->PageColors.insert(tmpName, tmp);
					importedColors.append(tmpName);
				}
			}
			else
			{
				for (it = m_Doc->PageColors.begin(); it != m_Doc->PageColors.end(); ++it)
				{
					if (it.value().getColorModel() == colorModelRGB)
					{
						it.value().getRGB(&hR, &hG, &hB);
						if ((Rc == hR) && (Gc == hG) && (Bc == hB))
						{
							tmpName = it.key();
							found = true;
							break;
						}
					}
				}
				if (!found)
				{
					tmp.setColorRGB(Rc, Gc, Bc);
					tmp.setSpotColor(false);
					tmp.setRegistrationColor(false);
					if (XarName.isEmpty())
						tmpName = "FromXara"+c.name();
					else
						tmpName = XarName;
					m_Doc->PageColors.insert(tmpName, tmp);
					importedColors.append(tmpName);
				}
			}
		}
		else
		{
			for (it = m_Doc->PageColors.begin(); it != m_Doc->PageColors.end(); ++it)
			{
				if (it.value().getColorModel() == colorModelRGB)
				{
					it.value().getRGB(&hR, &hG, &hB);
					if ((Rc == hR) && (Gc == hG) && (Bc == hB))
					{
						tmpName = it.key();
						found = true;
						break;
					}
				}
			}
			if (!found)
			{
				tmp.setColorRGB(Rc, Gc, Bc);
				tmp.setSpotColor(false);
				tmp.setRegistrationColor(false);
				if (XarName.isEmpty())
					tmpName = "FromXara"+c.name();
				else
					tmpName = XarName;
				m_Doc->PageColors.insert(tmpName, tmp);
				importedColors.append(tmpName);
			}
		}
	}
	XarColor color;
	color.colorType = colorType;
	color.colorModel = colorModel;
	color.colorRef = colorRef;
	color.component1 = c1;
	color.component2 = c2;
	color.component3 = c3;
	color.component1 = c4;
	color.name = tmpName;
	XarColorMap.insert(recordCounter, color);
}

void XarPlug::handleColorRGB(QDataStream &ts)
{
	QString tmpName = CommonStrings::None;
	ScColor tmp;
	ColorList::Iterator it;
	quint8 Rc, Gc, Bc;
	int hR, hG, hB;
	ts >> Rc >> Gc >> Bc;
	bool found = false;
	QColor c = QColor(Rc, Gc, Bc);
	for (it = m_Doc->PageColors.begin(); it != m_Doc->PageColors.end(); ++it)
	{
		if (it.value().getColorModel() == colorModelRGB)
		{
			it.value().getRGB(&hR, &hG, &hB);
			if ((Rc == hR) && (Gc == hG) && (Bc == hB))
			{
				tmpName = it.key();
				found = true;
				break;
			}
		}
	}
	if (!found)
	{
		tmp.setColorRGB(Rc, Gc, Bc);
		tmp.setSpotColor(false);
		tmp.setRegistrationColor(false);
		tmpName = "FromXara"+c.name();
		m_Doc->PageColors.insert(tmpName, tmp);
		importedColors.append(tmpName);
	}
	XarColor color;
	color.colorType = 0;
	color.colorModel = 2;
	color.colorRef = 0;
	color.component1 = 0;
	color.component2 = 0;
	color.component3 = 0;
	color.component1 = 0;
	color.name = tmpName;
	XarColorMap.insert(recordCounter, color);
}

double XarPlug::decodeColorComponent(quint32 data)
{
	double ret = 0.0;
	char man = (data & 0xFF000000) >> 24;
	if (man >= 0)
	{
		ret = (data & 0x00FFFFFF) / 16777215.0;
		ret = (ret + man);
	}
	else
	{
		ret = (~data & 0x00FFFFFF) / 16777215.0;
		ret = (ret + ~man) * -1;
	}
	return ret;
}

double XarPlug::decodeFixed16(quint32 data)
{
	double ret = 0.0;
	char man = (data & 0xFFFF0000) >> 16;
	if (man >= 0)
	{
		ret = (data & 0x0000FFFF) / 65536.0;
		ret = (ret + man);
	}
	else
	{
		ret = (~data & 0x0000FFFF) / 65536.0;
		ret = (ret + ~man) * -1;
	}
	return ret;
}

void XarPlug::readCoords(QDataStream &ts, double &x, double &y)
{
	qint32 xc, yc;
	ts >> xc >> yc;
	x = xc / 1000.0;
	y = yc / 1000.0;
}

void XarPlug::addToAtomic(quint32 dataLen, QDataStream &ts)
{
	quint32 l = dataLen / 4;
	quint32 val;
	for (quint32 a = 0; a < l; a++)
	{
		ts >> val;
		atomicTags.append(val);
	}
}

void XarPlug::addGraphicContext()
{
/*	XarStyle *gc2 = m_gc.top();
	XarStyle *gc = new XarStyle;
	if ( m_gc.top() )
		*gc = *( m_gc.top() );
	m_gc.push( gc );
	if (gc2->Elements.count() > 0)
		gc2->Elements.removeLast(); */
	XarStyle *gc2 = m_gc.top();
	XarStyle *gc = new XarStyle;
	if ( m_gc.top() )
		*gc = *( m_gc.top() );
	m_gc.push( gc );
	if (gc2->Elements.count() > 0)
	{
		PageItem* ite = gc2->Elements.last();
		gc->Elements.clear();
		gc->Elements.append(ite);
		gc2->Elements.removeLast();
	}
}

void XarPlug::popGraphicContext()
{
	XarStyle *gc = m_gc.pop();
	if (groupStack.count() > 0)
	{
		XarGroup gg = groupStack.top();
		if (gg.gcStackDepth == m_gc.count())
		{
			groupStack.pop();
			if (gg.index + 1 == Elements.count())
			{
				Elements.removeLast();
				m_Doc->Items->removeLast();
				gc->Elements.removeAll(gg.groupItem);
				delete gg.groupItem;
			}
			else
			{
				double minx = 99999.9;
				double miny = 99999.9;
				double maxx = -99999.9;
				double maxy = -99999.9;
				PageItem* groupItem = Elements.at(gg.index);
				for (int a = gg.index+1; a < Elements.count(); ++a)
				{
					PageItem* currItem = Elements.at(a);
					currItem->Groups.push(m_Doc->GroupCounter);
					double x1, x2, y1, y2;
					currItem->getVisualBoundingRect(&x1, &y1, &x2, &y2);
					minx = qMin(minx, x1);
					miny = qMin(miny, y1);
					maxx = qMax(maxx, x2);
					maxy = qMax(maxy, y2);
				}
				groupItem->setXYPos(minx, miny, true);
				groupItem->setWidthHeight(maxx - minx, maxy - miny, true);
				groupItem->SetRectFrame();
				groupItem->ClipEdited = true;
				groupItem->FrameType = 3;
				groupItem->setTextFlowMode(PageItem::TextFlowDisabled);
				groupItem->setItemName( tr("Group%1").arg(m_Doc->GroupCounter));
				if (gc->clipPath.size() > 0)
				{
					groupItem->PoLine = gc->clipPath.copy();
					groupItem->PoLine.translate(-minx + baseX, -miny + baseY);
					FPoint wh = getMaxClipF(&groupItem->PoLine);
					groupItem->setWidthHeight(wh.x(),wh.y());
					m_Doc->AdjustItemSize(groupItem);
				}
				groupItem->AutoName = false;
				groupItem->isGroupControl = true;
				groupItem->setFillTransparency(0);
				groupItem->setLineTransparency(0);
				groupItem->groupsLastItem = Elements.last();
				groupItem->Groups.push(m_Doc->GroupCounter);
				m_Doc->GroupCounter++;
			}
		}
	}
	if (gc->Elements.count() > 0)
	{
		for (int a = 0; a < gc->Elements.count(); a++)
		{
			PageItem *item = gc->Elements.at(a);
			if (item->isGroupControl)
				continue;
			if (!item->asPolyLine())
			{
				item->setFillColor(gc->FillCol);
				item->setFillTransparency(gc->FillOpacity);
				item->setFillBlendmode(gc->FillBlend);
			}
			item->setLineTransparency(gc->StrokeOpacity);
			item->setLineWidth(gc->LWidth);
			item->setLineColor(gc->StrokeCol);
			item->setLineJoin(gc->PLineJoin);
			item->setLineEnd(gc->PLineEnd);
			item->setFillEvenOdd(gc->fillRule);
			if (!gc->fillPattern.isEmpty())
			{
				item->setPattern(gc->fillPattern);
				item->setPatternTransform(gc->patternScaleX, gc->patternScaleY, gc->patternOffsetX, gc->patternOffsetY, gc->patternRotation, gc->patternSkewX, gc->patternSkewY);
				item->GrType = 8;
			}
			if ((gc->FillGradientType == 6) || (gc->FillGradientType == 7))
			{
				item->GrType = gc->FillGradientType;
				item->fill_gradient = gc->FillGradient;
				item->setGradientVector(gc->GradFillX1 - item->xPos(), gc->GradFillY1 - item->yPos(), gc->GradFillX2 - item->xPos(), gc->GradFillY2 - item->yPos(), gc->GradFillX1 - item->xPos(), gc->GradFillY1 - item->yPos(), gc->GrScale, gc->GrSkew);
			}
			if (gc->GradMask > 0)
			{
				item->GrMask = gc->GradMask;
				if ((item->GrMask == 1) || (item->GrMask == 2))
				{
					item->mask_gradient = gc->MaskGradient;
					item->setMaskVector(gc->GradMaskX1 - item->xPos(), gc->GradMaskY1 - item->yPos(), gc->GradMaskX2 - item->xPos(), gc->GradMaskY2 - item->yPos(), gc->GradMaskX1 - item->xPos(), gc->GradMaskY1 - item->yPos(), gc->GradMaskScale, gc->GradMaskSkew);
				}
				else
				{
					item->setMaskTransform(gc->patternMaskScaleX, gc->patternMaskScaleY, gc->patternMaskOffsetX, gc->patternMaskOffsetY, gc->patternMaskRotation, gc->patternMaskSkewX, gc->patternMaskSkewY);
					item->setPatternMask(gc->maskPattern);
				}
			}
		}
	}
	delete gc;
}
