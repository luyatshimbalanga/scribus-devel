/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#include "effectsdialog.h"
//#include "effectsdialog.moc"
#include <qvariant.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <q3whatsthis.h>
#include <qimage.h>
#include <q3textstream.h>
#include <q3widgetstack.h>
#include <qwidget.h>
#include <qslider.h>
#include <qpainter.h>
#include <qtoolbutton.h>
#include <q3popupmenu.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <Q3GridLayout>
#include <QPixmap>
#include <Q3VBoxLayout>
#include "colorcombo.h"
#include "cmsettings.h"
#include "sccombobox.h"
#include "scribusdoc.h"
#include "shadebutton.h"
#include "mspinbox.h"
#include "page.h"
#include "colorutil.h"
#include "curvewidget.h"

extern QPixmap loadIcon(QString nam);

EffectListItem::EffectListItem(Q3ListBox* parent, QString f) : Q3ListBoxText(parent, f)
{
}

const int EffectListItem::width(const Q3ListBox *listbox)
{
	return listbox->fontMetrics().width(text()) + 2;
}

const int EffectListItem::height(const Q3ListBox *listbox)
{
	QFontMetrics fontMetrics(listbox->fontMetrics());
	return fontMetrics.lineSpacing() + 2;
}

void EffectListItem::paint(QPainter *painter)
{
	QFontMetrics fontMetrics(painter->fontMetrics());
	if (!isSelectable())
		painter->setPen(QColor(128, 128, 128));
	painter->drawText(3, fontMetrics.lineSpacing()-fontMetrics.descent(), text());
}

