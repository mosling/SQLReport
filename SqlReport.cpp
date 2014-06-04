#include "SqlReport.h"
#include "editwidget.h"
#include "QTreeReporter.h"

#include <QRegExp>
#include <QtSql/QSqlRecord>
#include <QFileInfo>
#include <QProgressBar>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

SqlReport::SqlReport(QWidget *parentObj, Qt::WindowFlags flags)
	: QMainWindow(parentObj, flags),
	  mQuerySet(this),
	  activeQuerySetEntry(nullptr),
	  treeModel(),
	  constructorRunning(false),
	  sqlEditor(this),
	  templateEditor(this),
	  outputEditor(this)
{
	ui.setupUi(this);

	ui.progressBar->hide();
	ui.tvTable->setModel(&treeModel);
	ui.tvTable->header()->hide();

	// Combobox füllen
	ui.cbDbType->addItem("QODBC");
	ui.cbDbType->addItem("QPSQL");
	ui.cbDbType->addItem("QMYSQL");
	ui.cbDbType->addItem("QSQLITE");

	QSettings rc;
	this->restoreState(rc.value("properties").toByteArray());
	// Größe und Position des Hauptfensters anpassen
	int winx=rc.value("window_xpos","-1").toInt();
	int winy=rc.value("window_ypos","-1").toInt();
	int winw=rc.value("window_width","-1").toInt();
	int winh=rc.value("window_height","-1").toInt();
	if (winx!=-1 && winy!=-1 && winw!=-1 && winh!=-1)
	{
		this->frameGeometry().setRect(winx,winy,winw,winh);
	}
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
	rc.setValue("properties", QVariant(this->saveState()));
	rc.setValue("window_xpos",QVariant(this->x()));
	rc.setValue("window_ypos",QVariant(this->y()));
	rc.setValue("window_width",QVariant(this->frameGeometry().width()));
	rc.setValue("window_height",QVariant(this->frameGeometry().height()));

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
	ui.teReport->append(tr("using base path   : %1").arg(queryPath));
	ui.progressBar->show();

	if (activeQuerySetEntry->getBatchrun())
	{
		vpExecutor.createOutput(activeQuerySetEntry, ui.teReport,
								queryPath, baseInput );

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
						vpExecutor.createOutput(mQuerySet.getByName(queryName),
												ui.teReport, queryPath, queryInput);
					}
					else
					{
						ui.teReport->append(tr("unknown QuerySet '%1'").arg(queryName));
					}
				}
			}
			// remove the before created batch file
			batchFile.remove();
		}
	}
	else
	{
		vpExecutor.createOutput(activeQuerySetEntry, ui.teReport,
								queryPath, baseInput );
	}

	ui.progressBar->hide();
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
QString SqlReport::selectFile(QString desc, QString def, QString pattern, bool modify)
{
	QString defName(def);

	if (modify) defName = getAbsoluteFileName(def);

	QString str = QFileDialog::getOpenFileName(this, desc, defName, pattern);

	if (str.isEmpty())
	{
		str = def;
	}

	if (modify)
	{
		QFileInfo fi (str);
		str = fi.fileName();
	}

	return str;
}

void SqlReport::on_but_database_clicked()
{
	if (validQuerySet())
	{
		QString database = selectFile("Bitte Datenbank auswählen",
									  activeQuerySetEntry->getDatabase().getDbName(),
									  "Alle Dateien(*.*)", false);

		ui.database->setText(database);
		activeQuerySetEntry->setDbName(database);
	}
}

void SqlReport::on_but_output_clicked()
{
	if (validQuerySet())
	{
		QString output = selectFile("Bitte Zieldatei auswählen",
									activeQuerySetEntry->getOutputFile(),
									"Alle Dateien(*.*)", false);
		ui.output->setText(output);
		activeQuerySetEntry->setOutputFile(output);
	}
}

void SqlReport::on_but_outTemplate_clicked()
{
	if (validQuerySet())
	{
		QString outTemplate = selectFile("Bitte Template auswählen",
										 activeQuerySetEntry->getTemplateFile(),
										 "Templates (*.template);;Alle Dateien(*.*)", true);
		ui.outTemplate->setText(outTemplate);
		activeQuerySetEntry->setTemplateFile(outTemplate);
	}
}

void SqlReport::on_but_outSql_clicked()
{
	if (validQuerySet())
	{
		QString outSql = selectFile("Bitte SQL-Datei auswählen",
									activeQuerySetEntry->getSqlFile(),
									"SQL-Dateien (*.sql);;Alle Dateien(*.*)", true);
		ui.outSql->setText(outSql);
		activeQuerySetEntry->setSqlFile(outSql);
	}
}

void SqlReport::on_but_querySet_clicked()
{
	QString qsName = selectFile(tr("Please seletc QuerySet file"),
								mQuerySet.getQuerySetFileName(),
								tr("XML-Dateien (*.xml);;Alle Dateien(*.*)"), false);
	
	mQuerySet.writeXml("");
	readQuerySet(qsName);
}

void SqlReport::readQuerySet(QString &qsName)
{
	if (qsName.isEmpty()) qsName = "queryset.xml";
	QFile qsFile(qsName);

	if (!qsFile.exists())
	{
		qDebug() << "query file doesn't exists -> write existing data";
		mQuerySet.writeXml(qsName);
	}

	ui.lblQuerySetName->setText(QString("%1 (%2)").arg(qsName).arg(QFileInfo(qsName).absolutePath()));

	if (mQuerySet.readXml(qsName))
	{
		qDebug() << "XML read";
		ui.cbQuerySet->clear();
		qDebug() << "combobox cleared";
		QStringList vL;

		mQuerySet.getNames(vL);
		constructorRunning = true;
		ui.cbQuerySet->insertItems(0, vL);
		qDebug() << "combobox items inserted";
		constructorRunning = false;
		qDebug() << "set active query" << ui.cbQuerySet->itemText(0);
		setActiveQuerySetEntry(ui.cbQuerySet->itemText(0));
		qDebug() << "active query set";
	}
}

