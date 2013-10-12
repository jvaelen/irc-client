#include "irctabwidget.h"
#include <QTableWidget>
#include "ircchannelwidget.h"
#include <QVBoxLayout>
#include <QDebug>
#include <QKeyEvent>

IRCTabWidget::IRCTabWidget(QWidget *parent) :
    QWidget(parent)
{
    _tabWidget = new QTabWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addWidget(_tabWidget);
    setLayout(mainLayout);

}


IRCChannelWidget* IRCTabWidget::getCurrentChannelWidget()
{
    return dynamic_cast<IRCChannelWidget*>(_tabWidget->currentWidget());
}

void IRCTabWidget::addChannel(IRCChannelWidget* channel, QString name)
{
    int index = _tabWidget->addTab(channel, name);
    _tabWidget->setCurrentIndex(index);
}

void IRCTabWidget::removeChannel(IRCChannelWidget* channel)
{
    _tabWidget->removeTab(_tabWidget->indexOf(channel));
    dynamic_cast<IRCChannelWidget*>(_tabWidget->currentWidget())->stealFocus();
}
