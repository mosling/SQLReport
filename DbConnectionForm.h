//! MOONWAVE SYSTEM GMBH
//! copyright 2014

#ifndef DBCONNECTIONFORM_H
#define DBCONNECTIONFORM_H

#include "DbConnection.h"
#include <QDialog>

namespace Ui {
class DbConnectionForm;
}

class DbConnectionForm : public QDialog
{
	Q_OBJECT

public:
	explicit DbConnectionForm(DbConnection *dbCon, QWidget *parent = 0);
	~DbConnectionForm();

private slots:
	void on_pushButtonExit_clicked();

	void on_pushButtonCancel_clicked();

	void on_toolButtonName_clicked();

private:
	Ui::DbConnectionForm *ui;
	DbConnection *dbc;

};

#endif // DBCONNECTIONFORM_H
