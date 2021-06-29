#include "QueryExecutor.h"
#include "Utility.h"

#include <QRegularExpression>
#include <QInputDialog>
#include <QUrl>
#include <QHashIterator>
#include <QtQml/QJSValue>
#include <QJSValueIterator>
#include <QTime>
#include <QtQml/QJSValue>

QueryExecutor::~QueryExecutor()
{
	try
	{
		clearStructures();
		mQSE = nullptr;
	}
	catch (...)
	{
		// catch all exception
	}
}

void QueryExecutor::setPrepareQueriesFlag(bool flag)
{
	prepareQueries = flag;
}

void QueryExecutor::clearStructures()
{
	qDeleteAll(templatesMap);
	userInputs.clear();
	replacements.clear();
	queriesMap.clear();
	templatesMap.clear();
}

//! It is possible to ask the user for a input value, this
//! value is asked once and reused in all other cases.
void QueryExecutor::replaceLineUserInput(const QStringList &varList, QString &result, int lineCnt)
{
	Q_UNUSED(lineCnt)

	if (varList.size() > 0)
	{
		QString tmpName = varList[0];
		QString tmpDescr = varList[0];

		if (varList.size() > 1)
		{
			tmpDescr = varList.at(1);
		}

		tmpName = tmpName.mid(1);
        if (!userInputs.contains(tmpName))
		{
			// ask the user for the value
			bool ok;
			QString text = QInputDialog::getText(NULL, tmpName, tmpDescr,
												 QLineEdit::Normal, "", &ok);
			userInputs[tmpName] = text;

		}

        if (varList.size() > 2)
        {
            // the user variable contains modifications like ${?Name,Username,CAPITALIZE}
            QStringList tl;
            for (int i = 0; i < varList.size(); ++i)
            {
                if (i != 1 ) // ignore the description value
                {
                    tl << varList.at(i);
                }
            }
            replaceLineVariable(userInputs[tmpName].toUtf8(), tl, result, lineCnt);
        }
        else
        {
            result += userInputs[tmpName];
        }
	}
}

