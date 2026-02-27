#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingdialog.h"
#include "composedialog.h"
#include <QtConcurrent/QtConcurrent>
#include <QPushButton>
#include <QListWidgetItem>
#include <QFile>
#include <QTextStream>
#include <QCloseEvent>
#include <QDateTime>
#include <QIcon>
#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QProgressDialog>
#include <QMetaObject>
#include <QThread>
#include <functional>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , settingDialog(nullptr)
    , accountDialog(nullptr)
    , emailClient(nullptr)
    , trayIcon(nullptr)
    , checkTimer(nullptr)
    , uiUpdateTimer(nullptr)
    , threadPool(nullptr)
    , connectionWatcher(nullptr)
    , operationWatcher(nullptr)
    , currentView(Inbox)
{
    // 首先设置窗口属性
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // 初始化 UI
    ui->setupUi(this);

    // 初始化组件
    initializeComponents();

    // 设置 UI 和连接
    setupUI();
    loadTheme();
    updateEmailList();
    setupConnections();

    // 显示窗口
    show();
    activateWindow();
    raise();
    setFocus();

    // 启动初始任务
    startInitialTasks();
}

MainWindow::~MainWindow()
{
    // 清理线程相关资源
    if (connectionWatcher) {
        connectionWatcher->cancel();
        connectionWatcher->waitForFinished();
        delete connectionWatcher;
    }

    if (operationWatcher) {
        operationWatcher->cancel();
        operationWatcher->waitForFinished();
        delete operationWatcher;
    }

    if (threadPool) {
        threadPool->clear();
        threadPool->waitForDone();
    }

    delete ui;
}

void MainWindow::initializeComponents()
{
    settingDialog = new SettingDialog(this);
    accountDialog = new AccountDialog(this);
    emailClient = new EmailClient(this);
    trayIcon = new TrayIcon(this);

    checkTimer = new QTimer(this);
    uiUpdateTimer = new QTimer(this);
    threadPool = new QThreadPool(this);
    connectionWatcher = new QFutureWatcher<void>(this);
    operationWatcher = new QFutureWatcher<void>(this);

    // 配置线程池
    threadPool->setMaxThreadCount(QThread::idealThreadCount());
}

