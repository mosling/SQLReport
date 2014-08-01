#ifndef DBCONNECTION_H
#define DBCONNECTION_H

#include <QString>
#include <QtSql/QSqlDriver>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtXml/QDomDocument>
#include <QtCore/QXmlStreamWriter>
#include "QTreeReporter.h"

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
	QString name;
	QString dbType;
	QString dbName;
	QString host;
	QString username;
	QString password;
	quint32 port;
	
};

#endif // DBCONNECTION_H
