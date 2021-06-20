SQLReport
=========

A simple template based report generator using a SQL database as data source and 
normal SQL select statements as datasource.

To compile this project you need the current Qt libraries and the Qt-Creator.
You found this at [QT-project](https://www.qt.io/).

The documentation is available at [Gitbub-Wiki](https://github.com/mosling/SQLReport/wiki)

There is a sqlite database chinook [Chinook](https://chinookdatabase.codeplex.com/) which is used for the example templates.
You can download your own copy or use the stored database from the binaries directory.

Build Additional SqlDriver
==========================

  cmake -G"Ninja" C:/Qt\6.1.0\Src\qtbase\src\plugins\sqldrivers \
  -DCMAKE_INSTALL_PREFIX=C:\Qt\6.1.0\mingw81_64 -DInterbase_INCLUDE_DIR="C:\Program Files\Firebird\include" \
  -DInterbase_LIBRARY="C:\Program Files\Firebird\lib\fbclient_ms.lib" -DCMAKE_BUILD_TYPE=Release
  

