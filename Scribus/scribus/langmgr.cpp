/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
/***************************************************************************
    begin                : Jan 2005
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
 
#include <iostream>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMap>
#include <QObject>
#include <QStringList> 

#include "scconfig.h"
#include "langmgr.h"
#include "scpaths.h"

LanguageManager * LanguageManager::m_instance = 0;
LanguageManager * LanguageManager::instance()
{
	if(!m_instance)
	{
		m_instance = new LanguageManager;
		Q_ASSERT(m_instance);
		m_instance->init();
	}
	return m_instance;
}

void LanguageManager::languageChange()
{
	//TODO
}

void LanguageManager::init(bool generateInstalledList)
{
	//generateUntranslatedLangList();
	generateLangList();
	if (generateInstalledList)
	{
		generateInstalledGUILangList();
		generateInstalledSpellLangList();
		generateInstalledHyphLangList();
	}
}

void LanguageManager::generateLangList()
{
	// TODO get rid of the redundant key, the english name.
	// So internally language would always be manipulated as a code and otherwise presented translated.
/*
	langList.insert("af",       langPair("Afrikaans",           QObject::tr( "Afrikaans" )) );
	langList.insert("af_ZA",    langPair("Afrikaans",           QObject::tr( "Afrikaans" )) );
	langList.insert("an_ES",    langPair("Aragonese",           QObject::tr( "Aragonese" )) );
	langList.insert("ar",       langPair("Arabic",              QObject::tr( "Arabic" )) );
	langList.insert("be_BY",    langPair("Belarusian",          QObject::tr( "Belarusian" )) );
	langList.insert("bg",       langPair("Bulgarian",           QObject::tr( "Bulgarian" )) );
	langList.insert("bg_BG",    langPair("Bulgarian",           QObject::tr( "Bulgarian" )) );
	langList.insert("bn",       langPair("Bengali",             QObject::tr( "Bengali" )) );
	langList.insert("br",       langPair("Breton",              QObject::tr( "Breton" )) );
	langList.insert("ca",       langPair("Catalan",             QObject::tr( "Catalan" )) );
	langList.insert("cs",       langPair("Czech",               QObject::tr( "Czech" )) );
	langList.insert("cs_CZ",    langPair("Czech",               QObject::tr( "Czech" )) );
	langList.insert("cy",       langPair("Welsh",               QObject::tr( "Welsh" )) );
	langList.insert("da",       langPair("Danish",              QObject::tr( "Danish" )) );
	langList.insert("da_DK",    langPair("Danish",              QObject::tr( "Danish" )) );
	langList.insert("de",       langPair("German",              QObject::tr( "German" )) );
	langList.insert("de_1901",  langPair("German (Trad.)",      QObject::tr( "German (Trad.)" )) );
	langList.insert("de_CH",    langPair("German (Swiss)",      QObject::tr( "German (Swiss)" )) );
	langList.insert("de_DE",    langPair("German",              QObject::tr( "German" )) );
	langList.insert("dz",       langPair("Dzongkha",            QObject::tr( "Dzongkha" )) );
	langList.insert("el",       langPair("Greek",               QObject::tr( "Greek" )) );
	langList.insert("en",       langPair("English",             QObject::tr( "English" )) );
	langList.insert("en_AU",    langPair("English (Australia)", QObject::tr( "English (Australia)" )) );
	langList.insert("en_CA",    langPair("English (Canada)",    QObject::tr( "English (Canada)" )) );
	langList.insert("en_GB",    langPair("English (UK)",        QObject::tr( "English (UK)" )) );
	langList.insert("en_NZ",    langPair("English (New Zealand)", QObject::tr( "English (New Zealand)" )) );
	langList.insert("en_US",    langPair("English (USA)",       QObject::tr( "English (USA)" )) );
	langList.insert("en_ZA",    langPair("English (South Africa)", QObject::tr( "English (South Africa)" )) );
	langList.insert("eo",       langPair("Esperanto",           QObject::tr( "Esperanto" )) );
	langList.insert("es",       langPair("Spanish",             QObject::tr( "Spanish" )) );
	langList.insert("es_AR",    langPair("Spanish (Argentina)", QObject::tr( "Spanish (Argentina)" )) );
	langList.insert("es_ES",    langPair("Spanish",             QObject::tr( "Spanish" )) );
	langList.insert("es_LA",    langPair("Spanish (Latin)",     QObject::tr( "Spanish (Latin)" )) );
	langList.insert("et",       langPair("Estonian",            QObject::tr( "Estonian" )) );
	langList.insert("eu",       langPair("Basque",              QObject::tr( "Basque" )) );
	langList.insert("fi",       langPair("Finnish",             QObject::tr( "Finnish" )) );
	langList.insert("fr",       langPair("French",              QObject::tr( "French" )) );
	langList.insert("fr_FR",    langPair("French",              QObject::tr( "French" )) );
	langList.insert("gl",       langPair("Galician",            QObject::tr( "Galician" )) );
	langList.insert("he",       langPair("Hebrew",              QObject::tr( "Hebrew" )) );
	langList.insert("hr",       langPair("Croatian",            QObject::tr( "Croatian" )) );
	langList.insert("hu",       langPair("Hungarian",           QObject::tr( "Hungarian" )) );
	langList.insert("ia",       langPair("Latin",               QObject::tr( "Latin" )) );
	langList.insert("id",       langPair("Indonesian",          QObject::tr( "Indonesian" )) );
	langList.insert("is",       langPair("Icelandic",           QObject::tr( "Icelandic" )) );
	langList.insert("it",       langPair("Italian",             QObject::tr( "Italian" )) );
	langList.insert("it_IT",    langPair("Italian",             QObject::tr( "Italian" )) );
	langList.insert("ja",       langPair("Japanese",            QObject::tr( "Japanese" )) );
	langList.insert("km",       langPair("Khmer",               QObject::tr( "Khmer" )) );
	langList.insert("ko",       langPair("Korean",              QObject::tr( "Korean" )) );
	langList.insert("ku",       langPair("Kurdish",             QObject::tr( "Kurdish" )) );
	langList.insert("la",       langPair("Latin",               QObject::tr( "Latin" )) );
	langList.insert("lb",       langPair("Luxembourgish",       QObject::tr( "Luxembourgish" )) );
	langList.insert("lo",       langPair("Lao",                 QObject::tr( "Lao" )) );
	langList.insert("lt",       langPair("Lithuanian",          QObject::tr( "Lithuanian" )) );
	langList.insert("lt_LT",    langPair("Lithuanian",          QObject::tr( "Lithuanian" )) );
	langList.insert("nb",       langPair("Norwegian (Bokmål)",  QObject::trUtf8( "Norwegian (Bokm\303\245l)" )) );
	langList.insert("nb_NO",    langPair("Norwegian (Bokmål)",  QObject::trUtf8( "Norwegian (Bokm\303\245l)" )) );
	langList.insert("nl",       langPair("Dutch",               QObject::tr( "Dutch" )) );
	langList.insert("nn",       langPair("Norwegian (Nnyorsk)", QObject::tr( "Norwegian (Nnyorsk)" )) );
	langList.insert("nn_NO",    langPair("Norwegian (Nnyorsk)", QObject::tr( "Norwegian (Nnyorsk)" )) );
	langList.insert("no",       langPair("Norwegian",           QObject::tr( "Norwegian" )) );
	langList.insert("no_NO",    langPair("Norwegian",           QObject::tr( "Norwegian" )) );
	langList.insert("pl",       langPair("Polish",              QObject::tr( "Polish" )) );
	langList.insert("pl_PL",    langPair("Polish",              QObject::tr( "Polish" )) );
	langList.insert("pt",       langPair("Portuguese",          QObject::tr( "Portuguese" )) );
	langList.insert("pt_BR",    langPair("Portuguese (BR)",     QObject::tr( "Portuguese (BR)" )) );
	langList.insert("ro",       langPair("Romanian",            QObject::tr( "Romanian" )) );
	langList.insert("ru",       langPair("Russian",             QObject::tr( "Russian" )) );
	langList.insert("sa",       langPair("Sanskrit",            QObject::tr( "Sanskrit" )) );
	langList.insert("sk",       langPair("Slovak",              QObject::tr( "Slovak" )) );
	langList.insert("sk_SK",    langPair("Slovak",              QObject::tr( "Slovak" )) );
	langList.insert("sl",       langPair("Slovenian",           QObject::tr( "Slovenian" )) );
	langList.insert("sq",       langPair("Albanian",            QObject::tr( "Albanian" )) );
	langList.insert("sr",       langPair("Serbian",             QObject::tr( "Serbian" )) );
	langList.insert("sv",       langPair("Swedish",             QObject::tr( "Swedish" )) );
	langList.insert("th",       langPair("Thai",                QObject::tr( "Thai" )) );
	langList.insert("th_TH",    langPair("Thai",                QObject::tr( "Thai" )) );
	langList.insert("tr",       langPair("Turkish",             QObject::tr( "Turkish" )) );
	langList.insert("tr_TR",    langPair("Turkish",             QObject::tr( "Turkish" )) );
	langList.insert("uk",       langPair("Ukranian",            QObject::tr( "Ukranian" )) );
	langList.insert("uk_UA",    langPair("Ukranian",            QObject::tr( "Ukranian" )) );
	langList.insert("vi",       langPair("Vietnamese",          QObject::tr( "Vietnamese" )) );
	langList.insert("zh",       langPair("Chinese",             QObject::tr( "Chinese" )) );
	langList.insert("zh_TW",    langPair("Chinese (Trad.)",     QObject::tr( "Chinese (Trad.)" )) );
*/
	//Build table;
	langTable.append(LangDef("af",     "af_ZA", "Afrikaans",              QObject::tr( "Afrikaans" )));
	langTable.append(LangDef("an_ES",  "",      "Aragonese",              QObject::tr( "Aragonese" )) );
	langTable.append(LangDef("ar",     "",      "Arabic",                 QObject::tr( "Arabic" )) );
	langTable.append(LangDef("be_BY",  "",      "Belarusian",             QObject::tr( "Belarusian" )) );
	langTable.append(LangDef("bg",     "bg_BG", "Bulgarian",              QObject::tr( "Bulgarian" )) );
	langTable.append(LangDef("bn",     "",      "Bengali",                QObject::tr( "Bengali" )) );
	langTable.append(LangDef("br",     "",      "Breton",                 QObject::tr( "Breton" )) );
	langTable.append(LangDef("ca",     "",      "Catalan",                QObject::tr( "Catalan" )) );
	langTable.append(LangDef("cs",     "cs_CZ", "Czech",                  QObject::tr( "Czech" )) );
	langTable.append(LangDef("cy",     "",      "Welsh",                  QObject::tr( "Welsh" )) );
	langTable.append(LangDef("da",     "da_DK", "Danish",                 QObject::tr( "Danish" )) );
	langTable.append(LangDef("de",     "de_DE", "German",                 QObject::tr( "German" )) );
	langTable.append(LangDef("de_1901","",      "German (Trad.)",         QObject::tr( "German (Trad.)" )) );
	langTable.append(LangDef("de_CH",  "",      "German (Swiss)",         QObject::tr( "German (Swiss)" )) );
	langTable.append(LangDef("dz",     "",      "Dzongkha",               QObject::tr( "Dzongkha" )) );
	langTable.append(LangDef("el",     "",      "Greek",                  QObject::tr( "Greek" )) );
	langTable.append(LangDef("en_GB",  "en",    "English (UK)",           QObject::tr( "English (UK)" )) );
	langTable.append(LangDef("en_AU",  "",      "English (Australia)",    QObject::tr( "English (Australia)" )) );
	langTable.append(LangDef("en_CA",  "",      "English (Canada)",       QObject::tr( "English (Canada)" )) );
	langTable.append(LangDef("en_NZ",  "",      "English (New Zealand)",  QObject::tr( "English (New Zealand)" )) );
	langTable.append(LangDef("en_US",  "",      "English (US)",           QObject::tr( "English (US)" )) );
	langTable.append(LangDef("en_ZA",  "",      "English (South Africa)", QObject::tr( "English (South Africa)" )) );
	langTable.append(LangDef("eo",     "",      "Esperanto",              QObject::tr( "Esperanto" )) );
	langTable.append(LangDef("es",     "es_ES", "Spanish",                QObject::tr( "Spanish" )) );
	langTable.append(LangDef("es_AR",  "",      "Spanish (Argentina)",    QObject::tr( "Spanish (Argentina)" )) );
	langTable.append(LangDef("es_LA",  "",      "Spanish (Latin)",        QObject::tr( "Spanish (Latin)" )) );
	langTable.append(LangDef("et",     "",      "Estonian",               QObject::tr( "Estonian" )) );
	langTable.append(LangDef("eu",     "",      "Basque",                 QObject::tr( "Basque" )) );
	langTable.append(LangDef("fi",     "",      "Finnish",                QObject::tr( "Finnish" )) );
	langTable.append(LangDef("fr",     "fr_FR", "French",                 QObject::tr( "French" )) );
	langTable.append(LangDef("gl",     "",      "Galician",               QObject::tr( "Galician" )) );
	langTable.append(LangDef("he",     "",      "Hebrew",                 QObject::tr( "Hebrew" )) );
	langTable.append(LangDef("hr",     "",      "Croatian",               QObject::tr( "Croatian" )) );
	langTable.append(LangDef("hu",     "",      "Hungarian",              QObject::tr( "Hungarian" )) );
	langTable.append(LangDef("ia",     "",      "Latin",                  QObject::tr( "Latin" )) );
	langTable.append(LangDef("id",     "",      "Indonesian",             QObject::tr( "Indonesian" )) );
	langTable.append(LangDef("is",     "",      "Icelandic",              QObject::tr( "Icelandic" )) );
	langTable.append(LangDef("it",     "it_IT", "Italian",                QObject::tr( "Italian" )) );
	langTable.append(LangDef("ja",     "",      "Japanese",               QObject::tr( "Japanese" )) );
	langTable.append(LangDef("km",     "",      "Khmer",                  QObject::tr( "Khmer" )) );
	langTable.append(LangDef("ko",     "",      "Korean",                 QObject::tr( "Korean" )) );
	langTable.append(LangDef("ku",     "",      "Kurdish",                QObject::tr( "Kurdish" )) );
	langTable.append(LangDef("la",     "",      "Latin",                  QObject::tr( "Latin" )) );
	langTable.append(LangDef("lb",     "",      "Luxembourgish",          QObject::tr( "Luxembourgish" )) );
	langTable.append(LangDef("lo",     "",      "Lao",                    QObject::tr( "Lao" )) );
	langTable.append(LangDef("lt",     "lt_LT", "Lithuanian",             QObject::tr( "Lithuanian" )) );
	langTable.append(LangDef("nb",     "nb_NO", "Norwegian (Bokmål)",     QObject::trUtf8( "Norwegian (Bokm\303\245l)" )) );
	langTable.append(LangDef("nl",     "",      "Dutch",                  QObject::tr( "Dutch" )) );
	langTable.append(LangDef("nn",     "nn_NO", "Norwegian (Nnyorsk)",    QObject::tr( "Norwegian (Nnyorsk)" )) );
	langTable.append(LangDef("no",     "no_NO", "Norwegian",              QObject::tr( "Norwegian" )) );
	langTable.append(LangDef("pl",     "pl_PL", "Polish",                 QObject::tr( "Polish" )) );
	langTable.append(LangDef("pt",     "",      "Portuguese",             QObject::tr( "Portuguese" )) );
	langTable.append(LangDef("pt_BR",  "",      "Portuguese (BR)",        QObject::tr( "Portuguese (BR)" )) );
	langTable.append(LangDef("ro",     "",      "Romanian",               QObject::tr( "Romanian" )) );
	langTable.append(LangDef("ru",     "",      "Russian",                QObject::tr( "Russian" )) );
	langTable.append(LangDef("sa",     "",      "Sanskrit",               QObject::tr( "Sanskrit" )) );
	langTable.append(LangDef("sk",     "sk_SK", "Slovak",                 QObject::tr( "Slovak" )) );
	langTable.append(LangDef("sl",     "",      "Slovenian",              QObject::tr( "Slovenian" )) );
	langTable.append(LangDef("sq",     "",      "Albanian",               QObject::tr( "Albanian" )) );
	langTable.append(LangDef("sr",     "",      "Serbian",                QObject::tr( "Serbian" )) );
	langTable.append(LangDef("sv",     "",      "Swedish",                QObject::tr( "Swedish" )) );
	langTable.append(LangDef("th",     "th_TH", "Thai",                   QObject::tr( "Thai" )) );
	langTable.append(LangDef("tr",     "tr_TR", "Turkish",                QObject::tr( "Turkish" )) );
	langTable.append(LangDef("uk",     "uk_UA", "Ukranian",               QObject::tr( "Ukranian" )) );
	langTable.append(LangDef("vi",     "",      "Vietnamese",             QObject::tr( "Vietnamese" )) );
	langTable.append(LangDef("zh",     "",      "Chinese",                QObject::tr( "Chinese" )) );
	langTable.append(LangDef("zh_TW",  "",      "Chinese (Trad.)",        QObject::tr( "Chinese (Trad.)" )) );
}

