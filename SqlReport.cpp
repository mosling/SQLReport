#include "SqlReport.h"
#include "editwidget.h"
#include "QTreeReporter.h"
#include "DbConnectionForm.h"

#include <QRegExp>
#include <QtSql/QSqlRecord>
#include <QFileInfo>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

SqlReport::SqlReport(QWidget *parentObj, Qt::WindowFlags flags)
	: QMainWindow(parentObj, flags),
	  mQuerySet(this),
	  databaseSet(this),
	  activeQuerySetEntry(nullptr),
	  treeModel(),
	  sqlEditor(this),
	  templateEditor(this),
	  outputEditor(this)
{
	ui.setupUi(this);

	ui.tvTable->setModel(&treeModel);
	ui.tvTable->header()->hide();

	ui.comboBoxDatabase->setModel(&databaseSet);
	ui.cbQuerySet->setModel(&mQuerySet);

	QSettings rc;
	this->restoreState(rc.value("windowState").toByteArray());
	this->restoreGeometry(rc.value("geometry").toByteArray());
	// Größe und Position des Hauptfensters anpassen

	QString qsn = rc.value("queryset_name","scripts/queryset.xml").toString();
	ui.lineEditLocal->setText(rc.value("local_inputs","").toString());

	readQuerySet(qsn);
}

SqlReport::~SqlReport()
{
	try
	{
		QSettings rc;

		rc.setValue("queryset_name",mQuerySet.getQuerySetFileName());
		rc.setValue("local_inputs",ui.lineEditLocal->text());
		rc.setValue("geometry", this->saveGeometry());
		rc.setValue("windowState", QVariant(this->saveState()));

		activeQuerySetEntry = nullptr;
	}
	catch (...) {}
}

//! Press the Ok-button starts the processing. At this time we
//! create the database connection and parse the template file starting
//! with the ::MAIN entry.
//! If the active query set is BATCH we execute the
void SqlReport::on_But_OK_clicked()
{
	QueryExecutor vpExecutor;
	QString queryPath = QFileInfo(mQuerySet.getQuerySetFileName()).absolutePath();
	QString baseInput = ui.lineEditInput->text() + "|" + ui.lineEditLocal->text();

	// save open editor
	sqlEditor.saveFile();
	templateEditor.saveFile();

	// do the action
	updateQuerySet();
	ui.teReport->clear();
	ui.textEditError->clear();
	ui.teReport->append(tr("using base path   : %1").arg(queryPath));

	vpExecutor.setMsgWindow(ui.teReport);
	vpExecutor.setErrorWindow(ui.textEditError);
	vpExecutor.setDebugFlag(ui.checkBoxDebug->isChecked());

	if (activeQuerySetEntry->getBatchrun())
	{
		vpExecutor.createOutput(activeQuerySetEntry,
								databaseSet.getByName(activeQuerySetEntry->getDbName()),
								queryPath,
								baseInput );

		// Batch command, uses the created file to execute each line independently

		QFile batchFile(activeQuerySetEntry->getLastOutputFile());
		if (!batchFile.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QMessageBox::information(this, tr("Batch/SQL"),
									 tr("File '%1' doesn't exists")
									 .arg(activeQuerySetEntry->getLastOutputFile()));
		}
		else
		{
			QByteArray batchCommands = batchFile.readAll();
			QTextStream streamInSql(batchCommands);
			QString line;
			quint32 lineNr = 0;

			while ( !streamInSql.atEnd())
			{
				line = streamInSql.readLine().trimmed();
				lineNr++;
				if (line.startsWith("!!"))
				{
					QStringList qList = line.split("!!", QString::SkipEmptyParts);
					QString queryName("");
					QString queryInput = baseInput;
					if (qList.size() > 0)
					{
						queryName = qList.at(0);
						if (qList.size() > 1)
						{
							queryInput = queryInput + "|" + qList.at(1);
						}
					}
					if (mQuerySet.contains(queryName))
					{
						QuerySetEntry *tmpQuery = mQuerySet.getByName(queryName);
						vpExecutor.createOutput(tmpQuery,
												databaseSet.getByName(tmpQuery->getDbName()),
												queryPath,
												queryInput);
					}
					else
					{
						ui.textEditError->append(tr("ReportErr: unknown QuerySet '%1'").arg(queryName));
					}
				}
			}
			// remove the before created batch file
			batchFile.remove();
		}
	}
	else
	{
		vpExecutor.createOutput(activeQuerySetEntry,
								databaseSet.getByName(activeQuerySetEntry->getDbName()),
								queryPath,
								baseInput );
	}
}

