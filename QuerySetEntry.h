#ifndef QUERYSETENTRY_H
#define QUERYSETENTRY_H

#include "DbConnection.h"

#include <QDomNode>

class QuerySetEntry
{

public:
	explicit QuerySetEntry();
	~QuerySetEntry();

	QuerySetEntry& operator=(const QuerySetEntry& rhs);
	bool operator()(const QuerySetEntry *l, const QuerySetEntry *r);

	QString getName() const { return name; }
	void setName(const QString &value);

	QString getDbName() const { return dbname; }
	void setDbName(const QString &value);

	QString getDescr() const;
	void setDescr(const QString &value);

	QString getInputDefines() const;
	void setInputDefines(const QString &value);

	QString getSqlFile() const;
	void setSqlFile(const QString &value);

	QString getTemplateFile() const;
	void setTemplateFile(const QString &value);

	QString getOutputFile() const;
	void setOutputFile(const QString &value);

	QString getLastOutputFile() const;
	void setLastOutputFile(const QString &value);

	bool getBatchrun() const;
	void setBatchrun(bool value);

	bool getWithTimestamp() const;
	void setWithTimestamp(bool value);

	bool getAppendOutput() const;
	void setAppendOutput(bool value);

	bool getOutputUtf8() const;
	void setOutputUtf8(bool value);

	bool getOutputXml() const;
	void setOutputXml(bool value);

    bool getShowFirst() const;
    void setShowFirst(bool value);

	QString getLocale() const;
	void setLocale(const QString &value);

private:
	QString name;
	QString dbname;
	QString descr;
	QString inputDefines;
	QString sqlFile;
	QString templateFile;
	QString outputFile;
	QString lastOutputFile;
	bool batchrun;
	bool withTimestamp;
	bool appendOutput;
	bool outputUtf8;
	bool outputXml;
    bool showFirst;
	QString locale;
};

#endif // QUERYSETENTRY_H