void LanguageManager::generateInstalledGUILangList()
{
	QString path = ScPaths::instance().translationDir();
	QString langAbbrev;
	QMap<QString, langPair>::Iterator it;
	QDir dir(path , "*.*", QDir::Name, QDir::Files | QDir::NoSymLinks);
	if (dir.exists() && (dir.count() != 0))
	{
		for (uint i = 0; i < dir.count(); ++i) 
		{
			QFileInfo file(path + dir[i]);
			if (file.suffix().toLower() == "qm")
			{
				langAbbrev = file.completeSuffix().remove(".qm");
				int j=langTableIndex(langAbbrev);
				if (j!=-1)
				{
					langTable[j].m_transAvailable=true;
					langTable[j].m_transFile=file.absoluteFilePath();
					//qDebug()<<"Found installed GUI translation file:"<<langAbbrev<<" : "<<langTable[j].m_transFile<<langTable[j].m_transName;
				}
			}
		}
	}
}

void LanguageManager::generateInstalledHyphLangList()
{
	//Build our list of hyphenation dictionaries we have in the install dir
	//Grab the language abbreviation from it, get the full language text
	//Insert the name as key and a new string list into the map
	QString hyphDirName = QDir::toNativeSeparators(ScPaths::instance().dictDir());
	QDir hyphDir(hyphDirName, "hyph*.dic", QDir::Name, QDir::Files | QDir::NoSymLinks);
	if (!hyphDir.exists() || hyphDir.count() == 0)
	{
		qDebug()<<"No preinstalled hyphenation dictonaries or paths found";
		return;
	}

	//QString languageOfHyphFile;
//	qDebug()<<"Installed Hyphenation Dictonaries:";
	for (uint i = 0; i < hyphDir.count(); ++i)
	{
		QFileInfo file(hyphDir[i]);
		QString langAbbrev=file.baseName().section('_', 1);
		int j=langTableIndex(langAbbrev);
		if (j!=-1)
		{
			langTable[j].m_hyphAvailable=true;
			langTable[j].m_hyphFile=hyphDirName+hyphDir[i];
			//qDebug()<<"Found installed hyphenation dictionary:"<<langAbbrev<<" : "<<hyphDirName+hyphDir[i];
		}
	}
}

