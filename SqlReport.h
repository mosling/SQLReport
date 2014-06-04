#ifndef SQLREPORT_H
#define SQLREPORT_H

#include "EditWidget.h"
#include "QueryExecutor.h"
#include "ui_sqlReport.h"
#include "QuerySet.h"

#include <QStandardItemModel>

class SqlReport : public QMainWindow
{
	Q_OBJECT

public:
	explicit SqlReport(QWidget *parentObj = 0, Qt::WindowFlags = 0);
	~SqlReport();

protected:
	void setActiveQuerySetEntry(const QString aIdxName);
	void updateQuerySet();

private slots:
	void on_cbQuerySet_currentIndexChanged(int);
	void on_but_AddQuerySet_clicked();
	void on_but_DeleteQuerySet_clicked();
	void on_but_querySet_clicked();
	void on_but_outSql_clicked();
	void on_but_outTemplate_clicked();
	void on_but_output_clicked();
	void on_but_database_clicked();
	void on_But_OK_clicked();
	void on_btnEditSql_clicked();
	void on_btnEditTemplate_clicked();
	void on_btnShowOutput_clicked();
	void on_btnShowTables_clicked();
	void on_pushButtonExit_clicked();

private:
	bool validQuerySet();
	QString getAbsoluteFileName(QString fname) const;
	QString selectFile(QString desc, QString def,
		QString pattern, bool modify);
	void readQuerySet(QString &qsName);

	Ui_SqlReportClass ui;
	QuerySet mQuerySet;
	QuerySetEntry *activeQuerySetEntry;

	QStandardItemModel treeModel;
	bool constructorRunning;
	EditWidget sqlEditor;
	EditWidget templateEditor;
	EditWidget outputEditor;
};

#endif // SQLREPORT_H