EffectsDialog::EffectsDialog( QWidget* parent, PageItem* item, ScribusDoc* docc ) : QDialog( parent, "EffectsDialog", true, 0 )
{
	EffectsDialogLayout = new Q3HBoxLayout( this, 10, 5, "EffectsDialogLayout");
	setCaption( tr( "Image Effects" ) );
	setIcon(loadIcon("AppIcon.png"));
	currItem = item;
	effectsList = currItem->effectsInUse;
	doc = docc;
	currentOptions = 0;
	bool mode = false;
	CMSettings cms(docc, "", 0);
/*	if (currItem->pixm.imgInfo.RequestProps.count() != 0)
	{
		image.imgInfo.RequestProps = currItem->pixm.imgInfo.RequestProps;
		image.imgInfo.isRequest = true;
	} */
	image.LoadPicture(currItem->Pfile, cms, false, false, ScImage::RGBData, 72, &mode);
	int ix = image.width();
	int iy = image.height();
	imageScale = 1.0;
	if ((ix > 220) || (iy > 220))
	{
		double sx = ix / 220.0;
		double sy = iy / 220.0;
		if (sy < sx)
			image.createLowRes(sx);
		else
			image.createLowRes(sy);
		imageScale = QMIN(sx, sy);
	}
	layout16 = new Q3VBoxLayout( 0, 0, 5, "layout16");
	pixmapLabel1 = new QLabel( this, "pixmapLabel1" );
	pixmapLabel1->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)0, (QSizePolicy::SizeType)0, 0, 0, pixmapLabel1->sizePolicy().hasHeightForWidth() ) );
	pixmapLabel1->setMinimumSize( QSize( 220, 220 ) );
	pixmapLabel1->setMaximumSize( QSize( 220, 220 ) );
	pixmapLabel1->setFrameShape( QLabel::StyledPanel );
	pixmapLabel1->setFrameShadow( QLabel::Sunken );
	pixmapLabel1->setScaledContents( false );
	layout16->addWidget( pixmapLabel1 );
	textLabel5 = new QLabel( this, "textLabel5" );
	textLabel5->setText( tr( "Options:" ) );
	layout16->addWidget( textLabel5 );
	optionStack = new Q3WidgetStack( this, "optionStack" );
	optionStack->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)7, (QSizePolicy::SizeType)7, 0, 0, optionStack->sizePolicy().hasHeightForWidth() ) );
	optionStack->setMinimumSize( QSize( 220, 180 ) );
	optionStack->setFrameShape( Q3WidgetStack::GroupBoxPanel );
	WStackPage = new QWidget( optionStack, "WStackPage" );
	optionStack->addWidget( WStackPage, 0 );

	WStackPage_2 = new QWidget( optionStack, "WStackPage_2" );
	WStackPageLayout = new Q3VBoxLayout( WStackPage_2, 5, 5, "WStackPageLayout");
	WStackPageLayout->setAlignment( Qt::AlignTop );
	layout17 = new Q3HBoxLayout( 0, 0, 5, "layout7");
	textLabel3 = new QLabel( tr( "Color:" ), WStackPage_2, "textLabel3" );
	layout17->addWidget( textLabel3 );

	colData = new ColorCombo(false, WStackPage_2, "colData");
	colData->insertItems(doc->PageColors, ColorCombo::smallPixmaps);
	layout17->addWidget( colData );
	WStackPageLayout->addLayout( layout17 );

	layout19 = new Q3HBoxLayout( 0, 0, 5, "layout7");
	textLabel4 = new QLabel( tr( "Shade:" ), WStackPage_2, "textLabel4" );
	layout19->addWidget( textLabel4 );
	shade = new ShadeButton(WStackPage_2);
	shade->setValue(100);
	layout19->addWidget( shade );
	WStackPageLayout->addLayout( layout19 );
	optionStack->addWidget( WStackPage_2, 1 );

	WStackPage_3 = new QWidget( optionStack, "WStackPage_3" );
	WStackPage3Layout = new Q3VBoxLayout( WStackPage_3, 5, 5, "WStackPageLayout");
	WStackPage3Layout->setAlignment( Qt::AlignTop );
	layout20 = new Q3HBoxLayout( 0, 0, 5, "layout7");
	textLabel6 = new QLabel( tr( "Brightness:" ), WStackPage_3, "textLabel6" );
	layout20->addWidget( textLabel6, Qt::AlignLeft );
	textLabel7 = new QLabel( "0", WStackPage_3, "textLabel7" );
	layout20->addWidget( textLabel7, Qt::AlignRight );
	WStackPage3Layout->addLayout( layout20 );
	brightnessSlider = new QSlider( WStackPage_3, "Slider1_2" );
	brightnessSlider->setMinValue(-255);
	brightnessSlider->setMaxValue(255);
	brightnessSlider->setValue(0);
	brightnessSlider->setOrientation( Qt::Horizontal );
	brightnessSlider->setTickmarks( QSlider::Below );
	WStackPage3Layout->addWidget( brightnessSlider );
	optionStack->addWidget( WStackPage_3, 2 );

	WStackPage_4 = new QWidget( optionStack, "WStackPage_4" );
	WStackPage4Layout = new Q3VBoxLayout( WStackPage_4, 5, 5, "WStackPageLayout");
	WStackPage4Layout->setAlignment( Qt::AlignTop );
	layout21 = new Q3HBoxLayout( 0, 0, 5, "layout7");
	textLabel8 = new QLabel( tr( "Contrast:" ), WStackPage_4, "textLabel8" );
	layout21->addWidget( textLabel8, Qt::AlignLeft );
	textLabel9 = new QLabel( "0", WStackPage_4, "textLabel9" );
	layout21->addWidget( textLabel9, Qt::AlignRight );
	WStackPage4Layout->addLayout( layout21 );
	contrastSlider = new QSlider( WStackPage_4, "Slider2" );
	contrastSlider->setMinValue(-127);
	contrastSlider->setMaxValue(127);
	contrastSlider->setValue(0);
	contrastSlider->setOrientation( QSt::Horizontal );
	contrastSlider->setTickmarks( QSlider::Below );
	WStackPage4Layout->addWidget( contrastSlider );
	optionStack->addWidget( WStackPage_4, 3 );

	WStackPage_5 = new QWidget( optionStack, "WStackPage_5" );
	WStackPage5Layout = new Q3VBoxLayout( WStackPage_5, 5, 5, "WStackPageLayout");
	WStackPage5Layout->setAlignment( Qt::AlignTop );
	layout22 = new Q3HBoxLayout( 0, 0, 5, "layout7");
	textLabel10 = new QLabel( tr( "Radius:" ), WStackPage_5, "textLabel10" );
	layout22->addWidget( textLabel10 );
	shRadius = new MSpinBox( 0.0, 10.0, WStackPage_5, 1 );
	shRadius->setValue(0);
	layout22->addWidget( shRadius );
	WStackPage5Layout->addLayout( layout22 );
	layout23 = new Q3HBoxLayout( 0, 0, 5, "layout7");
	textLabel11 = new QLabel( tr("Value:"), WStackPage_5, "textLabel11" );
	layout23->addWidget( textLabel11 );
	shValue = new MSpinBox( 0.0, 5.0, WStackPage_5, 1 );
	shValue->setValue(1.0);
	layout23->addWidget( shValue );
	WStackPage5Layout->addLayout( layout23 );
	optionStack->addWidget( WStackPage_5, 4 );

	WStackPage_6 = new QWidget( optionStack, "WStackPage_6" );
	WStackPage6Layout = new Q3VBoxLayout( WStackPage_6, 5, 5, "WStackPageLayout");
	WStackPage6Layout->setAlignment( Qt::AlignTop );
	layout24 = new Q3HBoxLayout( 0, 0, 5, "layout7");
	textLabel12 = new QLabel( tr( "Radius:" ), WStackPage_6, "textLabel10" );
	layout24->addWidget( textLabel12 );
	blRadius = new MSpinBox( 0.0, 30.0, WStackPage_6, 1 );
	blRadius->setValue(0);
	layout24->addWidget( blRadius );
	WStackPage6Layout->addLayout( layout24 );
	optionStack->addWidget( WStackPage_6, 5 );

	WStackPage_7 = new QWidget( optionStack, "WStackPage_4" );
	WStackPage7Layout = new Q3VBoxLayout( WStackPage_7, 5, 5, "WStackPageLayout");
	WStackPage7Layout->setAlignment( Qt::AlignTop );
	layout26 = new Q3HBoxLayout( 0, 0, 5, "layout7");
	textLabel14 = new QLabel( tr( "Posterize:" ), WStackPage_7, "textLabel8" );
	layout26->addWidget( textLabel14, Qt::AlignLeft );
	textLabel15 = new QLabel( "0", WStackPage_7, "textLabel9" );
	layout26->addWidget( textLabel15, Qt::AlignRight );
	WStackPage7Layout->addLayout( layout26 );
	solarizeSlider = new QSlider( WStackPage_7, "Slider2" );
	solarizeSlider->setMinValue(1);
	solarizeSlider->setMaxValue(255);
	solarizeSlider->setValue(255);
	solarizeSlider->setOrientation( Qt::Horizontal );
	solarizeSlider->setTickmarks( QSlider::Below );
	WStackPage7Layout->addWidget( solarizeSlider );
	optionStack->addWidget( WStackPage_7, 6 );

	WStackPage_8 = new QWidget( optionStack, "WStackPage_8" );
	WStackPage8Layout = new Q3GridLayout( WStackPage_8, 1, 1, 4, 5, "WStackPage8Layout");
	textLabel1d = new QLabel( tr( "Color 1:" ), WStackPage_8, "textLabel1d" );
	WStackPage8Layout->addWidget( textLabel1d, 0, 0 );
	colData1 = new ColorCombo(false, WStackPage_8, "colData1");
	colData1->insertItems(doc->PageColors, ColorCombo::smallPixmaps);
	WStackPage8Layout->addMultiCellWidget( colData1, 0, 0, 1, 2);
	shade1 = new ShadeButton(WStackPage_8);
	shade1->setValue(100);
	WStackPage8Layout->addWidget( shade1, 1, 1 );
	CurveD1 = new CurveWidget( NULL );
	CurveD1Pop = new Q3PopupMenu();
	CurveD1Pop->insertItem(CurveD1);
	CurveD1Button = new QToolButton( WStackPage_8, "CurveD1Button" );
	CurveD1Button->setText( "" );
	CurveD1Button->setMaximumSize( QSize( 22, 22 ) );
	CurveD1Button->setPixmap(loadIcon("curve.png"));
	CurveD1Button->setPopup(CurveD1Pop);
	CurveD1Button->setPopupDelay(40);
	WStackPage8Layout->addWidget( CurveD1Button, 1, 2 );

	textLabel2d = new QLabel( tr( "Color 2:" ), WStackPage_8, "textLabel2d" );
	WStackPage8Layout->addWidget( textLabel2d, 2, 0 );
	colData2 = new ColorCombo(false, WStackPage_8, "colData2");
	colData2->insertItems(doc->PageColors, ColorCombo::smallPixmaps);
	WStackPage8Layout->addMultiCellWidget( colData2, 2, 2, 1, 2);
	shade2 = new ShadeButton(WStackPage_8);
	shade2->setValue(100);
	WStackPage8Layout->addWidget( shade2, 3, 1 );
	CurveD2 = new CurveWidget( NULL );
	CurveD2Pop = new Q3PopupMenu();
	CurveD2Pop->insertItem(CurveD2);
	CurveD2Button = new QToolButton( WStackPage_8, "CurveD2Button" );
	CurveD2Button->setText( "" );
	CurveD2Button->setMaximumSize( QSize( 22, 22 ) );
	CurveD2Button->setPixmap(loadIcon("curve.png"));
	CurveD2Button->setPopup(CurveD2Pop);
	CurveD2Button->setPopupDelay(40);
	WStackPage8Layout->addWidget( CurveD2Button, 3, 2 );
	QSpacerItem *spacerD1 = new QSpacerItem( 1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding );
	WStackPage8Layout->addItem( spacerD1, 4, 0 );
	optionStack->addWidget( WStackPage_8, 7 );

	WStackPage_9 = new QWidget( optionStack, "WStackPage_9" );
	WStackPage9Layout = new Q3GridLayout( WStackPage_9, 1, 1, 4, 5, "WStackPage9Layout");
	textLabel1t = new QLabel( tr( "Color 1:" ), WStackPage_9, "textLabel1t" );
	WStackPage9Layout->addWidget( textLabel1t, 0, 0 );
	colDatat1 = new ColorCombo(false, WStackPage_9, "colDatat1");
	colDatat1->insertItems(doc->PageColors, ColorCombo::smallPixmaps);
	WStackPage9Layout->addMultiCellWidget( colDatat1, 0, 0, 1, 2 );
	shadet1 = new ShadeButton(WStackPage_9);
	shadet1->setValue(100);
	WStackPage9Layout->addWidget( shadet1, 1, 1 );
	CurveT1 = new CurveWidget( NULL );
	CurveT1Pop = new Q3PopupMenu();
	CurveT1Pop->insertItem(CurveT1);
	CurveT1Button = new QToolButton( WStackPage_9, "CurveT1Button" );
	CurveT1Button->setText( "" );
	CurveT1Button->setMaximumSize( QSize( 22, 22 ) );
	CurveT1Button->setPixmap(loadIcon("curve.png"));
	CurveT1Button->setPopup(CurveT1Pop);
	CurveT1Button->setPopupDelay(40);
	WStackPage9Layout->addWidget( CurveT1Button, 1, 2 );
	textLabel2t = new QLabel( tr( "Color 2:" ), WStackPage_9, "textLabel2t" );
	WStackPage9Layout->addWidget( textLabel2t, 2, 0 );
	colDatat2 = new ColorCombo(false, WStackPage_9, "colDatat2");
	colDatat2->insertItems(doc->PageColors, ColorCombo::smallPixmaps);
	WStackPage9Layout->addMultiCellWidget( colDatat2, 2, 2, 1, 2 );
	shadet2 = new ShadeButton(WStackPage_9);
	shadet2->setValue(100);
	WStackPage9Layout->addWidget( shadet2, 3, 1 );
	CurveT2 = new CurveWidget( NULL );
	CurveT2Pop = new Q3PopupMenu();
	CurveT2Pop->insertItem(CurveT2);
	CurveT2Button = new QToolButton( WStackPage_9, "CurveT2Button" );
	CurveT2Button->setText( "" );
	CurveT2Button->setMaximumSize( QSize( 22, 22 ) );
	CurveT2Button->setPixmap(loadIcon("curve.png"));
	CurveT2Button->setPopup(CurveT2Pop);
	CurveT2Button->setPopupDelay(40);
	WStackPage9Layout->addWidget( CurveT2Button, 3, 2 );
	textLabel3t = new QLabel( tr( "Color 3:" ), WStackPage_9, "textLabel3t" );
	WStackPage9Layout->addWidget( textLabel3t, 4, 0 );
	colDatat3 = new ColorCombo(false, WStackPage_9, "colDatat3");
	colDatat3->insertItems(doc->PageColors, ColorCombo::smallPixmaps);
	WStackPage9Layout->addMultiCellWidget( colDatat3, 4, 4, 1, 2 );
	shadet3 = new ShadeButton(WStackPage_9);
	shadet3->setValue(100);
	WStackPage9Layout->addWidget( shadet3, 5, 1 );
	CurveT3 = new CurveWidget( NULL );
	CurveT3Pop = new Q3PopupMenu();
	CurveT3Pop->insertItem(CurveT3);
	CurveT3Button = new QToolButton( WStackPage_9, "CurveT3Button" );
	CurveT3Button->setText( "" );
	CurveT3Button->setMaximumSize( QSize( 22, 22 ) );
	CurveT3Button->setPixmap(loadIcon("curve.png"));
	CurveT3Button->setPopup(CurveT3Pop);
	CurveT3Button->setPopupDelay(40);
	WStackPage9Layout->addWidget( CurveT3Button, 5, 2 );
	optionStack->addWidget( WStackPage_9, 8 );

	WStackPage_10 = new QWidget( optionStack, "WStackPage_10" );
	WStackPage10Layout = new Q3GridLayout( WStackPage_10, 1, 1, 4, 5, "WStackPage10Layout");
	textLabel1q = new QLabel( tr( "Color 1:" ), WStackPage_10, "textLabel1q" );
	WStackPage10Layout->addWidget( textLabel1q, 0, 0 );
	colDataq1 = new ColorCombo(false, WStackPage_10, "colDataq1");
	colDataq1->insertItems(doc->PageColors, ColorCombo::smallPixmaps);
	WStackPage10Layout->addMultiCellWidget( colDataq1, 0, 0, 1, 2 );
	shadeq1 = new ShadeButton(WStackPage_10);
	shadeq1->setValue(100);
	WStackPage10Layout->addWidget( shadeq1, 1, 1 );
	CurveQ1 = new CurveWidget( NULL );
	CurveQ1Pop = new Q3PopupMenu();
	CurveQ1Pop->insertItem(CurveQ1);
	CurveQ1Button = new QToolButton( WStackPage_10, "CurveQ1Button" );
	CurveQ1Button->setText( "" );
	CurveQ1Button->setMaximumSize( QSize( 22, 22 ) );
	CurveQ1Button->setPixmap(loadIcon("curve.png"));
	CurveQ1Button->setPopup(CurveQ1Pop);
	CurveQ1Button->setPopupDelay(40);
	WStackPage10Layout->addWidget( CurveQ1Button, 1, 2 );
	textLabel2q = new QLabel( tr( "Color 2:" ), WStackPage_10, "textLabel2q" );
	WStackPage10Layout->addWidget( textLabel2q, 2, 0 );
	colDataq2 = new ColorCombo(false, WStackPage_10, "colDataq2");
	colDataq2->insertItems(doc->PageColors, ColorCombo::smallPixmaps);
	WStackPage10Layout->addMultiCellWidget( colDataq2, 2, 2, 1, 2 );
	shadeq2 = new ShadeButton(WStackPage_10);
	shadeq2->setValue(100);
	WStackPage10Layout->addWidget( shadeq2, 3, 1 );
	CurveQ2 = new CurveWidget( NULL );
	CurveQ2Pop = new Q3PopupMenu();
	CurveQ2Pop->insertItem(CurveQ2);
	CurveQ2Button = new QToolButton( WStackPage_10, "CurveQ2Button" );
	CurveQ2Button->setText( "" );
	CurveQ2Button->setMaximumSize( QSize( 22, 22 ) );
	CurveQ2Button->setPixmap(loadIcon("curve.png"));
	CurveQ2Button->setPopup(CurveQ2Pop);
	CurveQ2Button->setPopupDelay(40);
	WStackPage10Layout->addWidget( CurveQ2Button, 3, 2 );
	textLabel3q = new QLabel( tr( "Color 3:" ), WStackPage_10, "textLabel3q" );
	WStackPage10Layout->addWidget( textLabel3q, 4, 0 );
	colDataq3 = new ColorCombo(false, WStackPage_10, "colDataq3");
	colDataq3->insertItems(doc->PageColors, ColorCombo::smallPixmaps);
	WStackPage10Layout->addMultiCellWidget( colDataq3, 4, 4, 1, 2 );
	shadeq3 = new ShadeButton(WStackPage_10);
	shadeq3->setValue(100);
	WStackPage10Layout->addWidget( shadeq3, 5, 1 );
	CurveQ3 = new CurveWidget( NULL );
	CurveQ3Pop = new Q3PopupMenu();
	CurveQ3Pop->insertItem(CurveQ3);
	CurveQ3Button = new QToolButton( WStackPage_10, "CurveQ3Button" );
	CurveQ3Button->setText( "" );
	CurveQ3Button->setMaximumSize( QSize( 22, 22 ) );
	CurveQ3Button->setPixmap(loadIcon("curve.png"));
	CurveQ3Button->setPopup(CurveQ1Pop);
	CurveQ3Button->setPopupDelay(40);
	WStackPage10Layout->addWidget( CurveQ3Button, 5, 2 );
	textLabel4q = new QLabel( tr( "Color 4:" ), WStackPage_10, "textLabel4q" );
	WStackPage10Layout->addWidget( textLabel4q, 6, 0 );
	colDataq4 = new ColorCombo(false, WStackPage_10, "colDataq4");
	colDataq4->insertItems(doc->PageColors, ColorCombo::smallPixmaps);
	WStackPage10Layout->addMultiCellWidget( colDataq4, 6, 6, 1, 2 );
	shadeq4 = new ShadeButton(WStackPage_10);
	shadeq4->setValue(100);
	WStackPage10Layout->addWidget( shadeq4, 7, 1 );
	CurveQ4 = new CurveWidget( NULL );
	CurveQ4Pop = new Q3PopupMenu();
	CurveQ4Pop->insertItem(CurveQ4);
	CurveQ4Button = new QToolButton( WStackPage_10, "CurveQ4Button" );
	CurveQ4Button->setText( "" );
	CurveQ4Button->setMaximumSize( QSize( 22, 22 ) );
	CurveQ4Button->setPixmap(loadIcon("curve.png"));
	CurveQ4Button->setPopup(CurveQ1Pop);
	CurveQ4Button->setPopupDelay(40);
	WStackPage10Layout->addWidget( CurveQ4Button, 7, 2 );
	optionStack->addWidget( WStackPage_10, 9 );

	WStackPage_11 = new QWidget( optionStack, "WStackPage_11" );
	WStackPage11Layout = new Q3VBoxLayout( WStackPage_11, 5, 5, "WStackPageLayout");
	WStackPage11Layout->setAlignment( Qt::AlignTop );
	Kdisplay = new CurveWidget(WStackPage_11);
	WStackPage11Layout->addWidget( Kdisplay );
	optionStack->addWidget( WStackPage_11, 10 );

	layout16->addWidget( optionStack );
	EffectsDialogLayout->addLayout( layout16 );

	layout18 = new Q3VBoxLayout( 0, 0, 0, "layout18");
	layout10 = new Q3GridLayout( 0, 1, 1, 0, 5, "layout10");
	layout2 = new Q3VBoxLayout( 0, 0, 5, "layout2");
	textLabel1 = new QLabel( this, "textLabel1" );
	textLabel1->setText( tr( "Available Effects" ) );
	layout2->addWidget( textLabel1 );
	availableEffects = new Q3ListBox( this, "availableEffects" );
	availableEffects->clear();
	
	new EffectListItem(availableEffects, tr("Blur"));
	new EffectListItem(availableEffects, tr("Brightness"));
	new EffectListItem(availableEffects, tr("Colorize"));
	new EffectListItem(availableEffects, tr("Duotone"));
	new EffectListItem(availableEffects, tr("Tritone"));
	new EffectListItem(availableEffects, tr("Quadtone"));
	new EffectListItem(availableEffects, tr("Contrast"));
	new EffectListItem(availableEffects, tr("Grayscale"));
	new EffectListItem(availableEffects, tr("Curves"));
	new EffectListItem(availableEffects, tr("Invert"));
	new EffectListItem(availableEffects, tr("Posterize"));
	new EffectListItem(availableEffects, tr("Sharpen"));