void SqlReport::on_but_AddQuerySet_clicked()
{
	bool ok;

	QString newEntryName = QInputDialog::getText(this, "", "new entry name",
										 QLineEdit::Normal, "", &ok);

	QStringList vL;
	mQuerySet.getNames(vL);

	if (!vL.contains(newEntryName))
	{
		if (activeQuerySetEntry != nullptr)
		{
			updateQuerySet();
		}

		QuerySetEntry *tmpQSE = new QuerySetEntry();
		*tmpQSE = *activeQuerySetEntry;
		mQuerySet.insert(newEntryName, tmpQSE);
		activeQuerySetEntry = mQuerySet.getByName(newEntryName);
		ui.cbQuerySet->insertItem(0, newEntryName);
	}
	else
	{
		QMessageBox::information(this, tr("Eintrag existiert"),
								 tr("Ein Eintrag '%1' besteht bereits.").arg(newEntryName) );
	}
}


void SqlReport::on_but_DeleteQuerySet_clicked()
{
	QMessageBox msgBox;

	msgBox.setText("Entfernen des Eintrages?");
	msgBox.setInformativeText("Dateien bleiben erhalten.");
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	msgBox.setDefaultButton(QMessageBox::No);
	int ret = msgBox.exec();

	if (ret == QMessageBox::Yes)
	{
		mQuerySet.remove(activeQuerySetEntry);
		ui.cbQuerySet->removeItem(ui.cbQuerySet->currentIndex());
		ui.cbQuerySet->setCurrentIndex(0);
		setActiveQuerySetEntry(ui.cbQuerySet->itemText(0));
	}
}

void SqlReport::on_cbQuerySet_currentIndexChanged(int aIndex)
{
	if ( !constructorRunning) updateQuerySet();
	setActiveQuerySetEntry(ui.cbQuerySet->itemText(aIndex));
}

void SqlReport::on_btnEditSql_clicked()
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

void SqlReport::on_btnEditTemplate_clicked()
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

void SqlReport::on_btnShowOutput_clicked()
{
	if (!activeQuerySetEntry->getLastOutputFile().isEmpty())
	{
		outputEditor.newFile(activeQuerySetEntry->getLastOutputFile());
		outputEditor.show();
	}
	else
	{
		//! Sonst versuchen wir einen Dateinamen zu erzeugen, wenn
		//! dieser keine dynamsichen Teile enthält.
		if (!activeQuerySetEntry->getOutputFile().contains('$'))
		{
			outputEditor.newFile(getAbsoluteFileName(activeQuerySetEntry->getOutputFile()));
			outputEditor.show();
		}
		else
		{
			QMessageBox::information(this, tr("Keine Datei"),
									 tr("Bitte zuerst eine Ausgabe erzeugen (Start)"));
		}
	}
}

void SqlReport::on_btnShowTables_clicked()
{
	updateQuerySet();
	if (activeQuerySetEntry->connectDatabase())
	{
		QTreeReporter treeReporter;

		treeReporter.setReportRoot(treeModel.invisibleRootItem());
		activeQuerySetEntry->getDatabase().showDatabaseTables(&treeReporter);

		activeQuerySetEntry->getDatabase().closeDatabase();
		if (ui.dockWidget->isHidden()) ui.dockWidget->show();
	}
}

void SqlReport::on_pushButtonExit_clicked()
{
	updateQuerySet();
	mQuerySet.writeXml("");

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
		activeQuerySetEntry->setDbName(ui.database->text());
		activeQuerySetEntry->setDbType(ui.cbDbType->currentText());
		activeQuerySetEntry->setDbPort(ui.leDbPort->text().toInt());
		activeQuerySetEntry->setDbUsername(ui.leDbUser->text());
		activeQuerySetEntry->setDbPassword(ui.leDbPasswd->text());
		activeQuerySetEntry->setDbHost(ui.leDbHost->text());
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

	return result;
}

//! Diese Methode setzt die Werte aus dem aktiven QuerySet in der Oberfläche.
//! Wird keine solcher Indes gefunden, werden alle Felder gelöscht.
void SqlReport::setActiveQuerySetEntry(const QString aIdxName)
{
	activeQuerySetEntry = mQuerySet.getByName(aIdxName);
	if (activeQuerySetEntry != nullptr)
	{
		ui.database->setText(activeQuerySetEntry->getDatabase().getDbName());
		int mc = ui.cbDbType->count();
		for (int i=0; i < mc; ++i)
		{
			if (ui.cbDbType->itemText(i) == activeQuerySetEntry->getDatabase().getDbType())
			{
				ui.cbDbType->setCurrentIndex(i);
				i = mc;
			}
		}
		ui.leDbPort->setText(QString("%1").arg(activeQuerySetEntry->getDatabase().getPort()));
		ui.leDbUser->setText(activeQuerySetEntry->getDatabase().getUsername());
		ui.leDbPasswd->setText(activeQuerySetEntry->getDatabase().getPassword());
		ui.leDbHost->setText(activeQuerySetEntry->getDatabase().getHost());
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
		ui.database->setText("");
		ui.leDbPort->setText("");
		ui.leDbUser->setText("");
		ui.leDbPasswd->setText("");
		ui.leDbHost->setText("");
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