void LanguageManager::generateInstalledSpellLangList()
{
	QStringList dictionaryPaths;
	bool dictPathFound=findSpellingDictionaries(dictionaryPaths);
	if (!dictPathFound)
	{
		qDebug()<<"No preinstalled spelling dictonaries or paths found";
		return;
	}
	QMap<QString, QString> dictionaryMap;
	findSpellingDictionarySets(dictionaryPaths, dictionaryMap);
	if (dictionaryMap.count()==0)
		return;

	QMap<QString, QString>::iterator it = dictionaryMap.begin();
//	qDebug()<<"Installed Spelling Dictonaries:";
	while (it != dictionaryMap.end())
	{
		int j=langTableIndex(it.key());
		if (j!=-1)
		{
			langTable[j].m_spellAvailable=true;
			langTable[j].m_spellFile=it.value();
//			qDebug()<<"Found installed spelling dictionary:"<<it.key()<<" : "<<it.value();
		}
		++it;
	}
}



int LanguageManager::langTableIndex(const QString &abbrev)
{
//	qDebug()<<"langTableIndex: Trying to find:"<<abbrev;
	for (int i = 0; i < langTable.size(); ++i)
	{
//		qDebug()<<abbrev<<langTable[i].m_priAbbrev<<langTable[i].m_altAbbrev;
		if (langTable[i].m_priAbbrev==abbrev || langTable[i].m_altAbbrev==abbrev)
			return i;
	}
	return -1;
}

