#-------------------------------------------------
#
# Project created by QtCreator 2013-02-21T14:50:17
#
#-------------------------------------------------

QT       += core gui xml sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# using the c++11 version to compile our code, supported
# features depends at the used c++ compiler (normally gcc)
greaterThan(QT_MAJOR_VERSION, 4) CONFIG += c++11
!greaterThan(QT_MAJOR_VERSION, 4) QMAKE_CXXFLAGS += -std=c++11

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

OTHER_FILES += \
    README.md
