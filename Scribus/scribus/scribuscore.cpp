/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
/***************************************************************************
                          pageitem.h  -  description
                             -------------------
    copyright            : Scribus Team
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "scribuscore.h"

#include <cassert>
#include <iostream>
#include <QByteArray>
#include <QDebug>
#include <QGlobalStatic>
#include <QMessageBox>

#include "colormgmt/sccolormgmtenginefactory.h"
#include "commonstrings.h"
#include "filewatcher.h"
#include "localemgr.h"
#include "pluginmanager.h"
#include "prefsmanager.h"
#include "scimagecachemanager.h"
#include "scpaths.h"
#include "scribus.h"
#include "scribusapp.h"
#include "ui/splash.h"
#include "undomanager.h"
#include "util_debug.h"
#include "util_ghostscript.h"
#include "util_icon.h"


extern ScribusQApp* ScQApp;

ScribusCore::ScribusCore() : QObject(), defaultEngine(colorMgmtEngineFactory.createDefaultEngine())
{
	m_ScribusInitialized=false;
	m_SplashScreen=0;
	m_UseGUI=false;
	m_HaveCMS=false;
	m_currScMW=0;

	ScColorMgmtStrategy strategy;
	strategy.useBlackPointCompensation = true;
	strategy.useBlackPreservation      = false;
	defaultEngine.setStrategy(strategy);
}



ScribusCore::~ScribusCore()
{
	while (ScMWList.count() > 0)
	{
		ScribusMainWindow *mainWindow = ScMWList.takeAt(0);
		delete mainWindow;
	}
}

#ifndef NDEBUG
static void abort_on_error(QtMsgType t, const char * m)
{
	std::cerr << m << "\n";
	if(t==QtFatalMsg) assert(false);
//	if(t!=QtDebugMsg) assert(false);
}
#endif


int ScribusCore::init(bool useGUI, bool swapDialogButtonOrder, const QList<QString>& filesToUse)
{
	m_UseGUI=useGUI;
	m_Files=filesToUse;
	m_SwapDialogButtonOrder=swapDialogButtonOrder;
#if !defined(NDEBUG) && !defined(_WIN32)
	qInstallMsgHandler( & abort_on_error );
#endif

	return 0;
}

const QString& ScribusCore::getGuiLanguage() const
{
	return ScQApp->currGUILanguage();
}

int ScribusCore::startGUI(bool showSplash, bool showFontInfo, bool showProfileInfo, const QString newGuiLanguage, const QString prefsUserFile)
{
	ScribusMainWindow* scribus = new ScribusMainWindow();
	Q_CHECK_PTR(scribus);
	if (!scribus)
		return(EXIT_FAILURE);
	ScMWList.append(scribus);
	m_currScMW=0;
	int retVal=initScribusCore(showSplash, showFontInfo, showProfileInfo,newGuiLanguage, prefsUserFile);
	if (retVal == 1)
		return(EXIT_FAILURE);
	
	retVal = scribus->initScMW(true);
	if (retVal == 1)
		return(EXIT_FAILURE);
	
	closeSplash();
	m_ScribusInitialized=true;
	connect(ScQApp, SIGNAL(lastWindowClosed()), ScQApp, SLOT(quit()));

	scribus->show();
	int subsRet=scribus->ShowSubs();
	if (subsRet==0)
	{
		if (!m_Files.isEmpty())
		{
			for (int i = 0; i < m_Files.size(); ++i)
				scribus->loadDoc(m_Files.at(i));
		}
		else
		{
			if (PrefsManager::instance()->appPrefs.uiPrefs.showStartupDialog)
				scribus->startUpDialog();
			else
				scribus->setFocus();
		}
	}
	else if (subsRet==QMessageBox::Help)
	{
		scribus->slotRaiseOnlineHelp();
	}

	// A hook for plugins and scripts to trigger on. Some plugins and scripts
	// require the app to be fully set up (in particular, the main window to be
	// built and shown) before running their setup.
	emit appStarted();
	
	return EXIT_SUCCESS;
}

int ScribusCore::initScribusCore(bool showSplash, bool showFontInfo, bool showProfileInfo, 
								 const QString newGuiLanguage, const QString prefsUserFile)
{
	CommonStrings::languageChange();
	int retVal=0;
//FIXME	ExternalApp = 0;
	initSplash(showSplash);
	LocaleManager::instance();
	prefsManager = PrefsManager::instance();
	prefsManager->setup();
	//CB #4428 Get fonts before prefs are set to default
	bool haveFonts=false;
//#ifdef Q_OS_MAC
//	haveFonts=ScCore->initFonts(true);
//#else
	haveFonts=ScCore->initFonts(showFontInfo);
//#endif
	if (!haveFonts)
		return 1;
	prefsManager->initDefaults();
	prefsManager->initDefaultGUIFont(qApp->font());
	prefsManager->initArrowStyles();
	undoManager = UndoManager::instance();
	fileWatcher = new FileWatcher(this);
	pluginManager = new PluginManager();
//	setSplashStatus( tr("Initializing Plugins") );
//	pluginManager->initPlugs();
/* #4428, remove block later if ok
	bool haveFonts=false;
#ifdef Q_OS_MAC
	haveFonts=ScCore->initFonts(true);
#else
	haveFonts=ScCore->initFonts(showFontInfo);
#endif
	if (!haveFonts)
		return 1;
*/
	setSplashStatus( tr("Initializing Keyboard Shortcuts") );
	prefsManager->initDefaultActionKeys();
	setSplashStatus( tr("Reading Preferences") );
	if (prefsUserFile.isNull())
		prefsManager->ReadPrefs();
	else
		prefsManager->ReadPrefs(prefsUserFile);
	prefsManager->appPrefs.uiPrefs.showSplashOnStartup=showSplash;
