#include "SqlReport.h"
#include "EditWidget.h"
#include "QTreeReporter.h"
#include "DbConnectionForm.h"
#include "Utility.h"

#include <QRegularExpression>
#include <QtSql/QSqlRecord>
#include <QFileInfo>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QTime>

SqlReport::SqlReport(QWidget *parentObj, Qt::WindowFlags flags)
	: QMainWindow(parentObj, flags),
	  mQuerySet(this),
	  databaseSet(this),
	  activeQuerySetEntry(nullptr),
	  treeModel(),
      sqlEditor(this, "sqlEditor", true, true),
      templateEditor(this, "templateEdititor", true, true),
      outputEditor(this, "outputEditor", false, false),
      logger(new LogMessage(this))
{
	ui.setupUi(this);

	ui.tvTable->setModel(&treeModel);
	ui.tvTable->header()->hide();

	ui.comboBoxDatabase->setModel(&databaseSet);
	ui.cbQuerySet->setModel(&mQuerySet);

	sqlEditor.setWindowTitle(tr("SQL Editor"));
	templateEditor.setWindowTitle(tr("Template Editor"));
	outputEditor.setWindowTitle(tr("Show Report Output"));

    // connect sql and template editor
    sqlEditor.setConnectedWidget(&templateEditor);
    templateEditor.setConnectedWidget(&sqlEditor);

    // restore the window geometry for the widgets and the
    // window state for the main window
    QSettings rc("msk-soft", "sql-report");
    this->restoreState(rc.value("mainwindow/windowState").toByteArray());
    this->restoreGeometry(rc.value("mainwindow/geometry").toByteArray());

    sqlEditor.readSettings(rc);
    templateEditor.readSettings(rc);
    outputEditor.readSettings(rc);

    // set logger windows
    logger->setMsgWindow(ui.textEditReport);
    logger->setErrorWindow(ui.textEditError);

    // read query set for the query set name from the settings
    QString qsn = rc.value("mainwindow/queryset_name","scripts/queryset.xml").toString();
	readQuerySet(qsn);
}

SqlReport::~SqlReport()
{
	try
	{
		updateQuerySet();
		mQuerySet.writeXml("", databaseSet);

		sqlEditor.close();
		templateEditor.close();
		outputEditor.close();

        QSettings rc("msk-soft", "sql-report");
        rc.beginGroup("mainwindow");
		rc.setValue("queryset_name",mQuerySet.getQuerySetFileName());
		rc.setValue("geometry", this->saveGeometry());
        rc.setValue("windowState", QVariant(this->saveState()));
        rc.endGroup();

        templateEditor.storeSettings(rc);
        outputEditor.storeSettings(rc);
        sqlEditor.storeSettings(rc);

		writeLocalDefines(mQuerySet.getQuerySetFileName());

		activeQuerySetEntry = nullptr;
        delete logger;
        logger = nullptr;
	}
	catch (...) {}
}

