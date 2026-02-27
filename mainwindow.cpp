#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingdialog.h"
#include "composedialog.h"
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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , settingDialog(new SettingDialog(this))
    , accountDialog(new AccountDialog(this))
    , emailClient(new EmailClient(this))
    , trayIcon(new TrayIcon(this))
    , checkTimer(new QTimer(this))
    , currentView(Inbox)
{
    // 移除默认标题栏并设置透明背景以实现圆角
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    ui->setupUi(this);

    setupUI();
    loadTheme();
    updateEmailList();



    show();
    activateWindow();
    raise();
    setFocus();

    // 连接信号槽
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

    connect(settingDialog, &SettingDialog::themeChanged, this, &MainWindow::loadTheme);
    connect(trayIcon, &TrayIcon::activated, this, &MainWindow::trayIconActivated);
    connect(trayIcon, &TrayIcon::restoreRequested, this, &MainWindow::restoreFromTray);
    connect(accountDialog, &AccountDialog::accountChanged, this, &MainWindow::onAccountChanged);

    // 邮件客户端信号连接
    connect(emailClient, &EmailClient::newEmailReceived, this, &MainWindow::onEmailReceived);
    connect(emailClient, &EmailClient::connectionStatusChanged, this, &MainWindow::onConnectionStatusChanged);
    connect(emailClient, &EmailClient::emailSent, this, &MainWindow::onEmailSent);
    connect(emailClient, &EmailClient::errorOccurred, this, &MainWindow::onEmailError);

    // 自定义标题栏按钮连接
    connect(ui->minimizeButton, &QPushButton::clicked, this, &MainWindow::showMinimized);
    connect(ui->maximizeButton, &QPushButton::clicked, this, &MainWindow::toggleMaximize);
    connect(ui->closeButton, &QPushButton::clicked, this, &MainWindow::close);

    // 定时检查新邮件
    checkTimer->setInterval(60000); // 60秒检查一次
    connect(checkTimer, &QTimer::timeout, this, &MainWindow::checkNewEmails);
    checkTimer->start();

    // 尝试连接到邮件服务器
    connectToEmailServer();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::connectToEmailServer()
{
    EmailAccount currentAccount = accountDialog->getCurrentAccount();
    if (!currentAccount.email.isEmpty()) {
        emailClient->connectToServer(currentAccount);
    }
}

void MainWindow::onAccountChanged(const QString &email)
{
    qDebug() << "切换到账户:" << email;
    // 重新连接到新的邮件服务器
    connectToEmailServer();
}

void MainWindow::onEmailReceived(const QString &from, const QString &subject, const QString &body, bool isHtml, const QStringList &attachments)
{
    // 创建新邮件对象
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

    // 添加到收件箱
    inboxEmails.prepend(newEmail);

    // 更新界面
    if (currentView == Inbox) {
        updateEmailList();
    }

    // 显示通知
    showNotification("新邮件", QString("来自: %1\n主题: %2").arg(from).arg(subject));

    qDebug() << "收到新邮件 - 发件人:" << from << "主题:" << subject;
}

void MainWindow::onConnectionStatusChanged(bool connected)
{
    if (connected) {
        qDebug() << "邮件服务器连接成功";
        // 开始收取邮件
        emailClient->fetchEmails();
    } else {
        qDebug() << "邮件服务器断开连接";
    }
}

void MainWindow::onEmailSent(bool success)
{
    if (success) {
        QMessageBox::information(this, "发送成功", "邮件发送成功！");

        // 刷新已发送列表
        if (currentView == Sent) {
            updateEmailList();
        }
    } else {
        QMessageBox::warning(this, "发送失败", "邮件发送失败，请检查网络连接和账户设置。");
    }
}

void MainWindow::onEmailError(const QString &error)
{
    qWarning() << "邮件错误:" << error;
    QMessageBox::warning(this, "邮件错误", error);
}

void MainWindow::checkNewEmails()
{
    if (emailClient->isConnected()) {
        emailClient->fetchEmails();
    } else {
        // 如果未连接，尝试重新连接
        connectToEmailServer();
    }
}

void MainWindow::setupUI()
{
    setWindowTitle("Yanyn Email");
    setWindowIcon(QIcon(":/resources/icon.png"));
    resize(1200, 800);

    // 设置中央widget的圆角样式
    ui->centralwidget->setStyleSheet(R"(
        QWidget#centralwidget {
            background: white;
            border-radius: 16px;
            border: 1px solid #e0e0e0;
        }
    )");

    // 设置自定义标题栏
    setupTitleBar();

    // 设置左边栏更细
    ui->sidebarWidget->setMinimumWidth(60);
    ui->sidebarWidget->setMaximumWidth(60);

    // 设置按钮样式 - 圆角设计
    QString buttonStyle = R"(
        QPushButton {
            background: transparent;
            border: none;
            border-radius: 8px;
            padding: 0px;
            font-family: "Source Han Sans CN";
        }
        QPushButton:hover {
            background: rgba(0, 0, 0, 0.08);
        }
        QPushButton:checked {
            background: rgba(0, 120, 212, 0.15);
            border-radius: 8px;
        }
        QPushButton:pressed {
            background: rgba(0, 120, 212, 0.25);
            border-radius: 8px;
        }
    )";

    // 设置图标尺寸以适应更窄的边栏
    QSize iconSize(24, 24);

    // 设置各个按钮的图标 - 修正路径
    ui->inboxButton->setIcon(QIcon(":/resources/inbox.png"));
    ui->inboxButton->setIconSize(iconSize);
    ui->inboxButton->setCheckable(true);
    ui->inboxButton->setChecked(true);
    ui->inboxButton->setStyleSheet(buttonStyle);
    ui->inboxButton->setFixedSize(50, 50);
    ui->inboxButton->setToolTip("收件箱");

    ui->sendButton->setIcon(QIcon(":/resources/send.png"));
    ui->sendButton->setIconSize(iconSize);
    ui->sendButton->setCheckable(true);
    ui->sendButton->setStyleSheet(buttonStyle);
    ui->sendButton->setFixedSize(50, 50);
    ui->sendButton->setToolTip("已发送");

    ui->favoriteButton->setIcon(QIcon(":/resources/like.png"));
    ui->favoriteButton->setIconSize(iconSize);
    ui->favoriteButton->setCheckable(true);
    ui->favoriteButton->setStyleSheet(buttonStyle);
    ui->favoriteButton->setFixedSize(50, 50);
    ui->favoriteButton->setToolTip("收藏夹");

    ui->trashButton->setIcon(QIcon(":/resources/garbage.png"));
    ui->trashButton->setIconSize(iconSize);
    ui->trashButton->setCheckable(true);
    ui->trashButton->setStyleSheet(buttonStyle);
    ui->trashButton->setFixedSize(50, 50);
    ui->trashButton->setToolTip("垃圾箱");

    // 添加账户切换按钮
    ui->accountButton->setIcon(QIcon(":/resources/user.png"));
    ui->accountButton->setIconSize(iconSize);
    ui->accountButton->setStyleSheet(buttonStyle);
    ui->accountButton->setFixedSize(50, 50);
    ui->accountButton->setToolTip("账户管理");

    ui->settingButton->setIcon(QIcon(":/resources/setting.png"));
    ui->settingButton->setIconSize(iconSize);
    ui->settingButton->setStyleSheet(buttonStyle);
    ui->settingButton->setFixedSize(50, 50);
    ui->settingButton->setToolTip("设置");

    // 调整垂直布局的间距
    ui->verticalLayout->setSpacing(8);
    ui->verticalLayout->setContentsMargins(5, 10, 5, 10);

    // 设置侧边栏背景 - 圆角设计
    ui->sidebarWidget->setStyleSheet(R"(
        QWidget#sidebarWidget {
            background: #f8f9fa;
            border-top-left-radius: 16px;
            border-bottom-left-radius: 16px;
            font-family: "Source Han Sans CN";
        }
    )");

    // 设置邮件列表样式 - 圆角设计
    ui->emailList->setStyleSheet(R"(
        QListWidget {
            background: white;
            border: none;
            outline: none;
            border-radius: 8px;
            font-family: "Source Han Sans CN";
        }
        QListWidget::item {
            border-bottom: 1px solid #f0f0f0;
            padding: 12px;
            background: white;
            font-family: "Source Han Sans CN";
        }
        QListWidget::item:hover {
            background: #f8f9fa;
        }
        QListWidget::item:selected {
            background: #e6f3ff;
            border: none;
        }
    )");

    // 设置邮件内容区域 - 圆角设计
    ui->emailContent->setReadOnly(true);
    ui->emailContent->setStyleSheet(R"(
        QTextEdit {
            background: white;
            border: none;
            border-radius: 8px;
            font-family: "Source Han Sans CN";
            padding: 8px;
        }
    )");

    // 设置底部按钮样式（如果这些按钮在UI中存在）
    // 如果UI中不存在这些按钮，请注释掉以下代码
    if (ui->composeButton) {
        ui->composeButton->setStyleSheet(R"(
            QPushButton {
                background: #0078d4;
                color: white;
                border: none;
                border-radius: 6px;
                padding: 8px 16px;
                font-family: "Source Han Sans CN";
            }
            QPushButton:hover {
                background: #106ebe;
            }
        )");
        ui->composeButton->setIcon(QIcon(":/resources/send.png"));
        ui->composeButton->setIconSize(QSize(16, 16));
    }

    if (ui->replyButton) {
        ui->replyButton->setStyleSheet(R"(
            QPushButton {
                background: #28a745;
                color: white;
                border: none;
                border-radius: 6px;
                padding: 8px 16px;
                font-family: "Source Han Sans CN";
            }
            QPushButton:hover {
                background: #218838;
            }
        )");
    }

    if (ui->favoriteContentButton) {
        ui->favoriteContentButton->setStyleSheet(R"(
            QPushButton {
                background: transparent;
                border: 1px solid #dc3545;
                border-radius: 6px;
                padding: 6px 12px;
                font-family: "Source Han Sans CN";
                color: #dc3545;
            }
            QPushButton:hover {
                background: #dc3545;
                color: white;
            }
            QPushButton:checked {
                background: #dc3545;
                color: white;
            }
        )");
        ui->favoriteContentButton->setCheckable(true);
        ui->favoriteContentButton->setIcon(QIcon(":/resources/like.png"));
        ui->favoriteContentButton->setIconSize(QSize(14, 14));
    }

    // 设置分割器样式
    ui->splitter->setStyleSheet("QSplitter::handle { background: #f0f0f0; border-radius: 2px; }");

    // 初始隐藏收藏按钮（如果存在）
    if (ui->favoriteContentButton) {
        ui->favoriteContentButton->setVisible(false);
    }
}

