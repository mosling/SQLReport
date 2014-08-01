#include "QuerySet.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QXmlStreamWriter>

QuerySet::QuerySet(QObject *parentObj)
	: QAbstractListModel(parentObj),
	  querySetFileName(""),
	  lastErrMessage("")
{
}

QuerySet::~QuerySet()
{
	clear();
}

bool QuerySet::contains(const QString &aName) const
{
	return (nullptr != getByName((aName)));
}

QuerySetEntry *QuerySet::getByName(QString aName) const
{
	QListIterator<QuerySetEntry* > it(mQueries);
	while (it.hasNext())
	{
		QuerySetEntry *qse = it.next();
		if (qse->getName() == aName) return qse;
	}
	return nullptr;
}

//! add a new entry to the model
void QuerySet::append(QuerySetEntry *aEntry)
{
	beginInsertRows(QModelIndex(), 0, 0);
	mQueries.append(aEntry);
	endInsertRows();
}

void QuerySet::remove(QuerySetEntry *entry)
{
	beginRemoveRows(QModelIndex(), 0, 0);
	mQueries.removeOne(entry);
	endRemoveRows();

	delete entry;
}

void QuerySet::clear()
{
	foreach (QuerySetEntry *qse, mQueries)
	{
		remove(qse);
	}
	mQueries.clear();
}

//! Read the XML file
bool QuerySet::readXml(QString aFilename, DbConnectionSet &dbSet)
{
	lastErrMessage = "";

	if ( aFilename == querySetFileName )
		return true;

	QuerySetEntry *vQEntry;
	QString vName;
	QDomDocument doc("SqlQueries");
	QFile file(aFilename);

	if (!file.open(QIODevice::ReadOnly))
	{
		lastErrMessage = tr("can't open file '%1'").arg(aFilename);
		return false;
	}

	QString errMsg;
	qint32 errLine;
	qint32 errColumn;
	if (!doc.setContent(&file, &errMsg, &errLine, &errColumn))
	{
		lastErrMessage = tr ("error set XML content at line %1:%2 (%3)")
						 .arg(errLine)
						 .arg(errColumn)
						 .arg(errMsg);
		file.close();
		return false;
	}

	querySetFileName = aFilename;

	// bisherige Einträge löschen
	clear();
	dbSet.clear();

	// Jetzt das XML-Dokument parsen
	QDomElement docElem = doc.documentElement();

	vName = docElem.nodeName();
	qDebug() << "reading node " << vName;

	if(!docElem.isNull() && docElem.nodeName().toUpper() == "SQLREPORT")
	{
		QDomNode p = docElem.firstChild();

		int cnt = 5;
		while (!p.isNull() && cnt-- > 0)
		{
			qDebug() << "sqlreport child " << p.nodeName();

			if (p.nodeName().toUpper() == "QUERYSET")
			{
				QDomNode n = p.firstChild();
				qDebug() << "queryset node " << p.nodeName();

				while(!n.isNull())
				{
					QDomElement e = n.toElement();
					if(!e.isNull() && e.tagName().toUpper() == "QUERY")
					{
						vQEntry = new QuerySetEntry();
						QDomNode cn = n.firstChild();
						while (!cn.isNull())
						{
							QString ce = cn.toElement().tagName().toUpper();
							QString te = cn.firstChild().toText().data();
							if (ce == "DBNAME")       { vQEntry->setDbName(te); }
							if (ce == "DESCR")        { vQEntry->setDescr(te); }
							if (ce == "DEFINES")      { vQEntry->setInputDefines(te); }
							if (ce == "SQL")          { vQEntry->setSqlFile(te); }
							if (ce == "TEMPLATE")     { vQEntry->setTemplateFile(te); }
							if (ce == "OUTPUT")       { vQEntry->setOutputFile(te); }
							if (ce == "NAME")         { vQEntry->setName(te); }
							if (ce == "LOCALE")       { vQEntry->setLocale(te); }
							if (ce == "BATCHRUN")     { vQEntry->setBatchrun((te.toUpper()=="YES"));	}
							if (ce == "USETIMESTAMP") {	vQEntry->setWithTimestamp((te.toUpper()=="YES")); }
							if (ce == "APPENDOUTPUT") { vQEntry->setAppendOutput((te.toUpper()=="YES")); }
							if (ce == "UTF8")         { vQEntry->setOutputUtf8((te.toUpper()=="YES")); }
							if (ce == "ASXML")        { vQEntry->setOutputXml((te.toUpper()=="YES")); }
							cn = cn.nextSibling();
						}
						if ( !vQEntry->getName().isEmpty() )
						{
							append(vQEntry);
						}
						else
						{
							delete vQEntry;
						}
					}
					n = n.nextSibling();
				}
			}
			else if ("DATABASESET" == p.nodeName().toUpper())
			{
				qDebug() << "reading database set";
				dbSet.readXmlNode(p);
			}

			p = p.nextSibling();
		}
	}

	file.close();
	return true;
}

void QuerySet::writeXml(QString aFileName, DbConnectionSet &dbSet) const
{
	QString localFileName = aFileName;

	if (localFileName.isEmpty()) localFileName = querySetFileName;

	if (!localFileName.isEmpty())
	{
		QFile file(localFileName);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			return;
		}
		QXmlStreamWriter vStream(&file);

		vStream.setAutoFormatting(true);
		vStream.writeStartDocument("1.0");

		vStream.writeStartElement("SqlReport");
		vStream.writeStartElement("DatabaseSet");
		dbSet.writeXmlNode(vStream);
		vStream.writeEndElement();

		vStream.writeStartElement("QuerySet");
		QListIterator<QuerySetEntry* > it(mQueries);
		while (it.hasNext())
		{
			QuerySetEntry *qse = it.next();
			vStream.writeStartElement("Query");
			vStream.writeTextElement("name",         qse->getName() );
			vStream.writeTextElement("dbname",		 qse->getDbName() );
			vStream.writeTextElement("descr",		 qse->getDescr() );
			vStream.writeTextElement("defines",      qse->getInputDefines());
			vStream.writeTextElement("sql",			 qse->getSqlFile() );
			vStream.writeTextElement("template",	 qse->getTemplateFile() );
			vStream.writeTextElement("output",		 qse->getOutputFile());
			vStream.writeTextElement("locale",		 qse->getLocale());
			vStream.writeTextElement("batchrun",     qse->getBatchrun()?"yes":"no");
			vStream.writeTextElement("useTimestamp", qse->getWithTimestamp()?"yes":"no");
			vStream.writeTextElement("appendOutput", qse->getAppendOutput()?"yes":"no");
			vStream.writeTextElement("utf8",         qse->getOutputUtf8()?"yes":"no");
			vStream.writeTextElement("asXml",        qse->getOutputXml()?"yes":"no");
			vStream.writeEndElement();
		}
		vStream.writeEndDocument();
		file.close();
	}
}

qint32 QuerySet::rowCount(const QModelIndex &) const
{
	return mQueries.count();
}

QVariant QuerySet::data(const QModelIndex &index, int role) const
{
	if (!index.isValid()) return QVariant();

	if (index.row() >= mQueries.count()) return QVariant();

	if (role == Qt::DisplayRole)
	{
		return mQueries[index.row()]->getName();
	}
	else
	{
		return QVariant();
	}
}

QVariant QuerySet::headerData(int section,
									 Qt::Orientation orientation,
									 int role) const
{
	Q_UNUSED(section)
	Q_UNUSED(orientation)

	if (Qt::DisplayRole != role) return QVariant();
	return ("Query");
}
