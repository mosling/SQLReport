#include "QueryExecutor.h"
#include <QInputDialog>
#include <QUrl>

QueryExecutor::QueryExecutor(QObject *parentObj)
: QObject(parentObj),
  mTreeNodeChanged(false),
  mQSE(nullptr),
  mMsgWin(nullptr),
  mInputs(),
  mReplacements(),
  mLastReplacements(),
  mQueries(),
  mTemplates(),
  sqlFileName(""),
  templFileName(""),
  mExpressionMap(),
  fileOut(),
  streamOut(),
  uniqueId(0)
{
	mExpressionMap["GT"]	= 1;
	mExpressionMap["GE"]	= 2;
	mExpressionMap["LT"]	= 3;
	mExpressionMap["LE"]	= 4;
	mExpressionMap["EQ"]	= 5;
	mExpressionMap["NE"]	= 6;
}

QueryExecutor::~QueryExecutor()
{
	try
	{
		clearStructures();
		mExpressionMap.clear();
		mQSE = nullptr;
		mMsgWin = nullptr;
	}
	catch (...)
	{
		// catch all exception
	}
}

void QueryExecutor::clearStructures()
{
	qDeleteAll(mTemplates);
	mInputs.clear();
	mReplacements.clear();
	mQueries.clear();
	mTemplates.clear();
}

void QueryExecutor::showMsg(QString vErrStr)
{
	if (NULL != mMsgWin)
	{
		mMsgWin->append(vErrStr);
	}
}

//! Erzeugen des Namens der Ausgabedatei. Dabei wird die alte
//! Version mit explizitem Zeitstempel weiterhin unterstützt.
//! Der erzeugte Name wird in lastOutputFile gespeichert.
void QueryExecutor::createOutputFileName(const QString &basePath)
{
	QString mOutFileName = mQSE->getOutputFile();

	mOutFileName = replaceLine(mOutFileName, 0);

	if (!(mOutFileName.startsWith("/") || mOutFileName.at(1)==':') )
	{
		// find a relative path, prepend with basePath
		mOutFileName = basePath + "/" + mOutFileName;
	}

	if (mQSE->getWithTimestamp())
	{
		QFileInfo outInfo(mOutFileName);

		QString bn = outInfo.baseName();
		QString p  = outInfo.dir().absolutePath();
		QString sf = outInfo.suffix();
		
		QDateTime mNow = QDateTime::currentDateTime();
        mOutFileName = p + "/" + mNow.toString("yyyy-MM-dd")
            +"-"+bn+"-"+mNow.toString("hhmm")+"."+sf;
	}

	showMsg(tr("OUTPUT FILE NAME '%1'").arg(mOutFileName));
	mQSE->setLastOutputFile(mOutFileName);
}

//! Das QuerySet kann ausgewählt werden und alle Dateien 
//! müssen sich im selben Verzeichnis befinden.
//! Die Namen wurden schon während der Erfassung bereinigt.
void QueryExecutor::createInputFileNames(const QString &basePath)
{
	sqlFileName = basePath + "/" + mQSE->getSqlFile();
	templFileName = basePath + "/" + mQSE->getTemplateFile();
}

//! Erzeugen einer Liste von Strings, deren maximale Breite
//! width ist.
QStringList QueryExecutor::splitString(const QString &str, int width, const QString &startOfLine) const
{
	int len = str.length();
	QString sol("");
	int s = 0, e = 0, cnt = 0;
	QStringList l;

	for (int i=0; i < len; ++i)
	{
		if (str[i]==' ' || str[i]==',' || str[i]=='\t')
		{
			// ein mögliches Ende
			e = i;
		}

		cnt++;
		if (cnt == width)
		{
			// Eintrag erzeugen
			if (e == s) e = cnt; // keine Stelle gefunden, es wird einfach getrennt
			l.append(sol + str.mid(s, e-s).trimmed());
			sol = startOfLine;
			s = e;
			e = i;
			cnt = 0;
		}
	}

	if (cnt != 0)
	{
		l.append(sol + str.mid(s).trimmed());
	}

	return l;
}

