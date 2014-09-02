//! MOONWAVE SYSTEM GMBH
//! copyright 2014

#ifndef DBCONNECTIONSET_H
#define DBCONNECTIONSET_H

#include "DbConnection.h"
#include <QAbstractListModel>
#include <QList>
#include <QString>

class DbConnectionSet : public QAbstractListModel
{
	Q_OBJECT
	Q_CLASSINFO ("author", "St. Koehler")
	Q_CLASSINFO ("company", "com.github.mosling")

public:
	explicit DbConnectionSet(QObject *parentObj = 0);

	void append(DbConnection *dbcon);
	void remove(DbConnection *dbcon);
	void clear();

	void readXmlNode(const QDomNode &aNode);
	void writeXmlNode(QXmlStreamWriter &aStream);

	DbConnection *getByName(const QString &name) const;

	qint32 rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation,
						int role = Qt::DisplayRole) const;

signals:

public slots:

private:
	QList<DbConnection*> dbList;

};

#endif // DBCONNECTIONSET_H
