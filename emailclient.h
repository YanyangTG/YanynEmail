#ifndef EMAILCLIENT_H
#define EMAILCLIENT_H

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QString>
#include <QStringList>
#include <memory>
#include <chrono>
#include "accountdialog.h"  // 包含 EmailAccount 定义
#include "libs/mailio/include/imap.hpp"
#include "libs/mailio/include/pop3.hpp"
#include "libs/mailio/include/smtp.hpp"
#include "libs/mailio/include/message.hpp"

class EmailClient : public QObject
{
    Q_OBJECT

public:
    explicit EmailClient(QObject *parent = nullptr);
    ~EmailClient();

    void connectToServer(const EmailAccount &account);
    void disconnectFromServer();
    void fetchEmails();
    void sendEmail(const QString &to, const QString &subject,
                   const QString &body, const QStringList &attachments = QStringList());

    // 添加公共方法
    bool isConnected() const { return m_connected; }

signals:
    void connectionStatusChanged(bool connected);
    void newEmailReceived(const QString &from, const QString &subject,
                         const QString &body, bool isHtml,
                         const QStringList &attachments);
    void emailSent(bool success);
    void errorOccurred(const QString &error);

private slots:
    void onTimeout();

private:
    bool connectImapServer();
    bool connectPop3Server();
    bool fetchImapEmails();
    bool fetchPop3Emails();
    bool connectSmtpServer();
    bool sendSmtpEmail(const QString &to, const QString &subject,
                      const QString &body, const QStringList &attachments);

    EmailAccount m_currentAccount;
    bool m_connected;
    QTimer *m_timeoutTimer;
    QThread *m_workerThread;

    // 添加智能指针成员变量
    std::unique_ptr<mailio::imap> m_imap;
    std::unique_ptr<mailio::pop3> m_pop3;
    std::unique_ptr<mailio::smtp> m_smtp;

    QString m_lastError;  // 添加错误信息成员变量
};

#endif // EMAILCLIENT_H
