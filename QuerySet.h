#ifndef QUERYSET_H
#define QUERYSET_H

#include <QObject>
#include <QStringList>
#include <QMap>
#include <QtXML/QDomDocument>

#include "QuerySetEntry.h"

class QuerySet : public QObject
{
	Q_OBJECT

public:
	explicit QuerySet(QObject *parentObj = NULL);
	~QuerySet();

    bool readXml(QString aFilename);
	void writeXml(QString aFileName) const;
	void getNames(QStringList &aList) const;
	bool contains(const QString &aName) const;
	QuerySetEntry *getByName(QString aName) const;
	void remove(QuerySetEntry *entry);
	void insert(QString aName, QuerySetEntry *aEntry);
	QString getQuerySetFileName() const { return querySetFileName; }

private:
	void clear();

	QMap<QString, QuerySetEntry* > mQueries;
    QString querySetFileName;
};

#endif // QUERYSET_H
