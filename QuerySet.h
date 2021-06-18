#ifndef QUERYSET_H
#define QUERYSET_H

#include <QObject>
#include <QStringList>
#include <QList>
#include <QDomDocument>
#include <QAbstractListModel>

#include "DbConnectionSet.h"
#include "QuerySetEntry.h"

class QuerySet : public QAbstractListModel
{
	Q_OBJECT
	Q_CLASSINFO ("author", "St. Koehler")
	Q_CLASSINFO ("company", "com.github.mosling")

public:
	explicit QuerySet(QObject *parentObj = NULL);
	~QuerySet();

	bool readXml(QString aFilename, DbConnectionSet &dbSet);
	void writeXml(QString aFileName, DbConnectionSet &dbSet) const;
	void getNames(QStringList &aList) const;
	bool contains(const QString &aName) const;
	QuerySetEntry *getByName(QString aName) const;
    QuerySetEntry *getShowFirst() const;
	void append(QuerySetEntry *aEntry);
	void remove(QuerySetEntry *entry);
	void clear();
	QString getQuerySetFileName() const { return querySetFileName; }
	QString getLastError() const { return lastErrMessage; }

    void setShowFirst(QuerySetEntry *entry, bool b);
	qint32 rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation,
						int role = Qt::DisplayRole) const;
private:

	QList<QuerySetEntry* > mQueries;
    QString querySetFileName;
	QString lastErrMessage;
};

#endif // QUERYSET_H
