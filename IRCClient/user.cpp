#include <cassert>

#include "user.h"


User::User(const string &name)
{
    _modes.reserve(MODESCOUNT);
    for (unsigned i = 0; i < MODESCOUNT; ++i)
        _modes[i] = false;

    if (name[0] == '@') {
        _name = name.substr(1);
        setMode(OPERATOR, true);
    }
    else
        _name = name;
}



bool User::compNick(const User& other) const
{
    return _name == other._name;
}
bool User::compNick(const string& name) const
{
    if (name[0] == '@')
        return _name == name.substr(1);
    return _name == name;
}

bool User::compNick(const string& nick1, const string& nick2)
{
    string tempNick1 = nick1;
    string tempNick2 = nick2;

    // remove @ if it exsists
    if (tempNick1[0] == '@')
        tempNick1 = tempNick1.substr(1);
    if (tempNick2[0] == '@')
        tempNick2 = tempNick2.substr(1);

    return tempNick1 == tempNick2;
}

void User::setMode(UserMode mode, bool val)
{
    // -1 would overflow which will cause the assertion to fail
    assert((unsigned)mode < MODESCOUNT);
    _modes[(int)mode] = val;
}

void User::setMode(char mode, bool val)
{
    assert((unsigned)modeToChar(mode) < MODESCOUNT);
    _modes[(int)modeToChar(mode)] = val;
}

bool User::getMode(UserMode mode) const
{
    assert((unsigned)mode < MODESCOUNT);
    return _modes[(int) mode];
}

bool User::getMode(char mode) const
{
    assert((unsigned)modeToChar(mode) < MODESCOUNT);
    return _modes[(int) mode];
}

string User::toString() const
{
//    if (getMode(OPERATOR))
//        return '@' + _name;
//    else
        return _name;
}

User::UserMode User::modeToChar(char mode) const
{
    switch (mode) {
    case 'a':
        return AWAY;
    case 'i':
        return INVISIBLE;
    case 'w':
        return WALLOPS;
    case 'r':
        return RESTRICTED;
    case 'o':
        return OPERATOR;
    case 'O':
        return LOCALOPERATOR;
    case 's':
        return RECVNOTICES;
    default:
        return UNSUPPORTED;
    }
}

char User::charToMode(UserMode mode) const
{
    switch (mode) {
    case AWAY:
        return 'a';
    case INVISIBLE:
        return 'i';
    case WALLOPS:
        return 'w';
    case RESTRICTED:
        return 'r';
    case OPERATOR:
        return 'o';
    case LOCALOPERATOR:
        return 'O';
    case RECVNOTICES:
        return 's';
    default:
        return ' ';
    }
}
