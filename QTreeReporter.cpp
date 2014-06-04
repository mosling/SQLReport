#include "QTreeReporter.h"

QTreeReporter::QTreeReporter()
	: reportNode(nullptr)
{
}

QTreeReporter::~QTreeReporter()
{
	reportNode = nullptr;
}

//! Setzen des Wurzelknotens des Reports. Damit wird auch der aktuelle
//! Reportknoten gesetzt.
void QTreeReporter::setReportRoot (QStandardItem *aRoot)
{
	reportNode = aRoot;
}

//! ReportLevel um eins erhöhen. In unserem Fall wird das zuletzt erzeugte Item zum
//! neuen Vater für folgenden Einträge.
void QTreeReporter::incReportLevel ()
{
	QStandardItem *tmpItem, *lastReportItem ;
	tmpItem = reportNode;

	if (tmpItem != NULL)
	{
		if (tmpItem->rowCount() == 0)
		{
			lastReportItem = new QStandardItem();
			tmpItem->appendRow(lastReportItem);
		}
		reportNode = tmpItem->child(tmpItem->rowCount()-1);
	}
}

//! Der Vater des aktuellen Knotens wird zum aktuellen Knoten. Aber nur wenn der
//! aktuelle Knoten nicht schon die Wurzel der gesamten Reportes ist. Ohne Report
//! stehen beide Variablen auf NULL. Falls der so ermittelte Vater NULL ist, wird
//! die Wurzel verwendet.
void QTreeReporter::decReportLevel ()
{
	const QStandardItem *parent = reportNode->parent();

	if (reportNode != NULL && parent != NULL)
	{
		reportNode =  reportNode->parent();
	}
}

//! Manchmal möchte man einfach wieder unterhalb der ursprünglicher
//! Reportwurzel beginnen. Diese Methode ermöglicht es.
void QTreeReporter::resetReportLevel ()
{
	while (reportNode->parent() != NULL)
	{
		reportNode = reportNode->parent();
	}
}

//! Diese Methode schreibt eine Zeichenkette in den Reportstrom.
void QTreeReporter::reportMsg (const QString aMsg)
{
	if (reportNode != NULL)
	{
		reportNode->appendRow(new QStandardItem(aMsg));
	}
}