//	setSplashStatus( tr("Applying User Shortcuts") );
//	prefsManager->applyLoadedShortCuts();
		
	m_HaveGS = testGSAvailability();
	m_HavePngAlpha = testGSDeviceAvailability("pngalpha");
	m_HaveTiffSep = testGSDeviceAvailability("tiffsep");
	setSplashStatus( tr("Initializing Plugins") );
	pluginManager->initPlugs();
	
	ScCore->setSplashStatus( tr("Reading Color Profiles") );
	m_HaveCMS = false;
	getCMSProfiles(showProfileInfo);
	initCMS();

	setSplashStatus( tr("Initializing Image Cache") );
	ScImageCacheManager & icm = ScImageCacheManager::instance();
	icm.setEnabled(prefsManager->appPrefs.imageCachePrefs.cacheEnabled);
	icm.setMaxCacheSizeMiB(prefsManager->appPrefs.imageCachePrefs.maxCacheSizeMiB);
	icm.setMaxCacheEntries(prefsManager->appPrefs.imageCachePrefs.maxCacheEntries);
	icm.setCompressionLevel(prefsManager->appPrefs.imageCachePrefs.compressionLevel);
	icm.initialize();

	return retVal;
}


void ScribusCore::initSplash(bool showSplash)
{
	if (showSplash)
	{
		//QPixmap pix = loadIcon("Splash.png", true);
		QPixmap pix = loadIcon("scribus_splash.png", true);
		//ScSplashScreen::writeVersion(&pix);
		m_SplashScreen = new ScSplashScreen(pix, Qt::WindowStaysOnTopHint);
		if (m_SplashScreen != NULL)
			m_SplashScreen->show();
		if (m_SplashScreen != NULL && m_SplashScreen->isVisible())
			setSplashStatus(QObject::tr("Initializing..."));
	}
	else
		m_SplashScreen = NULL;
}

void ScribusCore::setSplashStatus(const QString& newText)
{
	if (m_SplashScreen != NULL)
        m_SplashScreen->setStatus( newText );
}

void ScribusCore::showSplash(bool shown)
{
	if (m_SplashScreen!=NULL && shown!=m_SplashScreen->isVisible())
		m_SplashScreen->setVisible(shown);
}

bool ScribusCore::splashShowing() const
{
	if (m_SplashScreen != NULL)
		return m_SplashScreen->isVisible();
	return false;
}

void ScribusCore::closeSplash()
{
	if (m_SplashScreen!=NULL)
	{
		m_SplashScreen->close();
		delete m_SplashScreen;
		m_SplashScreen = NULL;
	}
}

bool ScribusCore::usingGUI() const
{
	return m_UseGUI;
}

bool ScribusCore::isMacGUI() const
{
	// Do it statically for now
#if defined(Q_OS_MAC)
	return true;
#else
	return false;
#endif
}

bool ScribusCore::isWinGUI() const
{
	// Do it statically for now
#if defined(_WIN32)
	return true;
#else
	return false;
#endif
}

