QT       += core gui

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
    main.cpp \
    mainwindow.cpp \
    settingmanager.cpp \
    updatecontroller.cpp

HEADERS += \
    ../Utility/DumpUtil.h \
    ../Utility/IcrCriticalSection.h \
    ../Utility/ImCharset.h \
    ../Utility/ImPath.h \
    ../Utility/LogBuffer.h \
    ../Utility/LogMacro.h \
    ../Utility/LogUtil.h \
    mainwindow.h \
    settingmanager.h \
    updatecontroller.h

FORMS += \
    mainwindow.ui

INCLUDEPATH += ../Utility

# Enable PDB generation
QMAKE_CFLAGS_RELEASE += /Zi
QMAKE_CXXFLAGS_RELEASE += /Zi
QMAKE_LFLAGS_RELEASE += /DEBUG

# Enable log context
DEFINES += QT_MESSAGELOGCONTEXT