void QueryExecutor::replaceLineVariable(const QByteArray vStr, const QStringList &varList, QString &result, int lineCnt)
{
	Q_UNUSED(lineCnt)

	if (varList.size() > 0)
	{
        QString tmpName = varList.at(0);
		if (varList.size() > 1)
		{   
			QString vCmd = varList.at(1).trimmed().toUpper();

            if ("UPPERCASE" == vCmd || "UPPER" == vCmd)
			{
				result += vStr.toUpper();
			}
            else if ("LOWERCASE" == vCmd || "LOWER" == vCmd)
            {
                result += vStr.toLower();
            }
			else if ("CAPITALIZE" == vCmd)
			{
                result += vStr.mid(0,1).toUpper() + vStr.mid(1).toLower();
			}
            else if ("RTF" == vCmd)
            {
                if (vStr.startsWith("{\\rtf"))
                {
                    QString resultType = varList.size() > 2 ? varList.at(2) : "html";
                    result += convertRtf(vStr, resultType, true);
                }
                else
                {
                    logger->debugMsg(tr("No RTF String found -- use given string"));
                    result += vStr;
                }
            }
			else if ("IFEMPTY" == vCmd)
			{
				if ( 0 == vStr.size())
				{
					// suche den ersten Wert der nicht leer ist
					bool foundStr = false;
					for (qint32 i = 2; !foundStr && i < varList.size(); ++i)
					{
                        QString  tmpName2 = varList.at(i);
						if (replacements.contains(tmpName2))
						{
							if ( 0 != replacements[tmpName2].size())
							{
                                result += QString(replacements[tmpName2]);
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
                if (bOk)
                {
                    result += QString("%1").arg(i,0,16);
                }
                else
                {
                    result += vStr.toHex();
                }
			}
            else if ("BASE64" == vCmd)
            {
                result += QString(vStr.toBase64());
            }
			else if ("BOOL" == vCmd)
			{
				bool bOk = false;
				int i = vStr.toInt(&bOk);
				result += i==0 ? "false" : "true";
			}
			else if ("RMLF" == vCmd)
			{
                QByteArray ta(vStr);
                result += ta.replace("\r","").replace("\n","").simplified();
			}
			else if ("TREEMODE" == vCmd)
			{
                // Ausgabe erzeugen wenn
                // 1. sich ein höherer Knoten geändert hat;
                // 2. beim ersten Mal
                // 3. wenn sich Daten geändert haben
                if (mTreeNodeChanged || !treeReplacements.contains(tmpName)
                        || treeReplacements[tmpName] != vStr)
				{
                    result += QString(vStr);
                    treeReplacements[tmpName] = vStr;
					mTreeNodeChanged = true;
				}
				else if (varList.size() > 2)
				{
					// sonst optionalen Standardwert einsetzen
					result += varList.at(2);
				}
			}
			else if ("FMT" == vCmd)
			{
				// Ausgabe formatieren und alle Folgezeilen mit dem
				// angegebenen StartOfLine versehen
                qint32 tw = 78;
                bool bOk = varList.size() > 2 ? tw = varList.at(2).toInt(&bOk) : true;
				if (!bOk) tw = 78;                
                QString sol(varList.size() > 3 ? varList.at(3) : "");

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

					if (cumulationMap.contains(tmpName))
					{
						c += cumulationMap[tmpName];
					}
					cumulationMap[tmpName] = c;
					result += QString("%1").arg(c);
				}
			}
            else if (varList.at(1).contains("%1"))
			{
				// es wurde ein zweiter Teil angegeben, dieser wird
				// als Format verwendet, wenn ein Wert existiert
				if (!vStr.isEmpty())
				{
                    result += QString(varList.at(1)).arg(vStr);
				}
			}
            else
            {
                logger->errorMsg(tr("Not supported variable conversion '%1'.").arg(varList.at(1)));
            }
		}
		else
		{
            result += vStr;
		}
	}
}

void QueryExecutor::replaceLineGlobal(const QStringList &varList, QString &result, int lineCnt)
{
	if (varList.size() > 0)
	{
		QString tmpName = varList.at(0);
		if ("__LSEP" == tmpName)
		{
			if (!firstQueryResult)
			{
				result += varList.size() > 1 ? varList.at(1) : ",";
			}
		}
		else if ("__DATE" == tmpName)
		{
			QString tmpDateFormat("d MMMM yyyy");
			if (varList.size() > 1)
			{
				tmpDateFormat = varList.at(1);
			}
			result += getDate(tmpDateFormat);
		}
		else if ("__UNIQUEID" == tmpName)
		{
			result += QString("%1").arg(uniqueId);
		}
		else if (tmpName.startsWith("__LINECNT"))
		{
			quint32 tmpNumber = lineCnt;
			if (varList.size() > 1)
			{
				bool bOk;
				quint32 addNum = convertToNumber(varList.at(1), bOk);
				if (bOk) tmpNumber += addNum;
			}
			result += QString("%1").arg(tmpNumber,0,tmpName=="__LINECNTH" ? 16 : 10);
		}
		else if ("__TAB" == tmpName)
		{
			int tab = 0;
			bool bOk;
			if (varList.size() > 1)
			{
				tab = varList.at(1).toInt(&bOk);
				if (!bOk) tab = 0;
				if (result.length() < tab)
				{
					// Leerzeichen anfügen
					result += QString(tab-result.length(), ' ');
				}
			}
		}
		else if ("__LF" == tmpName)
		{
			result += "\n";
		}
		else if ("__CLEAR" == tmpName)
		{
			if (varList.size() > 1)
			{
				cumulationMap.remove(varList.at(1));
			}
		}
        else if ("__TREE_RESET" == tmpName)
        {
            if (varList.size() > 1)
            {
                if ( !treeReplacements.remove(varList.at(1)))
                {
                    logger->warnMsg(QString("__TREE_RESET: no tree entry named '%1'").arg(varList.at(1)));
                }
            }
            else
            {
                treeReplacements.clear();
            }
        }
	}
}

//! Erzeugen des Namens der Ausgabedatei. Dabei wird die alte
//! Version mit explizitem Zeitstempel weiterhin unterstützt.
//! Der erzeugte Name wird in lastOutputFile gespeichert.
void QueryExecutor::createOutputFileName(const QString &basePath)
{
	if (nullptr != mQSE)
	{
		QString mOutFileName = mQSE->getOutputFile();
		mOutFileName = replaceLine(mOutFileName, 0, false, false);

        if (mOutFileName.isEmpty())
        {
            mOutFileName = "output.txt";
            mQSE->setOutputFile(mOutFileName);
        }

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

        logger->infoMsg(tr("OUTPUT FILE NAME '%1'").arg(mOutFileName));
		mQSE->setLastOutputFile(mOutFileName);
	}
}

//! Das QuerySet kann ausgewählt werden und alle Dateien 
//! müssen sich im selben Verzeichnis befinden.
//! Die Namen wurden schon während der Erfassung bereinigt.
void QueryExecutor::createInputFileNames(const QString &basePath)
{
	if (nullptr != mQSE)
	{
        sqlFileName = mQSE->getSqlFile().isEmpty() ? QString("") : basePath + "/" + mQSE->getSqlFile();
        templateFileName = basePath + "/" + mQSE->getTemplateFile();
	}
	else
	{
		sqlFileName = "";
        templateFileName = "";
	}
}

//! Erzeugen einer Liste von Strings, deren maximale Breite
//! width ist.
QStringList QueryExecutor::splitString(const QString &str, int width, const QString &startOfLine) const
{
	qint32 len = str.length();
	QStringList l;
	QString sol("");

	qint32 idx   = 0;
	qint32 start = 0;
	qint32 split = 0;

	while (idx < len)
	{
		// gut Möglichkeiten zum Beginn einer neuen Zeile merken
		if (str[idx]==' ' || str[idx]==',' || str[idx]=='\t')
		{
			split = idx;
		}

		// existiert schon Zeilenumbruch, dann wird er übernommen
        if (str[idx] == QChar(0x0a) || str[idx] == QChar(0x0d))
		{
			l.append(sol + str.mid(start, idx-start).trimmed());
			sol = startOfLine;
			idx++;
			start = idx;
			split = start;
		}
		else if ((idx-start) != 0 && (idx-start) % width == 0)
		{
			// Eintrag erzeugen
			if (split == start) split = idx;
			l.append(sol + str.mid(start, split-start).trimmed());
			sol = startOfLine;
			start = split;
			split = start;
		}

		idx++;
	}

	if ((len-1-start) > 0)
	{
		l.append(sol + str.mid(start).trimmed());
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
        QRegularExpression binValue("[01]+");

        if (binValue.match(aNumStr).hasMatch())
		{
			int l = aNumStr.length() - 1;
			for (int i = l; i >= 0; --i)
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

//! A method to add the sql query to the internal map.
void QueryExecutor::addSqlQuery(const QString &name, const QString &sqlLine)
{
	if (prepareQueries)
	{
        logger->debugMsg(QString("Adding Prepared SQL Query '%1'").arg(name));

		QString tempPrepQuery = replaceLine(sqlLine, 0, true, false);
        logger->debugMsg(tr("prepared sql query: %1").arg(tempPrepQuery));
		QSqlQuery q(QSqlDatabase::database());
		bool qb = q.prepare(tempPrepQuery);
		if (!qb)
		{
            logger->errorMsg(tr("preparing sql query: %1").arg(tempPrepQuery));
		}
		else
		{
			preparedQueriesMap[name] = q;
		}
	}
	else
	{
        logger->debugMsg(QString("Adding SQL Query '%1'").arg(name));

		queriesMap[name] = sqlLine;
    }
}

QString QueryExecutor::convertRtf(QString rtfText, QString resultType, bool cleanupFont)
{
    QString unrtfCmd="unrtf";

    QProcess unrtfVersion;
    QStringList unrtfArgs;
    unrtfArgs.append("--version");
    unrtfVersion.setProgram(unrtfCmd);
    unrtfVersion.setArguments(unrtfArgs);
    unrtfVersion.start();
    bool started = false;
    if (!unrtfVersion.waitForFinished(10000))
    {
        // 10 seconds timeout
        unrtfVersion.kill();
    }
    else
    {
        started = unrtfVersion.exitCode() == 0;
    }

    if(started)
    {
       logger->infoMsg(QString("using unrtf version %1 to convert rtf -> %2")
                       .arg(QString::fromLocal8Bit(unrtfVersion.readAllStandardError()).trimmed(), resultType));

        // write to file
        QString pathRtf= QCoreApplication::applicationDirPath() + QString("/myfile.rtf");
        QFile fileRtf(pathRtf);
        if(!fileRtf.open(QIODevice::WriteOnly)){
                fileRtf.close();
            } else {
                QTextStream out(&fileRtf);
                out << rtfText.toLatin1();
                fileRtf.close();
            }

        QProcess program;
        unrtfArgs.clear();
        unrtfArgs.append(QString("--%1").arg(resultType.toLower()));
        unrtfArgs.append(pathRtf);
        program.setProgram(unrtfCmd);
        program.setArguments(unrtfArgs);
        program.start();
        int exitCode = 1;
        QString stdError;
        QString stdOutput = "";
        if (!program.waitForFinished(10000))
        {
            // 10 seconds timeout
            program.kill();
            stdError = "RTF converter not ready within 10s.";
        }
        else
        {
            exitCode = program.exitCode();
            stdOutput = QString::fromLocal8Bit(program.readAllStandardOutput());
            stdError = QString::fromLocal8Bit(program.readAllStandardError());
        }

        if (exitCode != 0)
        {
            logger->errorMsg(QString("%1").arg(stdError));
        }
        else
        {
            // extract body content from generated html
            QRegularExpressionMatch htmlMatch = htmlBody.match(stdOutput);
            if (htmlMatch.hasMatch())
            {
                if (cleanupFont)
                {
                    QString fontless = htmlMatch.captured(1).remove(fontElement);
                    return fontless.remove(spanElement);
                }
                else
                {
                    return htmlMatch.captured(1);
                }
            }
        }
    }
    else
    {
        logger->warnMsg("Please add unrtf to your PATH for better results.");
        QRegularExpression rtfTextExp("lang1031 ([^}]*)");
        QRegularExpressionMatch rtfMatch= rtfTextExp.match(rtfText);
        if (rtfMatch.hasMatch())
        {
            QString part = rtfMatch.captured(1);

            QMap<QString, QString> rtfToHtml;
            rtfToHtml.insert("\\'f6", "ö");
            rtfToHtml.insert("\\'e4", "ä");
            rtfToHtml.insert("\\'fc", "ü");
            rtfToHtml.insert("\\'c4", "Ä");
            rtfToHtml.insert("\\'d6", "Ö");
            rtfToHtml.insert("\\'dc", "Ü");
            rtfToHtml.insert("\\'df", "ß");
            if (mQSE->getOutputXml())
            {
                rtfToHtml.insert("\\par", "</p>");
            }
            else
            {
                rtfToHtml.insert("\\par", "");
            }

            QMapIterator<QString, QString> iter(rtfToHtml);
            while(iter.hasNext())
            {
                iter.next();
                part.replace(iter.key(), iter.value());
            }
            return part;
        }
    }

    return "";
}

//! This is the start method to fill the internal structures
//! to create the output.
//! This is also the place where the output stream is opened.
bool QueryExecutor::executeInputFiles()
{
	bool bRet = true;
	QString name, line;
	int lineNr = 0;

	// open and read the SQL-statements if existing
	if (!sqlFileName.isEmpty())
	{
		QFile fileSql(sqlFileName);
		if (!fileSql.open(QIODevice::ReadOnly | QIODevice::Text))
		{
            logger->errorMsg(tr("can't open sql file '%1'").arg(sqlFileName));
			bRet = false;
		}
		else
		{
			QTextStream streamInSql(&fileSql);
			QString sqlLine;
			name = "";
			while ( !streamInSql.atEnd())
			{
				line = streamInSql.readLine().trimmed();
				lineNr++;
				if (line.length() != 0 && !line.startsWith("::#"))  // ignore empty lines and comments
				{
					if (line.startsWith("::"))
					{
						if (!name.isEmpty())
						{
							addSqlQuery(name, sqlLine);
						}
						sqlLine = "";
						name = line.mid(2).trimmed();
					}
					else
					{
						if ( "" == name)
						{
                            logger->errorMsg(QString("no name for SQL at line %2")
                                    .arg(lineNr));
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
				addSqlQuery(name, sqlLine);
			}
		}
	}

	// open and read the template file
    QFile fileTemplate(templateFileName);
	if (!fileTemplate.open(QIODevice::ReadOnly | QIODevice::Text))
	{
        logger->errorMsg(tr("can't open template file '%1'").arg(templateFileName));
		bRet = false;
	}
	else
	{
        logger->infoMsg(QString("start executing template file '%1'").arg(templateFileName));
		QTextStream streamInTemplate(&fileTemplate);
		name = "";
		qint32 emptyLineCnt = 0;
		while ( !streamInTemplate.atEnd())
		{
			line = streamInTemplate.readLine();
			if (line.length() != 0)  // extra handling for empty lines using emptyLineCnt
			{
				if (!line.startsWith("::#")) //ignore comment lines
				{
					if (line.startsWith("::"))
					{
						emptyLineCnt = 0;
						name = line.mid(2).trimmed();
						if (!name.isEmpty())
						{
							if (templatesMap.contains(name))
							{
                                logger->warnMsg(tr("Overwrite Template '%1'"));
								delete (templatesMap[name]);
							}
							else
							{
                                logger->debugMsg(QString("Adding Template '%1'").arg(name));
							}
							templatesMap[name] = new QStringList();
						}
					}
					else if (!name.isEmpty())
					{
						for (qint32 i=0; i < emptyLineCnt; i++)
						{
							templatesMap[name]->append("");
						}
						emptyLineCnt = 0;
						templatesMap[name]->append(line);
					}
				}
			}
			else
			{
				emptyLineCnt++;
			}
		}
	}

	if (templatesMap.contains("Javascript"))
	{
        QJSValue result = scriptEngine.evaluate(templatesMap["Javascript"]->join('\n'));
        if (result.isError())
		{
            logger->errorMsg(tr("javascript error '%1' at line %2 ")
                    .arg(result.toString())
                    .arg(result.property("lineNumber").toInt()));
		}
        else if (logger->isDebug())
        {
            QJSValueIterator it(scriptEngine.globalObject());
             while (it.hasNext()) {
                 it.next();
                 logger->debugMsg(QString("ADD JS property %1").arg(it.name()));
              }
        }
	}

	if (nullptr != mQSE)
	{
		// open the output file, create missing path
		fileOut.setFileName(mQSE->getLastOutputFile());
		QFileInfo fileOutInfo(fileOut);
		if (!fileOutInfo.absoluteDir().exists())
		{
            logger->infoMsg(tr("create path %1")
                    .arg(fileOutInfo.absoluteDir().absolutePath()));
			fileOutInfo.absoluteDir().mkpath(fileOutInfo.absoluteDir().absolutePath());
		}

		if (!fileOut.open(mQSE->getAppendOutput() ? QIODevice::Append : QIODevice::WriteOnly))
		{
            logger->errorMsg(tr("Can't open file '%1'").arg(mQSE->getLastOutputFile()));
			bRet = false;
		}
		else
		{
			streamOut.setDevice(&fileOut);
			if (mQSE->getOutputUtf8())
			{
                streamOut.setEncoding(QStringEncoder::Encoding::Utf8);
			}
		}
	}
	else
	{
        logger->errorMsg("no active query set");
		bRet = false;
	}

	return bRet;
}

//! All parameters starting with ? (i.e. ${?username}) can be used
//! as command line parameters. The inputDefines holds a number of the
//! paramerters with the syntax.
//! parameter1:=Wert|parameter2:=Wert2|...
void QueryExecutor::setInputValues(const QString &inputDefines)
{
    QStringList pList = inputDefines.split(QLatin1Char('|'), Qt::SkipEmptyParts);
	foreach (QString pValue, pList)
	{
        QStringList pvList = pValue.split(QRegularExpression(":="));
		if (pvList.size() > 0)
		{
			QString param = pvList.at(0);
			QString value = (pvList.size() > 1) ? pvList.at(1) : "";
			bool overwrite = userInputs.contains(param);
			userInputs[param] = value;
            if (logger->isDebug())
			{
                logger->infoMsg(tr("%1 INPUT PARAM '%2' with value '%3'")
                        .arg(overwrite ? "OVERWRITE" : "      ADD", param, value));
			}
		}
	}
}

//! This methods replaces the all variables with the current value.
//! All variables has the syntax ${...} and we have normal, global and
//! user input variables.
//! Another special case is the flag sqlBinding, if this set all variables
//! replaced by :varname and can be used in prepared SQL queries.
//! If the simpleFormat is true we replace a expression line. In expressions every
//! variable starts with a dollar sign and follow the same naming conventions
//! as the normal replace name method call.
QString QueryExecutor::replaceLine(const QString &aLine, int aLineCnt, bool sqlBinding, bool simpleFormat)
{
    QRegularExpression rx;
	QString result = "";
    long long lpos=0, pos = 0;

	if (simpleFormat)
	{
		rx.setPattern("\\$([?]*[a-zA-Z_]+)"); // all expressions like $[?]... capture the [?]... part
	}
	else
	{
		rx.setPattern("\\$\\{([^\\}]*)\\}"); // all expressions like ${...} capture the ... part
	}

    QRegularExpressionMatchIterator i = rx.globalMatch(aLine);
    while (i.hasNext())
    {
      QRegularExpressionMatch match = i.next();

      QString tmpExpression = match.captured(1);
      QStringList tmpList = tmpExpression.split(',');
      QString tmpName     = tmpList.at(0);
      pos = match.capturedStart();

		// add the first part to the result
        result += aLine.mid(lpos,pos-lpos);
        lpos = pos + match.capturedLength();

		// first look for an expression evaluated by the script engine
		if ("EVAL" == tmpList.at(tmpList.size()-1).trimmed().toUpper())
		{
			if ("EVAL" != tmpList.at(tmpList.size()-1).toUpper())
			{
                logger->warnMsg(tr("sqlReport interprets <b>'%1'</b> as <b>'eval'</b>, please remove the surrounding whitspaces")
                        .arg(tmpList.at(tmpList.size()-1)));
			}
			tmpList.removeLast();
			tmpName = tmpList.join(',');
			QString expression = replaceLine(tmpName, aLineCnt, false, true);
            QJSValue expResult = scriptEngine.evaluate(expression).toString();
            if (!expResult.isError())
			{
				result += expResult.toString();
			}
			else
			{
                logger->errorMsg(tr("error '%1' at line %2 evaluate script /%3/")
                        .arg(expResult.toString())
                        .arg(expResult.property("lineNumber").toInt())
                        .arg(expression));
			}
		}
		// else check if we have a user variable
		else if (tmpName.startsWith("?"))
		{
			replaceLineUserInput(tmpList, result, aLineCnt);
		}
		// else check if the variable exists in the replacement list (columns from SQL)
		else if (replacements.contains(tmpName))
		{
            replaceLineVariable(replacements[tmpName], tmpList, result, aLineCnt);
		}
		// check if we have a global substitution
		else if (tmpName.startsWith("__"))
		{
			replaceLineGlobal(tmpList, result, aLineCnt);
		}
		else if (true == sqlBinding)
		{
			result += ":" + tmpName;
		}
		else
		{
			result += "['" + tmpName + "' is unknown]";
            logger->errorMsg(QString("unknown variable name <b>'%1'</b>").arg(tmpName));
		}
	}

	// add the last part to the result
	result += aLine.mid(lpos);

	return result;
}

QString QueryExecutor::getDate(const QString &aFormat) const
{
	QDateTime mNow = QDateTime::currentDateTime();
	QLocale l(mQSE != nullptr ? mQSE->getLocale() : "en-en");
	QString res = l.toString( mNow, aFormat);

	return res;
}

//! This method expands all lines of the given template list.
//! \return true if the calling method has to add a linefeed
bool QueryExecutor::replaceTemplate(const QStringList *aTemplLines, int aLineCnt)
{
    QRegularExpression rx("\\#\\{([^\\}]*)\\}"); // all expressions like #{...} capture the ... part
	QString tmpName, result;
	bool vRes = false;
	int vLineNum=0, pos=0, lpos=0;
	QString vStr("");

	vLineNum = aTemplLines->size();
    mTreeNodeChanged = false;
	result = "";
	for (int i = 0; i < vLineNum; ++i)
	{
		bool lastLine = ((i+1) == vLineNum);
		vStr = aTemplLines->at(i);
		lpos = 0;

        QRegularExpressionMatchIterator expit = rx.globalMatch(vStr);
        while (expit.hasNext())
        {
          QRegularExpressionMatch match = expit.next();
          pos = match.capturedStart();

            tmpName = match.captured(1);
			result = vStr.mid(lpos,pos-lpos);
			streamOut << replaceLine(result, aLineCnt, false, false);
            lpos = pos + match.capturedLength();
			// now add the subtemplate
			outputTemplate(tmpName);
		}

		QString tmpStr = vStr.mid(lpos);
		result = replaceLine(tmpStr, aLineCnt, false, false);
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
	QSqlQuery query;                // hold the sql query
	const QStringList *templLines;  // the lines of the template
	QString listSeperator("");      // used with the ,list modifier
	int lineCnt = 0;                //
	bool bRet = true;
	bool lastReplaceLinefeed = false;
    QHash<QString, QByteArray> overwrittenReplacements;
    QString lastTemplateName = currentTemplateBlockName;

	// split the given aTemplate into parts separated by comma
	QStringList ll = aTemplate.split(',');
	aTemplate = ll.at(0).trimmed();
	QString outputModifier = ll.size()>1 ? ll.at(1).trimmed().toUpper() : "";

	// first check the calling template string for more informations
	if ("LIST" == outputModifier)
	{
		if (ll.size() > 2)
		{
			ll.removeFirst(); // the template name
			ll.removeFirst(); // the output modifier
			listSeperator = ll.join(',');
		}
		else
		{
			listSeperator = ",";
		}
	}

	// exists a template with this name
	if (templatesMap.contains(aTemplate))
	{
		currentTemplateBlockName = aTemplate;
        logger->setContext(currentTemplateBlockName);
		templLines = templatesMap[aTemplate];

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

        logger->debugMsg(tr("output template %1 using query %2").arg(aTemplate, queryTemplate));

		if (queriesMap.contains(queryTemplate))
		{
			QString sqlQuery;

			if (prepareQueries && preparedQueriesMap.contains(queryTemplate))
			{
				// development case prepared queris
				query = preparedQueriesMap[queryTemplate];
                QVariantList list = query.boundValues();

                for (int i = 0; i < list.size(); ++i)
				{
                    QString bv = list.at(i).toString().toUtf8().data();
                    QString bv2= bv;
					bv2.remove(0,1);
                    if (logger->isTrace())
                    {
                        logger->traceMsg(tr("bound %1 to value %2").arg(i).arg(replacements[bv2]));
                    }
                    query.bindValue(i, replacements[bv2]);
				}
				bRet = query.exec();

                if (logger->isTrace())
                {
                    logger->traceMsg(tr("last query %1").arg(query.lastQuery()));
                }
			}
			else
			{
				sqlQuery = replaceLine(queriesMap[queryTemplate], lineCnt, false, false);
				bRet = query.exec(sqlQuery);
			}

			if (bRet)
			{
				QSqlRecord rec = query.record();
				int numCols = rec.count();
				bool empty = true;

                if (logger->isDebug())
				{
                    logger->debugMsg("SQL-Query: "+sqlQuery);
                    if (logger->isTrace())
					{
						for (int i=0; i<numCols; ++i)
						{
                            logger->traceMsg(QString("column %1 name '%2' : %3")
                                    .arg(i)
                                    .arg(rec.fieldName(i), rec.field(i).metaType().name()));
						}
                        logger->traceMsg(tr("Size of result is %1").arg(query.size()));
					}
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
                        QByteArray tmpStr = query.value(i).toByteArray();

                        if (empty) empty = query.isNull(i);
						if (mQSE->getOutputXml())
						{
							tmpStr.replace("<", "&lt;");
							tmpStr.replace(">","&gt;");
						}
						QString tmpFieldName = rec.fieldName(i);

						if (replacements.contains(tmpFieldName))
						{
							overwrittenReplacements[tmpFieldName] = replacements[tmpFieldName];
						}
						replacements[tmpFieldName] = tmpStr;

                        if (logger->isTrace())
                        {
                            logger->traceMsg(tr("column %1 with /%2/").arg(i).arg(replacements[tmpFieldName]));
                        }
					}
					if (!empty)
					{
						lastReplaceLinefeed = replaceTemplate(templLines, lineCnt);
						uniqueId++;
						lineCnt++;
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
                logger->errorMsg(tr("executing SQL '%1' (%2)").arg(sqlQuery, errText));
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
            logger->errorMsg(tr("template %1 isn't defined").arg(aTemplate));
		}
	}

	//! last action is to restore the overwritten replacements
    QHashIterator<QString,QByteArray> it(overwrittenReplacements);
	while (it.hasNext())
	{
		it.next();
		replacements[it.key()] = it.value();
	}

    currentTemplateBlockName = lastTemplateName;
    logger->setContext(currentTemplateBlockName);

	return bRet;
}


bool QueryExecutor::createOutput(QuerySetEntry *aQSE,
								 DbConnection *dbc,
								 const QString &basePath,
								 const QString &inputDefines)
{
	bool b = true;
	mQSE = aQSE;
    QElapsedTimer t;
	t.start();

	clearStructures();								// remove the internal structure
	setInputValues(inputDefines);                   // set the input parameters from (input defines and local definese)
	createOutputFileName(basePath);                 // create variable mOutFileName
	createInputFileNames(basePath);					// create absolute input file names
	if (nullptr != dbc)
	{
		b = dbc->connectDatabase();                 // connect to database and set _tableprefix
		if (b)
		{
            replacements["_tableprefix"] = QByteArray(dbc->getTablePrefix().toUtf8());
            if (logger->isDebug())
			{
                logger->debugMsg(tr("Set parameter ${_tableprefix} to '%1'").arg(replacements["_tableprefix"]));
			}
		}
		else
		{
            logger->errorMsg(dbc->getLastErrorString());
		}

        if (QStringConverter::encodingForName(dbc->getDbEncoding().toStdString().c_str()) != QStringConverter::Utf8)
        {

            decodeDatabase = QStringDecoder(dbc->getDbEncoding().toStdString().c_str());
        }
	}

	b = b && executeInputFiles();                   // read the sql and the template file into the internal structure
	b = b && outputTemplate("MAIN");				// start process with the MAIN template
	streamOut.flush();
	fileOut.close();								// flush and close the output file

	// close the database connection
	if (nullptr != dbc)
	{
		dbc->closeDatabase();				        // close the database connection
	}

    logger->infoMsg(tr("query execution time: %1; using %2 different parameters")
			.arg(Utility::formatMilliSeconds(t.elapsed()))
            .arg(replacements.size()));

	if (!b)
	{
        logger->infoMsg(tr("creating output creates some errors (see error window)"));
	}

	return b;
}
