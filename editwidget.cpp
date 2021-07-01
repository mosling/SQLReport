#include <QMessageBox>
#include <QTextStream>
#include <QFileDialog>
#include <QtPrintSupport/QPrinter>

#include "EditWidget.h"
#include "Utility.h"
#include "ui_editwidget.h"

EditWidget::EditWidget(QWidget *parentObj, bool showToc) :
	QWidget(parentObj),
	ui(new Ui::EditWidget),
	highlighter(nullptr),
	currentFileName(""),
    searchString(""),
    tableOfContent(nullptr),
    connectedWidget(nullptr)
{
    ui->setupUi(this);
	this->setWindowFlags(Qt::Window);

	highlighter = new SqlReportHighlighter(ui->teEditor->document());
	this->setStyleSheet("selection-color: yellow; selection-background-color: blue");

    if (showToc)
    {
        tableOfContent = new QStringListModel();
        ui->lvToc->setModel(tableOfContent);
    }
    else
    {
        ui->lvToc->hide();
    }
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
            updateTableOfContent();
		}
		else
		{
			QMessageBox::information(this, tr("Fileinfo"),
									 tr("File '%1' not found.").arg(aFileName),QMessageBox::Ok);
			ui->teEditor->clear();
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

void EditWidget::setLineWrapMode(QTextEdit::LineWrapMode lwp)
{
	ui->teEditor->setLineWrapMode(lwp);
    ui->pushButtonWrap->setChecked(lwp!=QTextEdit::NoWrap);
}

void EditWidget::placeCursorAtNode(QString nodeName)
{
    qsizetype idx = tableOfContent->stringList().indexOf(nodeName);
    if (idx > -1)
    {
        ui->lvToc->setCurrentIndex(ui->lvToc->model()->index(idx, 0));
        ui->teEditor->moveCursor(QTextCursor::End);
        ui->teEditor->moveCursor(QTextCursor::StartOfLine);
        QString stmp = QString("^%1$").arg(nodeName);
        ui->teEditor->find(QRegularExpression(stmp), QTextDocument::FindBackward);
    }
}


//!
void EditWidget::keyPressEvent(QKeyEvent *event)
{
	if (Qt::Key_F3 == event->key() && !searchString.isEmpty())
	{
		on_pushButtonFind_clicked();
	}

    if (Qt::Key_F7 == event->key() && ui->lvToc->isVisible())
    {
        QTextCursor cursor = ui->teEditor->textCursor();
        cursor.select(QTextCursor::WordUnderCursor);
        ui->teEditor->setTextCursor(cursor);
        QString word = cursor.selectedText();
        updateCursorNode(QString("::%1").arg(word));
    }

    QWidget::keyPressEvent(event);
}

bool EditWidget::on_btnSave_clicked()
{
	QFile file(currentFileName);
	if (!file.open(QFile::WriteOnly | QFile::Text))
	{
		QMessageBox::warning(this, tr("Application"),
							 tr("Cannot write file %1:\n%2.")
                             .arg(currentFileName, file.errorString()));
		return false;
	}

	QTextStream out(&file);
	QApplication::setOverrideCursor(Qt::WaitCursor);
    out << ui->teEditor->toPlainText().replace('\r', "");
	QApplication::restoreOverrideCursor();

	ui->teEditor->document()->setModified(false);
    return true;
}

bool EditWidget::on_btnSaveAs_clicked()
{
    QString newFileName = QFileDialog::getSaveFileName(this, "Please select new file", currentFileName, "All Files(*.*)");

    if (!newFileName.isEmpty())
    {
        QString tmp(currentFileName);
        currentFileName = newFileName;
        bool r = on_btnSave_clicked();
        currentFileName = tmp;
        return r;
    }
    return true;
}

void EditWidget::on_pushButtonFind_clicked()
{
	bool findSomething = ui->teEditor->find(searchString);

	if (!findSomething)
	{
		// start from beginning
		ui->teEditor->moveCursor(QTextCursor::Start);
		ui->teEditor->find(searchString);
	}
}

void EditWidget::on_pushButtonPdf_clicked()
{
	QString fileName = QFileDialog::getSaveFileName(this, "Export PDF",
													QString(currentFileName + ".pdf"), "*.pdf");
	if (!fileName.isEmpty())
	{
		if (QFileInfo(fileName).suffix().isEmpty())
			fileName.append(".pdf");

		QPrinter printer(QPrinter::HighResolution);
		printer.setOutputFormat(QPrinter::PdfFormat);
		printer.setOutputFileName(fileName);
		ui->teEditor->document()->print(&printer);
	}
}

void EditWidget::on_lineEditFind_textChanged(QString str)
{
	searchString = str;
}

void EditWidget::on_lineEditFind_returnPressed()
{
	ui->pushButtonFind->setFocus();
	on_pushButtonFind_clicked();
}

void EditWidget::on_pushButtonWrap_toggled(bool b)
{
    ui->teEditor->setLineWrapMode(b ? QTextEdit::WidgetWidth: QTextEdit::NoWrap);
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

void EditWidget::updateTableOfContent()
{
    if (tableOfContent == nullptr)
    {
        return;
    }

    QStringList tocList;
    foreach(QString s, ui->teEditor->toPlainText().split(QRegularExpression("[\n]")))
    {
        if (s.startsWith("::") && !s.startsWith("::#"))
        {
            tocList << s;
        }
    }
    tocList.sort();
    tableOfContent->setStringList(tocList);
}

void EditWidget::updateCursorNode(QString nodeName)
{
    if (ui->lvToc->isVisible())
    {
        placeCursorAtNode(nodeName);

        if (connectedWidget != nullptr && ui->cbSync->isChecked())
        {
            connectedWidget->placeCursorAtNode(nodeName);
        }
    }
}

void EditWidget::on_lvToc_clicked(const QModelIndex &index)
{
    QString itemText = index.data(Qt::DisplayRole).toString();
    updateCursorNode(itemText);
}


void EditWidget::on_teEditor_textChanged()
{
    updateTableOfContent();
}
