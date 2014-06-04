#ifndef DBCONNECTION_H
#define DBCONNECTION_H

#include <QString>
#include <QtSql/QSqlDriver>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtXml/QDomDocument>
#include <QtCore/QXmlStreamWriter>
#include "QTreeReporter.h"

class DBConnection
{

public:
	DBConnection();
	~DBConnection();

	void showDbError() const;
	QString getConnectionName() const;
	bool connectDatabase();
	void closeDatabase() const;
	void showTableList(QSql::TableType aType, QString aHead, QTreeReporter *tr) const;
	void showDatabaseTables(QTreeReporter *tr) const;

	void readXmlNode(const QDomNode &aNode);
	void writeXmlNode(QXmlStreamWriter &aStream);

	QString getDbType() const;
	void setDbType(const QString &value);

	QString getDbName() const;
	void setDbName(const QString &value);

	QString getHost() const;
	void setHost(const QString &value);

	QString getUsername() const;
	void setUsername(const QString &value);

	QString getPassword() const;
	void setPassword(const QString &value);

	quint32 getPort() const;
	void setPort(const quint32 &value);

private:
	QString dbType;
	QString dbName;
	QString host;
	QString username;
	QString password;
	quint32 port;
	
};

#endif // DBCONNECTION_H
