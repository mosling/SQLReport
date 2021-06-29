#ifndef QUERYEXECUTOR_H
#define QUERYEXECUTOR_H

#include <QObject>
#include "QuerySet.h"
#include "DBConnection.h"
#include "logmessage.h"
#include <QDateTime>
#include <QFile>
#include <QtSql/QtSql>
#include <QTextStream>
#include <QStringList>

#include <QDebug>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlQuery>
#include <QtQml/QJSEngine>
#include <string>
#include <stdio.h>
#include <ctime>

#include <QTextEdit>
#include <qsettings.h>

class QueryExecutor : public QObject
{
	Q_OBJECT
	Q_CLASSINFO ("author", "St. Koehler")
	Q_CLASSINFO ("company", "com.github.mosling")

public:
    explicit QueryExecutor(QObject *parentObj = nullptr)
        : QObject(parentObj),
          logger(new LogMessage(this)),
          mTreeNodeChanged(false),
          mQSE(nullptr),
          userInputs(),
          replacements(),
          treeReplacements(),
          cumulationMap(),
          queriesMap(),
          preparedQueriesMap(),
          templatesMap(),
          sqlFileName(""),
          templateFileName(""),
          scriptEngine(),
          decodeDatabase(QStringDecoder(QStringDecoder::Utf8)),
          fileOut(),
          streamOut(),
          uniqueId(0),
          firstQueryResult(false),
          prepareQueries(false),
          currentTemplateBlockName(""),
          fontElement("<[/]*font[^>]*>"),
          spanElement("<[/]*span[^>]*>"),
          htmlBody("<body[^>]*>((.|[\\n\\r])*)</body>")
    {
    }

	~QueryExecutor();

    void setLogger(LogMessage *l)
    {
        if (l != nullptr)
        {
            logger = l;
        }
    }

	void setPrepareQueriesFlag(bool flag);

	bool createOutput(QuerySetEntry *aQSE, DbConnection *dbc,
					  const QString &basePath, const QString &inputDefines);

protected:
	bool replaceTemplate(const QStringList *aTemplLines, int aLineCnt);
	QString replaceLine(const QString &aLine, int aLineCnt, bool sqlBinding, bool simpleFormat);
	bool outputTemplate(QString aTemplate);
	QString getDate(const QString &aFormat) const;
	void clearStructures();

private:
	void replaceLineUserInput(const QStringList &varList, QString &result, int lineCnt);
    void replaceLineVariable(const QByteArray vStr, const QStringList &varList, QString &result, int lineCnt);
	void replaceLineGlobal(const QStringList &varList, QString &result, int lineCnt);
	void showDbError(QString vErrStr);
	bool connectDatabase();
	void createOutputFileName(const QString &basePath);
	void createInputFileNames(const QString &basePath);
	bool executeInputFiles();
	void setInputValues(const QString &inputDefines);
	QStringList splitString(const QString &str, int width, const QString &startOfLine) const;
	quint32 convertToNumber(QString aNumStr, bool &aOk) const;
	void addSqlQuery(const QString &name, const QString &sqlLine);
    QString convertRtf(QString rtfText, QString resultType, bool cleanupFont);

    LogMessage *logger;
	bool mTreeNodeChanged;
	QuerySetEntry *mQSE;
	QHash <QString, QString> userInputs;
    QHash <QString, QByteArray> replacements;
    QHash <QString, QByteArray> treeReplacements;
	QMap <QString, quint32> cumulationMap;
	QMap <QString, QString> queriesMap;
	QMap <QString, QSqlQuery> preparedQueriesMap;
	QMap <QString, QStringList* > templatesMap;
	QString sqlFileName;
    QString templateFileName;
    QJSEngine scriptEngine;
    QStringDecoder decodeDatabase;

	QFile fileOut;
	QTextStream streamOut;

	int uniqueId;
	bool firstQueryResult;
	bool prepareQueries;
	QString currentTemplateBlockName;
    QRegularExpression fontElement;
    QRegularExpression spanElement;
    QRegularExpression htmlBody;

};

#endif // QUERYEXECUTOR_H