void MainWindow::setupTitleBar()
{
    // 标题栏样式 - 圆角设计
    QString titleBarStyle = R"(
        QWidget#titleBar {
            background: #f8f9fa;
            border-top-left-radius: 16px;
            border-top-right-radius: 16px;
            border-bottom: 1px solid #e9ecef;
            font-family: "Source Han Sans CN";
        }
        QLabel#titleLabel {
            color: #333333;
            font-size: 14px;
            font-weight: bold;
            qproperty-alignment: AlignCenter;
            background: transparent;
            border: none;
            font-family: "Source Han Sans CN";
        }
        QLabel#iconLabel {
            background: transparent;
            border: none;
            border-radius: 6px;
        }
        QPushButton#titleButton {
            background: transparent;
            border: none;
            padding: 4px;
            border-radius: 4px;
            font-family: "Source Han Sans CN";
        }
        QPushButton#titleButton:hover {
            background: #e9ecef;
            border-radius: 4px;
        }
        QPushButton#closeButton {
            background: transparent;
            border: none;
            padding: 4px;
            border-radius: 4px;
            font-family: "Source Han Sans CN";
        }
        QPushButton#closeButton:hover {
            background: #dc3545;
            color: white;
            border-radius: 4px;
        }
    )";

    ui->titleBar->setStyleSheet(titleBarStyle);
    ui->titleBar->setFixedHeight(36);

    // 设置简约的标题栏按钮图标
    ui->minimizeButton->setText("—");
    ui->minimizeButton->setStyleSheet("QPushButton { font-size: 16px; font-weight: bold; color: #666; } QPushButton:hover { background: #e9ecef; }");

    ui->maximizeButton->setText("□");
    ui->maximizeButton->setStyleSheet("QPushButton { font-size: 12px; font-weight: bold; color: #666; } QPushButton:hover { background: #e9ecef; }");

    ui->closeButton->setText("×");
    ui->closeButton->setStyleSheet("QPushButton { font-size: 16px; font-weight: bold; color: #666; } QPushButton:hover { background: #dc3545; color: white; }");

    ui->minimizeButton->setFixedSize(28, 28);
    ui->maximizeButton->setFixedSize(28, 28);
    ui->closeButton->setFixedSize(28, 28);

    // 设置左上角图标 - 修正路径
    QPixmap iconPixmap(":/resources/icon.png");
    if (!iconPixmap.isNull()) {
        // 缩放图标到合适大小
        QPixmap scaledPixmap = iconPixmap.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        ui->iconLabel->setPixmap(scaledPixmap);
        ui->iconLabel->setStyleSheet("background: transparent; border: none;");
        ui->iconLabel->setAlignment(Qt::AlignCenter);
    } else {
        // 如果图标文件不存在，使用备用方案
        ui->iconLabel->setText("✉");
        ui->iconLabel->setStyleSheet("font-size: 16px; color: #0078d4; background: transparent; border: none;");
        ui->iconLabel->setAlignment(Qt::AlignCenter);
        qWarning() << "图标文件 :/resources/icon.png 未找到，使用备用图标";
    }
}

