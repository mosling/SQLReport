#include "DbConnectionSet.h"
#include <QDebug>

DbConnectionSet::DbConnectionSet(QObject *parentObj) :
    QAbstractListModel(parentObj)
{
}

void DbConnectionSet::append(DbConnection *dbcon)
{
    beginInsertRows(QModelIndex(), 0, 0);
    dbList.append(dbcon);
    endInsertRows();
}

void DbConnectionSet::remove(DbConnection *dbcon)
{
    beginRemoveRows(QModelIndex(), 0, 0);
    dbList.removeOne(dbcon);
    endRemoveRows();

    delete dbcon;
}

void DbConnectionSet::clear()
{
    foreach(DbConnection *dbc, dbList)
    {
        remove(dbc);
    }
    dbList.clear();
}

//! Reading the DatabaseEntry part of the QuerySet file. This contains a number
//! of database entries, every with a name which is used to store it in the map.
//! \param aNode is the DATABASESET node
void DbConnectionSet::readXmlNode(const QDomNode &aNode)
{
    qDebug() << "connection set reading " << aNode.nodeName();

    QDomNode n = aNode.firstChild();
    while(!n.isNull())
    {
        qDebug() << "first child " << n.nodeName();

        if(n.nodeName().toUpper() == "DATABASE")
        {
            DbConnection *dbCon = new DbConnection(this);
            dbCon->readXmlNode(n);
            append(dbCon);
        }
        n = n.nextSibling();
    }
}

void DbConnectionSet::writeXmlNode(QXmlStreamWriter &aStream)
{
    foreach(DbConnection *dbc, dbList)
    {
        dbc->writeXmlNode(aStream);
    }
}

//! Return the DbConnection object that is connected to
//! the name.
DbConnection *DbConnectionSet::getByName(const QString &name) const
{
    foreach(DbConnection *dbc, dbList)
    {
        if(name == dbc->getName()) return dbc;
    }

    return nullptr;
}

qint32 DbConnectionSet::rowCount(const QModelIndex &) const
{
    return dbList.count();
}

QVariant DbConnectionSet::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) return QVariant();

    if(index.row() >= dbList.count()) return QVariant();

    if(role == Qt::DisplayRole)
    {
        return dbList[index.row()]->getName();
    }
    else
    {
        return QVariant();
    }
}

QVariant DbConnectionSet::headerData(int section,
                                     Qt::Orientation orientation,
                                     int role) const
{
    Q_UNUSED(section)
    Q_UNUSED(orientation)

    if(Qt::DisplayRole != role) return QVariant();
    return ("Database");
}
