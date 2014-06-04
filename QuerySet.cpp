#include "QuerySet.h"
#include <QFile>
#include <QTextStream>
#include <QXmlStreamWriter>

QuerySet::QuerySet(QObject *parentObj)
	: QObject(parentObj),
	  querySetFileName("")
{
}

QuerySet::~QuerySet()
{
    clear();
}

void QuerySet::getNames(QStringList &aList) const
{
	QList<QString> vL = mQueries.keys();
	foreach( QString str, vL) aList.append(str);
}

bool QuerySet::contains(const QString &aName) const
{
	return mQueries.contains(aName);
}

QuerySetEntry *QuerySet::getByName(QString aName) const
{
	if (mQueries.contains(aName))
	{
		return mQueries[aName];
	}
	
	return NULL;
}

void QuerySet::insert(QString aName, QuerySetEntry *aEntry)
{
    mQueries[aName] = aEntry;
}

void QuerySet::remove(QuerySetEntry *entry)
{
	mQueries.remove(mQueries.key(entry));
}

void QuerySet::clear()
{
	try
	{
		qDeleteAll (mQueries);
		mQueries.clear();
	}
	catch (...) {}
}

bool QuerySet::readXml(QString aFilename)
{
    if ( aFilename == querySetFileName )
        return true;

	QuerySetEntry *vQEntry;
	QString vName;
	QDomDocument doc("SqlQueries");
	QFile file(aFilename);

    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }
    if (!doc.setContent(&file))
    {
		file.close();
        return false;
	}

    querySetFileName = aFilename;

    // bisherige Einträge löschen
    clear();

	// Jetzt das XML-Dokument parsen
	QDomElement docElem = doc.documentElement();

	vName = docElem.nodeName();
	if(!docElem.isNull() && docElem.nodeName().toUpper() == "QUERYSET")
	{
		QDomNode n = docElem.firstChild();
		while(!n.isNull())
		{
			QDomElement e = n.toElement(); 
			if(!e.isNull() && e.tagName().toUpper() == "QUERY")
			{	
				vQEntry = new QuerySetEntry();
				vName = "";
				QDomNode cn = n.firstChild();
				while (!cn.isNull())
				{
					QString ce = cn.toElement().tagName();
					QString te = cn.firstChild().toText().data();
					ce = ce.toUpper();
					if (ce == "DATABASE")     { vQEntry->loadDatabase(cn); }
					if (ce == "DESCR")        { vQEntry->setDescr(te); }
					if (ce == "DEFINES")      { vQEntry->setInputDefines(te); }
					if (ce == "SQL")          { vQEntry->setSqlFile(te); }
					if (ce == "TEMPLATE")     { vQEntry->setTemplateFile(te); }
					if (ce == "OUTPUT")       { vQEntry->setOutputFile(te); }
					if (ce == "NAME")         { vName = te; }
					if (ce == "LOCALE")       { vQEntry->setLocale(te); }
					if (ce == "BATCHRUN")     { vQEntry->setBatchrun((te.toUpper()=="YES"));	}
					if (ce == "USETIMESTAMP") {	vQEntry->setWithTimestamp((te.toUpper()=="YES")); }
					if (ce == "APPENDOUTPUT") { vQEntry->setAppendOutput((te.toUpper()=="YES")); }
					if (ce == "UTF8")         { vQEntry->setOutputUtf8((te.toUpper()=="YES")); }
					if (ce == "ASXML")        { vQEntry->setOutputXml((te.toUpper()=="YES")); }
					cn = cn.nextSibling();
				}
				if (vName != "")
				{
					mQueries[vName] = vQEntry;
				}
				else
				{
					delete vQEntry;
				}
			}
			n = n.nextSibling();
		}
	}

	file.close();
    return true;
}

void QuerySet::writeXml(QString aFileName) const
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
        vStream.writeStartElement("QuerySet");

        QMapIterator<QString, QuerySetEntry* > it(mQueries);
        while (it.hasNext())
        {
            it.next();
            vStream.writeStartElement("Query");
            vStream.writeTextElement("name", it.key());
			it.value()->saveDatabase(vStream);
			vStream.writeTextElement("descr",		 it.value()->getDescr() );
			vStream.writeTextElement("defines",      it.value()->getInputDefines());
			vStream.writeTextElement("sql",			 it.value()->getSqlFile() );
			vStream.writeTextElement("template",	 it.value()->getTemplateFile() );
			vStream.writeTextElement("output",		 it.value()->getOutputFile());
			vStream.writeTextElement("locale",		 it.value()->getLocale());
			vStream.writeTextElement("batchrun",     it.value()->getBatchrun()?"yes":"no");
			vStream.writeTextElement("useTimestamp", it.value()->getWithTimestamp()?"yes":"no");
			vStream.writeTextElement("appendOutput", it.value()->getAppendOutput()?"yes":"no");
			vStream.writeTextElement("utf8",         it.value()->getOutputUtf8()?"yes":"no");
			vStream.writeTextElement("asXml",        it.value()->getOutputXml()?"yes":"no");
            vStream.writeEndElement();
        }
        vStream.writeEndDocument();
        file.close();
    }
}