//! Press the Ok-button starts the processing. At this time we
//! create the database connection and parse the template file starting
//! with the ::MAIN entry.
//! If the active query set is BATCH we execute the generated query statements atsrting with !!
void SqlReport::on_But_OK_clicked()
{
	QueryExecutor vpExecutor;
	QString queryPath = QFileInfo(mQuerySet.getQuerySetFileName()).absolutePath();
    QString baseInput = ui.lineEditInput->text() + "|" + ui.lineEditLocal->text();
	bool vRes = false;

	// save open editor
	sqlEditor.saveFile();
	templateEditor.saveFile();

	// do the action
	updateQuerySet();
    ui.textEditReport->clear();
	ui.textEditError->clear();

    logger->setDebugFlag(ui.checkBoxDebug->checkState());
    vpExecutor.setLogger(logger);
	vpExecutor.setPrepareQueriesFlag(ui.checkBoxPrepare->isChecked());

	if (activeQuerySetEntry->getBatchrun())
	{
        QElapsedTimer batchTime;
		batchTime.start();
		vRes = vpExecutor.createOutput(activeQuerySetEntry,
									   databaseSet.getByName(activeQuerySetEntry->getDbName()),
									   queryPath,
									   baseInput );

		// Batch command, uses the created file to execute each line independently
		if (vRes)
		{
			QFile batchFile(activeQuerySetEntry->getLastOutputFile());
			if (!batchFile.open(QIODevice::ReadOnly | QIODevice::Text))
			{
				QMessageBox::information(this, tr("Batch/SQL"),
										 tr("File '%1' doesn't exists")
										 .arg(activeQuerySetEntry->getLastOutputFile()));
			}
			else
			{
                QString batchCommandsString = QString(batchFile.readAll());
                QStringList batchCommand = batchCommandsString.split('\n');
                qint32 cntCommands = 0;
                foreach(QString str, batchCommand)
                {
                    if (str.startsWith("!!"))
                    {
                        cntCommands++;
                    }
                }

				quint32 lineNr = 0;
                quint32 queryNr = 0;

                foreach (QString line, batchCommand)
				{
                    line = line.trimmed();
					lineNr++;
					if (line.startsWith("!!"))
					{
                        queryNr++;
                        QStringList qList = line.split("!!", Qt::SkipEmptyParts);
						QString queryName("");
						QString queryInput = baseInput;
						if (qList.size() > 0)
						{
							queryName = qList.at(0);
                            for (qsizetype i = 1; i < qList.size(); ++i)
							{
                                queryInput = queryInput + "|" + qList.at(i);
							}
						}
						if (mQuerySet.contains(queryName))
						{
                            logger->infoMsg(QString("--[ %1/%2 ]--------------------------------------------------")
                                            .arg(queryNr).arg(cntCommands));
							QuerySetEntry *tmpQuery = mQuerySet.getByName(queryName);
							vpExecutor.createOutput(tmpQuery,
													databaseSet.getByName(tmpQuery->getDbName()),
													queryPath,
													queryInput);

						}
						else
						{
                            logger->errorMsg(tr("Unknown QuerySet '%1' at line %2").arg(queryName).arg(lineNr));
						}
					}
				}

				// remove the before created batch file
				batchFile.remove();
                logger->infoMsg(tr("batch execution time: %1").arg(Utility::formatMilliSeconds(batchTime.elapsed())));
			}
		}
	}
	else
	{
		vpExecutor.createOutput(activeQuerySetEntry,
								databaseSet.getByName(activeQuerySetEntry->getDbName()),
								queryPath,
								baseInput );
        ui.output->setText(activeQuerySetEntry->getOutputFile());
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

//! This method replaces all non usable character with an underline.
QString SqlReport::getSettingsGroupName(QString str) const
{
	QString settingGroup = str;
	settingGroup.replace(':', '_');
	settingGroup.replace('/','_');
	settingGroup.replace('\\', '_');
	return settingGroup;
}

//! Auswahl einer Datei. Ist das Modify-Flag gesetzt wird vor den
//! übergebenen Dateinamen (def) der aktuelle Path der QuerySet-Datei
//! gesetzt und danach wieder entfernt.
QString SqlReport::selectFile(QString desc, QString def, QString pattern, bool modify, bool &cancel)
{
	QString defName(def);
	cancel = false;

    if (modify)
    {
        defName = getAbsoluteFileName(def);
    }

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
//! we start with the 'new connection' connection. This works also without
//! active query, (normally at the start by impatient customer:-)
void SqlReport::on_but_database_clicked()
{

    DbConnection *currentDbConnection = databaseSet.getByName(ui.comboBoxDatabase->currentText());

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
		QMessageBox msgBox;

		msgBox.setText(tr("Remove database connection '%1'?").arg(activeQuerySetEntry->getDbName()));
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::No);
		int ret = msgBox.exec();

		if (ret == QMessageBox::Yes)
		{
			DbConnection *currentDbConnection = databaseSet.getByName(activeQuerySetEntry->getDbName());
			databaseSet.remove(currentDbConnection);
		}
	}
}

void SqlReport::on_but_output_clicked()
{
	if (validQuerySet())
	{
		bool selectCancel;
		QString output = selectFile("Please select Target file",
									activeQuerySetEntry->getOutputFile(),
                                    "All Files(*.*)", true, selectCancel);
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
		writeLocalDefines(mQuerySet.getQuerySetFileName());
		mQuerySet.clear();
		databaseSet.clear();
        ui.textEditReport->clear();
		ui.textEditError->clear();

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
        logger->errorMsg("query file doesn't exists -> write existing data");
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
            ui.lineQuerySetName->setText(QString("%1 (%2)").arg(qsName, p));
		}

		if (mQuerySet.rowCount() > 0)
		{
            QuerySetEntry *e = mQuerySet.getShowFirst();
            if (e != nullptr)
            {
                ui.cbQuerySet->setCurrentText(e->getName());
            }
            else
            {
                ui.cbQuerySet->setCurrentIndex(0);
            }
            readLocalDefines(mQuerySet.getQuerySetFileName());
		}
		else
		{
			// we start with empty activeQuerySet
			activeQuerySetEntry = nullptr;
		}
	}
	else
	{
        logger->errorMsg(mQuerySet.getLastError());
	}
}

void SqlReport::readLocalDefines(const QString &qsName)
{
	QSettings rc;
	QString qsnGroup = getSettingsGroupName(qsName);

	if (rc.childGroups().contains(qsnGroup))
	{
		rc.beginGroup(qsnGroup);
		ui.lineEditLocal->setText(rc.value("local_inputs","").toString());
		rc.endGroup();
	}
	else
	{
		ui.lineEditLocal->setText(rc.value("local_inputs","").toString());
	}
}

