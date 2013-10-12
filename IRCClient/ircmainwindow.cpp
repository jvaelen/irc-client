#include "ircmainwindow.h"
#include "ircchannelwidget.h"
#include "ircclient.h"
#include "irctabwidget.h"
#include "channel.h"
#include <QGridLayout>
#include <QPlainTextEdit>
#include <QList>
#include <QDebug>

IRCMainWindow::IRCMainWindow(QWidget *parent) :
    QMainWindow(parent), _closeAct(this)
{
    _subject = 0;
    _defaultChannel = new IRCChannelWidget(this);
    _defaultChannel->removeUserList();
    _tabView = new IRCTabWidget(this);
    _conTest = false;

    setCentralWidget(_tabView);
    // add the default channel to start with
    _tabView->addChannel(_defaultChannel, "Status");
    setWindowTitle("Baloen IRC Client");
    connect(this, SIGNAL(notified()), this, SLOT(updateGui()));
    resize(900,600);


    /* close shortcut (sends /part message, nothing more, the channel is cleaned up
        afterwards after the server has acknoledged the part) */
    _closeAct.setShortcut(QKeySequence::Close);
    addAction(&_closeAct);
    connect(&_closeAct, SIGNAL(triggered()), this, SLOT(partChannel()));
}

IRCMainWindow::~IRCMainWindow()
{
    delete _defaultChannel;
}

void IRCMainWindow::partChannel()
{
    IRCChannelWidget* temp = _tabView->getCurrentChannelWidget();
    if (temp) // if there is a widget selected
        temp->sendPart();
}

void IRCMainWindow::notify(Subject *subject)
{
    // the first time IRCClient gets notified by his subject, his _subject pointer is zero and should be set to the concrete subject
    if (_subject == 0) {
        _subject = dynamic_cast<IRCClient*>(subject);
        _defaultChannel->setClient(_subject);
    }
    else {
        /* IRCMainWindow is the observer of IRCClient. It gets notified when:
         *  1. the connection is set up (so the observer knows the subject)
         *  2. when the user joins a new channel
         *  3. when there are messages to show in the default channel
         *
         * You can *ONLY* update the GUI via slot/signal system, so we store the information that we want to show in temp variables and scheduele
         * a GUI update by connecting a signal that is called at the end of the notify with a slot that actually does the real update.
         * We use a mutex because updateGUI can be called from another thread (the main thread) than notify() (from the reciever thread). */

        queue<TextMessage> newMsgs = _subject->getLastMsgs();
        while (!newMsgs.empty()) {
            addMsgToQueue(newMsgs.front());
            newMsgs.pop();
        }

        list<Channel*> channels = _subject->getChannels();
        updateActiveChannels(channels);

    }

    // for efficiency
    bool isCon =_subject->isConnected();

    // if _conText and isCon are different
    if (_conTest ^ isCon) {
        if (!isCon && _conTest)
            addMsgToQueue(TextMessage("", "Connection lost.", TextMessage::INFO));
        else
            addMsgToQueue(TextMessage("", "Connection established.", TextMessage::INFO));
        _conTest = isCon;
    }

    list<IRCClient::FileRecvReq> temp = _subject->getFileReqs();
    if (!temp.empty())
        _defaultChannel->setFileReq(temp);
    // trigger signal
    notified();
}


void IRCMainWindow::addMsgToQueue(const TextMessage& msg)
{
    /* we have to use a mutex because notify() gets called from the Reciever thread and updateGUI() gets schedueled in the main thread
     * because of the signal/slot connection. This means that the gui could access the queue at the same time it gets altered because of notify */
    _msgMtx.lock();
    _msgsToShow.push(msg);
    _msgMtx.unlock();
}


void IRCMainWindow::updateActiveChannels(list<Channel*> channels)
{
    QList<Channel*> channels_q = QList<Channel*>::fromStdList(channels);
    QList<Channel*> activeChannels = QList<Channel*>::fromStdList(_activeChannels);


    // remove channels that are not in channels_q but are in activeChannels
    for (QList<Channel*>::iterator it = activeChannels.begin(); it != activeChannels.end(); ++it) {
        if ( channels_q.indexOf((*it)) == -1 ) {
            _activeChannels.remove(*it);
            _channelsToRemoveMtx.lock();
            // if we have to remove a channel, it means we once had to create it so it exist in our _map
            _channelsToRemove.push_back(_map.value(*it));
            _channelsToRemoveMtx.unlock();
        }
    }


    // add channels that are not in the activeList but are in channels_q
    for (QList<Channel*>::iterator it = channels_q.begin(); it != channels_q.end(); ++it) {
        if ( activeChannels.indexOf((*it)) == -1 ) {
            _activeChannels.push_back(*it);
            _channelsToAddMtx.lock();
            _channelsToAdd.push_back(*it);
            _channelsToAddMtx.unlock();
        }
    }

}


void IRCMainWindow::updateGui()
{
    // show all messages on the screen
    _msgMtx.lock();
    while (!_msgsToShow.empty()) {
        _defaultChannel->showMessage(_msgsToShow.front());
        _msgsToShow.pop();
    }
    _msgMtx.unlock();

    // update user list
    _newUsrsMtx.lock();
    for (list<string>::iterator it = _newUsrs.begin(); it != _newUsrs.end(); ++it) {
        _defaultChannel->addUser(*it);
    }
    _newUsrsMtx.unlock();
    _leftUsrsMtx.lock();
    for (list<string>::iterator it = _leftUsrs.begin(); it != _leftUsrs.end(); ++it)
        _defaultChannel->removeUser(*it);
    _leftUsrsMtx.unlock();

    // update channels
    // add channels that have to be add
    _channelsToAddMtx.lock();
    for (QList<Channel*>::iterator it = _channelsToAdd.begin(); it != _channelsToAdd.end(); ++it) {
        // create new tabs and register observers
        IRCChannelWidget *newChannel = new IRCChannelWidget(this,_subject);
        newChannel->setDefaultChannel(_defaultChannel);
        _map.insert(*it, newChannel);
        (*it)->registerObserver(newChannel);
        _tabView->addChannel(newChannel, QString::fromStdString((*it)->getName()));
    }
    // added channels, clear list
    _channelsToAdd.clear();
    _channelsToAddMtx.unlock();

    // remove channels that have to be removed (the tabs, the Channel*'s are deleted in the Core)
    _channelsToRemoveMtx.lock();
    for (QList<IRCChannelWidget*>::iterator it = _channelsToRemove.begin(); it != _channelsToRemove.end(); ++it)
        _tabView->removeChannel(*it);
    _channelsToRemove.clear();
    _channelsToRemoveMtx.unlock();

}

void IRCMainWindow::sendMessage(const QString& msg)
{
    if (_subject != 0) {
        _subject->sendMsg(msg.toStdString());
    }
}
