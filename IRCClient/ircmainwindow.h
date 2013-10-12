#ifndef IRCMAINWINDOW_H
#define IRCMAINWINDOW_H

#include "observer.h"
#include <QMainWindow>
#include <QMutex>
#include <QList>
#include <QHash>
#include <QAction>

#include <string>
#include <queue>
#include "textmessage.h"
#include "ircclient.h"

class QPlainTextEdit;
class QLineEdit;
class IRCChannelWidget;
class IRCTabWidget;

using std::string;

class IRCMainWindow : public QMainWindow, public Observer
{
    Q_OBJECT

public:
    explicit IRCMainWindow(QWidget *parent = 0);
    ~IRCMainWindow();
    void notify(Subject *subject);
    // sends a message via IRCClient
    void sendMessage(const QString& msg);

private slots:
    // updates the gui in a threadsafe manner
    void updateGui();
    void partChannel();
signals:
    // notify the GUI that there are updates
    void notified();

private:
    // adds a message that has to be printed to the queue
    void addMsgToQueue(const TextMessage& msg);

    IRCClient *_subject;

    // central widget, the tabbed view
    IRCTabWidget* _tabView;
    // the default 'channel' that the user joins before he joins actual channels
    IRCChannelWidget* _defaultChannel;
    // holds all messages that have to be shown
    queue<TextMessage> _msgsToShow;
    // holds all users that have to be added
    list<string> _newUsrs;
    // holds all users that have to be deleted
    list<string> _leftUsrs;

    // all active channels
    list<Channel*> _activeChannels;
    // holds all channels that have to be added in GUI
    QList<Channel*> _channelsToAdd;
    /* when a new IRCChannelWidget is created we hold its relation subject/observer so that when we get notified that we have to remove a Channel*
     * we can remove the correct IRCChannelWidget */
    QHash<Channel*, IRCChannelWidget*> _map;
    // holds all channels that have to be removed in GUI
    QList<IRCChannelWidget*> _channelsToRemove;
    /* updates the activeChannels list by removing channels that are not in channels and adding channels that are in channels but not in activeChannels
     * we don't just use _activeChannels = getChannels() because that would bring a big overhead of creating IRCChannelWidgets and that would
     * remove previous messages in active channels */
    void updateActiveChannels(list<Channel*> channels);

    // used to check if we have lost connection / established connection.
    bool _conTest;
    // close a given room
    QAction _closeAct;

    QMutex _msgMtx;
    QMutex _leftUsrsMtx;
    QMutex _newUsrsMtx;
    QMutex _channelsToRemoveMtx;
    QMutex _channelsToAddMtx;

};

#endif // IRCMAINWINDOW_H