const QString LanguageManager::getLangFromAbbrev(QString langAbbrev, bool getTranslated)
{
//	qDebug()<<"Trying to find:"<<langAbbrev;
	int i=langTableIndex(langAbbrev);
//	qDebug()<<"Index of"<<langAbbrev<<":"<<i;
	if (i==-1)
	{
		if (langAbbrev.length()>5)
		{
			langAbbrev.truncate(5);
			i=langTableIndex(langAbbrev);
		}
	}
	if (i!=-1)
	{
		if (getTranslated)
			return langTable[i].m_transName;
		else
			return langTable[i].m_name;
	}
	//qDebug()<<langAbbrev<<"not found";
	return "";
}

const QString LanguageManager::getAbbrevFromLang(QString lang, bool getFromTranslated, bool useInstalled)
{
	QMap<QString, langPair>::Iterator it;
	if (lang == "English" || lang == QObject::tr( "English"))
		useInstalled = false;
	for (int i = 0; i < langTable.size(); ++i)
	{
		if (useInstalled)
		{
			if (langTable[i].m_transAvailable && (langTable[i].m_name==lang || langTable[i].m_transName==lang))
				return langTable[i].m_priAbbrev;
		}
		else
		{
			//qDebug()<<lang<<langTable[i].m_priAbbrev<<langTable[i].m_name<<langTable[i].m_transName;
			if (langTable[i].m_name==lang || langTable[i].m_transName==lang)
				return langTable[i].m_priAbbrev;
		}
	}
	return "";
}

