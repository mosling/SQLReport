#-------------------------------------------------
#
# Project created by QtCreator 2013-02-21T14:50:17
#
#-------------------------------------------------

QT       += core gui xml sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = sqlReport
TEMPLATE = app

include (../../moonwave.pri)

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
    DbConnectionForm.cpp

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
    DbConnectionForm.h

FORMS    += \
    SqlReport.ui \
    editwidget.ui \
    DbConnectionForm.ui