void MainWindow::setupConnections()
{
    // UI 按钮连接
    connect(ui->inboxButton, &QPushButton::clicked, this, &MainWindow::onInboxClicked);
    connect(ui->sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(ui->favoriteButton, &QPushButton::clicked, this, &MainWindow::onFavoriteClicked);
    connect(ui->trashButton, &QPushButton::clicked, this, &MainWindow::onTrashClicked);
    connect(ui->accountButton, &QPushButton::clicked, this, &MainWindow::onAccountClicked);
    connect(ui->settingButton, &QPushButton::clicked, this, &MainWindow::onSettingClicked);
    connect(ui->emailList, &QListWidget::itemClicked, this, &MainWindow::onEmailItemClicked);
    connect(ui->composeButton, &QPushButton::clicked, this, &MainWindow::onComposeClicked);
    connect(ui->replyButton, &QPushButton::clicked, this, &MainWindow::onReplyClicked);
    connect(ui->favoriteContentButton, &QPushButton::clicked, this, &MainWindow::onFavoriteContentClicked);

    // 标题栏按钮
    connect(ui->minimizeButton, &QPushButton::clicked, this, &MainWindow::onMinimizeClicked);
    connect(ui->maximizeButton, &QPushButton::clicked, this, &MainWindow::toggleMaximize);
    connect(ui->closeButton, &QPushButton::clicked, this, &MainWindow::close);

    // 设置和账户变化
    connect(settingDialog, &SettingDialog::themeChanged, this, &MainWindow::loadTheme);
    connect(accountDialog, &AccountDialog::accountChanged, this, &MainWindow::onAccountChanged);

    // 系统托盘
    connect(trayIcon, &TrayIcon::activated, this, &MainWindow::trayIconActivated);
    connect(trayIcon, &TrayIcon::restoreRequested, this, &MainWindow::restoreFromTray);

    // 邮件客户端信号
    connect(emailClient, &EmailClient::newEmailReceived, this, &MainWindow::onEmailReceived);
    connect(emailClient, &EmailClient::connectionStatusChanged, this, &MainWindow::onConnectionStatusChanged);
    connect(emailClient, &EmailClient::emailSent, this, &MainWindow::onEmailSent);
    connect(emailClient, &EmailClient::errorOccurred, this, &MainWindow::onEmailError);

    // 线程完成信号
    connect(connectionWatcher, &QFutureWatcher<void>::finished, this, &MainWindow::onEmailConnectionFinished);
    connect(operationWatcher, &QFutureWatcher<void>::finished, this, &MainWindow::onEmailOperationFinished);

    // 定时器
    checkTimer->setInterval(60000); // 60秒检查一次新邮件
    connect(checkTimer, &QTimer::timeout, this, &MainWindow::checkNewEmails);

    uiUpdateTimer->setInterval(100); // 100ms UI 更新间隔
    connect(uiUpdateTimer, &QTimer::timeout, this, [this]() {
        if (!isOperating.load()) {
            uiUpdateTimer->stop();
        }
    });
}

void MainWindow::setupThreading()
{
    // 线程池配置已在 initializeComponents 中完成
}

void MainWindow::startInitialTasks()
{
    // 启动定时检查
    checkTimer->start();

    // 延迟启动邮件连接，避免阻塞 UI 初始化
    QTimer::singleShot(1000, this, &MainWindow::connectToEmailServerAsync);
}

void MainWindow::connectToEmailServerAsync()
{
    if (isConnecting.exchange(true)) {
        return; // 已经在连接中
    }

    if (!accountDialog) {
        isConnecting.store(false);
        qDebug() << "账户对话框未初始化";
        return;
    }

    EmailAccount currentAccount = accountDialog->getCurrentAccount();
    if (currentAccount.email.isEmpty()) {
        isConnecting.store(false);
        qDebug() << "没有配置邮件账户，跳过连接";
        return;
    }

    qDebug() << "开始异步连接邮件服务器...";

    if (!emailClient) {
        isConnecting.store(false);
        qDebug() << "邮件客户端未初始化";
        return;
    }

    QFuture<void> future = QtConcurrent::run([this, currentAccount]() {
        emailClient->connectToServer(currentAccount);
    });

    if (connectionWatcher) {
        connectionWatcher->setFuture(future);
    }
}

void MainWindow::performEmailOperationAsync(const std::function<void()>& operation)
{
    if (isOperating.exchange(true)) {
        return; // 已经在执行操作
    }

    if (!operation) {
        isOperating.store(false);
        return;
    }

    QFuture<void> future = QtConcurrent::run([this, operation]() {
        operation();
    });

    if (operationWatcher) {
        operationWatcher->setFuture(future);
    }

    if (uiUpdateTimer) {
        uiUpdateTimer->start();
    }
}

void MainWindow::onEmailConnectionFinished()
{
    isConnecting.store(false);
    qDebug() << "邮件连接操作完成";
}

void MainWindow::onEmailOperationFinished()
{
    isOperating.store(false);
    qDebug() << "邮件操作完成";
}

void MainWindow::onAccountChanged(const QString &email)
{
    qDebug() << "切换到账户:" << email;
    connectToEmailServerAsync();
}

void MainWindow::onEmailReceived(const QString &from, const QString &subject, const QString &body, bool isHtml, const QStringList &attachments)
{
    // 线程安全地添加新邮件
    QMetaObject::invokeMethod(this, [this, from, subject, body, isHtml, attachments]() {
        Email newEmail;
        newEmail.id = QUuid::createUuid().toString();
        newEmail.sender = from;
        newEmail.subject = subject;
        newEmail.content = body;
        newEmail.isHtml = isHtml;
        newEmail.attachments = attachments;
        newEmail.time = QDateTime::currentDateTime();
        newEmail.isRead = false;
        newEmail.isFavorite = false;

        {
            QMutexLocker locker(&emailMutex);
            inboxEmails.prepend(newEmail);
        }

        if (currentView == Inbox) {
            updateEmailList();
        }

        showNotification("新邮件", QString("来自: %1\n主题: %2").arg(from, subject));
        qDebug() << "收到新邮件 - 发件人:" << from << "主题:" << subject;
    }, Qt::QueuedConnection);
}

void MainWindow::onConnectionStatusChanged(bool connected)
{
    QMetaObject::invokeMethod(this, [this, connected]() {
        updateUIState(connected);
        if (connected) {
            qDebug() << "邮件服务器连接成功";
            performEmailOperationAsync([this]() {
                emailClient->fetchEmails();
            });
        } else {
            qDebug() << "邮件服务器断开连接";
        }
    }, Qt::QueuedConnection);
}

void MainWindow::onEmailSent(bool success)
{
    QMetaObject::invokeMethod(this, [this, success]() {
        if (success) {
            QMessageBox::information(this, "发送成功", "邮件发送成功！");
            if (currentView == Sent) {
                updateEmailList();
            }
        } else {
            QMessageBox::warning(this, "发送失败", "邮件发送失败，请检查网络连接和账户设置。");
        }
    }, Qt::QueuedConnection);
}

void MainWindow::onEmailError(const QString &error)
{
    QMetaObject::invokeMethod(this, [this, error]() {
        handleEmailError(error);
    }, Qt::QueuedConnection);
}

void MainWindow::checkNewEmails()
{
    if (emailClient && emailClient->isConnected()) {
        performEmailOperationAsync([this]() {
            emailClient->fetchEmails();
        });
    } else {
        connectToEmailServerAsync();
    }
}

void MainWindow::updateUIState(bool connected)
{
    // 更新 UI 状态指示器
    QString statusText = connected ? "已连接" : "未连接";
    // 这里可以更新状态栏或其他 UI 元素
    qDebug() << "UI 状态更新:" << statusText;
}

void MainWindow::handleEmailError(const QString &error)
{
    qWarning() << "邮件错误:" << error;
    QMessageBox::warning(this, "邮件错误", error);
}

// UI 交互槽函数实现
void MainWindow::onInboxClicked()
{
    currentView = Inbox;
    ui->inboxButton->setChecked(true);
    ui->sendButton->setChecked(false);
    ui->favoriteButton->setChecked(false);
    ui->trashButton->setChecked(false);
    updateEmailList();
    if (ui->favoriteContentButton) {
        ui->favoriteContentButton->setVisible(false);
    }
}

void MainWindow::onSendClicked()
{
    currentView = Sent;
    ui->inboxButton->setChecked(false);
    ui->sendButton->setChecked(true);
    ui->favoriteButton->setChecked(false);
    ui->trashButton->setChecked(false);
    updateEmailList();
    if (ui->favoriteContentButton) {
        ui->favoriteContentButton->setVisible(false);
    }
}

void MainWindow::onFavoriteClicked()
{
    currentView = Favorite;
    ui->inboxButton->setChecked(false);
    ui->sendButton->setChecked(false);
    ui->favoriteButton->setChecked(true);
    ui->trashButton->setChecked(false);
    updateEmailList();
    if (ui->favoriteContentButton) {
        ui->favoriteContentButton->setVisible(false);
    }
}

void MainWindow::onTrashClicked()
{
    currentView = Trash;
    ui->inboxButton->setChecked(false);
    ui->sendButton->setChecked(false);
    ui->favoriteButton->setChecked(false);
    ui->trashButton->setChecked(true);
    updateEmailList();
    if (ui->favoriteContentButton) {
        ui->favoriteContentButton->setVisible(false);
    }
}

void MainWindow::onAccountClicked()
{
    if (accountDialog) {
        accountDialog->exec();
    }
}

void MainWindow::onSettingClicked()
{
    if (settingDialog) {
        settingDialog->exec();
    }
}

void MainWindow::onMinimizeClicked()
{
    if (settingDialog && settingDialog->getMinimizeToTray()) {
        hide();
    } else {
        showMinimized();
    }
}

void MainWindow::onEmailItemClicked(QListWidgetItem *item)
{
    if (!item || !ui) return;

    QString emailId = item->data(Qt::UserRole).toString();

    QMutexLocker locker(&emailMutex);
    QList<Email> *currentEmails = nullptr;

    switch(currentView) {
    case Inbox: currentEmails = &inboxEmails; break;
    case Sent: currentEmails = &sentEmails; break;
    case Favorite: currentEmails = &favoriteEmails; break;
    case Trash: currentEmails = &trashEmails; break;
    }

    if (currentEmails) {
        Email *email = findEmailById(emailId, *currentEmails);
        if (email) {
            showEmailContent(*email);
            email->isRead = true;
            updateEmailList();
        }
    }
}

void MainWindow::onComposeClicked()
{
    ComposeDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        Email email = dialog.getEmail();

        if (accountDialog) {
            EmailAccount currentAccount = accountDialog->getCurrentAccount();
            email.sender = currentAccount.email;
        }

        performEmailOperationAsync([this, email]() {
            if (emailClient) {
                emailClient->sendEmail(email.sender, email.subject, email.content, email.attachments);
            }
        });

        email.id = QUuid::createUuid().toString();
        email.time = QDateTime::currentDateTime();
        sentEmails.prepend(email);

        if (currentView == Sent) {
            updateEmailList();
        }
    }
}