bool ScribusCore::reverseDialogButtons() const
{
	if (m_SwapDialogButtonOrder)
		return true;
	//Win32 - dont switch
	#if defined(_WIN32)
		return false;
	//Mac Aqua - switch
	#elif defined(Q_OS_MAC)
		return true;
	#else
	//Gnome - switch
	QString gnomesession= ::getenv("GNOME_DESKTOP_SESSION_ID");
	if (!gnomesession.isEmpty())
		return true;

	//KDE/KDE Aqua - dont switch
	//Best guess for now if we are running under KDE
	QString kdesession= ::getenv("KDE_FULL_SESSION");
	if (!kdesession.isEmpty())
		return false;
	#endif
	return false;
}

//Returns false when there are no fonts
bool ScribusCore::initFonts(bool showFontInfo)
{
	setSplashStatus( tr("Searching for Fonts") );
	bool haveFonts=prefsManager->GetAllFonts(showFontInfo);
	if (!haveFonts)
	{
		closeSplash();
		QString mess = tr("There are no fonts found on your system.");
		mess += "\n" + tr("Exiting now.");
		QMessageBox::critical(0, tr("Fatal Error"), mess, 1, 0, 0);
	}
	else
		setSplashStatus( tr("Font System Initialized") );
	return haveFonts;
}


void ScribusCore::getCMSProfiles(bool showInfo)
{
	QString profDir;
	QStringList profDirs;
	MonitorProfiles.clear();
	PrinterProfiles.clear();
	InputProfiles.clear();
	InputProfilesCMYK.clear();
	QString pfad = ScPaths::instance().shareDir();
	pfad += "profiles/";
	profDirs = ScPaths::getSystemProfilesDirs();
	profDirs.prepend( prefsManager->appPrefs.pathPrefs.colorProfiles );
	profDirs.prepend( pfad );
	for(int i = 0; i < profDirs.count(); i++)
	{
		profDir = profDirs[i];
		if(!profDir.isEmpty())
		{
			if(profDir.right(1) != "/")
				profDir += "/";
			getCMSProfilesDir(profDir, showInfo, true);
		}
	}
	if ((!PrinterProfiles.isEmpty()) && (!InputProfiles.isEmpty()) && (!MonitorProfiles.isEmpty()))
		m_HaveCMS = true;
	else
		m_HaveCMS = false;
}

void ScribusCore::getCMSProfilesDir(QString pfad, bool showInfo, bool recursive)
{
	QString profileName;
	QList<ScColorProfileInfo> profileInfos = defaultEngine.getAvailableProfileInfo(pfad, recursive);
	for (int i = 0; i < profileInfos.count(); ++i)
	{
		const ScColorProfileInfo& profInfo = profileInfos.at(i);
		profileName = profInfo.description;
		if (profileName.isEmpty())
		{
			if (showInfo)
				sDebug(QString("Color profile %1 is broken : no valid description").arg(profInfo.file));
			continue;
		}
		if (!profInfo.debug.isEmpty())
		{
			if (showInfo)
				sDebug(profInfo.debug);
			continue;
		}
		switch (static_cast<int>(profInfo.deviceClass))
		{
		case Class_Input:
			if (profInfo.colorSpace == ColorSpace_Rgb)
			{
				if (!InputProfiles.contains(profileName))
					InputProfiles.insert(profileName, profInfo.file);
			}
			break;
		case Class_ColorSpace:
			if (profInfo.colorSpace == ColorSpace_Rgb)
			{
				if (!InputProfiles.contains(profileName))
					InputProfiles.insert(profileName, profInfo.file);
			}
			if (profInfo.colorSpace == ColorSpace_Cmyk)
			{
				if (!InputProfilesCMYK.contains(profileName))
					InputProfilesCMYK.insert(profileName, profInfo.file);
			}
			break;
		case Class_Display:
			if (profInfo.colorSpace == ColorSpace_Rgb)
			{
				if (!MonitorProfiles.contains(profileName))
					MonitorProfiles.insert(profileName, profInfo.file);
				if (!InputProfiles.contains(profileName))
					InputProfiles.insert(profileName, profInfo.file);
			}
			if (profInfo.colorSpace == ColorSpace_Cmyk)
			{
				if (!InputProfilesCMYK.contains(profileName))
					InputProfilesCMYK.insert(profileName, profInfo.file);
			}
			break;
		case Class_Output:
			// Disable rgb printer profile detection until effective support
			// PrinterProfiles.insert(nam, pfad + d[dc], false);
			if (profInfo.colorSpace == ColorSpace_Cmyk)
			{
				if (!PDFXProfiles.contains(profileName))
					PDFXProfiles.insert(profileName, profInfo.file);
				if (!InputProfilesCMYK.contains(profileName))
					InputProfilesCMYK.insert(profileName, profInfo.file);
				if (!PrinterProfiles.contains(profileName))
					PrinterProfiles.insert(profileName, profInfo.file);
			}
			break;
		}
		if (showInfo)
			sDebug( QString("Color profile %1 loaded from %2").arg(profileName).arg(profInfo.file) );
	}
}

