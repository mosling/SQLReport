#include "SqlReport.h"

//! The application startup, setting some values and the default INI Format
//! and location. This depends on the system where sqlReport runs, please look
//! at the Qt documentation.
int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	app.setOrganizationName  ("mosling");
	app.setApplicationName   ("SqlReport");
	app.setOrganizationDomain("https://github.com/mosling/SQLReport");
	app.setApplicationVersion("1.2");

	QSettings::setDefaultFormat(QSettings::IniFormat);
	app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

	SqlReport report;
	report.setVisible(true);

	qint32 res = app.exec();
	
	return res;
}
