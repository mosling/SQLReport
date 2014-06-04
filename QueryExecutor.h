#ifndef QUERYEXECUTOR_H
#define QUERYEXECUTOR_H

#include <QObject>
#include "QuerySet.h"
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

	bool createOutput(QuerySetEntry *aQSE, QTextEdit *aMsgWin,
					  const QString &basePath, const QString &inputDefines);

protected:
	bool replaceTemplate(const QStringList *aTemplLines, int aLineCnt);
	QString replaceLine(const QString &aLine, int aLineCnt);
	bool outputTemplate(QString aTemplate);
	QString getDate(const QString &aFormat) const;
	void clearStructures();

private:
	void showDbError(QString vErrStr);
	void showMsg(QString vErrStr);
	bool connectDatabase();
	void createOutputFileName(const QString &basePath);
	void createInputFileNames(const QString &basePath);
	bool executeInputFiles();
	void setInputValues(const QString &inputDefines);
	QStringList splitString(const QString &str, int width, const QString &startOfLine) const;

	bool mTreeNodeChanged;
	QuerySetEntry *mQSE;
	QTextEdit *mMsgWin;
	QHash <QString, QString> mInputs;
	QHash <QString, QString> mReplacements;
	QHash <QString, QString> mLastReplacements;
	QMap <QString, QString> mQueries;
	QMap <QString, QStringList* > mTemplates;
	QString sqlFileName;
	QString templFileName;
	QMap<QString, int> mExpressionMap;

	QFile fileOut;
	QTextStream streamOut;

	int uniqueId;

};

#endif // QUERYEXECUTOR_H