const QString LanguageManager::getLangFromTransLang(QString transLang)
{
	for (int i = 0; i < langTable.size(); ++i)
	{
		if (langTable[i].m_transName==transLang)
			return langTable[i].m_name;
	}
	return "";
}

const QString LanguageManager::getTransLangFromLang(QString lang)
{
	// Seems something is missing here!
	QString enLang(QObject::tr( "English"));
	if ((lang == "English") || (lang == enLang))
		return enLang;
	
	for (int i = 0; i < langTable.size(); ++i)
	{
		if (langTable[i].m_name==lang)
			return langTable[i].m_transName;
	}
	return "";
}

const QString LanguageManager::getShortAbbrevFromAbbrev(QString langAbbrev)
{
	//	qDebug()<<"Trying to find:"<<langAbbrev;
		int i=langTableIndex(langAbbrev);
	//	qDebug()<<"Index of"<<langAbbrev<<":"<<i;
		if (i==-1)
		{
			if (langAbbrev.length()>5)
			{
				langAbbrev.truncate(5);
				i=langTableIndex(langAbbrev);
			}
		}
		if (i!=-1)
			return langTable[i].m_priAbbrev;
		//qDebug()<<langAbbrev<<"not found";
		return "";
}

const QString LanguageManager::getAlternativeAbbrevfromAbbrev(QString langAbbrev)
{
	int i=langTableIndex(langAbbrev);
	if (i!=-1)
		return langTable[i].m_altAbbrev;
	return "";
}

