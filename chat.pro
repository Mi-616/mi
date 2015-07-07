#-------------------------------------------------
#
# Project created by QtCreator 2011-12-19T19:53:56
#
#-------------------------------------------------

QT       += widgets
QT += network
TARGET = chat
TEMPLATE = app

SOURCES += main.cpp\
        widget.cpp \
    tcpclient.cpp \
    tcpserver.cpp \
    dialog.cpp

HEADERS  += widget.h \
    tcpclient.h \
    tcpserver.h \
    dialog.h

FORMS    += widget.ui \
    tcpclient.ui \
    tcpserver.ui \
    dialog.ui

RESOURCES += \
    images.qrc
