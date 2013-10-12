/* Author: Balazs Nemeth
 * Description: For each channel (or private message to a user), a channel object is created, all the message sent through and received from the server are going to channels
 *              even when the user is sending a private message, a new channel is created.
 *              The IRCClient class will demultiplex the messages received from the server and send them to channels
 */


#ifndef CHANNEL_H
#define CHANNEL_H


#include <queue>
#include <string>
#include <QMutex>

#include "subject.h"
#include "textmessage.h"
#include "user.h"

// foreward declare IRCClient here
class IRCClient;

using namespace std;

class Channel : public Subject
{
public:
    Channel(IRCClient* client, const string& name);
    // sets the topic for the channel
    void setTopic(const string& topic);
    string getTopic();
    string getName() const { return _name; }
    // add Msg is used by the IRCClient to pass messages in the class
    void addRecvedMsg(const TextMessage& textMsg);
    // sends a message to the IRCClient if it starts with a '/' otherwise the message is sent to the channel in question
    void sendMsg(const string& message);
    // empties the queue and returns what was in the queue
    queue<TextMessage> getLastMsgs();

    // queries to control the users in the channel
    list<User> getAddUsers();
    list<User> getRemUsers();
    void addUser(const User& user);
    void remUser(const User& user);
    // converts a given nick to a user, if the nick is not found, a user with an empty nick is returned
    User str2User(const string& userNick);
private:
    // sending messages it delegated to the client (after /msg is passed in front of it)
    IRCClient* _client;
    // name of the channel, no need to put a mutex on this because it never changes
    string _name;
    // the topic can change any time
    QMutex _topicMtx;
    string _topic;
    // last messages
    queue<TextMessage> _lastMgs;
    // mutex for the queu
    QMutex _lastMsgsMtx;

    // mutex for the 2 lists of users
    QMutex _addUsersMtx;
    QMutex _remUsersMtx;
    /* 2 lists that hold all the users that have been added or removed
       The channel object doens't keep track of the users in the channel,
       only the users that have left or joined since the last calls to getAddUsers() and getRemUsers() */
    list<User> _addUsers;
    list<User> _remUsers;
    list<User> _currentUsers;
    QMutex _currentUsersMtx;
};

#endif // CHANNEL_H