// 绘制窗口圆角和阴影
void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制窗口背景和圆角
    QPainterPath path;
    path.addRoundedRect(QRectF(0, 0, width(), height()), 16, 16);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255));
    painter.drawPath(path);

    // 绘制边框
    painter.setPen(QPen(QColor(220, 220, 220), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
}


void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (isMaximized()) {
        // 最大化时不允许拖动
        return;
    }

    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (isMaximized()) {
        // 最大化时不允许拖动
        return;
    }

    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (settingDialog->getMinimizeToTray()) {
        hide();
        event->ignore();
    } else {
        event->accept();
    }
}

void MainWindow::toggleMaximize()
{
    if (isMaximized()) {
        showNormal();
        ui->maximizeButton->setText("□");
        // 恢复窗口可拖动
        setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        show();
    } else {
        showMaximized();
        ui->maximizeButton->setText("❐"); // 改变图标表示还原
        // 最大化时移除圆角
        setAttribute(Qt::WA_TranslucentBackground, false);
    }
}

void MainWindow::loadTheme()
{
    QString theme = settingDialog->getCurrentTheme();

    if (theme == "dark") {
        QFile file(":/resources/dark.qss");
        if (file.open(QFile::ReadOnly)) {
            QString styleSheet = QLatin1String(file.readAll());
            qApp->setStyleSheet(styleSheet);
        }
        // 深色主题下的样式调整
        ui->sidebarWidget->setStyleSheet(R"(
            QWidget#sidebarWidget {
                background: #2b2b2b;
                border-top-left-radius: 16px;
                border-bottom-left-radius: 16px;
            }
        )");
        ui->titleBar->setStyleSheet(R"(
            QWidget#titleBar {
                background: #1e1e1e;
                border-bottom: 1px solid #555555;
                border-top-left-radius: 16px;
                border-top-right-radius: 16px;
            }
            QLabel#titleLabel {
                color: #ffffff;
                background: transparent;
                border: none;
            }
            QLabel#iconLabel {
                background: transparent;
                border: none;
                color: #ffffff;
            }
            QPushButton#titleButton {
                background: transparent;
                border: none;
                color: #ffffff;
            }
            QPushButton#titleButton:hover {
                background: #555555;
                border-radius: 4px;
            }
            QPushButton#closeButton {
                background: transparent;
                border: none;
                color: #ffffff;
            }
            QPushButton#closeButton:hover {
                background: #dc3545;
                border-radius: 4px;
            }
        )");

    } else if (theme == "light") {
        QFile file(":/resources/light.qss");
        if (file.open(QFile::ReadOnly)) {
            QString styleSheet = QLatin1String(file.readAll());
            qApp->setStyleSheet(styleSheet);
        }
        // 浅色主题下的样式调整
        ui->sidebarWidget->setStyleSheet(R"(
            QWidget#sidebarWidget {
                background: #f5f5f5;
                border-top-left-radius: 16px;
                border-bottom-left-radius: 16px;
            }
        )");
        ui->titleBar->setStyleSheet(R"(
            QWidget#titleBar {
                background: #f8f9fa;
                border-bottom: 1px solid #dee2e6;
                border-top-left-radius: 16px;
                border-top-right-radius: 16px;
            }
            QLabel#titleLabel {
                color: #333333;
                background: transparent;
                border: none;
            }
            QLabel#iconLabel {
                background: transparent;
                border: none;
            }
            QPushButton#titleButton {
                background: transparent;
                border: none;
            }
            QPushButton#titleButton:hover {
                background: #e9ecef;
                border-radius: 6px;
            }
            QPushButton#closeButton {
                background: transparent;
                border: none;
            }
            QPushButton#closeButton:hover {
                background: #dc3545;
                color: white;
                border-radius: 6px;
            }
        )");

    } else {
        // 默认蔚蓝色主题
        QFile file(":/resources/blue.qss");
        if (file.open(QFile::ReadOnly)) {
            QString styleSheet = QLatin1String(file.readAll());
            qApp->setStyleSheet(styleSheet);
        }
        // 蓝色主题下的样式调整
        ui->sidebarWidget->setStyleSheet(R"(
            QWidget#sidebarWidget {
                background: #f0f8ff;
                border-top-left-radius: 16px;
                border-bottom-left-radius: 16px;
            }
        )");
        ui->titleBar->setStyleSheet(R"(
            QWidget#titleBar {
                background: #e6f3ff;
                border-bottom: 1px solid #b3d9ff;
                border-top-left-radius: 16px;
                border-top-right-radius: 16px;
            }
            QLabel#titleLabel {
                color: #333333;
                background: transparent;
                border: none;
            }
            QLabel#iconLabel {
                background: transparent;
                border: none;
            }
            QPushButton#titleButton {
                background: transparent;
                border: none;
            }
            QPushButton#titleButton:hover {
                background: #d1ebff;
                border-radius: 6px;
            }
            QPushButton#closeButton {
                background: transparent;
                border: none;
            }
            QPushButton#closeButton:hover {
                background: #dc3545;
                color: white;
                border-radius: 6px;
            }
        )");
    }
}

