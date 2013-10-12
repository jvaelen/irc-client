#include <QtGui/QApplication>
#include "ircclient.h"
#include "ircmainwindow.h"
#include "socketconnection.h"

#define RUNTESTCODE 1

int main(int argc, char *argv[])
{
    // Belgian timezone
    TextMessage::setGMTOffset(1);
    QApplication a(argc, argv);
    IRCMainWindow w;
    w.show();
    IRCClient testHandler;
    testHandler.registerObserver(&w);
    testHandler.notifyObservers();
#ifdef RUNTESTCODE
    // testing code
    testHandler.setNick("Baloenu");
    testHandler.setRealName("Uhasselt student");

    testHandler.connect("127.0.0.1", "6667");
#endif
    return a.exec();
}
