//! MOONWAVE SYSTEM GMBH
//! copyright 2014

#include "DbConnectionForm.h"
#include "ui_DbConnectionForm.h"

#include <QFileDialog>

DbConnectionForm::DbConnectionForm(DbConnection *dbCon, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DbConnectionForm),
	dbc(dbCon)
{
	ui->setupUi(this);

	// Combobox füllen
	ui->cbDbType->addItem("QODBC");
	ui->cbDbType->addItem("QPSQL");
	ui->cbDbType->addItem("QMYSQL");
	ui->cbDbType->addItem("QSQLITE");

	if (nullptr != dbCon)
	{
		ui->lineEditName->setText(dbc->getName());
		ui->cbDbType->setCurrentText(dbc->getDbType());
		ui->lineEditDbName->setText(dbCon->getDbName());
		ui->lineEditHost->setText(dbc->getHost());
		ui->lineEditPort->setText(QString("%1").arg(dbc->getPort()));
		ui->lineEditUser->setText(dbc->getUsername());
		ui->lineEditPassword->setText(dbc->getPassword());
	}
}

DbConnectionForm::~DbConnectionForm()
{
	delete ui;
}

void DbConnectionForm::on_pushButtonExit_clicked()
{
	if (nullptr != dbc)
	{
		dbc->setName(ui->lineEditName->text());
		dbc->setDbName(ui->lineEditDbName->text());
		dbc->setHost(ui->lineEditHost->text());
		dbc->setDbType(ui->cbDbType->currentText());
		bool bOk;
		quint32 p = ui->lineEditPort->text().toUInt(&bOk, 10);
		if (bOk)
		{
			dbc->setPort(p);
		}
		dbc->setUsername(ui->lineEditUser->text());
		dbc->setPassword(ui->lineEditPassword->text());
	}

	this->close();
}

void DbConnectionForm::on_pushButtonCancel_clicked()
{
	this->close();
}

void DbConnectionForm::on_toolButtonName_clicked()
{
	QString str = QFileDialog::getOpenFileName(this,
											   "Please select database",
											   dbc->getDbName(),
											   "Alle Dateien(*.*)"
											   );
	if (!str.isEmpty())
	{
		dbc->setDbName(str);
		ui->lineEditDbName->setText(str);
	}
}