//! Im QuerySet werden nur die Dateinamen gespeichert. Um wirklich
//! zugreifen zu können, benötigen wir den absoluten Namen. Diesen
//! erzeugen wir durch das Hinzufügen des absoluten Pfades zum queryset,
//! falls nicht ein absoluter Name angegeben wurde.
QString SqlReport::getAbsoluteFileName(QString fname) const
{
	QFileInfo f(fname);

	if (!f.isAbsolute())
	{
		QFileInfo fi(mQuerySet.getQuerySetFileName());
		return fi.absolutePath() + "/" + fname;
	}

	return fname;
}

//! Auswahl einer Datei. Ist das Modify-Flag gesetzt wird vor den
//! übergebenen Dateinamen (def) der aktuelle Path der QuerySet-Datei
//! gesetzt und danach wieder entfernt.
QString SqlReport::selectFile(QString desc, QString def, QString pattern, bool modify, bool &cancel)
{
	QString defName(def);
	cancel = false;

	if (modify) defName = getAbsoluteFileName(def);

	QString str = QFileDialog::getOpenFileName(this, desc, defName, pattern);

	if (str.isEmpty())
	{
		str = def;
		cancel = true;
	}

	if (modify)
	{
		QFileInfo fi (str);
		QString qsPath = QFileInfo(mQuerySet.getQuerySetFileName()).absolutePath();

		if (fi.absoluteFilePath().startsWith(qsPath))
		{
			str = fi.absoluteFilePath().remove(0, qsPath.size() + 1);
		}
		else
		{
			str = fi.absoluteFilePath();
		}
	}

	return str;
}

//! Here we enter die Database connection dialog. If we have no existing database
//! we start with the 'new connection' connection.
void SqlReport::on_but_database_clicked()
{

	if (validQuerySet())
	{
		DbConnection *currentDbConnection = databaseSet.getByName(activeQuerySetEntry->getDbName());

		if (nullptr == currentDbConnection)
		{
			QString nc("new connection");
			currentDbConnection = databaseSet.getByName(nc);
			if (nullptr == currentDbConnection)
			{
				currentDbConnection = new DbConnection(&databaseSet);
				currentDbConnection->setName(nc);
				databaseSet.append(currentDbConnection);
			}
		}

		DbConnectionForm dbForm(currentDbConnection, this);
		dbForm.exec();
	}
}

//! Add a new database connection to the system. This is named 'new connection'
void SqlReport::on_but_AddDatabase_clicked()
{
	DbConnection *currentDbConnection = new DbConnection(&databaseSet);
	currentDbConnection->setName("new connection");
	databaseSet.append(currentDbConnection);

	DbConnectionForm dbForm(currentDbConnection, this);
	dbForm.exec();
}

//! Delete the selected database connection from the list
//! of available connections. There is no check if other
//! queries use this connection. The object itself is deleted
//! by the databaseSet object, that has created the connection object.
void SqlReport::on_but_DeleteDatabase_clicked()
{
	if (validQuerySet())
	{
		DbConnection *currentDbConnection = databaseSet.getByName(activeQuerySetEntry->getDbName());
		databaseSet.remove(currentDbConnection);
	}
}

void SqlReport::on_but_output_clicked()
{
	if (validQuerySet())
	{
		bool selectCancel;
		QString output = selectFile("Please select Target file",
									activeQuerySetEntry->getOutputFile(),
									"All Files(*.*)", false, selectCancel);
		if (!selectCancel)
		{
			ui.output->setText(output);
			activeQuerySetEntry->setOutputFile(output);
		}
	}
}

