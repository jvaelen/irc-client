#ifndef IRCTABWIDGET_H
#define IRCTABWIDGET_H

#include <QWidget>
#include <QString>

class QTabWidget;
class IRCChannelWidget;

class IRCTabWidget : public QWidget
{
    Q_OBJECT
public:
    explicit IRCTabWidget(QWidget *parent = 0);
    void addChannel(IRCChannelWidget* channel, QString name);
    void removeChannel(IRCChannelWidget* channel);
    IRCChannelWidget* getCurrentChannelWidget();


private:
    QTabWidget *_tabWidget;

};

#endif // IRCTABWIDGET_H
