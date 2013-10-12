/* Author: Balazs Nemeth
 * Description: This class handles all irc protocol specific stuff. It constructs the correct messages following the 2812 rfc
 */

#ifndef DCCWIDGET_H
#define DCCWIDGET_H

#include "dcc.h"
#include "observer.h"
#include <QDialog>
#include <QTimer>
#include <QMutex>

class QProgressBar;
class QGridLayout;
class QPushButton;
class QLabel;

class DCCWidget : public QDialog, public Observer
{
    Q_OBJECT
public:
    DCCWidget(QWidget *parent = 0);

    void notify(Subject *subject);
    void show();
private slots:
    void updateGUI();
    void cancelDCC();
signals:
    void notified();
private:
    QString bytesToSize(long bytes);
    string convertTime(unsigned long time);
    DCC* _subject;
    QGridLayout* _mainLayout;
    QPushButton* _cancelButton;
    QProgressBar* _progressBar;

    QLabel* _fileInfo;
    QLabel* _sizeInfo;
    QLabel* _pathInfo;
    QLabel* _toInfo;
    QLabel* _sentInfo;
    QLabel* _timeLeftInfo;
    QLabel* _speedInfo;
    QLabel* _statusInfo;
    QLabel* _elapsedInfo;
    // periodically check new information
    QTimer _timer;
    long _lastSentRecv;
    long _speedAvg;
    QMutex _updateGUI;
    unsigned long _secsElapsed;
    QLabel* _fileVal;
    QLabel* _sizeVal;
    QLabel* _pathVal;
    QLabel* _toVal;
    QLabel* _sentVal;
    QLabel* _timeLeftVal;
    QLabel* _speedVal;
    QLabel* _statusVal;
    QLabel* _elapsedVal;
};

#endif // DCCWIDGET_H
