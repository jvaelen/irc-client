#include "channel.h"
#include "ircclient.h"
#include "ircoutmessage.h"
#include "textmessage.h"
#include <QDebug>


Channel::Channel(IRCClient* client, const string &name) : _name(name)
{
    _client = client;
}

void Channel::addRecvedMsg(const TextMessage& textMsg)
{
    _lastMsgsMtx.lock();
    _lastMgs.push(textMsg);
    _lastMsgsMtx.unlock();

    // now notify the observers of this channel
    notifyObservers();
}

void Channel::sendMsg(const string& message)
{
    /* if the message has no / to start with, add /msg #getName() in front
        so that the server knows this message is going for that specific channel/user */
    IRCOutMessage out(message);

    // if no parameter for the channel is supplied, asume that the user means the current channel
    if (out.getType() == IRCOutMessage::KICK && out.getParam()[0] != '#') {
        if (out.hasExtraParams())
            _client->sendMsg("/kick " + getName() + " " + out.getParam() + " :"+ out.getLastParams());
        else
            _client->sendMsg("/kick " + getName() + " " + out.getParam());
    }
    if (out.getType() == IRCOutMessage::TOPIC && out.getParam()[0] != '#') {
        if (out.hasExtraParams())
            _client->sendMsg("/topic " + getName() + " " + out.getParam() + " " + out.getLastParams());
        else // just show the topic
            addRecvedMsg(TextMessage("", "topic for this channel is: " + getTopic(), TextMessage::INFO));
    }
    // if no paramter for the channel or user is supplied, assume that the user means the current channel
    else if (out.getType() == IRCOutMessage::MODE && (out.getParam()[0] == '+' || out.getParam()[0] == '-'))
        _client->sendMsg("/mode " + getName() + " " + out.getParam());
    else if (out.getType() == IRCOutMessage::INVITE && !out.hasExtraParams()) // if no channel is supplied, the current channel is assumed
        _client->sendMsg("/invite " + out.getParam() + " " + getName());
    else if (message[0] != '/')
        _client->sendMsg("/msg " + getName() + " " + message );
    else if (message == "/part")
        _client->sendMsg("/part " + getName());
    else // this means that the message is a command so delegate it strait to the client
        _client->sendMsg(message);
}

queue<TextMessage> Channel::getLastMsgs()
{
    queue<TextMessage> tempQueue;
    _lastMsgsMtx.lock();
    while (!_lastMgs.empty()) {
        tempQueue.push(_lastMgs.front());
        _lastMgs.pop();
    }
    _lastMsgsMtx.unlock();
    return tempQueue;
}


list<User> Channel::getAddUsers()
{
    list<User> tempList;
    _addUsersMtx.lock();
    tempList = _addUsers;
    _addUsers.clear();
    _addUsersMtx.unlock();
    return tempList;
}

list<User> Channel::getRemUsers()
{
    list<User> tempList;
    _remUsersMtx.lock();
    tempList = _remUsers;
    _remUsers.clear();
    _remUsersMtx.unlock();
    return tempList;
}

void Channel::addUser(const User& user)
{
    _addUsersMtx.lock();
    _addUsers.push_back(user);
    _addUsersMtx.unlock();

    _currentUsersMtx.lock();
    _currentUsers.push_back(user);
    _currentUsersMtx.unlock();

    notifyObservers();
}

void Channel::remUser(const User& user)
{
    _remUsersMtx.lock();
    _remUsers.push_back(user);
    _remUsersMtx.unlock();


    // because each nickname is unique, remove the single value from the list
    bool found = false;
    _currentUsersMtx.lock();
    for (list<User>::iterator i = _currentUsers.begin(); !found && i != _currentUsers.end(); ++i) {
        if ((found = ((*i).getName() == user.getName()))) // compare by name
            _currentUsers.erase(i);
    }
    _currentUsersMtx.unlock();

    notifyObservers();
}

User Channel::str2User(const string& userNick)
{
    User retVal("");
    bool found = false;
    _currentUsersMtx.lock();
    for (list<User>::const_iterator i = _currentUsers.begin(); !found && i != _currentUsers.end(); ++i) {
        if ((found = ((*i).compNick(userNick))))
            retVal = (*i); // make a copy to return
    }
    _currentUsersMtx.unlock();
    return retVal;
}


void Channel::setTopic(const string& topic)
{
    _topicMtx.lock();
    _topic = topic;
    _topicMtx.unlock();

    addRecvedMsg(TextMessage("", "topic is set to [" + getTopic() + "]", TextMessage::INFO));
    notifyObservers();
}

string Channel::getTopic()
{
    string temp;
    _topicMtx.lock();
    temp = _topic;
    _topicMtx.unlock();
    return temp;
}