//! Hier werden die folgenden Aktionen gemacht:
//! - Eingabedateien lesen und in die internen Strukturen kopieren
//! - Öffnen des Ausgabestroms
bool QueryExecutor::executeInputFiles()
{
	bool bRet = true;
	QString name, line;
	int lineNr = 0;

	// open and read the SQL-statements
	QFile fileSql(sqlFileName);
	if (!fileSql.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		return false;
	}

	QTextStream streamInSql(&fileSql);
	QString sqlLine;
	name = "";
	while ( !streamInSql.atEnd())
	{
		line = streamInSql.readLine().trimmed();
		lineNr++;
		if (line.length() != 0 && !line.startsWith("//"))  // ignore empty lines and comments
		{
			if (line.startsWith("::")) 
			{
				if (!name.isEmpty()) mQueries[name] = sqlLine;
				sqlLine = "";
				name = line.mid(2);
			}
			else
			{
				if ( "" == name)
				{
					showMsg(QString("ERROR: no name for SQL at line %2")
						.arg(lineNr));
				}
				else
				{
					sqlLine += " " + line;  // add a space to prevent concatening input words
				}
			}
		}	
	}
	if (!name.isEmpty()) mQueries[name] = sqlLine;


	// open and read the template file
	QStringList *tempList = NULL;
	QFile fileTemplate(templFileName);
	if (!fileTemplate.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		return false;
	}
	
	QTextStream streamInTemplate(&fileTemplate);
	name = "";
	quint32 emptyLineCounter = 0;
	while ( !streamInTemplate.atEnd())
	{
		line = streamInTemplate.readLine();
		if (line.length() != 0)  // ignore empty lines first
		{
			if (line.startsWith("::")) 
			{
				if (name != "")
				{
					showMsg(QString("Adding Template '%1'").arg(name));
					mTemplates[name] = tempList;
				}
				name = line.mid(2);
				tempList = new QStringList();
				// ignore the last empty lines
				emptyLineCounter = 0;
			}
			else
			{
				// adding empty lines in the middle
				for ( ; emptyLineCounter > 0; --emptyLineCounter)
				{
					tempList->append("");
				}
				tempList->append(line);
			}
		}
		else
		{
			emptyLineCounter++;
		}
	}
	// letzten Eintrag machen
	mTemplates[name] = tempList;

	// open the output file, create missing path
	fileOut.setFileName(mQSE->getLastOutputFile());
	QFileInfo fileOutInfo(fileOut);
	if (!fileOutInfo.absoluteDir().exists())
	{
		showMsg(tr("create path %1")
				.arg(fileOutInfo.absoluteDir().absolutePath()));
		fileOutInfo.absoluteDir().mkpath(fileOutInfo.absoluteDir().absolutePath());
	}

	if (!fileOut.open(mQSE->getAppendOutput() ? QIODevice::Append : QIODevice::WriteOnly))
	{
		showMsg(tr("Can't open file '%1'").arg(mQSE->getLastOutputFile()));
		return false;
	}

	streamOut.setDevice(&fileOut);

	if (mQSE->getOutputUtf8())
	{
		streamOut.setCodec("UTF-8");
	}

	return bRet;
}

//! Alle Parameter die durch ein {? Feld eingegeben werden können,
//! können auch beim Start der Eingabe mit übergeben werden. In der Form
//! parameter:=Wert|...
void QueryExecutor::setInputValues(const QString &inputDefines)
{
	QStringList pList = inputDefines.split("|", QString::SkipEmptyParts);
	foreach (QString pValue, pList)
	{
		QStringList pvList = pValue.split(QRegExp(":="));
		if (pvList.size() > 0)
		{
			QString param = pvList.at(0);
			QString value = (pvList.size() > 1) ? pvList.at(1) : "";
			bool overwrite = mInputs.contains(param);
			mInputs[param] = value;
			showMsg(tr("%1 INPUT PARAM '%2' with value '%3'")
					.arg(overwrite ? "OVERWRITE" : "ADD")
					.arg(param)
					.arg(value));
		}
	}
}