void SqlReport::writeLocalDefines(const QString &qsName)
{
	QSettings rc;

	rc.beginGroup(getSettingsGroupName(qsName));
	rc.setValue("local_inputs",ui.lineEditLocal->text());
	rc.endGroup();
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
        logger->errorMsg(tr("There is no active query set, which can removed."));
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
										 tr("Please create output first (Start)\n(Uncheck batch checkbox to see the result.)"));
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
        if (currentDbConnection->connectDatabase())
        {
            QTreeReporter treeReporter;
            currentDbConnection->setLogger(logger);

            logger->infoMsg(tr("Get database structure ..."));
            treeReporter.setReportRoot(treeModel.invisibleRootItem());
            currentDbConnection->showDatabaseTables(&treeReporter, ui.cbWithForeignKeys->isChecked());
            logger->infoMsg(tr("Ready."));

            currentDbConnection->closeDatabase();

            if (ui.dockWidget->isHidden()) ui.dockWidget->show();
        }
	}
}

//! After pressing the exit button we close the
//! window and the destructor writes the last
//! query set and close the database.
void SqlReport::on_pushButtonExit_clicked()
{
    this->close();
}

//! Es werden die Werte der Oberfläche in das aktive QuerySet übernommen.
void SqlReport::updateQuerySet()
{
	if ( nullptr != activeQuerySetEntry)
	{
		activeQuerySetEntry->setDbName(ui.comboBoxDatabase->currentText());
        activeQuerySetEntry->setDescr(ui.lineEditDescr->text());
		activeQuerySetEntry->setInputDefines(ui.lineEditInput->text());
		activeQuerySetEntry->setSqlFile(ui.outSql->text());
		activeQuerySetEntry->setTemplateFile(ui.outTemplate->text());
		activeQuerySetEntry->setOutputFile(ui.output->text());
		activeQuerySetEntry->setBatchrun(ui.checkBoxBatchRun->checkState()==Qt::Checked?true:false);
		activeQuerySetEntry->setWithTimestamp(ui.cbTimeStamp->checkState()==Qt::Checked?true:false);
		activeQuerySetEntry->setAppendOutput(ui.cbAppendOutput->checkState()==Qt::Checked?true:false);
		activeQuerySetEntry->setOutputUtf8(ui.cbOutputUtf8->checkState()==Qt::Checked?true:false);
		activeQuerySetEntry->setOutputXml(ui.cbOutputXml->checkState()==Qt::Checked?true:false);
        mQuerySet.setShowFirst(activeQuerySetEntry, ui.cbShowFirst->checkState()==Qt::Checked);
	}
}

//! Check if we have an active query set
bool SqlReport::validQuerySet()
{
	bool result = activeQuerySetEntry != nullptr;

	if (!result)
	{
        QMessageBox::information(this, tr("No active query entry"),
                                 tr("Please select a query or create a new."));
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
        ui.lineEditDescr->setText(activeQuerySetEntry->getDescr());
		ui.lineEditInput->setText(activeQuerySetEntry->getInputDefines());
		ui.outSql->setText(activeQuerySetEntry->getSqlFile());
		ui.outTemplate->setText(activeQuerySetEntry->getTemplateFile());
		ui.output->setText(activeQuerySetEntry->getOutputFile());
		ui.checkBoxBatchRun->setCheckState(activeQuerySetEntry->getBatchrun()?Qt::Checked:Qt::Unchecked);
		ui.cbTimeStamp->setCheckState(activeQuerySetEntry->getWithTimestamp()?Qt::Checked:Qt::Unchecked);
		ui.cbAppendOutput->setCheckState(activeQuerySetEntry->getAppendOutput()?Qt::Checked:Qt::Unchecked);
		ui.cbOutputUtf8->setCheckState(activeQuerySetEntry->getOutputUtf8()?Qt::Checked:Qt::Unchecked);
		ui.cbOutputXml->setCheckState(activeQuerySetEntry->getOutputXml()?Qt::Checked:Qt::Unchecked);
        ui.cbShowFirst->setCheckState(activeQuerySetEntry->getShowFirst()?Qt::Checked:Qt::Unchecked);
	}
	else
	{
		ui.comboBoxDatabase->setCurrentIndex(0);
		ui.lineEditInput->setText("");
        ui.lineEditDescr->setText("");
		ui.outSql->setText("");
		ui.outTemplate->setText("");
		ui.output->setText("");
		ui.checkBoxBatchRun->setCheckState(Qt::Unchecked);
		ui.cbTimeStamp->setCheckState(Qt::Unchecked);
		ui.cbAppendOutput->setCheckState(Qt::Unchecked);
		ui.cbOutputUtf8->setCheckState(Qt::Unchecked);
		ui.cbOutputXml->setCheckState(Qt::Unchecked);
        ui.cbShowFirst->setCheckState(Qt::Unchecked);
	}
}
