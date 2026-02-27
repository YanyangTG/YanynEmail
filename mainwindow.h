#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QList>
#include <QUuid>
#include <QDateTime>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainterPath>
#include <QPainter>

#include "settingdialog.h"
#include "trayicon.h"
#include "accountdialog.h"  // 这里已经包含了Email定义
#include "emailclient.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// 移除重复的Email结构体定义，使用accountdialog.h中的定义

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onInboxClicked();
    void onSendClicked();
    void onFavoriteClicked();
    void onTrashClicked();
    void onAccountClicked();
    void onSettingClicked();
    void onEmailItemClicked(QListWidgetItem *item);
    void checkNewEmails();
    void showNotification(const QString &title, const QString &message);
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void restoreFromTray();
    void toggleMaximize();

    // 邮件客户端相关槽函数
    void onEmailReceived(const QString &from, const QString &subject, const QString &body, bool isHtml = false, const QStringList &attachments = QStringList());
    void onConnectionStatusChanged(bool connected);
    void onEmailSent(bool success);
    void onEmailError(const QString &error);
    void onAccountChanged(const QString &email);
    void onComposeClicked();  // 添加缺失的声明
    void onReplyClicked();    // 添加缺失的声明
    void onFavoriteContentClicked();  // 添加缺失的声明

private:
    Ui::MainWindow *ui;
    SettingDialog *settingDialog;
    AccountDialog *accountDialog;
    EmailClient *emailClient;
    TrayIcon *trayIcon;
    QTimer *checkTimer;
    QPoint m_dragPosition;
    QString currentEmailId;  // 添加当前邮件ID

    // 邮件数据 - 使用accountdialog.h中的Email定义
    QList<Email> inboxEmails;
    QList<Email> sentEmails;
    QList<Email> favoriteEmails;
    QList<Email> trashEmails;

    // 当前视图
    enum ViewType {
        Inbox,
        Sent,
        Favorite,
        Trash
    } currentView;

    void setupUI();
    void setupTitleBar();
    void loadTheme();
    void updateEmailList();
    void showEmailContent(const Email &email);
    void connectToEmailServer();
};

#endif // MAINWINDOW_H
