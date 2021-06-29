#-------------------------------------------------
#
# Project created by QtCreator 2013-02-21T14:50:17
#
#-------------------------------------------------

QT += core gui xml sql printsupport qml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# using the c++11 version to compile our code, supported
# features depends at the used c++ compiler (normally gcc)
greaterThan(QT_MAJOR_VERSION, 4) CONFIG += c++11
!greaterThan(QT_MAJOR_VERSION, 4) QMAKE_CXXFLAGS += -std=c++11

TARGET = sqlReport
TEMPLATE = app
DEFINES *= QT_USE_QSTRINGBUILDER

CONFIG(release, debug|release){
	DEFINES += QT_NO_DEBUG_OUTPUT QT_NO_WARNING_OUTPUT
}

RC_ICONS = sqlreport.ico

SOURCES += main.cpp \
    SqlReportHighlighter.cpp \
    SqlReport.cpp \
    QuerySetEntry.cpp \
    QuerySet.cpp \
    QueryExecutor.cpp \
    QTreeReporter.cpp \
    editwidget.cpp \
    DbConnectionSet.cpp \
    DbConnection.cpp \
    DbConnectionForm.cpp \
    Utility.cpp \
    logmessage.cpp

HEADERS  += \
    SqlReportHighlighter.h \
    SqlReport.h \
    QuerySetEntry.h \
    QuerySet.h \
    QueryExecutor.h \
    QTreeReporter.h \
    EditWidget.h \
    DbConnectionSet.h \
    DbConnection.h \
    DbConnectionForm.h \
    Utility.h \
    logmessage.h

FORMS    += \
    SqlReport.ui \
    editwidget.ui \
    DbConnectionForm.ui

OTHER_FILES += \
    README.md \
    sqlreport.ico

DISTFILES += \
    config/config.xml \
    packages/com.koehlers.sqlreporter/meta/installscript.qs \
    packages/com.koehlers.sqlreporter/meta/license.txt \
    packages/com.koehlers.sqlreporter/meta/package.xml \
    sqlreport.asciidoc
