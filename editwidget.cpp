#include <QMessageBox>
#include <QTextStream>
#include "editwidget.h"
#include "ui_editwidget.h"

EditWidget::EditWidget(QWidget *parentObj) :
	QDialog(parentObj),
	ui(new Ui::EditWidget),
	highlighter(nullptr),
	currentFileName("")

{
    ui->setupUi(this);
	highlighter = new SqlReportHighlighter(ui->teEditor->document());
}

EditWidget::~EditWidget()
{
    delete ui;
}

bool EditWidget::newFile(QString aFileName)
{
	bool bRet = true;

	if (aFileName != currentFileName)
	{
		QFile file(aFileName);
		if (file.open(QFile::ReadOnly | QFile::Text))
		{
			ui->teEditor->setPlainText(file.readAll());
		}
		else
		{
			QMessageBox::information(this, tr("Fileinfo"),
									 tr("File '%1' not found.").arg(aFileName),QMessageBox::Ok);
			bRet = false;
		}

		currentFileName = aFileName;
		ui->leFileName->setText(currentFileName);
	}

	return bRet;
}

//! Aufruf der Speicherfunkion, falls das Fenster sichtbar ist.
void EditWidget::saveFile()
{
	if (this->isVisible()) on_btnSave_clicked();
}

bool EditWidget::on_btnSave_clicked()
{
	QFile file(currentFileName);
	if (!file.open(QFile::WriteOnly | QFile::Text))
	{
		QMessageBox::warning(this, tr("Application"),
							 tr("Cannot write file %1:\n%2.")
							 .arg(currentFileName)
							 .arg(file.errorString()));
		return false;
	}

	QTextStream out(&file);
	QApplication::setOverrideCursor(Qt::WaitCursor);
	out << ui->teEditor->toPlainText();
	QApplication::restoreOverrideCursor();

	ui->teEditor->document()->setModified(false);
	return true;
}

void EditWidget::on_pushButtonFind_clicked()
{
	static QString lastFind = "";

	QString f = ui->lineEditFind->text();
	if (f != lastFind)
	{
		ui->teEditor->moveCursor(QTextCursor::Start);
		lastFind = f;
	}

	ui->teEditor->find(f);
}

//! Wenn die Speichern-Nachfrage abgebrochen wurde, dann wird das
//! Ereignis ignoriert. Sonst wird der Editor geleert, damit eine Veränderung
//! der Datei im Dateisystem nicht zu Verwirrung führt.
void EditWidget::closeEvent(QCloseEvent *event)
{
	if (!maybeSave())
	{
		event->ignore();
	}
	else
	{
		this->close();
		ui->teEditor->clear();
		currentFileName = "";
	}
}

void EditWidget::reject()
{
}

bool EditWidget::maybeSave()
{
	if (ui->teEditor->document()->isModified())
	{
		QMessageBox::StandardButton ret;
		ret = QMessageBox::warning(this, tr("Application"),
								   tr("The document has been modified.\n"
									  "Do you want to save your changes?"),
								   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
		if (ret == QMessageBox::Save)
			return on_btnSave_clicked();
		else if (ret == QMessageBox::Cancel)
			return false;
	}
	return true;
}