void LanguageManager::fillInstalledStringList(QStringList *stringListToFill, bool addDefaults) 
{
	if (stringListToFill)
	{
		QMap<QString, QString>::Iterator it;

		if (addDefaults) 
		{
			stringListToFill->append("");
			stringListToFill->append( QObject::tr( "English" ));
		}

		for (int i = 0; i < langTable.size(); ++i)
			stringListToFill->append(langTable[i].m_transName);
	}
}

void LanguageManager::fillInstalledGUIStringList(QStringList *stringListToFill, bool addDefaults)
{
	if (stringListToFill)
	{
		if (addDefaults)
		{
			stringListToFill->append("");
			stringListToFill->append( QObject::tr( "English" ));
		}

		for (int i = 0; i < langTable.size(); ++i)
		{
			//qDebug()<<langTable[i].m_transName<<langTable[i].m_transAvailable;
			if (langTable[i].m_transAvailable)
				stringListToFill->append(langTable[i].m_transName);
		}
	}
	stringListToFill->sort();
}

void LanguageManager::fillInstalledHyphStringList(QStringList *stringListToFill)
{
	if (stringListToFill)
	{
		for (int i = 0; i < langTable.size(); ++i)
		{
			//qDebug()<<langTable[i].m_transName<<langTable[i].m_hyphAvailable;
			if (langTable[i].m_hyphAvailable)
				stringListToFill->append(langTable[i].m_transName);
		}
	}
	stringListToFill->sort();
}