void SqlReport::on_but_outTemplate_clicked()
{
	if (validQuerySet())
	{
		bool selectCancel;
		QString outTemplate = selectFile("Please select Template file",
										 activeQuerySetEntry->getTemplateFile(),
										 "Templates (*.template);;All Files(*.*)", true, selectCancel);

		if (!selectCancel)
		{
			ui.outTemplate->setText(outTemplate);
			activeQuerySetEntry->setTemplateFile(outTemplate);
		}
	}
}

void SqlReport::on_but_outSql_clicked()
{
	if (validQuerySet())
	{
		bool selectCancel;
		QString outSql = selectFile("Please select SQL file",
									activeQuerySetEntry->getSqlFile(),
									"SQL-Files (*.sql);;Alle Dateien(*.*)", true, selectCancel);

		if (!selectCancel)
		{
			ui.outSql->setText(outSql);
			activeQuerySetEntry->setSqlFile(outSql);
		}
	}
}

//!
void SqlReport::on_toolButtonQuerySetNew_clicked()
{
	QString qsName = QFileDialog::getSaveFileName(this, tr("Please select new QuerySet file"),
												  mQuerySet.getQuerySetFileName(),
												  tr("XML-Files (*.xml)"));

	if (!qsName.isEmpty())
	{
		mQuerySet.writeXml("", databaseSet);
		mQuerySet.clear();
		databaseSet.clear();

		mQuerySet.writeXml(qsName, databaseSet);
		readQuerySet(qsName);
	}
}

//! Select an existing query set xml file
void SqlReport::on_but_querySet_clicked()
{
	bool selectCancel;
	QString qsName = selectFile(tr("Please select QuerySet file"),
								mQuerySet.getQuerySetFileName(),
								tr("XML-Files (*.xml);;All Files(*.*)"), false, selectCancel);
	
	if (!selectCancel)
	{
		mQuerySet.writeXml("", databaseSet);
		mQuerySet.clear();
		databaseSet.clear();

		readQuerySet(qsName);
	}
}

//! Einlesen einer QueryDatei, dabei werden die Datenbankverbindungen
//! gefüllt und die einzelnen Abfragen erstellt.
void SqlReport::readQuerySet(QString &qsName)
{
	if (qsName.isEmpty()) qsName = "queryset.xml";
	QFile qsFile(qsName);

	if (!qsFile.exists())
	{
		ui.teReport->append("query file doesn't exists -> write existing data");
		mQuerySet.writeXml(qsName, databaseSet);
	}

	if (mQuerySet.readXml(qsName, databaseSet))
	{
		QString p = QFileInfo(qsName).absolutePath();
		if (qsName.startsWith(p))
		{
			ui.lineQuerySetName->setText(QString("%1").arg(qsName));
		}
		else
		{
			ui.lineQuerySetName->setText(QString("%1 (%2)").arg(qsName).arg(p));
		}

		if (mQuerySet.rowCount() > 0)
		{
			ui.cbQuerySet->setCurrentIndex(0);
			setActiveQuerySetEntry(ui.cbQuerySet->itemText(0));
		}
		else
		{
			// we start with empty activeQuerySet
			activeQuerySetEntry = nullptr;
		}
	}
	else
	{
		ui.textEditError->append(mQuerySet.getLastError());
	}
}

void SqlReport::on_but_AddQuerySet_clicked()
{
	bool ok;

	QString newEntryName = QInputDialog::getText(this, "", "new entry name",
												 QLineEdit::Normal, "", &ok);

	if (!mQuerySet.contains(newEntryName))
	{
		QuerySetEntry *tmpQSE = new QuerySetEntry();

		if (activeQuerySetEntry != nullptr)
		{
			updateQuerySet();
			*tmpQSE = *activeQuerySetEntry;
		}

		tmpQSE->setName(newEntryName);

		mQuerySet.append(tmpQSE);
		// minimal one existing entry (index 0)
		ui.cbQuerySet->setCurrentText(newEntryName);
	}
	else
	{
		QMessageBox::information(this, tr("Eintrag existiert"),
								 tr("Ein Eintrag '%1' besteht bereits.").arg(newEntryName) );
	}
}


