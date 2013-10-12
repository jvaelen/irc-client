#ifndef IRCChannelWidget_H
#define IRCChannelWidget_H

#include <QHash>
#include <QWidget>
#include <QString>
#include <QMutex>
#include <QAction>
#include <QMenu>
#include <QShortcut>
#include <string>
#include <list>
#include <queue>
#include "ircclient.h"
#include "observer.h"
#include "user.h"

using std::string;
using std::list;
using std::queue;

class QPushButton;
class QLineEdit;
class QPlainTextEdit;
class TextMessage;
class QListWidget;
class Channel;
class DCCWidget;

class IRCChannelWidget : public QWidget, public Observer
{
    Q_OBJECT
public:
    explicit IRCChannelWidget(QWidget *parent = 0, IRCClient* client = 0);
    ~IRCChannelWidget();

    void setUsername(const QString& name);

    // puts a message on the screen
    void showMessage(const TextMessage& msg);

    // adds the user to the list
    void addUser(User usr);
    // removes the user from the list
    void removeUser(User usr);

    void setClient(IRCClient* client) { _client = client; }

    // the default channel and private messages do not contain userlists
    void removeUserList();


    void setFileReq(const list<IRCClient::FileRecvReq>& fileReqs);
    void notify(Subject* subject);
    // sets focus onto the textfield
    void stealFocus();
    // sends a part command to the server for the current channel
    void sendPart();
    void addDccWidget(DCCWidget* newDCCWidget);
    // sets the default channel which is always available
    void setDefaultChannel(IRCChannelWidget* defaultChannel) {_defaultChannel = defaultChannel; }
private slots:
    void textEntered();
    void updateGui();
    void sendFile();
    void showUserOptionsMenu(const QPoint & pos);
    void sendWhoIs();
    void kickUser();
    void setMsg();
signals:
    void notified();

private:
    // text input
    QLineEdit *_lineEdit;
    QAction _kickAction;
    QAction _sendFileAction;
    QAction _whoIsAction;
    // message screenL
    QPlainTextEdit *_msgScreen;
    // button to submit text
    QPushButton *_submitText;
    // list of all the users in the current channel
    QListWidget *_users;
    QMenu _userOptions;
    Channel* _subject;
    IRCChannelWidget* _defaultChannel;
    QAction _privMsg;
    // all current DCCWidgets
    list<DCCWidget*> _dccWidgets;
    void addMsgToQueue(const TextMessage& msg);

    // holds all messages that have to be shown
    queue<TextMessage> _msgsToShow;
    // holds all users that have to be added
    list<User> _newUsrs;
    // holds all users that have to be deleted
    list<User> _leftUsrs;
    // holds all the users that are in the channel at a given time
    list<User> _activeUsers;

    list<IRCClient::FileRecvReq> _fileReqs;

    QMutex _fileReqsMtx;
    QMutex _msgMtx;
    QMutex _leftUsrsMtx;
    QMutex _newUsrsMtx;

    // primarely to get the nickname at any time
    IRCClient *_client;

    void constructMainLayout();
};

#endif // IRCChannelWidget_H
