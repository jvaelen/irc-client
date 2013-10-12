#ifndef IRCINMESSAGE_H
#define IRCINMESSAGE_H

#include <string>
#include <vector>
#include <map>

using namespace std;

class IRCInMessage
{
public:
    // enumeration to check the type of the message
    enum MessageType {UNSUPPORTED       ,
                      PING              ,
                      NOTICE            ,
                      SERVERMESSAGE     ,
                      ERROR             ,
                      KICK              ,
                      INVITE            ,
                      RPL_NAMREPLY      ,
                      RPL_NOTOPIC       ,
                      RPL_MOTD          ,
                      RPL_TOPIC         ,
                      TOPIC             ,
                      USERJOIN          ,
                      USERPART          ,
                      ERR_NICKNAMEINUSE ,
                      RPL_WHOISUSER     ,
                      RPL_WHOISSERVER   ,
                      RPL_WHOISOPERATOR ,
                      RPL_WHOISIDLE     ,
                      RPL_WHOISCHANNELS ,
                      RPL_WHOISHOST     ,
                      ERR_NOSUCHNICK    ,
                      ERR_NOTEXTTOSEND  ,
                      RPL_ENDOFWHOIS    ,
                      USERQUIT          ,
                      DCCFILE           , // a dcc request
                      ERR_CHANOPRIVSNEEDED,
                      PRIVMSG};
    IRCInMessage(const string& message);
    // checks the COMMAND field and returns the type of te message
    MessageType getType() const;
    // returns a given paramter
    string getParam(unsigned index) const;
    // returns all the parameters concatinated wiht spaces starting from the #index parameter
    string getLastParams(unsigned index, bool clearColon = false) const;
    unsigned getParamCount() const;
    bool hasPrefix() const {return _hasPrefix;}
    string getPrefix() const { return _prefix;}
    static bool initMap();
private:
    vector <string> _tokens;
    string _prefix;
    bool _hasPrefix;
    static map<string, MessageType> _messageMap;
    static bool _initDone;
};

#endif // IRCINMESSAGE_H
