QT       += core gui network webenginewidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../Utility/DumpUtil.cpp \
    ../Utility/IcrCriticalSection.cpp \
    ../Utility/ImCharset.cpp \
    ../Utility/ImPath.cpp \
    ../Utility/LogBuffer.cpp \
    ../Utility/LogUtil.cpp \
    ../Utility/SyncHttpClient.cpp \
    browserwindow.cpp \
    collectcontroller.cpp \
    jscodemanager.cpp \
    logincontroller.cpp \
    main.cpp \
    mainwindow.cpp \
    qcompressor.cpp \
    settingmanager.cpp \
    statusmanager.cpp \
    uiutil.cpp \
    zhaobiaohttpclient.cpp

HEADERS += \
    ../Utility/DumpUtil.h \
    ../Utility/IcrCriticalSection.h \
    ../Utility/ImCharset.h \
    ../Utility/ImPath.h \
    ../Utility/LogBuffer.h \
    ../Utility/LogMacro.h \
    ../Utility/LogUtil.h \
    ../Utility/SyncHttpClient.h \
    browserwindow.h \
    collectcontroller.h \
    datamodel.h \
    jscodemanager.h \
    logincontroller.h \
    mainwindow.h \
    qcompressor.h \
    settingmanager.h \
    statusmanager.h \
    uiutil.h \
    zhaobiaohttpclient.h

FORMS += \
    browserwindow.ui \
    mainwindow.ui

INCLUDEPATH += ../Utility

# QXlsx
include(../QXlsx/QXlsx.pri)
INCLUDEPATH += ../QXlsx/header/

# Enable PDB generation
QMAKE_CFLAGS_RELEASE += /Zi
QMAKE_CXXFLAGS_RELEASE += /Zi
QMAKE_LFLAGS_RELEASE += /DEBUG

# Enable log context
DEFINES += QT_MESSAGELOGCONTEXT

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../QGumboParser/QGumboParser/release/ -lQGumboParser
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../QGumboParser/QGumboParser/debug/ -lQGumboParser

INCLUDEPATH += $$PWD/../QGumboParser/QGumboParser
DEPENDPATH += $$PWD/../QGumboParser/QGumboParser

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../QGumboParser/QGumboParser/release/libQGumboParser.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../QGumboParser/QGumboParser/debug/libQGumboParser.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../QGumboParser/QGumboParser/release/QGumboParser.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../QGumboParser/QGumboParser/debug/QGumboParser.lib