void MainWindow::onReplyClicked()
{
    if (currentEmailId.isEmpty()) {
        QMessageBox::information(this, "提示", "请先选择要回复的邮件");
        return;
    }

    // 实现回复逻辑
    ComposeDialog dialog(this);
    // 设置回复模式
    dialog.exec();
}

void MainWindow::onFavoriteContentClicked()
{
    if (currentEmailId.isEmpty() || !ui || !ui->favoriteContentButton) return;

    QMutexLocker locker(&emailMutex);
    Email *targetEmail = nullptr;

    switch(currentView) {
    case Inbox: targetEmail = findEmailById(currentEmailId, inboxEmails); break;
    case Sent: targetEmail = findEmailById(currentEmailId, sentEmails); break;
    case Favorite: targetEmail = findEmailById(currentEmailId, favoriteEmails); break;
    case Trash: targetEmail = findEmailById(currentEmailId, trashEmails); break;
    }

    if (targetEmail) {
        targetEmail->isFavorite = !targetEmail->isFavorite;
        ui->favoriteContentButton->setChecked(targetEmail->isFavorite);
        ui->favoriteContentButton->setText(targetEmail->isFavorite ? "已收藏" : "收藏");

        if (currentView == Favorite) {
            updateEmailList();
        }
    }
}

