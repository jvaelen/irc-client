    #include "ircchannelwidget.h"
#include "textmessage.h"
#include "ircmainwindow.h"
#include "dccwidget.h"

#include <QLineEdit>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QTimer>
#include <QGridLayout>
#include <QListWidget>
#include <QList>
#include <QDebug>
#include <cassert>
#include <QFileDialog>
#include <QMessageBox>

IRCChannelWidget::IRCChannelWidget(QWidget *parent, IRCClient* client) :
    QWidget(parent), _kickAction(this), _sendFileAction(this), _whoIsAction(this), _userOptions(this), _defaultChannel(0), _privMsg(this)
{
    _subject = 0;
    _client = client;
    _msgScreen = new QPlainTextEdit(this);
    // we use the text-edit to display text
    _msgScreen->setReadOnly(true);
    _lineEdit = new QLineEdit(this);
    _submitText = new QPushButton(this);
    _submitText->setText("submit");
    _submitText->setMinimumWidth(180);
    _users = new QListWidget(this);
    _users->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);

    // on entering or pressing submit we let the GUI know that we want to send text
    connect(_lineEdit, SIGNAL(returnPressed()),this, SLOT(textEntered()));
    connect(_submitText, SIGNAL(clicked()), this, SLOT(textEntered()));
    connect(this, SIGNAL(notified()), this, SLOT(updateGui()));

    // create our grid layout
    constructMainLayout();

    stealFocus();

    // create rightclickmenu and actions connected to functions handeling the actions
    _users->setContextMenuPolicy(Qt::CustomContextMenu);
    _whoIsAction.setText("Who Is");
    _sendFileAction.setText("Send File");
    _kickAction.setText("Kick");
    _privMsg.setText("Privmsg");
    _userOptions.addAction(&_whoIsAction);
    _userOptions.addAction(&_sendFileAction);
    _userOptions.addAction(&_kickAction);
    _userOptions.addAction(&_privMsg);


    connect(_users, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(showUserOptionsMenu(const QPoint &)));
    connect(&_whoIsAction, SIGNAL(triggered()), this, SLOT(sendWhoIs()));
    connect(&_sendFileAction, SIGNAL(triggered()), this, SLOT(sendFile()));
    connect(&_kickAction, SIGNAL(triggered()), this, SLOT(kickUser()));
    connect(&_privMsg, SIGNAL(triggered()), this, SLOT(setMsg()));
}

void IRCChannelWidget::stealFocus()
{
    /* Effectively, this invokes the setFocus() slot of the QLineEdit instance right
     * after the event system is "free" to do so, i.e. sometime after the widget is completely constructed. */
    QTimer::singleShot(0, _lineEdit, SLOT(setFocus()));
}

void IRCChannelWidget::sendFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, "open file");
    if (fileName.isEmpty())
        return ;
    // newDCC contains a pointer to the dcc object just created
    DCC* newDCC = _client->sendFile(fileName.toStdString(), _subject->str2User(_users->selectedItems().front()->text().toStdString()));
    DCCWidget* newDCCWidget = new DCCWidget(_defaultChannel);
    newDCC->registerObserver(newDCCWidget);
    _defaultChannel->addDccWidget(newDCCWidget);
}

void IRCChannelWidget::setMsg()
{
    _lineEdit->setText("/msg " + _users->selectedItems().front()->text() + " ");
    stealFocus();
}

void IRCChannelWidget::kickUser()
{
    _subject->sendMsg("/kick " + _subject->str2User(_users->selectedItems().front()->text().toStdString()).getName());
}

void IRCChannelWidget::addDccWidget(DCCWidget *newDCCWidget)
{
    newDCCWidget->show();
    newDCCWidget->raise();
    _dccWidgets.push_back(newDCCWidget);
}

IRCChannelWidget::~IRCChannelWidget()
{
    delete _msgScreen;
    delete _lineEdit;
}

