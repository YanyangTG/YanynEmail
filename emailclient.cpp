#include "emailclient.h"
#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QtConcurrent/QtConcurrent>

EmailClient::EmailClient(QObject *parent) : QObject(parent)
    , m_connected(false)
    , m_timeoutTimer(new QTimer(this))
    , m_workerThread(new QThread(this))
{
    // 设置超时定时器
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &EmailClient::onTimeout);
}

EmailClient::~EmailClient()
{
    disconnectFromServer();
}

void EmailClient::connectToServer(const EmailAccount &account)
{
    // 启动超时定时器（10秒超时）
    m_timeoutTimer->start(10000);
    m_currentAccount = account;
    m_connected = false;

    qDebug() << "=== 连接邮件服务器 ===";
    qDebug() << "服务器:" << account.imapServer;
    qDebug() << "端口:" << account.imapPort;
    qDebug() << "加密:" << account.imapEncryption;
    qDebug() << "协议:" << account.protocol;

    bool success = false;

    if (account.protocol == "imap") {
        success = connectImapServer();
    } else {
        success = connectPop3Server();
    }

    if (success) {
        m_connected = true;
        emit connectionStatusChanged(true);
        qDebug() << "成功连接到邮件服务器";
    } else {
        emit errorOccurred("连接失败: 无法连接到邮件服务器");
    }

    // 停止超时定时器
    m_timeoutTimer->stop();
}

bool EmailClient::connectImapServer()
{
    try {
        // 创建 imap 对象作为成员变量使用
        m_imap = std::make_unique<mailio::imap>(m_currentAccount.imapServer.toStdString(),
                                               m_currentAccount.imapPort);

        // 设置SSL/TLS
        if (m_currentAccount.imapEncryption == "ssl") {
            m_imap->start_tls(true);
        }

        // 使用正确的认证方法
        m_imap->authenticate(m_currentAccount.email.toStdString(),
                            m_currentAccount.password.toStdString(),
                            mailio::imap::auth_method_t::LOGIN);

        return true;
    } catch (const std::exception& e) {
        emit errorOccurred(QString("IMAP连接失败: %1").arg(e.what()));
        return false;
    }
}

bool EmailClient::connectPop3Server()
{
    try {
        // 创建 pop3 对象作为成员变量使用
        m_pop3 = std::make_unique<mailio::pop3>(m_currentAccount.imapServer.toStdString(),
                                               m_currentAccount.imapPort);

        // 设置SSL/TLS
        if (m_currentAccount.imapEncryption == "ssl") {
            m_pop3->start_tls(true);
        }

        // 使用正确的认证方法
        m_pop3->authenticate(m_currentAccount.email.toStdString(),
                            m_currentAccount.password.toStdString(),
                            mailio::pop3::auth_method_t::LOGIN);

        return true;
    } catch (const std::exception& e) {
        emit errorOccurred(QString("POP3连接失败: %1").arg(e.what()));
        return false;
    }
}

void EmailClient::disconnectFromServer()
{
    qDebug() << "断开邮件服务器连接...";
    m_connected = false;
    m_imap.reset();
    m_pop3.reset();
    m_smtp.reset();
    emit connectionStatusChanged(false);
    qDebug() << "已断开邮件服务器连接";
}

void EmailClient::fetchEmails()
{
    if (!m_connected) {
        emit errorOccurred("未连接到服务器");
        return;
    }

    bool success = false;

    if (m_currentAccount.protocol == "imap") {
        success = fetchImapEmails();
    } else {
        success = fetchPop3Emails();
    }

    if (!success) {
        emit errorOccurred("获取邮件失败");
    }
}


bool EmailClient::fetchImapEmails()
{
    try {
        // 使用成员变量 m_imap 而不是局部变量
        if (!m_imap) {
            emit errorOccurred("IMAP连接未建立");
            return false;
        }

        // 列出文件夹，需要提供文件夹名称
        auto folders = m_imap->list_folders("INBOX");

        // 获取统计信息，需要提供邮箱名称（注释掉未使用的变量）
        // auto stats = m_imap->statistics("INBOX");

        // 选择INBOX
        if (folders.folders.find("INBOX") != folders.folders.end()) {
            m_imap->select("INBOX");

            // 获取邮件数量 - 修正成员名称
            auto mailbox_stats = m_imap->statistics("INBOX");
            int total_messages = mailbox_stats.messages_no;  // 修正为 messages_no

            // 获取最新的几封邮件（避免加载过多）
            int start_msg = std::max(1, total_messages - 10);
            for (int i = start_msg; i <= total_messages; ++i) {
                try {
                    mailio::message msg;
                    m_imap->fetch(i, msg);

                    QString from = QString::fromStdString(msg.from_to_string());
                    QString subject = QString::fromStdString(msg.subject());
                    QString body = QString::fromStdString(msg.content());

                    // 简化HTML检测逻辑
                    bool is_html = false; // 暂时简化处理

                    QStringList attachments;
                    // 简化附件处理 - 修正参数传递
                    if (msg.attachments_size() > 0) {
                        for (size_t j = 0; j < msg.attachments_size(); ++j) {
                            std::ostringstream att_stream;
                            mailio::string_t att_name;  // 使用正确的类型
                            msg.attachment(j, att_stream, att_name);
                            // 修正获取附件名称的方式
                            attachments << QString::fromStdString(static_cast<std::string>(att_name));
                        }
                    }

                    emit newEmailReceived(from, subject, body, is_html, attachments);
                } catch (const std::exception& e) {
                    qDebug() << "获取邮件" << i << "失败:" << e.what();
                }
            }
        }

        return true;
    } catch (const std::exception& e) {
        m_lastError = QString::fromStdString(e.what());
        emit errorOccurred(m_lastError);
        return false;
    }
}

