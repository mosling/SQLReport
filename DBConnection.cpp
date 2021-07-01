#include "DBConnection.h"

#include <QCoreApplication>
#include <QDebug>
#include <QMessageBox>
#include <QInputDialog>
#include <QStringBuilder>
#include <QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlField>
#include <QtSql/QSqlIndex>

DbConnection::DbConnection(QObject *parentObj) :
    QObject(parentObj),
    logger(nullptr),
    name(""),
    dbType(""),
    dbEncoding("ISO-8859-1"), // the internal name for Latin1
    dbName(""),
    dbOptions(""),
    tablePrefix(""),
    host(""),
    username(""),
    password(""),
    port(0),
    passwordSave(false)
{
}

DbConnection::~DbConnection()
{

}

void DbConnection::readXmlNode(const QDomNode &aNode)
{
    QDomNode cn = aNode.firstChild();
    while(!cn.isNull())
    {
        QString ce = cn.toElement().tagName();
        qDebug() << "adding element " << ce;
        QString te = cn.firstChild().toText().data();
        ce = ce.toUpper();
        if(ce == "NAME")   { name = te; }
        if(ce == "DBNAME") { dbName = te; }
        if(ce == "DBENCODING") { dbEncoding = te; }
        if(ce == "DBOPTIONS") { dbOptions = te; }
        if(ce == "PREFIX") { tablePrefix = te; }
        if(ce == "HOST")   { host = te; }
        if(ce == "USER")   { username = te; }
        if(ce == "PASS")   { password = te; }
        if(ce == "TYPE")   { dbType = te; }
        if(ce == "PORT")   { port = te.toInt(); }
        if(ce == "SAVE")   { passwordSave = (te == "yes") ? true : false; }
        cn = cn.nextSibling();
    }
}

void DbConnection::writeXmlNode(QXmlStreamWriter &aStream)
{
    aStream.writeStartElement("Database");
    aStream.writeTextElement("type", dbType);
    aStream.writeTextElement("name", name);
    aStream.writeTextElement("dbencoding", dbEncoding);
    aStream.writeTextElement("prefix", tablePrefix);
    aStream.writeTextElement("host", host);
    aStream.writeTextElement("port", QString("%1").arg(port));
    aStream.writeTextElement("dbname", dbName);
    aStream.writeTextElement("dboptions", dbOptions);
    aStream.writeTextElement("user", username);
    if(passwordSave)
    {
        aStream.writeTextElement("pass", password);
    }
    aStream.writeTextElement("save", passwordSave ? "yes" : "no");
    aStream.writeEndElement();
}

void DbConnection::setName(const QString &value)
{
    name = value;
}

void DbConnection::setDbType(const QString &value)
{
    dbType = value;
}

void DbConnection::setDbName(const QString &value)
{
    dbName = value;
}

void DbConnection::setDbEncoding(const QString &value)
{
    dbEncoding = value;
}

void DbConnection::setDbOptions(const QString &value)
{
    dbOptions = value;
}

void DbConnection::setTablePrefix(const QString &value)
{
    tablePrefix = value;
}

void DbConnection::setHost(const QString &value)
{
    host = value;
}

void DbConnection::setUsername(const QString &value)
{
    username = value;
}

void DbConnection::setPassword(const QString &value)
{
    password = value;
}

void DbConnection::setPort(const quint32 &value)
{
    port = value;
}

void DbConnection::setPasswordSave(const bool value)
{
    passwordSave = value;
}

void DbConnection::showDbError()
{
    QSqlDatabase db = QSqlDatabase::database();

    if(db.lastError().isValid())
    {
        QString str = QByteArray(db.lastError().text().toLocal8Bit());
        QMessageBox::critical(NULL, "DB-Error", str, QMessageBox::Ok);
        logger->errorMsg(str);
    }
}

QString DbConnection::getConnectionName() const
{
    QString vDbName = dbName;

    if(vDbName.endsWith(".mdb") || vDbName.endsWith(".accdb"))
    {
        vDbName = "DRIVER={Microsoft Access Driver (*.mdb, *.accdb)};DSN='';DBQ=" + vDbName;
    }

    return vDbName;
}

