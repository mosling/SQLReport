#include "DBConnection.h"

#include <QMessageBox>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlField>
#include <QtSql/QSqlIndex>
#include <QDebug>

DBConnection::DBConnection() :
	dbType(""),
	dbName(""),
	host(""),
	username(""),
	password(""),
	port(0)
{
}

DBConnection::~DBConnection()
{

}

void DBConnection::readXmlNode(const QDomNode &aNode)
{
	QDomNode cn = aNode.firstChild();
	while (!cn.isNull())
	{
		QString ce = cn.toElement().tagName();
		QString te = cn.firstChild().toText().data();
		ce = ce.toUpper();
		if (ce == "NAME") { dbName = te; }
		if (ce == "HOST") { host = te; }
		if (ce == "USER") { username = te; }
		if (ce == "PASS") { password = te; }
		if (ce == "TYPE") { dbType = te; }
		if (ce == "PORT") { port = te.toInt(); }
		cn = cn.nextSibling();
	}
}

void DBConnection::writeXmlNode(QXmlStreamWriter &aStream)
{
	aStream.writeStartElement("Database");
	aStream.writeTextElement("type", dbType);
	aStream.writeTextElement("host", host);
	aStream.writeTextElement("port", QString("%1").arg(port));
	aStream.writeTextElement("name", dbName);
	aStream.writeTextElement("user", username);
	aStream.writeTextElement("pass", password);
	aStream.writeEndElement();
}

QString DBConnection::getDbType() const
{
	return dbType;
}

void DBConnection::setDbType(const QString &value)
{
	dbType = value;
}
QString DBConnection::getDbName() const
{
	return dbName;
}

void DBConnection::setDbName(const QString &value)
{
	dbName = value;
}

QString DBConnection::getHost() const
{
	return host;
}

void DBConnection::setHost(const QString &value)
{
	host = value;
}

QString DBConnection::getUsername() const
{
	return username;
}

void DBConnection::setUsername(const QString &value)
{
	username = value;
}

QString DBConnection::getPassword() const
{
	return password;
}

void DBConnection::setPassword(const QString &value)
{
	password = value;
}

quint32 DBConnection::getPort() const
{
	return port;
}

void DBConnection::setPort(const quint32 &value)
{
	port = value;
}

void DBConnection::showDbError() const
{
	QSqlDatabase db = QSqlDatabase::database();

	if (db.lastError().isValid())
	{
		QString err = QString("%1").arg(db.lastError().text());
		QMessageBox::critical(NULL, "DB-Error", err, QMessageBox::Ok);
	}
}

QString DBConnection::getConnectionName() const
{
	QString vDbName = dbName;
	if (vDbName.endsWith(".mdb") || vDbName.endsWith(".accdb"))
	{
		vDbName = "DRIVER={Microsoft Access Driver (*.mdb, *.accdb)};DSN='';DBQ=" + vDbName;
	}

	return vDbName;
}

bool DBConnection::connectDatabase()
{
	QSqlDatabase db = QSqlDatabase::database();

	qDebug() << "connect database";

	if (!QSqlDatabase::contains() || db.driverName() != dbType)
	{
		if (QSqlDatabase::contains())
		{
			QSqlDatabase::removeDatabase(db.connectionName());
		}
		qDebug() << "adding database type connection " << dbType;
		db = QSqlDatabase::addDatabase(dbType);
	}

	if (!dbName.isEmpty()) db.setDatabaseName(getConnectionName());
	if (!host.isEmpty()) db.setHostName(host);
	if (port != 0) db.setPort(port);
	if (!username.isEmpty()) db.setUserName(username);
	if (!password.isEmpty()) db.setPassword(password);

	bool ok = db.open();
	if (ok != true)
	{
		showDbError();
	}

	return ok;
}

void DBConnection::closeDatabase() const
{
	qDebug() << "close database";
	QSqlDatabase db = QSqlDatabase::database();
	if ( db.isOpen() )
	{
		db.close();
	}
}

void DBConnection::showTableList(QSql::TableType aType, QString aHead, QTreeReporter *tr) const
{
	tr->reportMsg(aHead);
	tr->incReportLevel();

	QSqlDatabase db = QSqlDatabase::database();
	QStringList tableList = db.tables(aType);
	foreach (QString tn, tableList)
	{
		tr->reportMsg(tn);
		tr->incReportLevel();
		QSqlRecord tableRecord = db.record(tn);
		if (!tableRecord.isEmpty())
		{
			for (int c=0; c<tableRecord.count(); ++c)
			{
				QSqlField f = tableRecord.field(c);
				tr->reportMsg(QString("%2 %3 %4,%5 %6 '%7' %8 %9")
							  .arg(f.name())
							  .arg(QVariant::typeToName(f.type()))
							  .arg(f.length())
							  .arg(f.precision())
							  .arg(f.isNull()?"NULL":"")
							  .arg(f.defaultValue().toString())
							  .arg(f.isAutoValue()?"serial":"")
							  .arg(f.requiredStatus()==QSqlField::Required?"required":(f.requiredStatus()==QSqlField::Optional?"optional":""))
							  );
			}
			QSqlIndex index = db.primaryIndex(tn);
			if (!index.isEmpty())
			{
				tr->reportMsg("primary key " +index.name());
				tr->incReportLevel();
				for (int c=0; c<index.count(); ++c)
				{
					QSqlField f = index.field(c);
					tr->reportMsg(QString(" %1: %2")
								  .arg(c)
								  .arg(f.name())
								  );
				}
				tr->decReportLevel();
			}
		}
		tr->decReportLevel();
	}
	tr->decReportLevel();
}

void DBConnection::showDatabaseTables(QTreeReporter *tr) const
{
	QSqlDatabase db = QSqlDatabase::database();
	bool b;

	tr->reportMsg("database info <" + dbName + ">");
	tr->incReportLevel();

	QString features[] = {
		"Transactions","QuerySize","BLOB","Unicode",
		"PreparedQueries","NamedPlaceHolders","PositionalPlaceHolders",
		"LastInsertId", "BatchOperations", "SimpleLocking",
		"LowPrecisionNumbers", "EventNotification", "FinishQuery",
		"MultipleResultSets", ""};

	QString s;

	tr->reportMsg("Capabilities");
	tr->incReportLevel();

	for (int i=0; !features[i].isEmpty(); ++i)
	{
		s = features[i]+": ";
		b = db.driver()->hasFeature(static_cast<QSqlDriver::DriverFeature>(i));
		s+=b?"yes":"no";
		tr->reportMsg(s);
	}
	tr->decReportLevel();

	showTableList(QSql::Tables,"List of Tables", tr);
	showTableList(QSql::Views,"List of Views", tr);
	showTableList(QSql::SystemTables,"List of System Tables", tr);
}
