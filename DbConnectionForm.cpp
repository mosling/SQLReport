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
    QStringList drivers = QSqlDatabase::drivers();
    ui->cbDbType->addItems(drivers);

    QStringList dbEncList;
    for (int i = QStringConverter::Utf8; i != QStringConverter::LastEncoding; ++i )
    {
        dbEncList.append(QStringConverter::nameForEncoding(static_cast<QStringConverter::Encoding>(i)));
    }
    ui->cbDbEncoding->addItems(dbEncList);


	if (nullptr != dbc)
	{
		ui->lineEditName->setText(dbc->getName());
        if (!drivers.contains(dbc->getDbType()))
        {
            // add unknown database connection type
            ui->cbDbType->addItem(dbc->getDbType());
        }
		ui->cbDbType->setCurrentText(dbc->getDbType());
        ui->cbDbEncoding->setCurrentText(dbc->getDbEncoding());
        ui->lineEditDbName->setText(dbc->getDbName());
        ui->lineEditOptions->setText(dbc->getDbOptions());
		ui->lineEditTablePrefix->setText(dbc->getTablePrefix());
		ui->lineEditHost->setText(dbc->getHost());
		ui->lineEditPort->setText(QString("%1").arg(dbc->getPort()));
		ui->lineEditUser->setText(dbc->getUsername());
		ui->lineEditPassword->setText(dbc->getPassword());
		ui->checkBoxPasswordSave->setChecked(dbc->getPasswordSave());
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
        dbc->setDbEncoding(ui->cbDbEncoding->currentText());
        dbc->setDbOptions(ui->lineEditOptions->text());
		dbc->setTablePrefix(ui->lineEditTablePrefix->text());
		dbc->setHost(ui->lineEditHost->text());
		dbc->setDbType(ui->cbDbType->currentText());
		bool bOk;
		quint32 p = ui->lineEditPort->text().toUInt(&bOk, 10);
        dbc->setPort(bOk ? p : 0);
        dbc->setUsername(ui->lineEditUser->text());
		dbc->setPassword(ui->lineEditPassword->text());
		dbc->setPasswordSave(ui->checkBoxPasswordSave->checkState() == Qt::Checked);
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