QStringList LanguageManager::languageList(bool getTranslated)
{
	QStringList sl;
	for (int i = 0; i < langTable.size(); ++i)
	{
		if (getTranslated)
			sl<<langTable[i].m_transName;
		else
			sl<<langTable[i].m_name;
	}
	sl.sort();
	return sl;
}

void LanguageManager::printInstalledList()
{
	for (int i = 0; i < langTable.size(); ++i)
		qDebug() << langTable[i].m_priAbbrev.leftJustified(6) << ": " << langTable[i].m_name;
}

QString LanguageManager::numericSequence(QString seq)
{
	QString retSeq;
	const int nsBengali=0,nsDevanagari=1,nsGujarati=2,nsGurumukhi=3,nsKannada=4,nsMalayalam=5,nsOriya=6,nsTamil=7,nsTelugu=8,nsTibetan=9,nsLepcha=10;
	switch (1)
	{
		case nsBengali:
			retSeq+="";
			break;
		case nsDevanagari:
			retSeq+="०१२३४५६७८९";
			break;
		case nsGujarati:
			retSeq+="૦૧૨૩૪૫૬૭૮૯";
			break;
		case nsGurumukhi:
			retSeq+="੦੧੨੩੪੫੬੭੮੯";
			break;
		case nsKannada:
			retSeq+="";
			break;
		case nsMalayalam:
			retSeq+="";
			break;
		case nsOriya:
			retSeq+="";
			break;
		case nsTamil:
			retSeq+="";
			break;
		case nsTelugu:
			retSeq+="";
			break;
		case nsTibetan:
			retSeq+="";
			break;
		case nsLepcha:
			retSeq+="";
			break;
		default:
			retSeq="0123456789";
			break;
	}
	return retSeq;
}

bool LanguageManager::findSpellingDictionaries(QStringList &sl)
{
	sl=ScPaths::instance().spellDirs();
	if (sl.count()==0)
		return false;
	return true;
}