void SqlReport::on_but_DeleteQuerySet_clicked()
{
	if (nullptr != activeQuerySetEntry)
	{
		QMessageBox msgBox;

		msgBox.setText(tr("Remove query set '%1'?").arg(activeQuerySetEntry->getName()));
		msgBox.setInformativeText("Dateien bleiben erhalten.");
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::No);
		int ret = msgBox.exec();

		if (ret == QMessageBox::Yes)
		{
			QuerySetEntry *tmpQSE = activeQuerySetEntry;
			mQuerySet.remove(tmpQSE);

			if (tmpQSE == activeQuerySetEntry)
			{
				// no new value set by the model, the old QuerySetEntry was
				// deleted, set the active set to null.
				activeQuerySetEntry = nullptr;
			}
		}
	}
	else
	{
		ui.textEditError->append(tr("There is no active query set, which can removed."));
	}
}


void SqlReport::on_cbQuerySet_currentIndexChanged(int aIndex)
{
	updateQuerySet();
	setActiveQuerySetEntry(ui.cbQuerySet->itemText(aIndex));
	qDebug() << "set active query to " << ui.cbQuerySet->itemText(aIndex);
}

void SqlReport::on_btnEditSql_clicked()
{
	if (validQuerySet())
	{
		activeQuerySetEntry->setSqlFile(ui.outSql->text());

		if (!activeQuerySetEntry->getSqlFile().isEmpty())
		{
			sqlEditor.newFile(getAbsoluteFileName(activeQuerySetEntry->getSqlFile()));
			sqlEditor.show();
		}
		else
		{
			QMessageBox::information(this, tr("Keine Datei"),
									 tr("Bitte erst eine Datei auswählen oder eingeben") );
		}
	}
}

void SqlReport::on_btnEditTemplate_clicked()
{
	if (validQuerySet())
	{
		activeQuerySetEntry->setTemplateFile(ui.outTemplate->text());

		if (!activeQuerySetEntry->getTemplateFile().isEmpty())
		{
			templateEditor.newFile(getAbsoluteFileName(activeQuerySetEntry->getTemplateFile()));
			templateEditor.show();
		}
		else
		{
			QMessageBox::information(this, tr("Keine Datei"),
									 tr("Bitte erst eine Datei auswählen oder eingeben"));
		}
	}
}

//! If the file not exist, there is no need to show the empty file.
void SqlReport::on_btnShowOutput_clicked()
{
	if (validQuerySet())
	{
		if (!activeQuerySetEntry->getLastOutputFile().isEmpty())
		{
			if (outputEditor.newFile(activeQuerySetEntry->getLastOutputFile()))
			{
				outputEditor.show();
			}
		}
		else
		{
			//! We try to create a filename if the pattern doesn't contains dynamic parts
			if (!activeQuerySetEntry->getOutputFile().contains('$'))
			{
				if (outputEditor.newFile(getAbsoluteFileName(activeQuerySetEntry->getOutputFile())))
				{
					outputEditor.show();
				}
			}
			else
			{
				QMessageBox::information(this, tr("No file found!"),
										 tr("Please create output first (Start)\nOr uncheck batch to see the result."));
			}
		}
	}
}

void SqlReport::on_btnShowTables_clicked()
{
	updateQuerySet();

	DbConnection *currentDbConnection = databaseSet.getByName(ui.comboBoxDatabase->currentText());

	if (nullptr != currentDbConnection)
	{
		currentDbConnection->connectDatabase();
		QTreeReporter treeReporter;

		treeReporter.setReportRoot(treeModel.invisibleRootItem());
		currentDbConnection->showDatabaseTables(&treeReporter);

		currentDbConnection->closeDatabase();

		if (ui.dockWidget->isHidden()) ui.dockWidget->show();
	}
}

void SqlReport::on_btnClear_clicked()
{
	ui.teReport->clear();
	ui.textEditError->clear();
}

//! After pressing the exit button the we write the existing
//! query file.
void SqlReport::on_pushButtonExit_clicked()
{
	updateQuerySet();
	mQuerySet.writeXml("", databaseSet);

	sqlEditor.close();
	templateEditor.close();
	outputEditor.close();

	this->close();
}

