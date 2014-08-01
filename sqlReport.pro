#-------------------------------------------------
#
# Project created by QtCreator 2013-02-21T14:50:17
#
#-------------------------------------------------

QT       += core gui xml sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = sqlReport
TEMPLATE = app

SOURCES += main.cpp \
    SqlReportHighlighter.cpp \
    SqlReport.cpp \
    QuerySetEntry.cpp \
    QuerySet.cpp \
    QueryExecutor.cpp \
    QTreeReporter.cpp \
    editwidget.cpp \
    DBConnection.cpp

HEADERS  += \
    SqlReportHighlighter.h \
    SqlReport.h \
    QuerySetEntry.h \
    QuerySet.h \
    QueryExecutor.h \
    QTreeReporter.h \
    EditWidget.h \
    DBConnection.h

FORMS    += \
    SqlReport.ui \
    editwidget.ui