void IRCChannelWidget::notify(Subject *subject)
{
    if (!_subject)
        _subject = dynamic_cast<Channel*>(subject);

    queue<TextMessage> newMsgs = _subject->getLastMsgs();
    while (!newMsgs.empty()) {
        addMsgToQueue(newMsgs.front());
        newMsgs.pop();
    }

    list<User> newUsrs = _subject->getAddUsers();
    list<User> leftUsrs = _subject->getRemUsers();
    _newUsrsMtx.lock();
    _newUsrs.insert(_newUsrs.end(),newUsrs.begin(), newUsrs.end());
    _newUsrsMtx.unlock();
    _leftUsrsMtx.lock();
    _leftUsrs.insert(_leftUsrs.begin(), leftUsrs.begin(), leftUsrs.end());
    _leftUsrsMtx.unlock();

    notified();

}

void IRCChannelWidget::addMsgToQueue(const TextMessage& msg)
{
    /* we have to use a mutex because notify() gets called from the Reciever thread and updateGUI() gets schedueled in the main thread
     * because of the signal/slot connection. This means that the gui could access the queue at the same time it gets altered because of notify */
    _msgMtx.lock();
    _msgsToShow.push(msg);
    _msgMtx.unlock();
}

void IRCChannelWidget::setFileReq(const list<IRCClient::FileRecvReq>& fileReqs)
{
    _fileReqsMtx.lock();
    cout << "adding items" << endl;
    _fileReqs = fileReqs;
    _fileReqsMtx.unlock();
    notified();
}

void IRCChannelWidget::updateGui()
{
    // show all messages on the screen
    _msgMtx.lock();
    while (!_msgsToShow.empty()) {
        showMessage(_msgsToShow.front());
        _msgsToShow.pop();
    }
    _msgMtx.unlock();

    // update user list
    _newUsrsMtx.lock();
    for (list<User>::iterator it = _newUsrs.begin(); it != _newUsrs.end(); ++it)
        addUser(*it);
    _newUsrs.clear();
    _newUsrsMtx.unlock();
    _leftUsrsMtx.lock();
    for (list<User>::iterator it = _leftUsrs.begin(); it != _leftUsrs.end(); ++it)
        removeUser(*it);
    _leftUsrs.clear();
    _leftUsrsMtx.unlock();



    QMessageBox msgBox;
    QString fileName;
    int ret;

    // statuswindow takes care of recv  files
    if (_client)
    {
        _fileReqsMtx.lock();
        // for each filerequest, show a dialog
        for (list<IRCClient::FileRecvReq>::iterator i = _fileReqs.begin(); i != _fileReqs.end(); ++i) {

            msgBox.setText("Receiving file from user");
            msgBox.setInformativeText("User " + QString::fromStdString((*i)._userName) + " is trying to send you the file: " + QString::fromStdString((*i)._fileName));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            ret = msgBox.exec();
            if (ret == QMessageBox::Yes)
            {
                fileName = QFileDialog::getSaveFileName(this, "save file");
                if (!fileName.isEmpty())
                {
                    (*i)._path = fileName.toStdString();
                    DCCWidget* dccWidget = new DCCWidget(this);
                    if (!fileName.isEmpty())
                        _client->recvFile(*i)->registerObserver(dccWidget);
                    dccWidget->show();
                }
            }

        }
        _fileReqsMtx.unlock();
    }
}


void IRCChannelWidget::constructMainLayout()
{
    // no member because on the destruction of this widget, the mainLayout is automatically deleted
    QGridLayout *mainLayout = new QGridLayout();
    mainLayout->addWidget(_msgScreen, 0, 0);
    mainLayout->addWidget(_lineEdit, 1, 0);
    mainLayout->addWidget(_submitText, 1, 1);
    mainLayout->addWidget(_users, 0, 1);
    setLayout(mainLayout);
}

void IRCChannelWidget::textEntered()
{
    QString str = _lineEdit->text();

    // ignore enters with no text
    if (str.isEmpty()) return;

    /* show in GUI, set type to OWNMESSAGE, this will show the message even if the nickname is
       the same as _client->getNick(). The type has to be set to OWNMESSAGE because messages
       from the user will not be shown in the GUI (in some cases, in example when the user is sending
       messages in a pirvate chat to another user, the message is echoed back by the user,
       for that reason messages that come from the same user as the current user are not shown (unless OWNMESSAGE is set) */
    showMessage(TextMessage(_client->getNick(), str.toStdString(), TextMessage::OWNMESSAGE));

    // send message to an actual channel
    if (_subject)
        _subject->sendMsg(str.toStdString());
    // send message to the status windows IF it's not /part
    else if (str != "/part")
        _client->sendMsg(str.toStdString());
    // show the user that you can't leave the status window
    else
        showMessage(TextMessage(_client->getNick(), "Cannot leave the status window.", TextMessage::ERROR));

    // clear lineedit
    _lineEdit->clear();
}


