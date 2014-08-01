#include "QueryExecutor.h"
#include <QInputDialog>
#include <QUrl>
#include <QHashIterator>

QueryExecutor::QueryExecutor(QObject *parentObj)
	: QObject(parentObj),
	  mTreeNodeChanged(false),
	  mQSE(nullptr),
	  mMsgWin(nullptr),
	  mErrorWin(nullptr),
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
	  uniqueId(0),
	  firstQueryResult(false),
	  lastErrorFilename(""),
	  debugOutput(false)
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
		mErrorWin = nullptr;
	}
	catch (...)
	{
		// catch all exception
	}
}

void QueryExecutor::setMsgWindow(QTextEdit *te)
{
	mMsgWin = te;
}

void QueryExecutor::setErrorWindow(QTextEdit *te)
{
	mErrorWin = te;
}

void QueryExecutor::setDebugFlag(bool flag)
{
	debugOutput = flag;
}

void QueryExecutor::clearStructures()
{
	qDeleteAll(mTemplates);
	mInputs.clear();
	mReplacements.clear();
	mQueries.clear();
	mTemplates.clear();
}

//! show message string
void QueryExecutor::showMsg(QString vMsgStr, LogLevel ll)
{
	if (LogLevel::DBG != ll || debugOutput )
	{
		if (nullptr != mErrorWin && LogLevel::MSG != ll)
		{
			if (lastErrorFilename != templFileName)
			{
				mErrorWin->append(templFileName);
				lastErrorFilename = templFileName;
			}

			QString logStr("");
			switch (ll)
			{
			case LogLevel::DBG:  logStr = "DEBUG"; break;
			case LogLevel::ERR:  logStr = "ERROR"; break;
			case LogLevel::WARN: logStr = "WARN "; break;
			case LogLevel::MSG:  logStr = "INFO "; break;
			default: break;
			}

			mErrorWin->append(QString("%1: %2")
							  .arg(logStr)
							  .arg(vMsgStr));
		}
		else if (nullptr != mMsgWin)
		{
			mMsgWin->append(vMsgStr);
		}
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

//! Convert a number given as string into a 32Bit unsigned integer. The
//! representation follows the c++ convention with some addtitional enhancements:
//! * number can be grouped by underlines
//! * interprets 0b as binary number (0 and 1 allowed only)
//! * prefix $ and suffix h or H identify a hexadecimal number
quint32 QueryExecutor::convertToNumber(QString aNumStr, bool &aOk) const
{
	int vRes = 0x0;
	int vBase = 10;  // Basis der Zahl 2, 10 oder 16
	aOk = true;

	aNumStr.replace("_","");
	aNumStr = aNumStr.trimmed();

	if (aNumStr.startsWith("0x") || aNumStr.startsWith("0X") )
	{
		aNumStr.remove(0,2);
		vBase = 16;
	}
	else if (aNumStr.startsWith("$"))
	{
		aNumStr.remove(0,1);
		vBase = 16;
	}
	else if (aNumStr.endsWith("H") || aNumStr.endsWith("h") )
	{
		aNumStr.truncate(aNumStr.length()-1);
		vBase = 16;
	}
	else if (aNumStr.startsWith("0b") || aNumStr.startsWith("0B") )
	{
		aNumStr.remove(0,2);
		vBase = 2;
	}

	if (16 == vBase)
	{
		vRes = static_cast<quint32>(aNumStr.toUInt(&aOk, 16));
	}
	else if (2 == vBase)
	{
		QRegExp binValue("[01]+");

		if (binValue.exactMatch(aNumStr))
		{
			int l = aNumStr.length() - 1;
			for (int i = l; l >= 0; --i)
			{
				vRes <<= 1;
				if ('1' == aNumStr[i])
				{
					vRes |= 0x1;
				}
			}
		}
		else
		{
			aOk = false;
		}
	}
	else
	{
		vRes = static_cast<quint32>(aNumStr.toUInt(&aOk, 10));
	}

	if (!aOk)
	{
		vRes = 0;
	}

	return vRes;
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
		showMsg(tr("can't open sql file '%1'").arg(templFileName), LogLevel::ERR);
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
				if (!name.isEmpty())
				{
					showMsg(QString("Adding SQL Query '%1'").arg(name), LogLevel::DBG);
					mQueries[name] = sqlLine;
				}
				sqlLine = "";
				name = line.mid(2);
			}
			else
			{
				if ( "" == name)
				{
					showMsg(QString("no name for SQL at line %2")
							.arg(lineNr), LogLevel::ERR);
				}
				else
				{
					sqlLine += " " + line;  // add a space to prevent concatening input words
				}
			}
		}
	}
	if (!name.isEmpty())
	{
		showMsg(QString("Adding SQL Query '%1'").arg(name), LogLevel::DBG);
		mQueries[name] = sqlLine;
	}

	// open and read the template file
	QStringList *tempList = NULL;
	QFile fileTemplate(templFileName);
	if (!fileTemplate.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		showMsg(tr("can't open template file '%1'").arg(templFileName), LogLevel::ERR);
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
				if (!name.isEmpty())
				{
					showMsg(QString("Adding Template '%1'").arg(name), LogLevel::DBG);
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
	if (!name.isEmpty())
	{
		showMsg(QString("Adding Template '%1'").arg(name), LogLevel::DBG);
		mTemplates[name] = tempList;
	}

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
		showMsg(tr("Can't open file '%1'").arg(mQSE->getLastOutputFile()), LogLevel::ERR);
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
					.arg(value), LogLevel::DBG);
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
				else if ("CUMULATE" == vCmd)
				{
					bool bOk = false;
					quint32 number = vStr.toUInt(&bOk);
					if (bOk)
					{
						quint32 c = number;

						if (mCumulationMap.contains(tmpName))
						{
							c += mCumulationMap[tmpName];
						}
						mCumulationMap[tmpName] = c;
						result += QString("%1").arg(c);
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
		else if (tmpName == "__LSEP")
		{
			if (!firstQueryResult)
			{
				result += tmpList.size() > 1 ? tmpList.at(1) : ",";
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
		else if (tmpName.startsWith("__LINECNT"))
		{
			quint32 tmpNumber = aLineCnt;
			if (tmpList.size() > 1)
			{
				bool bOk;
				quint32 addNum = convertToNumber(tmpList.at(1), bOk);
				if (bOk) tmpNumber += addNum;
			}
			result += QString("%1").arg(tmpNumber,0,tmpName=="__LINECNTH" ? 16 : 10);
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
		else if (tmpName == "__CLEAR")
		{
			if (tmpList.size() > 1)
			{
				mCumulationMap.remove(tmpList.at(1));
			}
		}
		else if (tmpName == "__LF")
		{
			// do nothing use the line feed from the line :-)
		}
		else
		{
			result += "?? unknown variable name: '" + tmpName + "' ??";
			showMsg(QString("unknown variable name '%1'").arg(tmpName), LogLevel::ERR);
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

//! This method expands all lines of the given template list.
//! \return true if the calling method has to add a linefeed
bool QueryExecutor::replaceTemplate(const QStringList *aTemplLines, int aLineCnt)
{
	QRegExp rx("\\#\\{([^\\}]*)\\}"); // all expressions like #{...}
	QString tmpName, result;
	bool vRes = false;
	int vLineNum=0, pos=0, lpos=0;
	QString vStr("");

	vLineNum = aTemplLines->size();
	mTreeNodeChanged = false;
	for (int i = 0; i < vLineNum; ++i)
	{
		bool lastLine = ((i+1) == vLineNum);
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
		if (result.endsWith("\\"))
		{
			// remove the last backslash sign
			result = result.mid(0,result.length()-1);
			streamOut << result;
		}
		else
		{
			// we need to add a linefeed, but not for the last
			// line, that is added in outputTemplate
			if (!lastLine)
			{
				streamOut << result << "\n";
			}
			else
			{
				streamOut << result;
				vRes = true;
			}
		}
	}

	return vRes;
}

//! This is the heart of the executor. This method controls the SQL
//! query execution, the output generating and is called recursiv
//! to execute inner templates.
//! All SQL results stored as QString in the hash mReplacements. Same
//! column names hides the outer names and will be restored when leaving the
//! method.
bool QueryExecutor::outputTemplate(QString aTemplate)
{
	QSqlQuery query;
	const QStringList *templLines;
	QString listSeperator("");
	int lineCnt = 0;
	bool bRet = true;
	bool lastReplaceLinefeed = false;
	QHash<QString, QString> overwrittenReplacements;

	// first check the calling template string for more informations
	if (aTemplate.contains("list"))
	{
		QStringList ll = aTemplate.split(' ');
		aTemplate = ll.at(0);

		if (ll.size() > 2)
		{
			ll.removeFirst(); // the template name
			ll.removeFirst(); // the list modifier
			listSeperator = ll.join(' ');
		}
		else
		{
			listSeperator = ",";
		}
	}

	// exists a template with this name
	if (mTemplates.contains(aTemplate))
	{
		templLines = mTemplates[aTemplate];

		// exists a query with the template name ?
		// or up to the time we can handle saved result set we can reuse a
		// SQL query by writing his name and add a different output template
		// using a dot (::ARTICLE.NAMES)

		QString queryTemplate = aTemplate;
		if (aTemplate.contains('.'))
		{
			queryTemplate = aTemplate.split('.').at(0);
			if (aTemplate.endsWith("_EMPTY"))
			{
				queryTemplate += "_EMPTY";
			}
		}

		showMsg(tr("output template %1 using query %2").arg(aTemplate).arg(queryTemplate), LogLevel::DBG);

		if (mQueries.contains(queryTemplate))
		{
			QString sqlQuery = replaceLine(mQueries[queryTemplate], lineCnt);
			bRet = query.exec(sqlQuery);

			if (bRet)
			{
				QSqlRecord rec = query.record();
				int numCols = rec.count();
				bool empty = true;

				if (debugOutput)
				{
					showMsg("SQL-Query: "+sqlQuery, LogLevel::DBG);
					for (int i=0; i<numCols; ++i)
					{
						showMsg(QString("column %1 name '%2'").arg(i).arg(rec.fieldName(i)), LogLevel::DBG );
					}
					showMsg(tr("Size of result is %1").arg(query.size()), LogLevel::DBG );
				}

				firstQueryResult = true;
				while (query.next())
				{
					QCoreApplication::processEvents();
					// add a optional list seperator
					if (!firstQueryResult && !listSeperator.isEmpty())
					{
						streamOut << listSeperator;
					}
					// add the deferred linefeed from the last replaceTemplate call
					if (lastReplaceLinefeed)
					{
						streamOut << "\n";
					}

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
						QString tmpFieldName = rec.fieldName(i);

						if (mReplacements.contains(tmpFieldName))
						{
							overwrittenReplacements[tmpFieldName] = mReplacements[tmpFieldName];
						}
						mReplacements[tmpFieldName] = tmpStr;
						showMsg(tr("column %1 is /%2/").arg(i).arg(tmpStr), LogLevel::DBG);
					}
					if (!empty)
					{
						lastReplaceLinefeed = replaceTemplate(templLines, lineCnt);
						uniqueId++;
						lineCnt++;
						showMsg(QString("%1 %2").arg(aTemplate).arg(uniqueId), LogLevel::DBG);
					}
					firstQueryResult = false;
				}

				if (empty)
				{
					outputTemplate(aTemplate+"_EMPTY");
				}
				else
				{
					// add the deferred linefeed from the last replaceTemplate call
					if (lastReplaceLinefeed)
					{
						streamOut << "\n";
					}
				}
			}
			else
			{
				QString errText = query.lastError().text();
				streamOut << "## error executing " << sqlQuery << " ## " << errText << "##";
				showMsg(tr("executing SQL '%1' (%2)").arg(sqlQuery).arg(errText), LogLevel::ERR);
			}
		}
		else
		{
			// a standalone template without new data, at this position we
			// can ignore the return value, because a not data driven template
			// can't create a list.
			(void) replaceTemplate(templLines, lineCnt);
		}
	}
	else
	{
		if (!aTemplate.endsWith("_EMPTY"))
		{
			showMsg(tr("template %1 isn't defined").arg(aTemplate), LogLevel::ERR);
		}
	}

	//! last action is to restore the overwritten replacements
	QHashIterator<QString,QString> it(overwrittenReplacements);
	while (it.hasNext())
	{
		it.next();
		mReplacements[it.key()] = it.value();
	}

	return bRet;
}


bool QueryExecutor::createOutput(QuerySetEntry *aQSE,
								 DbConnection *dbc,
								 const QString &basePath,
								 const QString &inputDefines)
{
	bool b = true;
	mQSE = aQSE;

	clearStructures();								// Bereinigen der internen Strukturen
	setInputValues(inputDefines);                   // create mInput Einträge
	createOutputFileName(basePath);                 // erzeuge mOutFileName
	createInputFileNames(basePath);					// ergänzen der Input-Dateien um den Pfad
	if (nullptr != dbc)
	{
		b = dbc->connectDatabase();                 // Verbindung zur Datenbank herstellen
	}
	b = b && executeInputFiles();                   // Einlesen der
	b = b && outputTemplate("MAIN");				// Abarbeitung mit MAIN starten
	streamOut.flush();								// Ausgabe-Datei schreiben
	fileOut.close();								// Ausgabe-Datei schließen

	showMsg(QString("%1 result line processed.").arg(uniqueId));

	// Verbindung in jedem Fall wieder schließen
	if (nullptr != dbc)
	{
		dbc->closeDatabase();				        // Beenden der Datenbank-Verbindung.
	}

	return b;
}
