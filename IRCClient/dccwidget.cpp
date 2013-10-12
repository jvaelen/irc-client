#include <cassert>
#include <QGridLayout>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <sstream>
#include "dccwidget.h"
#include <QDebug>

DCCWidget::DCCWidget(QWidget *parent) : QDialog(parent), _subject(0), _timer(this), _secsElapsed(0)
{
    _lastSentRecv =0;
    _speedAvg = 0;
    _mainLayout = new QGridLayout(this);
    _cancelButton = new QPushButton(this);
    _progressBar = new QProgressBar(this);
    _progressBar->setRange(0, 100);
    _cancelButton->setText("Cancel");
    setLayout(_mainLayout);


    _fileInfo = new QLabel("File: ", this);
    _fileInfo->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    _fileInfo->setMinimumWidth(100);

    _sizeInfo = new QLabel("Size: ", this);
    _pathInfo = new QLabel("Path: ", this);
    _toInfo   = new QLabel("User: ", this);
    _sentInfo = new QLabel("Sent or Received: ", this);
    _timeLeftInfo = new QLabel("Time left: ", this);
    _speedInfo = new QLabel("Speed: ", this);
    _statusInfo = new QLabel("Status: ", this);
    _elapsedInfo = new QLabel("Time elapsed: ", this);

    _fileVal = new QLabel("N/A", this);
    _fileVal->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    _fileVal->setMinimumWidth(700);
    _sizeVal = new QLabel("N/A", this);
    _pathVal = new QLabel("N/A", this);
    _toVal   = new QLabel("N/A", this);
    _sentVal = new QLabel("N/A", this);
    _timeLeftVal = new QLabel("N/A", this);
    _speedVal = new QLabel("N/A", this);
    _statusVal = new QLabel("N/A", this);
    _elapsedVal = new QLabel("N/A", this);



    _mainLayout->addWidget(_fileInfo,     0, 0);
    _mainLayout->addWidget(_sizeInfo,     1, 0);
    _mainLayout->addWidget(_pathInfo,     2, 0);
    _mainLayout->addWidget(_toInfo,       3, 0);
    _mainLayout->addWidget(_sentInfo,     4, 0);
    _mainLayout->addWidget(_timeLeftInfo, 5, 0);
    _mainLayout->addWidget(_speedInfo,    6, 0);
    _mainLayout->addWidget(_statusInfo,   7, 0);
    _mainLayout->addWidget(_elapsedInfo,  8, 0);

    _mainLayout->addWidget(_fileVal,     0, 1);
    _mainLayout->addWidget(_sizeVal,     1, 1);
    _mainLayout->addWidget(_pathVal,     2, 1);
    _mainLayout->addWidget(_toVal,       3, 1);
    _mainLayout->addWidget(_sentVal,     4, 1);
    _mainLayout->addWidget(_timeLeftVal, 5, 1);
    _mainLayout->addWidget(_speedVal,    6, 1);
    _mainLayout->addWidget(_statusVal,   7, 1);
    _mainLayout->addWidget(_elapsedVal,  8, 1);

    _mainLayout->addWidget(_progressBar,  9, 0, 1, 2);
    _mainLayout->addWidget(_cancelButton, 10, 0, 1, 2, Qt::AlignCenter);
    setFixedSize(sizeHint() );
    connect(&_timer, SIGNAL(timeout()), this, SLOT(updateGUI()));
    connect(this, SIGNAL(notified()), this , SLOT(updateGUI())); // make the update gui go over the main thread
    connect(_cancelButton, SIGNAL(released()), this, SLOT(cancelDCC()));
}


void DCCWidget::notify(Subject *subject)
{

    _subject = dynamic_cast<DCC*>(subject);

    if(_subject->getStopped()) {
        _timer.stop(); // stop the timer
        notified();
    }
}

void DCCWidget::cancelDCC()
{
    if (!_subject)
        return ;
    if(_subject->getStopped()) {
        _timer.stop(); // stop the timerS/
    }

    _subject->reqStop();
    _subject->wait();
    close();
}

string DCCWidget::convertTime(unsigned long time)
{
    stringstream ss;
    unsigned long hours = time / 3600;
    time -= hours*3600;
    unsigned long mins = time / 60;
    time -= mins*60;

    if (hours < 10)
        ss << "0" << hours;
    else
        ss << hours;
    ss << ':';
    if (mins < 10)
        ss << "0" << mins;
    else
        ss << mins;
    ss << ':';

    if (time < 10)
        ss << "0" << time;
    else
        ss << time;
    return ss.str();
}

void DCCWidget::updateGUI()
{
    _updateGUI.lock();
    // if there is a subject
    if (_subject) {
        _secsElapsed++;
        _fileVal->setText(_subject->getFileName().c_str());;
        _pathVal->setText(_subject->getPath().c_str());
        _toVal->setText(_subject->getUser().getName().c_str());

        long totalBytes = _subject->getTotalBytes();
        long sentBytes = _subject->getSentRecvBytes();
        // exp moving avarage to flatten out network fluctuations
        _speedAvg = .7* _speedAvg + .3*(sentBytes - _lastSentRecv);
        _lastSentRecv = sentBytes;
        _sizeVal->setText(bytesToSize(totalBytes));
        _sentVal->setText(bytesToSize(sentBytes));
        //_timeLeftVal
        _speedVal->setText(bytesToSize(_speedAvg) + "ps");
        _elapsedVal->setText(convertTime(_secsElapsed).c_str());
        _statusVal->setText(_subject->getStatus().c_str());
        if (totalBytes && totalBytes != -1)
        {
            _progressBar->setValue((sentBytes*100)/totalBytes);
            _timeLeftVal->setText(convertTime((totalBytes - sentBytes)/float(_speedAvg)).c_str());
        }
        else
            _progressBar->setValue(50);
        if (_subject->getStopped())
            _cancelButton->setText("Close");
    }
    _updateGUI.unlock();
}

QString DCCWidget::bytesToSize(long bytes)
{
    QString retVal;
    if (bytes > 1024)
    {
        retVal = " KB";
        bytes = bytes / 1024;
        if (bytes > 1024)
        {
            bytes = bytes/ 1024;
            retVal = " MB";
            if (bytes > 1024)
            {
                bytes = bytes/1024;
                retVal = " GB";
                if (bytes > 1024)
                {
                    bytes = bytes/1024;
                    retVal = " TB";
                }
            }
        }
    }
    else
        retVal += " B";
    return QString::number(bytes) + retVal;
}

void DCCWidget::show()
{
    assert(_subject);
    setWindowTitle("file transfer to: " + QString::fromStdString(_subject->getUser().getName()));
    // before showing anything, update the gui for the first time showing any information already available
    updateGUI();
    QDialog::show();
    _timer.setInterval(1000);
    _timer.start();
}