void MainWindow::updateEmailList()
{
    ui->emailList->clear();

    QList<Email> currentEmails;
    switch(currentView) {
    case Inbox:
        currentEmails = inboxEmails;
        break;
    case Sent:
        currentEmails = sentEmails;
        break;
    case Favorite:
        currentEmails = favoriteEmails;
        break;
    case Trash:
        currentEmails = trashEmails;
        break;
    }

    for (const Email &email : currentEmails) {
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

void MainWindow::showEmailContent(const Email &email)
{
    QString htmlContent;

    if (email.isHtml) {
        // HTML邮件
        htmlContent = email.content;
    } else {
        // 纯文本邮件，转换为HTML格式
        htmlContent = QString("<pre style='font-family: \"Source Han Sans CN\"; white-space: pre-wrap;'>%1</pre>")
                          .arg(email.content.toHtmlEscaped());
    }

    // 添加附件信息
    if (!email.attachments.isEmpty()) {
        htmlContent += "<hr><h4>附件:</h4><ul>";
        for (const QString &attachment : email.attachments) {
            htmlContent += QString("<li>%1</li>").arg(attachment);
        }
        htmlContent += "</ul>";
    }

    ui->emailContent->setHtml(htmlContent);

    // 显示收藏按钮并设置状态
    ui->favoriteContentButton->setVisible(true);
    ui->favoriteContentButton->setChecked(email.isFavorite);
    ui->favoriteContentButton->setText(email.isFavorite ? "已收藏" : "收藏");

    // 保存当前显示的邮件ID
    currentEmailId = email.id;
}

void MainWindow::onInboxClicked()
{
    currentView = Inbox;
    ui->inboxButton->setChecked(true);
    ui->sendButton->setChecked(false);
    ui->favoriteButton->setChecked(false);
    ui->trashButton->setChecked(false);
    updateEmailList();
    ui->favoriteContentButton->setVisible(false);
}

void MainWindow::onSendClicked()
{
    currentView = Sent;
    ui->inboxButton->setChecked(false);
    ui->sendButton->setChecked(true);
    ui->favoriteButton->setChecked(false);
    ui->trashButton->setChecked(false);
    updateEmailList();
    ui->favoriteContentButton->setVisible(false);
}

void MainWindow::onFavoriteClicked()
{
    currentView = Favorite;
    ui->inboxButton->setChecked(false);
    ui->sendButton->setChecked(false);
    ui->favoriteButton->setChecked(true);
    ui->trashButton->setChecked(false);
    updateEmailList();
    ui->favoriteContentButton->setVisible(false);
}

void MainWindow::onTrashClicked()
{
    currentView = Trash;
    ui->inboxButton->setChecked(false);
    ui->sendButton->setChecked(false);
    ui->favoriteButton->setChecked(false);
    ui->trashButton->setChecked(true);
    updateEmailList();
    ui->favoriteContentButton->setVisible(false);
}

void MainWindow::onAccountClicked()
{
    accountDialog->exec();
}

void MainWindow::onSettingClicked()
{
    settingDialog->exec();
}

void MainWindow::onComposeClicked()
{
    ComposeDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        // 获取邮件内容并发送
        Email email = dialog.getEmail();

        // 设置发件人为当前账户
        EmailAccount currentAccount = accountDialog->getCurrentAccount();
        email.sender = currentAccount.email;

        // 真实发送邮件
        emailClient->sendEmail(email.sender, email.subject, email.content, email.attachments);

        // 添加到已发送列表
        email.id = QUuid::createUuid().toString();
        email.time = QDateTime::currentDateTime();
        email.isHtml = dialog.isHtml(); // 需要为ComposeDialog添加这个方法
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

    // 查找当前显示的邮件
    Email originalEmail;
    bool found = false;

    QList<Email> *currentEmails = nullptr;
    switch(currentView) {
    case Inbox:
        currentEmails = &inboxEmails;
        break;
    case Sent:
        currentEmails = &sentEmails;
        break;
    case Favorite:
        currentEmails = &favoriteEmails;
        break;
    case Trash:
        currentEmails = &trashEmails;
        break;
    }

    if (currentEmails) {
        for (const Email &email : *currentEmails) {
            if (email.id == currentEmailId) {
                originalEmail = email;
                found = true;
                break;
            }
        }
    }

    if (!found) {
        QMessageBox::warning(this, "错误", "找不到要回复的邮件");
        return;
    }

    // 打开回复对话框
    ComposeDialog dialog(this);
    dialog.setReplyMode(originalEmail.sender, originalEmail.subject);
    if (dialog.exec() == QDialog::Accepted) {
        // 获取邮件内容并发送
        Email email = dialog.getEmail();

        // 设置发件人为当前账户
        EmailAccount currentAccount = accountDialog->getCurrentAccount();
        email.sender = currentAccount.email;

        // 真实发送邮件
        emailClient->sendEmail(email.sender, email.subject, email.content, email.attachments);

        // 添加到已发送列表
        email.id = QUuid::createUuid().toString();
        email.time = QDateTime::currentDateTime();
        email.isHtml = dialog.isHtml();
        sentEmails.prepend(email);

        if (currentView == Sent) {
            updateEmailList();
        }
    }
}

void MainWindow::onFavoriteContentClicked()
{
    if (currentEmailId.isEmpty()) {
        return;
    }

    // 查找当前显示的邮件
    Email *targetEmail = nullptr;

    QList<Email> *currentEmails = nullptr;
    switch(currentView) {
    case Inbox:
        currentEmails = &inboxEmails;
        break;
    case Sent:
        currentEmails = &sentEmails;
        break;
    case Favorite:
        currentEmails = &favoriteEmails;
        break;
    case Trash:
        currentEmails = &trashEmails;
        break;
    }

    if (currentEmails) {
        for (Email &email : *currentEmails) {
            if (email.id == currentEmailId) {
                targetEmail = &email;
                break;
            }
        }
    }

    if (!targetEmail) {
        return;
    }

    // 切换收藏状态
    targetEmail->isFavorite = !targetEmail->isFavorite;

    // 更新收藏按钮状态
    ui->favoriteContentButton->setChecked(targetEmail->isFavorite);
    ui->favoriteContentButton->setText(targetEmail->isFavorite ? "已收藏" : "收藏");

    // 如果当前在收藏夹视图，更新列表
    if (currentView == Favorite) {
        updateEmailList();
    }

    // 如果从其他文件夹收藏，添加到收藏夹
    if (targetEmail->isFavorite && currentView != Favorite) {
        // 检查是否已经在收藏夹中
        bool alreadyInFavorites = false;
        for (const Email &favEmail : favoriteEmails) {
            if (favEmail.id == targetEmail->id) {
                alreadyInFavorites = true;
                break;
            }
        }

        if (!alreadyInFavorites) {
            favoriteEmails.append(*targetEmail);
        }
    }
    // 如果取消收藏且当前在收藏夹，从收藏夹移除
    else if (!targetEmail->isFavorite && currentView == Favorite) {
        for (int i = 0; i < favoriteEmails.size(); ++i) {
            if (favoriteEmails[i].id == targetEmail->id) {
                favoriteEmails.removeAt(i);
                break;
            }
        }
        updateEmailList();
        ui->favoriteContentButton->setVisible(false);
    }
}

void MainWindow::onEmailItemClicked(QListWidgetItem *item)
{
    QString emailId = item->data(Qt::UserRole).toString();

    QList<Email> *currentEmails = nullptr;
    switch(currentView) {
    case Inbox:
        currentEmails = &inboxEmails;
        break;
    case Sent:
        currentEmails = &sentEmails;
        break;
    case Favorite:
        currentEmails = &favoriteEmails;
        break;
    case Trash:
        currentEmails = &trashEmails;
        break;
    }

    if (currentEmails) {
        for (Email &email : *currentEmails) {
            if (email.id == emailId) {
                showEmailContent(email);
                email.isRead = true;
                // 更新列表显示
                QString displayText = QString("%1\n%2\n%3")
                                          .arg(email.sender)
                                          .arg(email.subject)
                                          .arg(email.time.toString("MM-dd hh:mm"));
                item->setText(displayText);
                break;
            }
        }
    }
}

void MainWindow::showNotification(const QString &title, const QString &message)
{
    trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 5000);
}

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
}