//! Diese Funktion ersetzt die variablen Vorkommen
//! innerhalb einer Zeichenkette durch die aktuellen Werte.
QString QueryExecutor::replaceLine(const QString &aLine, int aLineCnt)
{
	QRegExp rx("\\$\\{([^\\}]*)\\}"); // all expressions like ${...}
	QString tmpName;
	QString tmpDescr;
	QString result = "";
	QStringList tmpList;
	int lpos=0, pos = 0;

	while ((pos = rx.indexIn(aLine, lpos)) != -1)
	{
		tmpName = rx.cap(1);
		tmpList = tmpName.split(',');
		tmpName = tmpList.at(0);
		result += aLine.mid(lpos,pos-lpos);
		lpos = pos + rx.matchedLength();
		// now add the variable part
		if (tmpName.startsWith("?"))
		{
			tmpDescr = tmpName;
			if (tmpList.size() > 1)
			{
				tmpDescr = tmpList.at(1);
			}

			tmpName = tmpName.mid(1);
			if (mInputs.contains(tmpName))
			{
				result += mInputs[tmpName];
			}
			else
			{
				// ask the user for the value
				bool ok;
				QString text = QInputDialog::getText(NULL, tmpName, tmpDescr,
					QLineEdit::Normal, "", &ok);
				result += text;
				mInputs[tmpName] = text;
			}
		}
		else if (mReplacements.contains(tmpName))
		{
			if (tmpList.size() > 1)
			{
				QString vStr = mReplacements[tmpName];
				QString vCmd = tmpList.at(1).trimmed().toUpper();
				if ("UPPERCASE" == vCmd)
				{
					result += vStr.toUpper();
				}
				else if ("CAPITALIZE" == vCmd)
				{
					vStr[0] = vStr[0].toUpper();
					result += vStr;
				}
				else if ("IFEMPTY" == vCmd)
				{
					if ( 0 == vStr.size())
					{
						// suche den ersten Wert der nicht leer ist
						bool foundStr = false;
						for (qint32 i = 2; !foundStr && i < tmpList.size(); ++i)
						{
							QString tmpName2 = tmpList.at(i);
							if (mReplacements.contains(tmpName2))
							{
								if ( 0 != mReplacements[tmpName2].size())
								{
									result += mReplacements[tmpName2];
									foundStr = true;
								}
							}
							else
							{
								result += tmpName2;
								foundStr = true;
							}
						}
					}
					else
					{
						result += vStr;
					}
				}
				else if ("HEX" == vCmd)
				{
					// hexadecimal output
					bool bOk = false;
					int i = vStr.toInt(&bOk);
					result += QString("0x%1").arg(i,0,16);
				}
				else if ("BOOL" == vCmd)
				{
					bool bOk = false;
					int i = vStr.toInt(&bOk);
					result += i==0 ? "false" : "true";
				}
				else if ("RMLF" == vCmd)
				{
					vStr = vStr.replace(QRegExp("[\r\n]"),"");
					result += vStr.simplified();
				}
				else if ("TREEMODE" == vCmd)
				{
					// Ausgabe erzeugen wenn 1. sich ein höherer Knoten geändert hat;
					// 2. beim ersten Mal und 3. wenn sich Daten geändert haben
					if (mTreeNodeChanged || !mLastReplacements.contains(tmpName)
							|| mLastReplacements[tmpName] != vStr)
					{
						result += vStr;
						mLastReplacements[tmpName] = vStr;
						mTreeNodeChanged = true;
					}
					else if (tmpList.size() > 2)
					{
						// sonst optionalen Standardwert einsetzen
						result += tmpList.at(2);
					}
				}
				else if ("FMT" == vCmd)
				{
					// Ausgabe formatieren und alle Folgezeilen mit dem
					// angegebenen StartOfLine versehen
					bool bOk = false;
					qint32 tw = tmpList.at(2).toInt(&bOk);
					if (!bOk) tw = 78;
					QString sol("");
					if (tmpList.size() > 2) sol = tmpList.at(3);
					QStringList l = splitString(vStr, tw, sol);
					int ls = l.size();
					for (int i = 0; i < ls; ++i)
					{
						result += l.at(i);
						if (i < (ls-1)) result += "\n";
					}
				}
				else if (mExpressionMap.contains(vCmd))
				{
					// einen einfachen Vergleich gefunden, der zu einer Ausgabe
					// führt, wenn er wahr ist.
					bool bOk1 = false;
					bool bOk2 = false;
					quint32 op1 = vStr.toUInt(&bOk1);
					quint32 op2 = tmpList.at(2).toUInt(&bOk2);
					if (bOk1 && bOk2)
					{
						switch (mExpressionMap[vCmd])
						{
						case 1: bOk1 = op1 > op2; break;
						case 2: bOk1 = op1 >= op2; break;
						case 3: bOk1 = op1 < op2; break;
						case 4: bOk1 = op1 <= op2; break;
						case 5: bOk1 = op1 == op2; break;
						case 6: bOk1 = op1 != op2; break;
						default:
							bOk1 = true; break;
						}

						if (bOk1 && tmpList.size() > 2)
						{
							result += tmpList.at(3);
						}
					}
				}
				else
				{
					// es wurde ein zweiter Teil angegeben, dieser wird
					// als Format verwendet, wenn ein Wert existiert
					if (!vStr.isEmpty())
					{
						result += QString(tmpList.at(1)).arg(vStr);
					}
				}
			}
			else
			{
				result += mReplacements[tmpName];
			}
		}
		else if (tmpName == "__DATE")
		{
			QString tmpDateFormat("d MMMM yyyy");
			if (tmpList.size() > 1)
			{
				tmpDateFormat = tmpList.at(1);
			}
			result += getDate(tmpDateFormat);
		}
		else if (tmpName == "__UNIQUEID")
		{
			result += QString("%1").arg(uniqueId);
		}
		else if (tmpName == "__LINECNT")
		{
			result += QString("%1").arg(aLineCnt);
		}
		else if (tmpName == "__TAB")
		{
			int tab = 0;
			bool bOk; 
			if (tmpList.size() > 1)
			{
				tab = tmpList.at(1).toInt(&bOk);
				if (!bOk) tab = 0;
				if (result.length() < tab)
				{
					// Leerzeichen anfügen
					result += QString(tab-result.length(), ' ');
				}
			}
		}
		else
		{
			result += "?? unknown variable name: '" + tmpName + "' ??";
			showMsg(QString("unknown variable name '%1'").arg(tmpName));
		}
	}
	
	result += aLine.mid(lpos);

	return result;
}

