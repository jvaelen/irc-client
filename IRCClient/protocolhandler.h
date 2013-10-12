/* Author: Balazs Nemeth
 * Description: Abstract baseclass for protocols which derive from this class and implement handleMsg function
 */

#ifndef PROTOCOLHANDLER_H
#define PROTOCOLHANDLER_H

#include <string>

using namespace std;

class ProtocolHandler
{
public:
    ProtocolHandler();
    // pure virtual function that needs to be implemented in the deriven class
    virtual void handleMsg(const string& message) = 0;
    // the function that will be used to warn the protocol that the connection closed
    virtual void setConnected(bool connected) = 0;
};

#endif // PROTOCOLHANDLER_H
