#ifndef ACCOUNTDIALOG_H
#define ACCOUNTDIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include <QDateTime>  // 添加这行

namespace Ui {
class AccountDialog;
}

struct Email {
    QString id;
    QString sender;
    QString subject;
    QString content;
    bool isHtml;
    QStringList attachments;
    QDateTime time;  // 现在QDateTime已包含
    bool isRead;
    bool isFavorite;
};

struct EmailAccount {
    QString name;
    QString email;
    QString password;
    QString type; // yanyn, 163, 126, yeah, outlook, hotmail, gmail, custom
    QString protocol; // imap, pop3
    QString imapServer;
    int imapPort;
    QString imapEncryption;
    QString smtpServer;
    int smtpPort;
    QString smtpEncryption;
    bool isActive;
};

class AccountDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AccountDialog(QWidget *parent = nullptr);
    ~AccountDialog();

    QString getCurrentAccountEmail() const;
    EmailAccount getCurrentAccount() const;

signals:
    void accountChanged(const QString &email);

private slots:
    void onAddAccountClicked();
    void onDeleteAccountClicked();
    void onSwitchAccountClicked();
    void onTestConnectionClicked();
    void onAccountTypeChanged(int index);
    void onCustomSettingsToggled(bool checked);
    void onAccountItemClicked(QListWidgetItem *item);

private:
    Ui::AccountDialog *ui;
    QList<EmailAccount> accounts;
    int currentAccountIndex;

    void loadAccounts();
    void saveAccounts();
    void updateAccountList();
    void setupEmailTypeComboBox();
    void setupCustomEmailSettings();
    void testEmailConnection(const EmailAccount &account);
};

#endif // ACCOUNTDIALOG_H