/*
	availableEffects->insertItem( tr("Blur"));
	availableEffects->insertItem( tr("Brightness"));
	availableEffects->insertItem( tr("Colorize"));
	availableEffects->insertItem( tr("Duotone"));
	availableEffects->insertItem( tr("Tritone"));
	availableEffects->insertItem( tr("Quadtone"));
	availableEffects->insertItem( tr("Contrast"));
	availableEffects->insertItem( tr("Grayscale"));
	availableEffects->insertItem( tr("Curves"));
	availableEffects->insertItem( tr("Invert"));
	availableEffects->insertItem( tr("Posterize"));
	availableEffects->insertItem( tr("Sharpen"));
*/
	availableEffects->setMinimumSize(fontMetrics().width( tr( "Available Effects" ))+40, 180);
	layout2->addWidget( availableEffects );
	layout10->addLayout( layout2, 0, 0 );

	layout1 = new Q3VBoxLayout( 0, 0, 5, "layout1");
	QSpacerItem* spacer1 = new QSpacerItem( 21, 61, QSizePolicy::Minimum, QSizePolicy::Expanding );
	layout1->addItem( spacer1 );
	toEffects = new QPushButton( this, "toEffects" );
	toEffects->setText( tr( ">>" ) );
	toEffects->setEnabled(false);
	layout1->addWidget( toEffects );
	fromEffects = new QPushButton( this, "fromEffects" );
	fromEffects->setText( tr( "<<" ) );
	fromEffects->setEnabled(false);
	layout1->addWidget( fromEffects );
	QSpacerItem* spacer2 = new QSpacerItem( 31, 81, QSizePolicy::Minimum, QSizePolicy::Expanding );
	layout1->addItem( spacer2 );
	layout10->addLayout( layout1, 0, 1 );

	layout8 = new Q3VBoxLayout( 0, 0, 5, "layout8");
	textLabel2 = new QLabel( this, "textLabel2" );
	textLabel2->setText( tr( "Effects in use" ) );
	layout8->addWidget( textLabel2 );
	usedEffects = new Q3ListBox( this, "usedEffects" );
	usedEffects->setMinimumSize(fontMetrics().width( tr( "Available Effects" ))+40, 180);
	usedEffects->clear();
	effectValMap.clear();
	for (uint a = 0; a < effectsList.count(); ++a)
	{
		if ((*effectsList.at(a)).effectCode == ScImage::EF_INVERT)
		{
			usedEffects->insertItem( tr("Invert"));
			effectValMap.insert(usedEffects->item(usedEffects->count()-1), "");
		}
		if ((*effectsList.at(a)).effectCode == ScImage::EF_GRAYSCALE)
		{
			usedEffects->insertItem( tr("Grayscale"));
			effectValMap.insert(usedEffects->item(usedEffects->count()-1), "");
		}
		if ((*effectsList.at(a)).effectCode == ScImage::EF_COLORIZE)
		{
			usedEffects->insertItem( tr("Colorize"));
			effectValMap.insert(usedEffects->item(usedEffects->count()-1), (*effectsList.at(a)).effectParameters);
			availableEffects->item(2)->setSelectable(false);
			availableEffects->item(3)->setSelectable(false);
			availableEffects->item(4)->setSelectable(false);
			availableEffects->item(5)->setSelectable(false);
		}
		if ((*effectsList.at(a)).effectCode == ScImage::EF_BRIGHTNESS)
		{
			usedEffects->insertItem( tr("Brightness"));
			effectValMap.insert(usedEffects->item(usedEffects->count()-1), (*effectsList.at(a)).effectParameters);
		}
		if ((*effectsList.at(a)).effectCode == ScImage::EF_CONTRAST)
		{
			usedEffects->insertItem( tr("Contrast"));
			effectValMap.insert(usedEffects->item(usedEffects->count()-1), (*effectsList.at(a)).effectParameters);
		}
		if ((*effectsList.at(a)).effectCode == ScImage::EF_SHARPEN)
		{
			usedEffects->insertItem( tr("Sharpen"));
			effectValMap.insert(usedEffects->item(usedEffects->count()-1), (*effectsList.at(a)).effectParameters);
		}
		if ((*effectsList.at(a)).effectCode == ScImage::EF_BLUR)
		{
			usedEffects->insertItem( tr("Blur"));
			effectValMap.insert(usedEffects->item(usedEffects->count()-1), (*effectsList.at(a)).effectParameters);
		}
		if ((*effectsList.at(a)).effectCode == ScImage::EF_SOLARIZE)
		{
			usedEffects->insertItem( tr("Posterize"));
			effectValMap.insert(usedEffects->item(usedEffects->count()-1), (*effectsList.at(a)).effectParameters);
		}
		if ((*effectsList.at(a)).effectCode == ScImage::EF_DUOTONE)
		{
			usedEffects->insertItem( tr("Duotone"));
			effectValMap.insert(usedEffects->item(usedEffects->count()-1), (*effectsList.at(a)).effectParameters);
			availableEffects->item(2)->setSelectable(false);
			availableEffects->item(3)->setSelectable(false);
			availableEffects->item(4)->setSelectable(false);
			availableEffects->item(5)->setSelectable(false);
		}
		if ((*effectsList.at(a)).effectCode == ScImage::EF_TRITONE)
		{
			usedEffects->insertItem( tr("Tritone"));
			effectValMap.insert(usedEffects->item(usedEffects->count()-1), (*effectsList.at(a)).effectParameters);
			availableEffects->item(2)->setSelectable(false);
			availableEffects->item(3)->setSelectable(false);
			availableEffects->item(4)->setSelectable(false);
			availableEffects->item(5)->setSelectable(false);
		}
		if ((*effectsList.at(a)).effectCode == ScImage::EF_QUADTONE)
		{
			usedEffects->insertItem( tr("Quadtone"));
			effectValMap.insert(usedEffects->item(usedEffects->count()-1), (*effectsList.at(a)).effectParameters);
			availableEffects->item(2)->setSelectable(false);
			availableEffects->item(3)->setSelectable(false);
			availableEffects->item(4)->setSelectable(false);
			availableEffects->item(5)->setSelectable(false);
		}
		if ((*effectsList.at(a)).effectCode == ScImage::EF_GRADUATE)
		{
			usedEffects->insertItem( tr("Curves"));
			effectValMap.insert(usedEffects->item(usedEffects->count()-1), (*effectsList.at(a)).effectParameters);
		}
	}
	layout8->addWidget( usedEffects );
	layout7 = new Q3HBoxLayout( 0, 0, 5, "layout7");
	QSpacerItem* spacer4 = new QSpacerItem( 10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum );
	layout7->addItem( spacer4 );
	effectUp = new QPushButton( this, "effectUp" );
	effectUp->setText( "" );
	effectUp->setPixmap(loadIcon("Raiselayer.png"));
	effectUp->setEnabled(false);
	layout7->addWidget( effectUp );
	effectDown = new QPushButton( this, "effectDown" );
	effectDown->setText( "" );
	effectDown->setPixmap(loadIcon("Lowerlayer.png"));
	effectDown->setEnabled(false);
	layout7->addWidget( effectDown );
	QSpacerItem* spacer5 = new QSpacerItem( 10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum );
	layout7->addItem( spacer5 );
	layout8->addLayout( layout7 );
	layout10->addLayout( layout8, 0, 2 );
	layout18->addLayout( layout10 );

	layout9 = new Q3HBoxLayout( 0, 0, 5, "layout9");
	QSpacerItem* spacer3 = new QSpacerItem( 111, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
	layout9->addItem( spacer3 );

	okButton = new QPushButton( this, "okButton" );
	okButton->setText( tr( "OK" ) );
	layout9->addWidget( okButton );

	cancelButton = new QPushButton( this, "cancelButton" );
	cancelButton->setText( tr( "Cancel" ) );
	layout9->addWidget( cancelButton );
	layout18->addLayout( layout9 );
	EffectsDialogLayout->addLayout( layout18 );
	optionStack->raiseWidget(0);
	createPreview();
	availableEffects->repaintContents();
	resize( minimumSizeHint() );

	// signals and slots connections
	connect( okButton, SIGNAL( clicked() ), this, SLOT( leaveOK() ) );
	connect( cancelButton, SIGNAL( clicked() ), this, SLOT( reject() ) );
	connect( usedEffects, SIGNAL( clicked(Q3ListBoxItem*) ), this, SLOT( selectEffect(Q3ListBoxItem*) ) );
	connect( usedEffects, SIGNAL( doubleClicked(Q3ListBoxItem*) ), this, SLOT( moveFromEffects() ) );
	connect( availableEffects, SIGNAL( clicked(Q3ListBoxItem*) ), this, SLOT( selectAvailEffect(Q3ListBoxItem*) ) );
	connect( availableEffects, SIGNAL( doubleClicked(Q3ListBoxItem*) ), this, SLOT( selectAvailEffectDbl(Q3ListBoxItem*) ) );
	connect( toEffects, SIGNAL( clicked() ), this, SLOT( moveToEffects() ) );
	connect( fromEffects, SIGNAL( clicked() ), this, SLOT( moveFromEffects() ) );
	connect( effectUp, SIGNAL( clicked() ), this, SLOT( moveEffectUp() ) );
	connect( effectDown, SIGNAL( clicked() ), this, SLOT( moveEffectDown() ) );
	connect( colData, SIGNAL(activated(int)), this, SLOT( createPreview()));
	connect( shade, SIGNAL(clicked()), this, SLOT(createPreview()));
	connect( colData1, SIGNAL(activated(int)), this, SLOT( createPreview()));
	connect( shade1, SIGNAL(clicked()), this, SLOT(createPreview()));
	connect( colData2, SIGNAL(activated(int)), this, SLOT( createPreview()));
	connect( shade2, SIGNAL(clicked()), this, SLOT(createPreview()));
	connect( colDatat1, SIGNAL(activated(int)), this, SLOT( createPreview()));
	connect( shadet1, SIGNAL(clicked()), this, SLOT(createPreview()));
	connect( colDatat2, SIGNAL(activated(int)), this, SLOT( createPreview()));
	connect( shadet2, SIGNAL(clicked()), this, SLOT(createPreview()));
	connect( colDatat3, SIGNAL(activated(int)), this, SLOT( createPreview()));
	connect( shadet3, SIGNAL(clicked()), this, SLOT(createPreview()));
	connect( colDataq1, SIGNAL(activated(int)), this, SLOT( createPreview()));
	connect( shadeq1, SIGNAL(clicked()), this, SLOT(createPreview()));
	connect( colDataq2, SIGNAL(activated(int)), this, SLOT( createPreview()));
	connect( shadeq2, SIGNAL(clicked()), this, SLOT(createPreview()));
	connect( colDataq3, SIGNAL(activated(int)), this, SLOT( createPreview()));
	connect( shadeq3, SIGNAL(clicked()), this, SLOT(createPreview()));
	connect( colDataq4, SIGNAL(activated(int)), this, SLOT( createPreview()));
	connect( shadeq4, SIGNAL(clicked()), this, SLOT(createPreview()));
	connect( brightnessSlider, SIGNAL(valueChanged(int)), this, SLOT(updateBright(int)));
	connect( brightnessSlider, SIGNAL(sliderReleased()), this, SLOT(createPreview()));
	connect( contrastSlider, SIGNAL(valueChanged(int)), this, SLOT(updateContrast(int)));
	connect( contrastSlider, SIGNAL(sliderReleased()), this, SLOT(createPreview()));
	connect( shRadius, SIGNAL(valueChanged(int)), this, SLOT(createPreview()));
	connect( shValue, SIGNAL(valueChanged(int)), this, SLOT(createPreview()));
	connect( blRadius, SIGNAL(valueChanged(int)), this, SLOT(createPreview()));
	connect( solarizeSlider, SIGNAL(valueChanged(int)), this, SLOT(updateSolarize(int)));
	connect( solarizeSlider, SIGNAL(sliderReleased()), this, SLOT(createPreview()));
	connect( Kdisplay->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
	connect( CurveD1->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
	connect( CurveD2->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
	connect( CurveT1->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
	connect( CurveT2->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
	connect( CurveT3->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
	connect( CurveQ1->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
	connect( CurveQ2->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
	connect( CurveQ3->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
	connect( CurveQ4->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
	tim.start();
}

void EffectsDialog::leaveOK()
{
	saveValues(true);
	accept();
}

void EffectsDialog::updateSolarize(int val)
{
	QString tmp;
	tmp.setNum(val);
	textLabel15->setText(tmp);
	createPreview();
}

void EffectsDialog::updateContrast(int val)
{
	QString tmp;
	tmp.setNum(val);
	textLabel9->setText(tmp);
	createPreview();
}

void EffectsDialog::updateBright(int val)
{
	QString tmp;
	tmp.setNum(val);
	textLabel7->setText(tmp);
	createPreview();
}

void EffectsDialog::createPreview()
{
	if (tim.elapsed() < 50)
		return;
	ScImage im(image);
	saveValues(false);
	im.applyEffect(effectsList, doc->PageColors, false);
	QPixmap Bild = QPixmap(pixmapLabel1->width(), pixmapLabel1->height());
	int x = (pixmapLabel1->width() - im.qImage().width()) / 2;
	int y = (pixmapLabel1->height() - im.qImage().height()) / 2;
	QPainter p;
	QBrush b(QColor(205,205,205), loadIcon("testfill.png"));
	p.begin(&Bild);
	p.fillRect(0, 0, pixmapLabel1->width(), pixmapLabel1->height(), b);
	p.drawImage(x, y, im.qImage());
	p.end();
	pixmapLabel1->setPixmap( Bild );
	tim.start();
}

void EffectsDialog::saveValues(bool final)
{
	selectEffectHelper(final);
	effectsList.clear();
	struct ScImage::imageEffect ef;
	for (uint e = 0; e < usedEffects->count(); ++e)
	{
		if (usedEffects->item(e)->text() == tr("Invert"))
		{
			ef.effectCode = ScImage::EF_INVERT;
			ef.effectParameters = "";
		}
		if (usedEffects->item(e)->text() == tr("Grayscale"))
		{
			ef.effectCode = ScImage::EF_GRAYSCALE;
			ef.effectParameters = "";
		}
		if (usedEffects->item(e)->text() == tr("Colorize"))
		{
			ef.effectCode = ScImage::EF_COLORIZE;
			ef.effectParameters = effectValMap[usedEffects->item(e)];
		}
		if (usedEffects->item(e)->text() == tr("Brightness"))
		{
			ef.effectCode = ScImage::EF_BRIGHTNESS;
			ef.effectParameters = effectValMap[usedEffects->item(e)];
		}
		if (usedEffects->item(e)->text() == tr("Contrast"))
		{
			ef.effectCode = ScImage::EF_CONTRAST;
			ef.effectParameters = effectValMap[usedEffects->item(e)];
		}
		if (usedEffects->item(e)->text() == tr("Sharpen"))
		{
			ef.effectCode = ScImage::EF_SHARPEN;
			ef.effectParameters = effectValMap[usedEffects->item(e)];
		}
		if (usedEffects->item(e)->text() == tr("Blur"))
		{
			ef.effectCode = ScImage::EF_BLUR;
			ef.effectParameters = effectValMap[usedEffects->item(e)];
		}
		if (usedEffects->item(e)->text() == tr("Posterize"))
		{
			ef.effectCode = ScImage::EF_SOLARIZE;
			ef.effectParameters = effectValMap[usedEffects->item(e)];
		}
		if (usedEffects->item(e)->text() == tr("Duotone"))
		{
			ef.effectCode = ScImage::EF_DUOTONE;
			ef.effectParameters = effectValMap[usedEffects->item(e)];
		}
		if (usedEffects->item(e)->text() == tr("Tritone"))
		{
			ef.effectCode = ScImage::EF_TRITONE;
			ef.effectParameters = effectValMap[usedEffects->item(e)];
		}
		if (usedEffects->item(e)->text() == tr("Quadtone"))
		{
			ef.effectCode = ScImage::EF_QUADTONE;
			ef.effectParameters = effectValMap[usedEffects->item(e)];
		}
		if (usedEffects->item(e)->text() == tr("Curves"))
		{
			ef.effectCode = ScImage::EF_GRADUATE;
			ef.effectParameters = effectValMap[usedEffects->item(e)];
		}
		effectsList.append(ef);
	}
}

void EffectsDialog::selectAvailEffectDbl(Q3ListBoxItem* c)
{
	if (c)
	{
		if (!c->isSelectable())
			return;
		else
			moveToEffects();
	}
}

void EffectsDialog::moveToEffects()
{
	usedEffects->insertItem(availableEffects->currentText());
	if (availableEffects->currentText() == tr("Invert"))
		effectValMap.insert(usedEffects->item(usedEffects->count()-1), "");
	if (availableEffects->currentText() == tr("Grayscale"))
		effectValMap.insert(usedEffects->item(usedEffects->count()-1), "");
	if (availableEffects->currentText() == tr("Brightness"))
		effectValMap.insert(usedEffects->item(usedEffects->count()-1), "0");
	if (availableEffects->currentText() == tr("Contrast"))
		effectValMap.insert(usedEffects->item(usedEffects->count()-1), "0");
	if (availableEffects->currentText() == tr("Sharpen"))
		effectValMap.insert(usedEffects->item(usedEffects->count()-1), "0 1");
	if (availableEffects->currentText() == tr("Blur"))
		effectValMap.insert(usedEffects->item(usedEffects->count()-1), "0 1");
	if (availableEffects->currentText() == tr("Posterize"))
		effectValMap.insert(usedEffects->item(usedEffects->count()-1), "255");
	if (availableEffects->currentText() == tr("Colorize"))
	{
		ColorList::Iterator it;
		it = doc->PageColors.begin();
		QString efval = it.key()+"\n100";
		effectValMap.insert(usedEffects->item(usedEffects->count()-1), efval);
		availableEffects->item(2)->setSelectable(false);
		availableEffects->item(3)->setSelectable(false);
		availableEffects->item(4)->setSelectable(false);
		availableEffects->item(5)->setSelectable(false);
	}
	if (availableEffects->currentText() == tr("Duotone"))
	{
		ColorList::Iterator it;
		it = doc->PageColors.begin();
		QString efval = it.key()+"\n"+it.key()+"\n100 100 2 0.0 0.0 1.0 1.0 0 2 0.0 0.0 1.0 1.0 0";
		effectValMap.insert(usedEffects->item(usedEffects->count()-1), efval);
		availableEffects->item(2)->setSelectable(false);
		availableEffects->item(3)->setSelectable(false);
		availableEffects->item(4)->setSelectable(false);
		availableEffects->item(5)->setSelectable(false);
	}
	if (availableEffects->currentText() == tr("Tritone"))
	{
		ColorList::Iterator it;
		it = doc->PageColors.begin();
		QString efval = it.key()+"\n"+it.key()+"\n"+it.key()+"\n100 100 100 2 0.0 0.0 1.0 1.0 0 2 0.0 0.0 1.0 1.0 0 2 0.0 0.0 1.0 1.0 0";
		effectValMap.insert(usedEffects->item(usedEffects->count()-1), efval);
		availableEffects->item(2)->setSelectable(false);
		availableEffects->item(3)->setSelectable(false);
		availableEffects->item(4)->setSelectable(false);
		availableEffects->item(5)->setSelectable(false);
	}
	if (availableEffects->currentText() == tr("Quadtone"))
	{
		ColorList::Iterator it;
		it = doc->PageColors.begin();
		QString efval = it.key()+"\n"+it.key()+"\n"+it.key()+"\n"+it.key()+"\n100 100 100 100 2 0.0 0.0 1.0 1.0 0 2 0.0 0.0 1.0 1.0 0 2 0.0 0.0 1.0 1.0 0 2 0.0 0.0 1.0 1.0 0";
		effectValMap.insert(usedEffects->item(usedEffects->count()-1), efval);
		availableEffects->item(2)->setSelectable(false);
		availableEffects->item(3)->setSelectable(false);
		availableEffects->item(4)->setSelectable(false);
		availableEffects->item(5)->setSelectable(false);
	}
	if (availableEffects->currentText() == tr("Curves"))
		effectValMap.insert(usedEffects->item(usedEffects->count()-1), "2 0.0 0.0 1.0 1.0 0");
	disconnect( usedEffects, SIGNAL( selected(Q3ListBoxItem*) ), this, SLOT( selectEffect(Q3ListBoxItem*) ) );
	usedEffects->setCurrentItem(usedEffects->item(usedEffects->count()-1));
	selectEffect(usedEffects->item(usedEffects->count()-1));
	connect( usedEffects, SIGNAL( selected(Q3ListBoxItem*) ), this, SLOT( selectEffect(Q3ListBoxItem*) ) );
	createPreview();
	availableEffects->repaintContents();
}

void EffectsDialog::moveFromEffects()
{
	disconnect( usedEffects, SIGNAL( selected(Q3ListBoxItem*) ), this, SLOT( selectEffect(Q3ListBoxItem*) ) );
	if ((usedEffects->currentText() == tr("Colorize")) || (usedEffects->currentText() == tr("Duotone")) || (usedEffects->currentText() == tr("Tritone")) || (usedEffects->currentText() == tr("Quadtone")))
	{
		availableEffects->item(2)->setSelectable(true);
		availableEffects->item(3)->setSelectable(true);
		availableEffects->item(4)->setSelectable(true);
		availableEffects->item(5)->setSelectable(true);
		availableEffects->repaintContents();
	}
	effectValMap.remove(usedEffects->item(usedEffects->currentItem()));
	usedEffects->removeItem(usedEffects->currentItem());
	currentOptions = 0;
	if (usedEffects->count() == 0)
		fromEffects->setEnabled(false);
	if (usedEffects->count() < 2)
	{
		effectUp->setEnabled(false);
		effectDown->setEnabled(false);
	}
	usedEffects->setSelected(usedEffects->item(usedEffects->currentItem()), true);
	selectEffect(usedEffects->item(usedEffects->currentItem()));
	createPreview();
	connect( usedEffects, SIGNAL( selected(Q3ListBoxItem*) ), this, SLOT( selectEffect(Q3ListBoxItem*) ) );
}

void EffectsDialog::moveEffectUp()
{
	int curr = usedEffects->currentItem();
	if (curr == 0)
		return;
	disconnect( usedEffects, SIGNAL( selected(Q3ListBoxItem*) ), this, SLOT( selectEffect(Q3ListBoxItem*) ) );
	usedEffects->clearSelection();
	Q3ListBoxItem *it = usedEffects->item(curr);
	usedEffects->takeItem(it);
	usedEffects->insertItem(it, curr-1);
	usedEffects->setCurrentItem(it);
	selectEffect(usedEffects->item(usedEffects->currentItem()));
	createPreview();
	connect( usedEffects, SIGNAL( selected(Q3ListBoxItem*) ), this, SLOT( selectEffect(Q3ListBoxItem*) ) );
}

void EffectsDialog::moveEffectDown()
{
	int curr = usedEffects->currentItem();
	if (curr == static_cast<int>(usedEffects->count())-1)
		return;
	disconnect( usedEffects, SIGNAL( selected(Q3ListBoxItem*) ), this, SLOT( selectEffect(Q3ListBoxItem*) ) );
	usedEffects->clearSelection();
	Q3ListBoxItem *it = usedEffects->item(curr);
	usedEffects->takeItem(it);
	usedEffects->insertItem(it, curr+1);
	usedEffects->setCurrentItem(it);
	selectEffect(usedEffects->item(usedEffects->currentItem()));
	createPreview();
	connect( usedEffects, SIGNAL( selected(Q3ListBoxItem*) ), this, SLOT( selectEffect(Q3ListBoxItem*) ) );
}

void EffectsDialog::selectEffect(Q3ListBoxItem* c)
{
	toEffects->setEnabled(false);
	selectEffectHelper();
	if (c)
	{
		fromEffects->setEnabled(true);
		if (usedEffects->count() > 1)
		{
			effectUp->setEnabled(true);
			effectDown->setEnabled(true);
			if (usedEffects->currentItem() == 0)
				effectUp->setEnabled(false);
			if (usedEffects->currentItem() == static_cast<int>(usedEffects->count())-1)
				effectDown->setEnabled(false);
		}
		if (c->text() == tr("Grayscale"))
			optionStack->raiseWidget(0);
		else if (c->text() == tr("Invert"))
			optionStack->raiseWidget(0);
		else if (c->text() == tr("Colorize"))
		{
			disconnect( colData, SIGNAL(activated(int)), this, SLOT( createPreview()));
			disconnect( shade, SIGNAL(clicked()), this, SLOT(createPreview()));
			QString tmpstr = effectValMap[c];
			QString col;
			int shading;
			Q3TextStream fp(&tmpstr, QIODevice::ReadOnly);
			fp >> col;
			fp >> shading;
			colData->setCurrentText(col);
			shade->setValue(shading);
			optionStack->raiseWidget(1);
			connect( colData, SIGNAL(activated(int)), this, SLOT( createPreview()));
			connect( shade, SIGNAL(clicked()), this, SLOT(createPreview()));
		}
		else if (c->text() == tr("Duotone"))
		{
			disconnect( colData1, SIGNAL(activated(int)), this, SLOT( createPreview()));
			disconnect( shade1, SIGNAL(clicked()), this, SLOT(createPreview()));
			disconnect( colData2, SIGNAL(activated(int)), this, SLOT( createPreview()));
			disconnect( shade2, SIGNAL(clicked()), this, SLOT(createPreview()));
			disconnect( CurveD1->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
			disconnect( CurveD2->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
			QString tmpstr = effectValMap[c];
			QString col1, col2;
			int shading1, shading2;
			Q3TextStream fp(&tmpstr, QIODevice::ReadOnly);
			col1 = fp.readLine();
			col2 = fp.readLine();
			fp >> shading1;
			fp >> shading2;
			colData1->setCurrentText(col1);
			shade1->setValue(shading1);
			colData2->setCurrentText(col2);
			shade2->setValue(shading2);
			int numVals;
			double xval, yval;
			FPointArray curve;
			curve.resize(0);
			fp >> numVals;
			for (int nv = 0; nv < numVals; nv++)
			{
				fp >> xval;
				fp >> yval;
				curve.addPoint(xval, yval);
			}
			CurveD1->cDisplay->setCurve(curve);
			int lin;
			fp >> lin;
			CurveD1->setLinear(lin);
			curve.resize(0);
			fp >> numVals;
			for (int nv = 0; nv < numVals; nv++)
			{
				fp >> xval;
				fp >> yval;
				curve.addPoint(xval, yval);
			}
			CurveD2->cDisplay->setCurve(curve);
			fp >> lin;
			CurveD2->setLinear(lin);
			optionStack->raiseWidget(7);
			connect( colData1, SIGNAL(activated(int)), this, SLOT( createPreview()));
			connect( shade1, SIGNAL(clicked()), this, SLOT(createPreview()));
			connect( colData2, SIGNAL(activated(int)), this, SLOT( createPreview()));
			connect( shade2, SIGNAL(clicked()), this, SLOT(createPreview()));
			connect( CurveD1->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
			connect( CurveD2->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
		}
		else if (c->text() == tr("Tritone"))
		{
			disconnect( colDatat1, SIGNAL(activated(int)), this, SLOT( createPreview()));
			disconnect( shadet1, SIGNAL(clicked()), this, SLOT(createPreview()));
			disconnect( colDatat2, SIGNAL(activated(int)), this, SLOT( createPreview()));
			disconnect( shadet2, SIGNAL(clicked()), this, SLOT(createPreview()));
			disconnect( colDatat3, SIGNAL(activated(int)), this, SLOT( createPreview()));
			disconnect( shadet3, SIGNAL(clicked()), this, SLOT(createPreview()));
			disconnect( CurveT1->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
			disconnect( CurveT2->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
			disconnect( CurveT3->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
			QString tmpstr = effectValMap[c];
			QString col1, col2, col3;
			int shading1, shading2, shading3;
			Q3TextStream fp(&tmpstr, QIODevice::ReadOnly);
			col1 = fp.readLine();
			col2 = fp.readLine();
			col3 = fp.readLine();
			fp >> shading1;
			fp >> shading2;
			fp >> shading3;
			colDatat1->setCurrentText(col1);
			shadet1->setValue(shading1);
			colDatat2->setCurrentText(col2);
			shadet2->setValue(shading2);
			colDatat3->setCurrentText(col3);
			shadet3->setValue(shading3);
			int numVals;
			double xval, yval;
			FPointArray curve;
			curve.resize(0);
			fp >> numVals;
			for (int nv = 0; nv < numVals; nv++)
			{
				fp >> xval;
				fp >> yval;
				curve.addPoint(xval, yval);
			}
			CurveT1->cDisplay->setCurve(curve);
			int lin;
			fp >> lin;
			CurveT1->setLinear(lin);
			curve.resize(0);
			fp >> numVals;
			for (int nv = 0; nv < numVals; nv++)
			{
				fp >> xval;
				fp >> yval;
				curve.addPoint(xval, yval);
			}
			CurveT2->cDisplay->setCurve(curve);
			fp >> lin;
			CurveT2->setLinear(lin);
			curve.resize(0);
			fp >> numVals;
			for (int nv = 0; nv < numVals; nv++)
			{
				fp >> xval;
				fp >> yval;
				curve.addPoint(xval, yval);
			}
			CurveT3->cDisplay->setCurve(curve);
			fp >> lin;
			CurveT3->setLinear(lin);
			optionStack->raiseWidget(8);
			connect( colDatat1, SIGNAL(activated(int)), this, SLOT( createPreview()));
			connect( shadet1, SIGNAL(clicked()), this, SLOT(createPreview()));
			connect( colDatat2, SIGNAL(activated(int)), this, SLOT( createPreview()));
			connect( shadet2, SIGNAL(clicked()), this, SLOT(createPreview()));
			connect( colDatat3, SIGNAL(activated(int)), this, SLOT( createPreview()));
			connect( shadet3, SIGNAL(clicked()), this, SLOT(createPreview()));
			connect( CurveT1->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
			connect( CurveT2->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
			connect( CurveT3->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
		}
		else if (c->text() == tr("Quadtone"))
		{
			disconnect( colDataq1, SIGNAL(activated(int)), this, SLOT( createPreview()));
			disconnect( shadeq1, SIGNAL(clicked()), this, SLOT(createPreview()));
			disconnect( colDataq2, SIGNAL(activated(int)), this, SLOT( createPreview()));
			disconnect( shadeq2, SIGNAL(clicked()), this, SLOT(createPreview()));
			disconnect( colDataq3, SIGNAL(activated(int)), this, SLOT( createPreview()));
			disconnect( shadeq3, SIGNAL(clicked()), this, SLOT(createPreview()));
			disconnect( colDataq4, SIGNAL(activated(int)), this, SLOT( createPreview()));
			disconnect( shadeq4, SIGNAL(clicked()), this, SLOT(createPreview()));
			disconnect( CurveQ1->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
			disconnect( CurveQ2->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
			disconnect( CurveQ3->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
			disconnect( CurveQ4->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
			QString tmpstr = effectValMap[c];
			QString col1, col2, col3, col4;
			int shading1, shading2, shading3, shading4;
			Q3TextStream fp(&tmpstr, QIODevice::ReadOnly);
			col1 = fp.readLine();
			col2 = fp.readLine();
			col3 = fp.readLine();
			col4 = fp.readLine();
			fp >> shading1;
			fp >> shading2;
			fp >> shading3;
			fp >> shading4;
			colDataq1->setCurrentText(col1);
			shadeq1->setValue(shading1);
			colDataq2->setCurrentText(col2);
			shadeq2->setValue(shading2);
			colDataq3->setCurrentText(col3);
			shadeq3->setValue(shading3);
			colDataq4->setCurrentText(col4);
			shadeq4->setValue(shading4);
			int numVals;
			double xval, yval;
			FPointArray curve;
			curve.resize(0);
			fp >> numVals;
			for (int nv = 0; nv < numVals; nv++)
			{
				fp >> xval;
				fp >> yval;
				curve.addPoint(xval, yval);
			}
			CurveQ1->cDisplay->setCurve(curve);
			int lin;
			fp >> lin;
			CurveQ1->setLinear(lin);
			curve.resize(0);
			fp >> numVals;
			for (int nv = 0; nv < numVals; nv++)
			{
				fp >> xval;
				fp >> yval;
				curve.addPoint(xval, yval);
			}
			CurveQ2->cDisplay->setCurve(curve);
			fp >> lin;
			CurveQ2->setLinear(lin);
			curve.resize(0);
			fp >> numVals;
			for (int nv = 0; nv < numVals; nv++)
			{
				fp >> xval;
				fp >> yval;
				curve.addPoint(xval, yval);
			}
			CurveQ3->cDisplay->setCurve(curve);
			fp >> lin;
			CurveQ3->setLinear(lin);
			curve.resize(0);
			fp >> numVals;
			for (int nv = 0; nv < numVals; nv++)
			{
				fp >> xval;
				fp >> yval;
				curve.addPoint(xval, yval);
			}
			CurveQ4->cDisplay->setCurve(curve);
			fp >> lin;
			CurveQ4->setLinear(lin);
			optionStack->raiseWidget(9);
			connect( colDataq1, SIGNAL(activated(int)), this, SLOT( createPreview()));
			connect( shadeq1, SIGNAL(clicked()), this, SLOT(createPreview()));
			connect( colDataq2, SIGNAL(activated(int)), this, SLOT( createPreview()));
			connect( shadeq2, SIGNAL(clicked()), this, SLOT(createPreview()));
			connect( colDataq3, SIGNAL(activated(int)), this, SLOT( createPreview()));
			connect( shadeq3, SIGNAL(clicked()), this, SLOT(createPreview()));
			connect( colDataq4, SIGNAL(activated(int)), this, SLOT( createPreview()));
			connect( shadeq4, SIGNAL(clicked()), this, SLOT(createPreview()));
			connect( CurveQ1->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
			connect( CurveQ2->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
			connect( CurveQ3->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
			connect( CurveQ4->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
		}
		else if (c->text() == tr("Brightness"))
		{
			disconnect( brightnessSlider, SIGNAL(valueChanged(int)), this, SLOT(updateBright(int)));
			disconnect( brightnessSlider, SIGNAL(sliderReleased()), this, SLOT(createPreview()));
			QString tmpstr = effectValMap[c];
			int brightness;
			Q3TextStream fp(&tmpstr, QIODevice::ReadOnly);
			fp >> brightness;
			brightnessSlider->setValue(brightness);
			QString tmp;
			tmp.setNum(brightness);
			textLabel7->setText(tmp);
			optionStack->raiseWidget(2);
			connect( brightnessSlider, SIGNAL(valueChanged(int)), this, SLOT(updateBright(int)));
			connect( brightnessSlider, SIGNAL(sliderReleased()), this, SLOT(createPreview()));
		}
		else if (c->text() == tr("Contrast"))
		{
			disconnect( contrastSlider, SIGNAL(valueChanged(int)), this, SLOT(updateContrast(int)));
			disconnect( contrastSlider, SIGNAL(sliderReleased()), this, SLOT(createPreview()));
			QString tmpstr = effectValMap[c];
			int contrast;
			Q3TextStream fp(&tmpstr, QIODevice::ReadOnly);
			fp >> contrast;
			contrastSlider->setValue(contrast);
			QString tmp;
			tmp.setNum(contrast);
			textLabel9->setText(tmp);
			optionStack->raiseWidget(3);
			connect( contrastSlider, SIGNAL(valueChanged(int)), this, SLOT(updateContrast(int)));
			connect( contrastSlider, SIGNAL(sliderReleased()), this, SLOT(createPreview()));
		}
		else if (c->text() == tr("Sharpen"))
		{
			disconnect( shRadius, SIGNAL(valueChanged(int)), this, SLOT(createPreview()));
			disconnect( shValue, SIGNAL(valueChanged(int)), this, SLOT(createPreview()));
			QString tmpstr = effectValMap[c];
			double radius, sigma;
			Q3TextStream fp(&tmpstr, QIODevice::ReadOnly);
			fp >> radius;
			fp >> sigma;
			shRadius->setValue(radius);
			shValue->setValue(sigma);
			optionStack->raiseWidget(4);
			connect( shRadius, SIGNAL(valueChanged(int)), this, SLOT(createPreview()));
			connect( shValue, SIGNAL(valueChanged(int)), this, SLOT(createPreview()));
		}
		else if (c->text() == tr("Blur"))
		{
			disconnect( blRadius, SIGNAL(valueChanged(int)), this, SLOT(createPreview()));
			QString tmpstr = effectValMap[c];
			double radius, sigma;
			Q3TextStream fp(&tmpstr, QIODevice::ReadOnly);
			fp >> radius;
			fp >> sigma;
			blRadius->setValue(radius);
			//			blValue->setValue(sigma);
			optionStack->raiseWidget(5);
			connect( blRadius, SIGNAL(valueChanged(int)), this, SLOT(createPreview()));
		}
		else if (c->text() == tr("Posterize"))
		{
			disconnect( solarizeSlider, SIGNAL(valueChanged(int)), this, SLOT(updateSolarize(int)));
			disconnect( solarizeSlider, SIGNAL(sliderReleased()), this, SLOT(createPreview()));
			QString tmpstr = effectValMap[c];
			int solarize;
			Q3TextStream fp(&tmpstr, QIODevice::ReadOnly);
			fp >> solarize;
			solarizeSlider->setValue(solarize);
			QString tmp;
			tmp.setNum(solarize);
			textLabel15->setText(tmp);
			optionStack->raiseWidget(6);
			connect( solarizeSlider, SIGNAL(valueChanged(int)), this, SLOT(updateSolarize(int)));
			connect( solarizeSlider, SIGNAL(sliderReleased()), this, SLOT(createPreview()));
		}
		else if (c->text() == tr("Curves"))
		{
			disconnect( Kdisplay->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
			QString tmpstr = effectValMap[c];
			Q3TextStream fp(&tmpstr, QIODevice::ReadOnly);
			int numVals;
			double xval, yval;
			FPointArray curve;
			curve.resize(0);
			fp >> numVals;
			for (int nv = 0; nv < numVals; nv++)
			{
				fp >> xval;
				fp >> yval;
				curve.addPoint(xval, yval);
			}
			Kdisplay->cDisplay->setCurve(curve);
			int lin;
			fp >> lin;
			if (lin == 1)
				Kdisplay->setLinear(true);
			else
				Kdisplay->setLinear(false);
			optionStack->raiseWidget(10);
			connect( Kdisplay->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
		}
		else
			optionStack->raiseWidget(0);
		currentOptions = c;
	}
	else
		optionStack->raiseWidget(0);
	disconnect( availableEffects, SIGNAL( clicked(Q3ListBoxItem*) ), this, SLOT( selectAvailEffect(Q3ListBoxItem*) ) );
	availableEffects->clearSelection();
	connect( availableEffects, SIGNAL( clicked(Q3ListBoxItem*) ), this, SLOT( selectAvailEffect(Q3ListBoxItem*) ) );
}

void EffectsDialog::selectAvailEffect(Q3ListBoxItem* c)
{
	if (c)
	{
		if (!c->isSelectable())
			toEffects->setEnabled(false);
		else
			toEffects->setEnabled(true);
	}
	fromEffects->setEnabled(false);
	effectUp->setEnabled(false);
	effectDown->setEnabled(false);
	disconnect( usedEffects, SIGNAL( selected(Q3ListBoxItem*) ), this, SLOT( selectEffect(Q3ListBoxItem*) ) );
	selectEffectHelper();
	currentOptions = 0;
	usedEffects->clearSelection();
	optionStack->raiseWidget(0);
	connect( usedEffects, SIGNAL( selected(Q3ListBoxItem*) ), this, SLOT( selectEffect(Q3ListBoxItem*) ) );
}

void EffectsDialog::selectEffectHelper(bool final)
{
	if (currentOptions != 0)
	{
		if (currentOptions->text() == tr("Colorize"))
		{
			QString efval = "";
			efval = colData->currentText();
			QString tmp;
			tmp.setNum(shade->getValue());
			efval += "\n"+tmp;
			effectValMap[currentOptions] = efval;
		}
		if (currentOptions->text() == tr("Brightness"))
		{
			QString tmp;
			tmp.setNum(brightnessSlider->value());
			effectValMap[currentOptions] = tmp;
		}
		if (currentOptions->text() == tr("Contrast"))
		{
			QString tmp;
			tmp.setNum(contrastSlider->value());
			effectValMap[currentOptions] = tmp;
		}
		if (currentOptions->text() == tr("Sharpen"))
		{
			QString efval = "";
			QString tmp;
			tmp.setNum(shRadius->value());
			efval += tmp;
			tmp.setNum(shValue->value());
			efval += " "+tmp;
			effectValMap[currentOptions] = efval;
		}
		if (currentOptions->text() == tr("Blur"))
		{
			QString efval = "";
			QString tmp;
			if (final)
				tmp.setNum(blRadius->value());
			else
				tmp.setNum(blRadius->value()/imageScale);
			efval += tmp;
			tmp.setNum(1.0);
			efval += " "+tmp;
			effectValMap[currentOptions] = efval;
		}
		if (currentOptions->text() == tr("Posterize"))
		{
			QString tmp;
			tmp.setNum(solarizeSlider->value());
			effectValMap[currentOptions] = tmp;
		}
		if (currentOptions->text() == tr("Duotone"))
		{
			QString efval = "";
			efval = colData1->currentText()+"\n";
			efval += colData2->currentText()+"\n";
			QString tmp;
			tmp.setNum(shade1->getValue());
			efval += tmp;
			tmp.setNum(shade2->getValue());
			efval += " "+tmp;
			FPointArray Vals = CurveD1->cDisplay->getCurve();
			tmp.setNum(Vals.size());
			efval += " "+tmp;
			for (uint p = 0; p < Vals.size(); p++)
			{
				FPoint pv = Vals.point(p);
				efval += QString(" %1 %2").arg(pv.x()).arg(pv.y());
			}
			if (CurveD1->cDisplay->isLinear())
				efval += " 1";
			else
				efval += " 0";
			Vals = CurveD2->cDisplay->getCurve();
			tmp.setNum(Vals.size());
			efval += " "+tmp;
			for (uint p = 0; p < Vals.size(); p++)
			{
				FPoint pv = Vals.point(p);
				efval += QString(" %1 %2").arg(pv.x()).arg(pv.y());
			}
			if (CurveD2->cDisplay->isLinear())
				efval += " 1";
			else
				efval += " 0";
			effectValMap[currentOptions] = efval;
		}
		if (currentOptions->text() == tr("Tritone"))
		{
			QString efval = "";
			efval = colDatat1->currentText()+"\n";
			efval += colDatat2->currentText()+"\n";
			efval += colDatat3->currentText()+"\n";
			QString tmp;
			tmp.setNum(shadet1->getValue());
			efval += tmp;
			tmp.setNum(shadet2->getValue());
			efval += " "+tmp;
			tmp.setNum(shadet3->getValue());
			efval += " "+tmp;
			FPointArray Vals = CurveT1->cDisplay->getCurve();
			tmp.setNum(Vals.size());
			efval += " "+tmp;
			for (uint p = 0; p < Vals.size(); p++)
			{
				FPoint pv = Vals.point(p);
				efval += QString(" %1 %2").arg(pv.x()).arg(pv.y());
			}
			if (CurveT1->cDisplay->isLinear())
				efval += " 1";
			else
				efval += " 0";
			Vals = CurveT2->cDisplay->getCurve();
			tmp.setNum(Vals.size());
			efval += " "+tmp;
			for (uint p = 0; p < Vals.size(); p++)
			{
				FPoint pv = Vals.point(p);
				efval += QString(" %1 %2").arg(pv.x()).arg(pv.y());
			}
			if (CurveT2->cDisplay->isLinear())
				efval += " 1";
			else
				efval += " 0";
			Vals = CurveT3->cDisplay->getCurve();
			tmp.setNum(Vals.size());
			efval += " "+tmp;
			for (uint p = 0; p < Vals.size(); p++)
			{
				FPoint pv = Vals.point(p);
				efval += QString(" %1 %2").arg(pv.x()).arg(pv.y());
			}
			if (CurveT3->cDisplay->isLinear())
				efval += " 1";
			else
				efval += " 0";
			effectValMap[currentOptions] = efval;
		}
		if (currentOptions->text() == tr("Quadtone"))
		{
			QString efval = "";
			efval = colDataq1->currentText()+"\n";
			efval += colDataq2->currentText()+"\n";
			efval += colDataq3->currentText()+"\n";
			efval += colDataq4->currentText()+"\n";
			QString tmp;
			tmp.setNum(shadeq1->getValue());
			efval += tmp;
			tmp.setNum(shadeq2->getValue());
			efval += " "+tmp;
			tmp.setNum(shadeq3->getValue());
			efval += " "+tmp;
			tmp.setNum(shadeq4->getValue());
			efval += " "+tmp;
			FPointArray Vals = CurveQ1->cDisplay->getCurve();
			tmp.setNum(Vals.size());
			efval += " "+tmp;
			for (uint p = 0; p < Vals.size(); p++)
			{
				FPoint pv = Vals.point(p);
				efval += QString(" %1 %2").arg(pv.x()).arg(pv.y());
			}
			if (CurveQ1->cDisplay->isLinear())
				efval += " 1";
			else
				efval += " 0";
			Vals = CurveQ2->cDisplay->getCurve();
			tmp.setNum(Vals.size());
			efval += " "+tmp;
			for (uint p = 0; p < Vals.size(); p++)
			{
				FPoint pv = Vals.point(p);
				efval += QString(" %1 %2").arg(pv.x()).arg(pv.y());
			}
			if (CurveQ2->cDisplay->isLinear())
				efval += " 1";
			else
				efval += " 0";
			Vals = CurveQ3->cDisplay->getCurve();
			tmp.setNum(Vals.size());
			efval += " "+tmp;
			for (uint p = 0; p < Vals.size(); p++)
			{
				FPoint pv = Vals.point(p);
				efval += QString(" %1 %2").arg(pv.x()).arg(pv.y());
			}
			if (CurveQ3->cDisplay->isLinear())
				efval += " 1";
			else
				efval += " 0";
			Vals = CurveQ4->cDisplay->getCurve();
			tmp.setNum(Vals.size());
			efval += " "+tmp;
			for (uint p = 0; p < Vals.size(); p++)
			{
				FPoint pv = Vals.point(p);
				efval += QString(" %1 %2").arg(pv.x()).arg(pv.y());
			}
			if (CurveQ4->cDisplay->isLinear())
				efval += " 1";
			else
				efval += " 0";
			effectValMap[currentOptions] = efval;
		}
		if (currentOptions->text() == tr("Curves"))
		{
			QString efval = "";
			FPointArray Vals = Kdisplay->cDisplay->getCurve();
			QString tmp;
			tmp.setNum(Vals.size());
			efval += tmp;
			for (uint p = 0; p < Vals.size(); p++)
			{
				FPoint pv = Vals.point(p);
				efval += QString(" %1 %2").arg(pv.x()).arg(pv.y());
			}
			if (Kdisplay->cDisplay->isLinear())
				efval += " 1";
			else
				efval += " 0";
			effectValMap[currentOptions] = efval;
		}
	}
}