QString QueryExecutor::getDate(const QString &aFormat) const
{
	QDateTime mNow = QDateTime::currentDateTime();
	QLocale l(mQSE->getLocale());
	QString res = l.toString( mNow, aFormat);

	return res;
}

bool QueryExecutor::replaceTemplate(const QStringList *aTemplLines, int aLineCnt)
{
	QRegExp rx("\\#\\{([^\\}]*)\\}"); // all expressions like #{...}
	QString tmpName, result;
	int vLineNum=0, pos=0, lpos=0;
	QString vStr("");

	vLineNum = aTemplLines->size();
	mTreeNodeChanged = false;
	for (int i = 0; i < vLineNum; ++i)
	{
		vStr = aTemplLines->at(i);
		lpos = 0;
		while ((pos = rx.indexIn(vStr, lpos)) != -1)
		{
			tmpName = rx.cap(1);
			result = vStr.mid(lpos,pos-lpos);
			streamOut << replaceLine(result, aLineCnt);
			lpos = pos + rx.matchedLength();
			// now add the subtemplate
			outputTemplate(tmpName);
		}

      QString tmpStr = vStr.mid(lpos);
      result = replaceLine(tmpStr, aLineCnt);
		if (!result.endsWith("\\")) 
		{
			streamOut << result << "\n";
		}
		else
		{
			result = result.mid(0,result.length()-1);
			streamOut << result;
		}
	}

	return true;
}

