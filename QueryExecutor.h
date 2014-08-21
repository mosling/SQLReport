#ifndef QUERYEXECUTOR_H
#define QUERYEXECUTOR_H

#include <QObject>
#include "QuerySet.h"
#include "DbConnection.h"
#include <QDateTime>
#include <QFile>
#include <QtSql/QtSql>
#include <QTextStream>
#include <QStringList>

#include <qdebug>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlQuery>
#include <string>
#include <stdio.h>
#include <ctime>

#include <QTextEdit>
#include <qsettings.h>

class QueryExecutor : public QObject
{
	Q_OBJECT
	Q_CLASSINFO ("author", "steffen")
	Q_CLASSINFO ("company", "Moonwave Systems GmbH")

public:
	explicit QueryExecutor(QObject *parentObj = nullptr);
	~QueryExecutor();

	void setMsgWindow(QTextEdit *te);
	void setErrorWindow(QTextEdit *te);
	void setDebugFlag(bool flag);
	void setPrepareQueriesFlag(bool flag);

	bool createOutput(QuerySetEntry *aQSE, DbConnection *dbc,
					  const QString &basePath, const QString &inputDefines);

	enum class LogLevel {ERR, WARN, MSG, DBG};

protected:
	bool replaceTemplate(const QStringList *aTemplLines, int aLineCnt);
	QString replaceLine(const QString &aLine, int aLineCnt, bool sqlBinding);
	bool outputTemplate(QString aTemplate);
	QString getDate(const QString &aFormat) const;
	void clearStructures();

private:
	void replaceLineUserInput(const QStringList &varList, QString &result, int lineCnt);
	void replaceLineVariable(const QStringList &varList, QString &result, int lineCnt);
	void replaceLineGlobal(const QStringList &varList, QString &result, int lineCnt);
	void showDbError(QString vErrStr);
	void showMsg(QString vMsgStr, LogLevel ll);
	bool connectDatabase();
	void createOutputFileName(const QString &basePath);
	void createInputFileNames(const QString &basePath);
	bool executeInputFiles();
	void setInputValues(const QString &inputDefines);
	QStringList splitString(const QString &str, int width, const QString &startOfLine) const;
	quint32 convertToNumber(QString aNumStr, bool &aOk) const;
	void addSqlQuery(const QString &name, const QString &sqlLine);

	bool mTreeNodeChanged;
	QuerySetEntry *mQSE;
	QTextEdit *mMsgWin;
	QTextEdit *mErrorWin;
	QHash <QString, QString> userInputs;
	QHash <QString, QString> replacements;
	QHash <QString, QString> lastReplacements;
	QMap <QString, quint32> cumulationMap;
	QMap <QString, QString> queriesMap;
	QMap <QString, QSqlQuery> preparedQueriesMap;
	QMap <QString, QStringList* > templatesMap;
	QString sqlFileName;
	QString templFileName;
	QMap<QString, int> mExpressionMap;

	QFile fileOut;
	QTextStream streamOut;

	int uniqueId;
	bool firstQueryResult;
	QString lastErrorFilename;
	bool debugOutput;
	bool prepareQueries;

};

#endif // QUERYEXECUTOR_H