void ScribusCore::InitDefaultColorTransforms(void)
{
	QString defaultRGBString  = "sRGB IEC61966-2.1";
	QString defaultCMYKString = "Fogra27L CMYK Coated Press";

	// Ouvre le profile RGB par d�fault
	if (InputProfiles.contains(defaultRGBString))
		defaultRGBProfile = defaultEngine.openProfileFromFile(InputProfiles[defaultRGBString]);
	else
		defaultRGBProfile = defaultEngine.createProfile_sRGB();

	// Ouvre le profile CMYK par d�faut
	if (InputProfilesCMYK.contains(defaultCMYKString))
	{
		defaultCMYKProfile = defaultEngine.openProfileFromFile(InputProfilesCMYK[defaultCMYKString]);
	}

	// Keep all chance to have monitor profile set
	monitorProfile = defaultRGBProfile;

	if (!defaultRGBProfile || !defaultCMYKProfile)
	{
		ResetDefaultColorTransforms();
		return;
	}

	// Default rgb profile may be a memory profile, if it is the case add it to the lists of 
	// rgb profiles (input and monitor) so that it can be used later in prefs
	if (!InputProfiles.contains(defaultRGBString))
		InputProfiles.insert(defaultRGBString, defaultRGBProfile.profilePath());
	if (!MonitorProfiles.contains(defaultRGBString))
		MonitorProfiles.insert(defaultRGBString, defaultRGBProfile.profilePath());

	// Open monitor profile as defined by user preferences
	QString displayProfile = prefsManager->appPrefs.colorPrefs.DCMSset.DefaultMonitorProfile;
	if (MonitorProfiles.contains(displayProfile))
		monitorProfile = defaultEngine.openProfileFromFile( MonitorProfiles[displayProfile] );
	if (monitorProfile.isNull())
	{
		prefsManager->appPrefs.colorPrefs.DCMSset.DefaultMonitorProfile = defaultRGBString;
		monitorProfile = defaultRGBProfile;
	}

	// Now create default color transforms (used mainly when color management is "disabled")
	int dcmsFlags        = Ctf_BlackPointCompensation;
	eRenderIntent intent = Intent_Relative_Colorimetric;

	defaultRGBToScreenSolidTrans  = defaultEngine.createTransform(defaultRGBProfile, Format_RGB_16, 
	            defaultRGBProfile, Format_RGB_16, intent, dcmsFlags);
	defaultRGBToScreenImageTrans  = defaultEngine.createTransform(defaultRGBProfile, Format_RGBA_8, 
	            defaultRGBProfile, Format_RGBA_8, intent, dcmsFlags);
	defaultCMYKToScreenImageTrans = defaultEngine.createTransform(defaultCMYKProfile, Format_CMYK_8,
	            defaultRGBProfile, Format_RGBA_8, intent, dcmsFlags);
	defaultRGBToCMYKTrans = defaultEngine.createTransform(defaultRGBProfile, Format_RGB_16,
				defaultCMYKProfile, Format_CMYK_16, intent, dcmsFlags);
	defaultCMYKToRGBTrans = defaultEngine.createTransform(defaultCMYKProfile, Format_CMYK_16,
				defaultRGBProfile, Format_RGB_16, intent, dcmsFlags);
	if (!defaultRGBToScreenSolidTrans  || !defaultRGBToScreenImageTrans || 
		!defaultCMYKToScreenImageTrans || !defaultRGBToCMYKTrans || 
		!defaultCMYKToRGBTrans )
	{
		ResetDefaultColorTransforms();
	}
}

void ScribusCore::ResetDefaultColorTransforms(void)
{
	defaultRGBProfile  = ScColorProfile();
	defaultCMYKProfile = ScColorProfile();
	defaultRGBToScreenSolidTrans = ScColorTransform();
	defaultRGBToScreenImageTrans = ScColorTransform();
	defaultCMYKToScreenImageTrans = ScColorTransform();
	defaultRGBToCMYKTrans = ScColorTransform();
	defaultCMYKToRGBTrans = ScColorTransform();
}

