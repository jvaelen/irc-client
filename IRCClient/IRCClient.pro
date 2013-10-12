#-------------------------------------------------
#
# Project created by QtCreator 2011-10-27T13:25:56
#
#-------------------------------------------------

QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.6.sdk
CFLAGS+=-nostdinc

QT       += core gui

TARGET = IRCClient
TEMPLATE = app


SOURCES += main.cpp\
        ircclient.cpp \
    subject.cpp \
    observer.cpp \
    socketconnection.cpp \
    protocolhandler.cpp \
    sender.cpp \
    textmessage.cpp \
    receiver.cpp \
    ircmainwindow.cpp \
    ircoutmessage.cpp \
    ircinmessage.cpp \
    irctabwidget.cpp \
    ircchannelwidget.cpp \
    channel.cpp \
    user.cpp \
    dcc.cpp \
    dccwidget.cpp \
    dccsender.cpp \
    dccreceiver.cpp

HEADERS  += ircclient.h \
    subject.h \
    observer.h \
    socketconnection.h \
    protocolhandler.h \
    sender.h \
    textmessage.h \
    receiver.h \
    ircmainwindow.h \
    ircoutmessage.h \
    ircinmessage.h \
    irctabwidget.h \
    ircchannelwidget.h \
    channel.h \
    user.h \
    dcc.h \
    dccwidget.h \
    dccsender.h \
    dccreceiver.h

FORMS    +=

RESOURCES += \
    ircclient.qrc


























