bool DbConnection::connectDatabase()
{
    QSqlDatabase db = QSqlDatabase::database();

    qDebug() << "connect database";

    if(!QSqlDatabase::contains() || db.driverName() != dbType)
    {
        if(QSqlDatabase::contains())
        {
            QSqlDatabase::removeDatabase(db.connectionName());
        }
        qDebug() << "adding database type connection " << dbType;
        db = QSqlDatabase::addDatabase(dbType);
    }

    if(!username.isEmpty() && !passwordSave && password.isEmpty())
    {
        // get Password
        bool ok;
        QString pwd = QInputDialog::getText(nullptr, "", "Please input password",
                                            QLineEdit::PasswordEchoOnEdit, "", &ok);
        if(ok)
        {
            setPassword(pwd);
        }
    }
    if(!dbName.isEmpty()) db.setDatabaseName(getConnectionName());
    if(!host.isEmpty()) db.setHostName(host);
    if(port != 0) db.setPort(port);
    if(!username.isEmpty()) db.setUserName(username);
    if(!password.isEmpty()) db.setPassword(password);

    if (!dbOptions.isEmpty()) {
        QStringList ol = dbOptions.split(QLatin1Char('|'), Qt::SkipEmptyParts);
        foreach(QString o, ol)
        {
            db.setConnectOptions(o);
        }
    }

    bool ok = db.open();
    if(ok != true)
    {
        showDbError();
    }

    return ok;
}

void DbConnection::closeDatabase() const
{
    qDebug() << "close database";
    QSqlDatabase db = QSqlDatabase::database();
    if(db.isOpen())
    {
        db.close();
    }
}

QString DbConnection::getFieldString(const QSqlField field) const
{
    return QString("%2 %3 %4,%5 %6 '%7' %8 %9")
      .arg(field.name(), field.metaType().name())
      .arg(field.length())
      .arg(field.precision())
      .arg(field.isNull() ? "NULL" : "",
           field.defaultValue().toString(),
           field.isAutoValue() ? "auto" : "",
           field.requiredStatus() == QSqlField::Required ? "required"
           : (field.requiredStatus() == QSqlField::Optional ? "optional" : ""))
      ;
}

QStringList DbConnection::getForeignKeyList(QString &tableName) const
{
    QStringList fkList;
    QString fkSql("");

    if ("QIBASE" == dbType)
    {
        fkSql = QString(
                    "SELECT rc.RDB$CONSTRAINT_NAME AS constraint_name," \
                    "       s.RDB$FIELD_NAME AS field_name, " \
                    "       i2.RDB$RELATION_NAME AS references_table,"\
                    "       s2.RDB$FIELD_NAME AS references_field"\
                    " FROM RDB$INDEX_SEGMENTS s"\
                    " LEFT JOIN RDB$INDICES i ON i.RDB$INDEX_NAME = s.RDB$INDEX_NAME"\
                    " LEFT JOIN RDB$RELATION_CONSTRAINTS rc ON rc.RDB$INDEX_NAME = s.RDB$INDEX_NAME"\
                    " LEFT JOIN RDB$REF_CONSTRAINTS refc ON rc.RDB$CONSTRAINT_NAME = refc.RDB$CONSTRAINT_NAME"\
                    " LEFT JOIN RDB$RELATION_CONSTRAINTS rc2 ON rc2.RDB$CONSTRAINT_NAME = refc.RDB$CONST_NAME_UQ"\
                    " LEFT JOIN RDB$INDICES i2 ON i2.RDB$INDEX_NAME = rc2.RDB$INDEX_NAME"\
                    " LEFT JOIN RDB$INDEX_SEGMENTS s2 ON i2.RDB$INDEX_NAME = s2.RDB$INDEX_NAME"\
                    " WHERE rc.RDB$CONSTRAINT_TYPE = 'FOREIGN KEY' AND i.RDB$RELATION_NAME = '%1'"\
                    " ORDER BY i.RDB$RELATION_NAME, s.RDB$FIELD_NAME").arg(tableName);
    }


    if (!fkSql.isEmpty())
    {
        QSqlQuery fkQuery;
        if (fkQuery.exec(fkSql))
        {
            QSqlRecord rec = fkQuery.record();
            int numCols = rec.count();

            while (fkQuery.next())
            {
                //get the sql values
                QString fkStr;
                for (int i=0; i<numCols; ++i)
                {
                    fkStr = QString("%1  %2").arg(fkStr.toUtf8(), fkQuery.value(i).toString());
                }

                fkList << QString("%1 | %2 --> %3(%4)")
                          .arg(fkQuery.value(0).toString().trimmed(),
                               fkQuery.value(1).toString().trimmed(),
                               fkQuery.value(2).toString().trimmed(),
                               fkQuery.value(3).toString().trimmed());
            }
        }
        else
        {
            logger->errorMsg(fkQuery.lastError().driverText());
        }
    }
    else
    {
        logger->infoMsg(QString("no foreign key readout method for database type %1").arg(dbType));
    }

    return fkList;
}

