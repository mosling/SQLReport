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

//! The application startup, setting some values and the default INI Format
//! and location. 
int main(int argc, char *argv[])
{
	QApplication application(argc, argv);
	application.setOrganizationName  ("mosling");
	application.setApplicationName   ("SqlReport");
	application.setOrganizationDomain("https://github.com/mosling/SQLReport");
	application.setApplicationVersion("1.1");

	QSettings::setDefaultFormat(QSettings::IniFormat);
	application.connect(&application, SIGNAL(lastWindowClosed()), &application, SLOT(quit()));

	SqlReport reporter;
	reporer.setVisible(true);

	qint32 res = application.exec();
	
	return res;
}