void ScribusCore::initCMS()
{
	if (m_HaveCMS)
	{
		ProfilesL::Iterator ip;
		if ((prefsManager->appPrefs.colorPrefs.DCMSset.DefaultImageRGBProfile.isEmpty()) || (!InputProfiles.contains(prefsManager->appPrefs.colorPrefs.DCMSset.DefaultImageRGBProfile)))
		{
			ip = InputProfiles.find("sRGB IEC61966-2.1");
			if (ip == InputProfiles.end())
				ip = InputProfiles.begin();
			prefsManager->appPrefs.colorPrefs.DCMSset.DefaultImageRGBProfile = ip.key();
		}
		if ((prefsManager->appPrefs.colorPrefs.DCMSset.DefaultImageCMYKProfile.isEmpty()) || (!InputProfilesCMYK.contains(prefsManager->appPrefs.colorPrefs.DCMSset.DefaultImageCMYKProfile)))
		{
			ip = InputProfilesCMYK.find("Fogra27L CMYK Coated Press");
			if (ip == InputProfilesCMYK.end())
				ip = InputProfilesCMYK.begin();
			prefsManager->appPrefs.colorPrefs.DCMSset.DefaultImageCMYKProfile = ip.key();
		}
		if ((prefsManager->appPrefs.colorPrefs.DCMSset.DefaultSolidColorRGBProfile.isEmpty()) || (!InputProfiles.contains(prefsManager->appPrefs.colorPrefs.DCMSset.DefaultSolidColorRGBProfile)))
		{
			ip = InputProfiles.find("sRGB IEC61966-2.1");
			if (ip == InputProfiles.end())
				ip = InputProfiles.begin();
			prefsManager->appPrefs.colorPrefs.DCMSset.DefaultSolidColorRGBProfile = ip.key();
		}
		if ((prefsManager->appPrefs.colorPrefs.DCMSset.DefaultSolidColorCMYKProfile.isEmpty()) || (!InputProfilesCMYK.contains(prefsManager->appPrefs.colorPrefs.DCMSset.DefaultSolidColorCMYKProfile)))
		{
			ip = InputProfilesCMYK.find("Fogra27L CMYK Coated Press");
			if (ip == InputProfilesCMYK.end())
				ip = InputProfilesCMYK.begin();
			prefsManager->appPrefs.colorPrefs.DCMSset.DefaultSolidColorCMYKProfile = ip.key();
		}
		if ((prefsManager->appPrefs.colorPrefs.DCMSset.DefaultMonitorProfile.isEmpty()) || (!MonitorProfiles.contains(prefsManager->appPrefs.colorPrefs.DCMSset.DefaultMonitorProfile)))
		{
			ip = MonitorProfiles.find("sRGB IEC61966-2.1");
			if (ip == MonitorProfiles.end())
				ip = MonitorProfiles.begin();
			prefsManager->appPrefs.colorPrefs.DCMSset.DefaultMonitorProfile = ip.key();
		}
		if ((prefsManager->appPrefs.colorPrefs.DCMSset.DefaultPrinterProfile.isEmpty()) || (!PrinterProfiles.contains(prefsManager->appPrefs.colorPrefs.DCMSset.DefaultPrinterProfile)))
		{
			ip = PrinterProfiles.find("Fogra27L CMYK Coated Press");
			if (ip == PrinterProfiles.end())
				ip = PrinterProfiles.begin();
			prefsManager->appPrefs.colorPrefs.DCMSset.DefaultPrinterProfile = ip.key();
		}
		InitDefaultColorTransforms();
	}
}

ScribusMainWindow * ScribusCore::primaryMainWindow( )
{
	if (ScMWList.count() == 0 || m_currScMW > ScMWList.count())
		return 0;
	ScribusMainWindow* mw=ScMWList.at(m_currScMW);
	if (!mw)
		return 0;
	return mw;
}

void ScribusCore::recheckGS()
{
	m_HaveGS = testGSAvailability();
	m_HavePngAlpha = testGSDeviceAvailability("pngalpha");
	m_HaveTiffSep = testGSDeviceAvailability("tiffsep");
}

bool ScribusCore::fileWatcherActive() const
{
	if (fileWatcher!=NULL)
		return fileWatcher->isActive();
	return false;
}