void DbConnection::showTableList(QSql::TableType aType, QString aHead, bool withFk, QTreeReporter *treeReporter) const
{
    qDebug() << aHead;
    treeReporter->reportMsg(aHead);
    treeReporter->incReportLevel();

    QSqlDatabase db = QSqlDatabase::database();
    QStringList tableList = db.tables(aType);
    foreach(QString tn, tableList)
    {
        qDebug() << "    " << tn;
        treeReporter->reportMsg(tn);
        treeReporter->incReportLevel();
        QSqlRecord tableRecord = db.record(tn);
        if(!tableRecord.isEmpty())
        {
            // Create a List with entries
            QStringList fieldList;
            for(int c = 0; c < tableRecord.count(); ++c)
            {
                fieldList << getFieldString(tableRecord.field(c));
            }

            fieldList.sort();
            foreach(QString f, fieldList)
            {
                treeReporter->reportMsg(f);
            }


            QSqlIndex index = db.primaryIndex(tn);
            if(!index.isEmpty())
            {
                treeReporter->reportMsg("primary key " + index.name());
                treeReporter->incReportLevel();
                for(int c = 0; c < index.count(); ++c)
                {
                    QSqlField f = index.field(c);
                    treeReporter->reportMsg(QString(" %1: %2")
                                            .arg(c)
                                            .arg(f.name())
                                           );
                }
                treeReporter->decReportLevel();
            }

            if (withFk)
            {
                QStringList foreignKeys = getForeignKeyList(tn);
                if (foreignKeys.size() > 0)
                {
                    treeReporter->reportMsg("foreign keys");
                    treeReporter->incReportLevel();
                    foreach(QString s, foreignKeys)
                    {
                        treeReporter->reportMsg(s);
                    }
                    treeReporter->decReportLevel();
                }
            }
        }
        treeReporter->decReportLevel();
        QCoreApplication::processEvents();

    }
    treeReporter->decReportLevel();
}

void DbConnection::showDatabaseTables(QTreeReporter *tr, bool withFk) const
{
    QSqlDatabase db = QSqlDatabase::database();
    bool b;

    tr->reportMsg("database info <" + dbName + ">");
    tr->incReportLevel();

    QString features[] = {
        "Transactions", "QuerySize", "BLOB", "Unicode",
        "PreparedQueries", "NamedPlaceHolders", "PositionalPlaceHolders",
        "LastInsertId", "BatchOperations", "SimpleLocking",
        "LowPrecisionNumbers", "EventNotification", "FinishQuery",
        "MultipleResultSets", ""
    };

    QString s;

    tr->reportMsg("Capabilities");
    tr->incReportLevel();

    for(int i = 0; !features[i].isEmpty(); ++i)
    {
        s = features[i] + ": ";
        b = db.driver()->hasFeature(static_cast<QSqlDriver::DriverFeature>(i));
        s += b ? "yes" : "no";
        tr->reportMsg(s);
    }
    tr->decReportLevel();

    showTableList(QSql::Tables, "List of Tables", withFk, tr);
    showTableList(QSql::Views, "List of Views", false, tr);
    showTableList(QSql::SystemTables, "List of System Tables", false, tr);
}