// 系统托盘相关
void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        restoreFromTray();
    }
}

void MainWindow::restoreFromTray()
{
    showNormal();
    activateWindow();
    raise();
}

void MainWindow::toggleMaximize()
{
    if (isMaximized()) {
        showNormal();
        if (ui) ui->maximizeButton->setText("□");
    } else {
        showMaximized();
        if (ui) ui->maximizeButton->setText("❐");
    }
}

// UI 相关方法
void MainWindow::setupUI()
{
    if (!ui) return;

    setWindowTitle("Yanyn Email");
    setWindowIcon(QIcon(":/resources/icon.png"));
    resize(1200, 800);

    // 设置样式表和其他 UI 元素
    if (ui->centralwidget) {
        ui->centralwidget->setStyleSheet(R"(
            QWidget#centralwidget {
                background: white;
                border-radius: 16px;
                border: 1px solid #e0e0e0;
            }
        )");
    }

    setupTitleBar();

    // 设置按钮样式
    QString buttonStyle = R"(
        QPushButton {
            background: transparent;
            border: none;
            border-radius: 8px;
            padding: 0px;
        }
        QPushButton:hover { background: rgba(0, 0, 0, 0.08); }
        QPushButton:checked { background: rgba(0, 120, 212, 0.15); }
    )";

    if (ui->inboxButton) ui->inboxButton->setStyleSheet(buttonStyle);
    if (ui->sendButton) ui->sendButton->setStyleSheet(buttonStyle);
    if (ui->favoriteButton) ui->favoriteButton->setStyleSheet(buttonStyle);
    if (ui->trashButton) ui->trashButton->setStyleSheet(buttonStyle);
    if (ui->accountButton) ui->accountButton->setStyleSheet(buttonStyle);
    if (ui->settingButton) ui->settingButton->setStyleSheet(buttonStyle);
}

