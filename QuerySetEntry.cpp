#include "QuerySetEntry.h"
#include "QDebug"

QuerySetEntry::QuerySetEntry() :
	name(""),
	dbname(""),
	descr(""),
	inputDefines(""),
	sqlFile(""),
	templateFile(""),
	outputFile(""),
	lastOutputFile(""),
	batchrun(false),
	withTimestamp(true),
	appendOutput(false),
	outputUtf8(false),
	outputXml(false),
    showFirst(false)
{
}

QuerySetEntry::~QuerySetEntry()
{

}

QuerySetEntry &QuerySetEntry::operator=(const QuerySetEntry &rhs)
{
	if (this != &rhs)
	{
		name          = rhs.name;
		dbname        = rhs.dbname;
		descr         = rhs.descr;
		inputDefines  = rhs.inputDefines;
		sqlFile       = rhs.sqlFile;
		templateFile  = rhs.templateFile;
		outputFile    = rhs.outputFile;
		lastOutputFile= "";
		batchrun      = rhs.batchrun;
		withTimestamp = rhs.withTimestamp;
		outputUtf8    = rhs.outputUtf8;
		outputXml     = rhs.outputXml;
		appendOutput  = rhs.appendOutput;
        showFirst     = false;
	}
	return *this;
}

//! Comparsion of two entries is done by the comparsion
//! of its names.
bool QuerySetEntry::operator()(const QuerySetEntry *l, const QuerySetEntry *r)
{
	return (l->getName() < r->getName());
}

void QuerySetEntry::setName(const QString &value)
{
	name = value;
}

void QuerySetEntry::setDbName(const QString &value)
{
	dbname = value;
}

QString QuerySetEntry::getDescr() const
{
	return descr;
}

void QuerySetEntry::setDescr(const QString &value)
{
	descr = value;
}

QString QuerySetEntry::getInputDefines() const
{
	return inputDefines;
}

void QuerySetEntry::setInputDefines(const QString &value)
{
	inputDefines = value;
}

QString QuerySetEntry::getSqlFile() const
{
	return sqlFile;
}

void QuerySetEntry::setSqlFile(const QString &value)
{
	sqlFile = value;
}

QString QuerySetEntry::getTemplateFile() const
{
	return templateFile;
}

void QuerySetEntry::setTemplateFile(const QString &value)
{
	templateFile = value;
}

QString QuerySetEntry::getOutputFile() const
{
	return outputFile;
}

void QuerySetEntry::setOutputFile(const QString &value)
{
	outputFile = value;
}

QString QuerySetEntry::getLastOutputFile() const
{
	return lastOutputFile;
}

void QuerySetEntry::setLastOutputFile(const QString &value)
{
	lastOutputFile = value;
}

bool QuerySetEntry::getBatchrun() const
{
	return batchrun;
}

void QuerySetEntry::setBatchrun(bool value)
{
	batchrun = value;
}

bool QuerySetEntry::getWithTimestamp() const
{
	return withTimestamp;
}

void QuerySetEntry::setWithTimestamp(bool value)
{
	withTimestamp = value;
}

bool QuerySetEntry::getAppendOutput() const
{
	return appendOutput;
}

void QuerySetEntry::setAppendOutput(bool value)
{
	appendOutput = value;
}

bool QuerySetEntry::getOutputUtf8() const
{
	return outputUtf8;
}

void QuerySetEntry::setOutputUtf8(bool value)
{
	outputUtf8 = value;
}

bool QuerySetEntry::getOutputXml() const
{
	return outputXml;
}

void QuerySetEntry::setOutputXml(bool value)
{
	outputXml = value;
}

bool QuerySetEntry::getShowFirst() const
{
    return showFirst;
}

void QuerySetEntry::setShowFirst(bool value)
{
    showFirst = value;
}
