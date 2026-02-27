#ifndef TRAYICON_H
#define TRAYICON_H

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>

class TrayIcon : public QObject
{
    Q_OBJECT

public:
    explicit TrayIcon(QObject *parent = nullptr);

    void showMessage(const QString &title, const QString &message,
                     QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information,
                     int millisecondsTimeoutHint = 10000);

signals:
    void restoreRequested();
    void activated(QSystemTrayIcon::ActivationReason reason);

private slots:
    void onActivated(QSystemTrayIcon::ActivationReason reason);
    void onRestoreAction();

private:
    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
    QAction *restoreAction;
    QAction *quitAction;
};

#endif // TRAYICON_H