void MainWindow::setupTitleBar()
{
    if (!ui || !ui->titleBar) return;

    ui->titleBar->setStyleSheet(R"(
        QWidget#titleBar {
            background: #f8f9fa;
            border-top-left-radius: 16px;
            border-top-right-radius: 16px;
            border-bottom: 1px solid #e9ecef;
        }
    )");

    if (ui->minimizeButton) ui->minimizeButton->setText("—");
    if (ui->maximizeButton) ui->maximizeButton->setText("□");
    if (ui->closeButton) ui->closeButton->setText("×");
}

void MainWindow::loadTheme()
{
    if (!settingDialog) return;

    QString theme = settingDialog->getCurrentTheme();
    // 根据主题加载不同的样式表
    if (theme == "dark") {
        // 深色主题
        if (qApp) qApp->setStyleSheet("/* 深色主题样式 */");
    } else if (theme == "light") {
        // 浅色主题
        if (qApp) qApp->setStyleSheet("/* 浅色主题样式 */");
    } else {
        // 默认主题
        if (qApp) qApp->setStyleSheet("/* 默认主题样式 */");
    }
}

void MainWindow::updateEmailList()
{
    if (!ui || !ui->emailList) return;

    ui->emailList->clear();

    QMutexLocker locker(&emailMutex);
    QList<Email> *currentEmails = nullptr;

    switch(currentView) {
    case Inbox: currentEmails = &inboxEmails; break;
    case Sent: currentEmails = &sentEmails; break;
    case Favorite: currentEmails = &favoriteEmails; break;
    case Trash: currentEmails = &trashEmails; break;
    }

    if (currentEmails) {
        for (const Email &email : *currentEmails) {
            QListWidgetItem *item = new QListWidgetItem();
            QString displayText = QString("%1\n%2\n%3")
                .arg(email.sender)
                .arg(email.subject)
                .arg(email.time.toString("MM-dd hh:mm"));

            if (!email.isRead) {
                displayText.prepend("● ");
            }

            item->setText(displayText);
            item->setData(Qt::UserRole, email.id);
            ui->emailList->addItem(item);
        }
    }
}

void MainWindow::showEmailContent(const Email &email)
{
    if (!ui) return;

    QString content = email.isHtml ? email.content :
        QString("<pre>%1</pre>").arg(email.content.toHtmlEscaped());

    if (!email.attachments.isEmpty()) {
        content += "<hr><h4>附件:</h4><ul>";
        for (const QString &attachment : email.attachments) {
            content += QString("<li>%1</li>").arg(attachment);
        }
        content += "</ul>";
    }

    if (ui->emailContent) {
        ui->emailContent->setHtml(content);
    }

    if (ui->favoriteContentButton) {
        ui->favoriteContentButton->setVisible(true);
        ui->favoriteContentButton->setChecked(email.isFavorite);
        ui->favoriteContentButton->setText(email.isFavorite ? "已收藏" : "收藏");
    }

    currentEmailId = email.id;
}

void MainWindow::showNotification(const QString &title, const QString &message)
{
    if (trayIcon) {
        trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 5000);
    }
}

// 事件处理
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (settingDialog && settingDialog->getMinimizeToTray()) {
        hide();
        event->ignore();
    } else {
        event->accept();
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !isMaximized()) {
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton && !isMaximized()) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(QRectF(0, 0, width(), height()), 16, 16);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255));
    painter.drawPath(path);
}

// 辅助方法
Email* MainWindow::findEmailById(const QString &id, QList<Email> &emailList)
{
    auto it = std::find_if(emailList.begin(), emailList.end(),
        [&id](const Email &email) { return email.id == id; });
    return (it != emailList.end()) ? &(*it) : nullptr;
}