//! Diese Funktion erzeugt die Ausgabe für ein Template,
//! die Funktion ruft sich selbst auf, wenn Subtemplates
//! benötigt werden.
bool QueryExecutor::outputTemplate(QString aTemplate)
{
	QSqlQuery query;
	const QStringList *templLines;
	int lineCnt = 0;
	bool bRet = true;

	// exists a template with this name
	if (mTemplates.contains(aTemplate))
	{
		templLines = mTemplates[aTemplate];

		// exists a query with the template name ?
		// or up to the time we can handle saved result set we can reuse a
		// SQL query by writing his name and add a different output template
		// using a dot
		QString queryTemplate = aTemplate;

		if (aTemplate.contains('.'))
		{
			queryTemplate = aTemplate.split('.').at(0);
		}

		if (mQueries.contains(queryTemplate))
		{
			QString sqlQuery = replaceLine(mQueries[queryTemplate], lineCnt);
			bRet = query.exec(sqlQuery);

			if (bRet)
			{
				QSqlRecord rec = query.record();
				int numCols = rec.count();
				bool empty = true;

				showMsg("SQL-Query: "+sqlQuery);
				for (int i=0; i<numCols; ++i)
				{
					showMsg(QString("column %1 name '%2'").arg(i).arg(rec.fieldName(i)) );
				}

				while (query.next())
				{
					QCoreApplication::processEvents();
					//get the sql values
					for (int i=0; i<numCols; ++i)
					{
						QString tmpStr = query.value(i).toString();

						if (empty) empty = query.isNull(i);
						if (mQSE->getOutputXml())
						{
							tmpStr.replace("<", "&lt;");
							tmpStr.replace(">","&gt;");
						}
						mReplacements[rec.fieldName(i)] = tmpStr;
						showMsg(tr("column %1 is /%2/").arg(i).arg(tmpStr));
					}
					if (!empty)
					{
						replaceTemplate(templLines, lineCnt);
						uniqueId++;
						lineCnt++;
						showMsg(QString("%1 %2").arg(aTemplate).arg(uniqueId));
					}
				}

				if (empty)
				{
					outputTemplate(aTemplate+"_EMPTY");
				}
			}
			else
			{
				QString errText = query.lastError().text();
				streamOut << "## error executing " << sqlQuery << " ## " << errText << "##";
				showMsg(sqlQuery);
				showMsg(errText);
			}
		}
		else
		{
			// a standalone template without new data
			replaceTemplate(templLines, lineCnt);
		}
	}

	return bRet;
}

bool QueryExecutor::createOutput(QuerySetEntry *aQSE,
								 QTextEdit *aMsgWin,
								 const QString &basePath,
								 const QString &inputDefines)
{
	bool b;

	mQSE = aQSE;
	mMsgWin = aMsgWin;
	
	clearStructures();								// Bereinigen der internen Strukturen
	setInputValues(inputDefines);                   // create mInput Einträge
	createOutputFileName(basePath);                 // erzeuge mOutFileName
	createInputFileNames(basePath);					// ergänzen der Input-Dateien um den Pfad
	b = mQSE->connectDatabase();                    // Verbindung zur Datenbank herstellen
	b = b && executeInputFiles();                   // Einlesen der
	b = b && outputTemplate("MAIN");				// Abarbeitung mit MAIN starten
	streamOut.flush();								// Ausgabe-Datei schreiben
	fileOut.close();								// Ausgabe-Datei schließen

	showMsg(QString("%1 result line processed.").arg(uniqueId));
	mMsgWin = NULL;
	// Verbindung in jedem Fall wieder schließen
	mQSE->getDatabase().closeDatabase();				// Beenden der Datenbank-Verbindung.

	return b;
}