void IRCChannelWidget::showMessage(const TextMessage& msg)
{
    QString message = QString::fromStdString(msg.toString());
    message = message.replace("&","&amp;").replace(">","&gt;").replace("<","&lt;");

    /* don't show commands (messages that start with '/') unless they are info messages, also if the user echo's back a message that the user sent
       in example in a private chat, don't show that*/

    /* messages that start with / are not shown unless they are info's and messages that
        come from a user with the same nick as the current user are not shown unless the messageType is set to OWNMESSAGE */
    if ((!msg.isCmd() || msg.getMessageType() == TextMessage::INFO) &&
            ((msg.getNick() != _client->getNick()) || (msg.getMessageType() == TextMessage::OWNMESSAGE))) {
        switch (msg.getMessageType(_client->getNick())) {
        case TextMessage::OWNMESSAGE:
        case TextMessage::STDMSG: {
            _msgScreen->appendHtml("<FONT COLOR=\"#000000\">" + message + "</FONT>");
            break;
        }
        case TextMessage::ERROR: {
            _msgScreen->appendHtml("<FONT COLOR=\"#ff0000\">" + message + "</FONT>");
            break;
        }
        case TextMessage::INFO: {
            _msgScreen->appendHtml("<FONT COLOR=\"#666666\">" + message + "</FONT>");
            break;
        }
        case TextMessage::DIRECTEDMSG: {
            _msgScreen->appendHtml("<FONT COLOR=\"#0000ff\">" + message + "</FONT>");
            break;
        }
        }
        // automatically scroll down
        QTextCursor c =  _msgScreen->textCursor();
        c.movePosition(QTextCursor::End);
        _msgScreen->setTextCursor(c);
        _msgScreen->ensureCursorVisible();
    }
}

void IRCChannelWidget::addUser(User usr)
{
    _users->addItem(QString::fromStdString(usr.toString()));
    _users->sortItems();
}

void IRCChannelWidget::sendWhoIs()
{
    QList<QListWidgetItem *> temp =_users->selectedItems();
    /* if somehow no item is selected, return to prevent sending a whois for nothing,
        this can happen if the qlistwidget desides to deslect everything */
    if (temp.empty())
        return ;
    string temp2 = temp.front()->text().toStdString();
    _subject->sendMsg("/whois " + _subject->str2User(temp2).getName() );
}

void IRCChannelWidget::sendPart()
{
    // user can only part an actual channel (there is always one "status" channel which can't be closed)
    if (_subject)
        _subject->sendMsg("/part");
}

void IRCChannelWidget::showUserOptionsMenu(const QPoint & pos)
{
    QPoint globalPos = _users->mapToGlobal(pos); // Map the global position to the userlist
    QModelIndex t = _users->indexAt(pos);

    // ignore if no user is selected
    if (t.row() == -1)
        return ;
    _users->item(t.row())->setSelected(true); // even a right click will select the item


    // only makes sense to send file to other user or send private message to other than then the current user
    if (User::compNick(_users->selectedItems().front()->text().toStdString(), _client->getNick()))
    {
        _sendFileAction.setEnabled(false);
        _privMsg.setEnabled(false);
    }
    else
    {
        _sendFileAction.setEnabled(true);
        _privMsg.setEnabled(true);
    }

    _userOptions.exec(globalPos);
}

void IRCChannelWidget::removeUser(User usr)
{
    QList<QListWidgetItem *> usrs = _users->findItems(QString::fromStdString(usr.toString()), Qt::MatchExactly);
    while (!usrs.empty())
        _users->takeItem(_users->row(usrs.takeFirst()));
}

void IRCChannelWidget::removeUserList()
{
    _users->close();
}
