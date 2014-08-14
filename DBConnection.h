#ifndef DBCONNECTION_H
#define DBCONNECTION_H

#include <QString>
#include <QtSql/QSqlDriver>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtXml/QDomDocument>
#include <QtCore/QXmlStreamWriter>
#include "QTreeReporter.h"

//! This class holds the information for a database connection and has some methods to
//! connect the database, react at errors and show the structure of the database in a
//! QTreeReporter.
class DbConnection : public QObject
{

	Q_OBJECT

public:
	explicit DbConnection(QObject *parentObj = nullptr);
	virtual ~DbConnection();

	void showDbError() const;
	QString getConnectionName() const;
	bool connectDatabase();
	void closeDatabase() const;
	void showTableList(QSql::TableType aType, QString aHead, QTreeReporter *tr) const;
	void showDatabaseTables(QTreeReporter *tr) const;

	void readXmlNode(const QDomNode &aNode);
	void writeXmlNode(QXmlStreamWriter &aStream);

	QString getName() const { return name; }
	void setName(const QString &value);

	QString getDbType() const {return dbType; }
	void setDbType(const QString &value);

	QString getDbName() const {return dbName; }
	void setDbName(const QString &value);

	QString getHost() const {return host; }
	void setHost(const QString &value);

	QString getUsername() const {return username; }
	void setUsername(const QString &value);

	QString getPassword() const {return password; }
	void setPassword(const QString &value);

	quint32 getPort() const {return port; }
	void setPort(const quint32 &value);

	bool getPasswordSave() const {return passwordSave; }
	void setPasswordSave(const bool value);

private:
	QString name;
	QString dbType;
	QString dbName;
	QString host;
	QString username;
	QString password;
	quint32 port;
	bool passwordSave;
	
};

#endif // DBCONNECTION_H