void LanguageManager::findSpellingDictionarySets(QStringList &dictionaryPaths, QMap<QString, QString> &dictionaryMap)
{
	for (int i=0; i<dictionaryPaths.count(); ++i)
	{
		// Find the dic and aff files in the location
		QDir dictLocation(dictionaryPaths.at(i));
		QStringList dictFilters("*.dic");
		QStringList dictList(dictLocation.entryList(dictFilters, QDir::Files, QDir::Name));
		dictList.replaceInStrings(".dic","");

		//Ensure we have aff+dic file pairs, remove any hyphenation dictionaries from the list
		QString dictName;
		foreach(dictName, dictList)
		{
			if (!QFile::exists(dictionaryPaths.at(i)+dictName+".aff"))
				dictList.removeAll(dictName);
			else
			{
				if (!dictionaryMap.contains(dictName))
				{
					if (dictName.length()<=5)
					{
						//QString shortAbbrev(LanguageManager::getShortAbbrevFromAbbrev(dictName));
						//qDebug()<<"findSpellingDictionarySets"<<dictName<<shortAbbrev;
						dictionaryMap.insert(dictName, dictionaryPaths.at(i)+dictName);
						//dictionaryMap.insert(shortAbbrev, dictionaryPaths.at(i)+dictName);
					}
					//qDebug()<<"Spell Finder:"<<dictName<<dictionaryPaths.at(i)+dictName;
					if (dictName.length()>5)
					{
						QString shortAbbrev(LanguageManager::getShortAbbrevFromAbbrev(dictName));
						//qDebug()<<shortAbbrev;
						dictionaryMap.insert(shortAbbrev, dictionaryPaths.at(i)+dictName);
					}
				}
			}
		}
//		qDebug()<<"Number of dictionaries/AFFs found in"<<dictionaryPaths.at(i)<<":"<<dictList.count();
	}
	//Now rescan dictionary map for any extra languages we can support with the files we have
	QMap<QString, QString>::iterator it = dictionaryMap.begin();
	while (it != dictionaryMap.end())
	{
		QString lang(it.key());
		if (lang.length()==5)
		{
			QString shortAbbrev(LanguageManager::getShortAbbrevFromAbbrev(lang));
			if (!dictionaryMap.contains(shortAbbrev))
			{
				//qDebug()<<"Adding extra spelling definitions for:"<<lang<<":"<<shortAbbrev;
				dictionaryMap.insert(shortAbbrev, it.value());
			}
			//else
				//qDebug()<<"Short abbreviation:"<<shortAbbrev<<"already exists for:"<<lang;
		}
		if (lang.length()==2)
		{
			QString altAbbrev(LanguageManager::getAlternativeAbbrevfromAbbrev(lang));
			if (!dictionaryMap.contains(altAbbrev))
			{
				//qDebug()<<"Adding extra spelling definitions for:"<<lang<<":"<<altAbbrev;
				dictionaryMap.insert(altAbbrev, it.value());
			}
			//else
				//qDebug()<<"Alt. abbreviation:"<<altAbbrev<<"already exists for:"<<lang;
		}
		++it;
	}
}

LanguageManager::~LanguageManager()
{
	langTable.clear();
//	langList.clear();
//	installedLangList.clear();
//	hyphLangList.clear();
}

const QString LanguageManager::getHyphFilename(const QString & langAbbrev)
{
	int j=langTableIndex(langAbbrev);
	if (j!=-1 && langTable[j].m_hyphAvailable)
	{
//		qDebug()<<"Found requested hyphenation dictionary:"<<langAbbrev<<" : "<<langTable[j].m_hyphFile;
		return langTable[j].m_hyphFile;
	}
	return QString();
}

/*
void LanguageManager::addHyphLang(const QString & lang, const QString & filename)
{
	hyphLangList[lang] = filename;
}

const QString LanguageManager::getHyphFilename(const QString & lang, bool langIsAbbreviated)
{
	if(langIsAbbreviated)
		return hyphLangList.value(lang);
	return hyphLangList.value(getAbbrevFromLang(lang, false, false));
}

const QStringList LanguageManager::hyphLangs()
{
	return hyphLangList.keys();
}
*/



