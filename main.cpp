#include "SqlReport.h"
#include "QuerySet.h"
#include <qtgui>
#include <QtSql/QtSql>
#include <qdebug>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlQuery>
#include <string>
#include <stdio.h>
#include <ctime>

#include <windows.h>
#include <wincon.h>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setOrganizationName("sksoft");
	a.setOrganizationDomain("sksoft.com");
	a.setApplicationName("SqlReport");
	a.setApplicationVersion("1.0");

	QSettings::setDefaultFormat(QSettings::IniFormat);

	SqlReport w;
	a.connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
	w.setVisible(true);
	a.exec();
	
	return 1;
}
