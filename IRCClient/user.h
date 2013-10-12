/* Author: Balazs Nemeth
 * Description: saves the modes of a user and it's name

    a - user is flagged as away;
    i - marks a users as invisible;
    w - user receives wallops;
    r - restricted user connection;
    o - operator flag;
    O - local operator flag;
    s - marks a user for receipt of server notices.
 */

#ifndef USER_H
#define USER_H
#include <vector>
#include <string>


using namespace std;


class User
{
public:
    enum UserMode {UNSUPPORTED = -1, AWAY, INVISIBLE, WALLOPS, RESTRICTED, OPERATOR, LOCALOPERATOR, RECVNOTICES};
    User(const string& name = "");
    string getName() const {return _name;}
    // to handle +i for example, use addMode(INVISIBLE, true) or addMode('i', true);
    void setMode(UserMode mode, bool val);
    void setMode(char mode, bool val);
    bool getMode(UserMode mode) const;
    bool getMode(char mode) const;
    string toString() const;
    // compares a given user with another user sollely through it's name
    bool compNick(const User& other) const;
    bool compNick(const string& name) const;
    // removes the @ in the front and then compares
    static bool compNick(const string& nick1, const string& nick2);
private:
    UserMode modeToChar(char mode) const;
    char charToMode(UserMode mode) const;
    vector<bool> _modes;
    // total number of modes
    static const unsigned MODESCOUNT = 7;
    string _name;
};

#endif // USER_H
