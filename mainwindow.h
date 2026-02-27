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
#include <QThreadPool>
#include <QFuture>
#include <QFutureWatcher>
#include <QMutex>
#include <QWaitCondition>
#include <atomic>
#include <QtConcurrent/QtConcurrent>

#include "settingdialog.h"
#include "trayicon.h"
#include "accountdialog.h"
#include "emailclient.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

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
    // UI 交互槽函数
    void onInboxClicked();
    void onSendClicked();
    void onFavoriteClicked();
    void onTrashClicked();
    void onAccountClicked();
    void onSettingClicked();
    void onMinimizeClicked();
    void onEmailItemClicked(QListWidgetItem *item);
    void onComposeClicked();
    void onReplyClicked();
    void onFavoriteContentClicked();

    // 系统托盘相关
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void restoreFromTray();
    void toggleMaximize();

    // 邮件客户端相关
    void onEmailReceived(const QString &from, const QString &subject, const QString &body, bool isHtml, const QStringList &attachments);
    void onConnectionStatusChanged(bool connected);
    void onEmailSent(bool success);
    void onEmailError(const QString &error);
    void onAccountChanged(const QString &email);

    // 定时任务
    void checkNewEmails();

    // 线程相关
    void onEmailConnectionFinished();
    void onEmailOperationFinished();

private:
    // UI 组件
    Ui::MainWindow *ui;
    SettingDialog *settingDialog;
    AccountDialog *accountDialog;
    EmailClient *emailClient;
    TrayIcon *trayIcon;

    // 定时器
    QTimer *checkTimer;
    QTimer *uiUpdateTimer;

    // 线程管理
    QThreadPool *threadPool;
    QFutureWatcher<void> *connectionWatcher;
    QFutureWatcher<void> *operationWatcher;

    // 线程同步
    mutable QMutex emailMutex;
    QWaitCondition emailCondition;
    std::atomic<bool> isConnecting{false};
    std::atomic<bool> isOperating{false};

    // 拖拽相关
    QPoint m_dragPosition;
    QString currentEmailId;

    // 邮件数据
    QList<Email> inboxEmails;
    QList<Email> sentEmails;
    QList<Email> favoriteEmails;
    QList<Email> trashEmails;

    // 当前视图状态
    enum ViewType {
        Inbox,
        Sent,
        Favorite,
        Trash
    } currentView;

    // 初始化方法
    void initializeComponents();
    void setupConnections();
    void setupThreading();
    void startInitialTasks();

    // UI 相关方法
    void setupUI();
    void setupTitleBar();
    void loadTheme();
    void updateEmailList();
    void showEmailContent(const Email &email);
    void showNotification(const QString &title, const QString &message);

    // 线程安全的邮件操作
    void connectToEmailServerAsync();
    void performEmailOperationAsync(const std::function<void()>& operation);

    // 辅助方法
    void updateUIState(bool connected);
    void handleEmailError(const QString &error);
    Email* findEmailById(const QString &id, QList<Email> &emailList);
};

#endif // MAINWINDOW_H