bool EmailClient::fetchPop3Emails()
{
    try {
        if (!m_pop3) {
            emit errorOccurred("POP3连接未建立");
            return false;
        }

        // 获取邮件列表
        auto message_list = m_pop3->list();

        // 获取最新的几封邮件
        int count = 0;
        for (auto it = message_list.rbegin(); it != message_list.rend() && count < 10; ++it, ++count) {
            try {
                mailio::message msg;
                m_pop3->fetch(it->first, msg);

                QString from = QString::fromStdString(msg.from_to_string());
                QString subject = QString::fromStdString(msg.subject());
                QString body = QString::fromStdString(msg.content());

                // 简化HTML检测逻辑
                bool is_html = false; // 暂时简化处理

                QStringList attachments;
                // 简化附件处理 - 修正参数传递
                if (msg.attachments_size() > 0) {
                    for (size_t j = 0; j < msg.attachments_size(); ++j) {
                        std::ostringstream att_stream;
                        mailio::string_t att_name;  // 使用正确的类型
                        msg.attachment(j, att_stream, att_name);
                        // 修正获取附件名称的方式
                        attachments << QString::fromStdString(static_cast<std::string>(att_name));
                    }
                }

                emit newEmailReceived(from, subject, body, is_html, attachments);
            } catch (const std::exception& e) {
                qDebug() << "获取POP3邮件失败:" << e.what();
            }
        }

        return true;
    } catch (const std::exception& e) {
        m_lastError = QString::fromStdString(e.what());
        emit errorOccurred(m_lastError);
        return false;
    }
}

void EmailClient::sendEmail(const QString &to, const QString &subject,
                           const QString &body, const QStringList &attachments)
{
    // 在新的线程中发送邮件 - 修正 QtConcurrent 警告
    QFuture<void> future = QtConcurrent::run([this, to, subject, body, attachments]() {
        bool success = sendSmtpEmail(to, subject, body, attachments);
        emit emailSent(success);
    });
    // 可以选择忽略返回值或者保存 future 以供后续使用
    Q_UNUSED(future);
}

bool EmailClient::connectSmtpServer()
{
    try {
        m_smtp = std::make_unique<mailio::smtp>(m_currentAccount.smtpServer.toStdString(),
                                               m_currentAccount.smtpPort);

        if (m_currentAccount.smtpEncryption == "ssl") {
            m_smtp->start_tls(true);
        }

        m_smtp->authenticate(m_currentAccount.email.toStdString(),
                            m_currentAccount.password.toStdString(),
                            mailio::smtp::auth_method_t::LOGIN);

        return true;
    } catch (const std::exception& e) {
        emit errorOccurred(QString("SMTP连接失败: %1").arg(e.what()));
        return false;
    }
}

bool EmailClient::sendSmtpEmail(const QString &to, const QString &subject,
                               const QString &body, const QStringList &attachments)
{
    try {
        // 如果SMTP连接不存在，创建新连接
        if (!m_smtp) {
            m_smtp = std::make_unique<mailio::smtp>(m_currentAccount.smtpServer.toStdString(),
                                                   m_currentAccount.smtpPort);

            if (m_currentAccount.smtpEncryption == "ssl") {
                m_smtp->start_tls(true);
            }

            m_smtp->authenticate(m_currentAccount.email.toStdString(),
                                m_currentAccount.password.toStdString(),
                                mailio::smtp::auth_method_t::LOGIN);
        }

        mailio::message msg;
        msg.from(mailio::mail_address(m_currentAccount.email.toStdString(),
                                     m_currentAccount.email.toStdString()));
        msg.add_recipient(mailio::mail_address(to.toStdString(), to.toStdString()));
        msg.subject(subject.toStdString());
        msg.content(body.toStdString());

        // 添加附件 - 修正附件添加方式
        std::list<std::tuple<std::istream&, mailio::string_t, mailio::mime::content_type_t>> attachments_list;
        for (const QString& filePath : attachments) {
            QFile file(filePath);
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray fileData = file.readAll();
                file.close();

                QFileInfo fileInfo(filePath);
                std::string filename = fileInfo.fileName().toStdString();

                // 创建字符串流
                std::string fileContent(fileData.constData(), fileData.size());
                std::istringstream fileStream(fileContent);

                // 创建正确的 content_type_t 对象
                mailio::mime::content_type_t contentType(mailio::mime::media_type_t::APPLICATION, "octet-stream");

                // 添加到附件列表
                attachments_list.push_back(std::make_tuple(
                    std::ref(fileStream),
                    mailio::string_t(filename),
                    contentType
                ));
            }
        }

        // 如果有附件则添加
        if (!attachments_list.empty()) {
            msg.attach(attachments_list);
        }

        m_smtp->submit(msg);
        return true;
    } catch (const std::exception& e) {
        m_lastError = QString::fromStdString(e.what());
        emit errorOccurred(m_lastError);
        return false;
    }
}


void EmailClient::onTimeout()
{
    emit errorOccurred("操作超时");
}
