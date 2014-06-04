#ifndef QUERYSETENTRY_H
#define QUERYSETENTRY_H

#include "DBConnection.h"

#include <QDomNode>

class QuerySetEntry
{

public:
	explicit QuerySetEntry();
	~QuerySetEntry();

	QuerySetEntry& operator=(const QuerySetEntry& rhs);

	//! Database interface
	const DBConnection &getDatabase() const { return database; }
	void setDbName(const QString &s) { database.setDbName(s); }
	void setDbType(const QString &s) { database.setDbType(s); }
	void setDbHost(const QString &s) { database.setHost(s); }
	void setDbPort(const quint32 p) { database.setPort(p); }
	void setDbUsername(const QString &s) { database.setUsername(s); }
	void setDbPassword(const QString &s) { database.setPassword(s); }
	bool connectDatabase() { return database.connectDatabase(); }
	void loadDatabase(const QDomNode &node) { database.readXmlNode(node); }
	void saveDatabase(QXmlStreamWriter &w) { database.writeXmlNode(w); }

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

	QString getLocale() const;
	void setLocale(const QString &value);

private:
	DBConnection database;
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
	QString locale;
};

#endif // QUERYSETENTRY_H
