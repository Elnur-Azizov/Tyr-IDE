QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# İSİM BURADA 🔥
TARGET = TyrIDE

SOURCES += \
    cevirici.cpp \
    dilmotoru.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    cevirici.h \
    dilmotoru.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

# ICON 🔥
RC_ICONS = favicon.ico

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
