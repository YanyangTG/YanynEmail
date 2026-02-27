#include "trayicon.h"
#include <QApplication>

TrayIcon::TrayIcon(QObject *parent)
    : QObject(parent)
{
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/resources/icon.png"));
    trayIcon->setToolTip("Yanyn Email");

    // 创建托盘菜单
    trayMenu = new QMenu();
    restoreAction = new QAction("恢复", this);
    quitAction = new QAction("退出", this);

    trayMenu->addAction(restoreAction);
    trayMenu->addSeparator();
    trayMenu->addAction(quitAction);

    trayIcon->setContextMenu(trayMenu);

    // 连接信号槽
    connect(trayIcon, &QSystemTrayIcon::activated, this, &TrayIcon::onActivated);
    connect(restoreAction, &QAction::triggered, this, &TrayIcon::onRestoreAction);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    trayIcon->show();
}

void TrayIcon::showMessage(const QString &title, const QString &message,
                           QSystemTrayIcon::MessageIcon icon,
                           int millisecondsTimeoutHint)
{
    trayIcon->showMessage(title, message, icon, millisecondsTimeoutHint);
}

void TrayIcon::onActivated(QSystemTrayIcon::ActivationReason reason)
{
    emit activated(reason);
}

void TrayIcon::onRestoreAction()
{
    emit restoreRequested();
}