//! Es werden die Werte der Oberfläche in das aktive QuerySet übernommen.
void SqlReport::updateQuerySet()
{
	if ( nullptr != activeQuerySetEntry)
	{
		activeQuerySetEntry->setDbName(ui.comboBoxDatabase->currentText());
		activeQuerySetEntry->setDescr(ui.leDescr->text());
		activeQuerySetEntry->setInputDefines(ui.lineEditInput->text());
		activeQuerySetEntry->setSqlFile(ui.outSql->text());
		activeQuerySetEntry->setTemplateFile(ui.outTemplate->text());
		activeQuerySetEntry->setOutputFile(ui.output->text());
		activeQuerySetEntry->setLocale(ui.leLocale->text());
		activeQuerySetEntry->setBatchrun(ui.checkBoxBatchRun->checkState()==Qt::Checked?true:false);
		activeQuerySetEntry->setWithTimestamp(ui.cbTimeStamp->checkState()==Qt::Checked?true:false);
		activeQuerySetEntry->setAppendOutput(ui.cbAppendOutput->checkState()==Qt::Checked?true:false);
		activeQuerySetEntry->setOutputUtf8(ui.cbOutputUtf8->checkState()==Qt::Checked?true:false);
		activeQuerySetEntry->setOutputXml(ui.cbOutputXml->checkState()==Qt::Checked?true:false);
	}
}

//! Check if we have an active query set
bool SqlReport::validQuerySet()
{
	bool result = activeQuerySetEntry != nullptr;

	if (!result)
	{
		QMessageBox::information(this, "No active query set entry",
								 "Please select a query set or create a new.");
	}
	else
	{
		updateQuerySet();
	}

	return result;
}

//! Diese Methode setzt die Werte aus dem aktiven QuerySet in der Oberfläche.
//! Wird keine solcher Index gefunden, werden alle Felder gelöscht.
void SqlReport::setActiveQuerySetEntry(const QString aIdxName)
{
	activeQuerySetEntry = mQuerySet.getByName(aIdxName);
	if (activeQuerySetEntry != nullptr)
	{
		ui.comboBoxDatabase->setCurrentText(activeQuerySetEntry->getDbName());
		ui.leDescr->setText(activeQuerySetEntry->getDescr());
		ui.lineEditInput->setText(activeQuerySetEntry->getInputDefines());
		ui.outSql->setText(activeQuerySetEntry->getSqlFile());
		ui.outTemplate->setText(activeQuerySetEntry->getTemplateFile());
		ui.output->setText(activeQuerySetEntry->getOutputFile());
		ui.leLocale->setText(activeQuerySetEntry->getLocale());
		ui.checkBoxBatchRun->setCheckState(activeQuerySetEntry->getBatchrun()?Qt::Checked:Qt::Unchecked);
		ui.cbTimeStamp->setCheckState(activeQuerySetEntry->getWithTimestamp()?Qt::Checked:Qt::Unchecked);
		ui.cbAppendOutput->setCheckState(activeQuerySetEntry->getAppendOutput()?Qt::Checked:Qt::Unchecked);
		ui.cbOutputUtf8->setCheckState(activeQuerySetEntry->getOutputUtf8()?Qt::Checked:Qt::Unchecked);
		ui.cbOutputXml->setCheckState(activeQuerySetEntry->getOutputXml()?Qt::Checked:Qt::Unchecked);
	}
	else
	{
		ui.comboBoxDatabase->setCurrentIndex(0);
		ui.lineEditInput->setText("");
		ui.leDescr->setText("");
		ui.outSql->setText("");
		ui.outTemplate->setText("");
		ui.output->setText("");
		ui.leLocale->setText("");
		ui.checkBoxBatchRun->setCheckState(Qt::Unchecked);
		ui.cbTimeStamp->setCheckState(Qt::Unchecked);
		ui.cbAppendOutput->setCheckState(Qt::Unchecked);
		ui.cbOutputUtf8->setCheckState(Qt::Unchecked);
		ui.cbOutputXml->setCheckState(Qt::Unchecked);
	}
}
