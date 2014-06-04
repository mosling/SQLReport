#ifndef QTreeReporter_H
#define QTreeReporter_H

#include <QStandardItem>

//! Diese Klasse füllt einen QTreeView und erzeugt für
//! jede Meldungskategorie einen eigenen Zweig.
class QTreeReporter
{

public:
	QTreeReporter();
	~QTreeReporter();
	
	virtual void setReportRoot (QStandardItem *aRoot);
	virtual void incReportLevel ();
	virtual void decReportLevel ();
	virtual void resetReportLevel ();
	virtual void reportMsg (const QString aMsg);

private:
	QStandardItem *reportNode;
};

#endif // QTreeReporter_H
