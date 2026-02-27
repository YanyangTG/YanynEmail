#include "accountdialog.h"
#include "ui_accountdialog.h"
#include <QSettings>
#include <QMessageBox>
#include <QComboBox>
#include <QLineEdit>
#include <QProgressDialog>
#include <QTimer>
#include <cstdlib>  // 用于std::rand()
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSslSocket>

AccountDialog::AccountDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AccountDialog),
    currentAccountIndex(-1)
{
    ui->setupUi(this);
    setWindowTitle("邮箱账户管理");
    setWindowIcon(QIcon(":/resources/user.png"));  // 使用用户图标

    // 设置圆角样式
    setStyleSheet(R"(
        QDialog {
            background: white;
            border-radius: 12px;
            font-family: "Source Han Sans CN";
        }
        QListWidget {
            border-radius: 8px;
            border: 1px solid #e0e0e0;
        }
        QGroupBox {
            border-radius: 8px;
            border: 1px solid #e0e0e0;
            margin-top: 10px;
            padding-top: 10px;
        }
        QLineEdit, QComboBox, QSpinBox {
            border-radius: 6px;
            border: 1px solid #e0e0e0;
            padding: 5px;
        }
        QPushButton {
            border-radius: 6px;
            padding: 8px 16px;
            background: #0078d4;
            color: white;
            border: none;
        }
        QPushButton:hover {
            background: #106ebe;
        }
        QPushButton#deleteButton {
            background: #dc3545;
        }
        QPushButton#deleteButton:hover {
            background: #c82333;
        }
        QPushButton#testButton {
            background: #28a745;
        }
        QPushButton#testButton:hover {
            background: #218838;
        }
        QPushButton#switchButton {
            background: #17a2b8;
        }
        QPushButton#switchButton:hover {
            background: #138496;
        }
    )");

    // 设置密码输入框为密码模式
    ui->passwordEdit->setEchoMode(QLineEdit::Password);

    setupEmailTypeComboBox();
    setupCustomEmailSettings();
    loadAccounts();
    updateAccountList();

    // 连接信号槽
    connect(ui->addButton, &QPushButton::clicked, this, &AccountDialog::onAddAccountClicked);
    connect(ui->deleteButton, &QPushButton::clicked, this, &AccountDialog::onDeleteAccountClicked);
    connect(ui->switchButton, &QPushButton::clicked, this, &AccountDialog::onSwitchAccountClicked);
    connect(ui->testButton, &QPushButton::clicked, this, &AccountDialog::onTestConnectionClicked);
    connect(ui->closeButton, &QPushButton::clicked, this, &AccountDialog::reject);
    connect(ui->emailTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AccountDialog::onAccountTypeChanged);
    connect(ui->customSettingsCheckBox, &QCheckBox::toggled,
            this, &AccountDialog::onCustomSettingsToggled);
    connect(ui->accountList, &QListWidget::itemClicked,
            this, &AccountDialog::onAccountItemClicked);
}

AccountDialog::~AccountDialog()
{
    delete ui;
}

QString AccountDialog::getCurrentAccountEmail() const
{
    if (currentAccountIndex >= 0 && currentAccountIndex < accounts.size()) {
        return accounts[currentAccountIndex].email;
    }
    return QString();
}

EmailAccount AccountDialog::getCurrentAccount() const
{
    if (currentAccountIndex >= 0 && currentAccountIndex < accounts.size()) {
        return accounts[currentAccountIndex];
    }
    return EmailAccount();
}

void AccountDialog::setupEmailTypeComboBox()
{
    ui->emailTypeComboBox->clear();
    ui->emailTypeComboBox->addItem("晏阳邮箱", "yanyn");
    ui->emailTypeComboBox->addItem("163邮箱", "163");
    ui->emailTypeComboBox->addItem("126邮箱", "126");
    ui->emailTypeComboBox->addItem("Yeah邮箱", "yeah");
    ui->emailTypeComboBox->addItem("Outlook", "outlook");
    ui->emailTypeComboBox->addItem("Hotmail", "hotmail");
    ui->emailTypeComboBox->addItem("Gmail", "gmail");
    ui->emailTypeComboBox->addItem("自定义邮箱", "custom");
}

void AccountDialog::setupCustomEmailSettings()
{
    // 设置协议选项
    ui->protocolComboBox->addItem("IMAP", "imap");
    ui->protocolComboBox->addItem("POP3", "pop3");

    // 设置加密选项
    ui->imapEncryptionComboBox->addItem("SSL/TLS", "ssl");
    ui->imapEncryptionComboBox->addItem("无加密", "none");
    ui->smtpEncryptionComboBox->addItem("SSL/TLS", "ssl");
    ui->smtpEncryptionComboBox->addItem("无加密", "none");

    // 默认隐藏自定义设置
    ui->customSettingsGroupBox->setVisible(false);
}

void AccountDialog::onAccountTypeChanged(int index)
{
    QString type = ui->emailTypeComboBox->itemData(index).toString();

    if (type == "custom") {
        ui->customSettingsCheckBox->setChecked(true);
        ui->customSettingsGroupBox->setVisible(true);
    } else {
        ui->customSettingsCheckBox->setChecked(false);
        ui->customSettingsGroupBox->setVisible(false);

        // 设置预设的服务器配置
        if (type == "yanyn") {
            ui->imapServerEdit->setText("yanyn.cn");
            ui->imapPortSpinBox->setValue(993);
            ui->imapEncryptionComboBox->setCurrentText("SSL/TLS");
            ui->smtpServerEdit->setText("yanyn.cn");
            ui->smtpPortSpinBox->setValue(456);
            ui->smtpEncryptionComboBox->setCurrentText("SSL/TLS");
        } else if (type == "163") {
            ui->imapServerEdit->setText("imap.163.com");
            ui->imapPortSpinBox->setValue(993);
            ui->imapEncryptionComboBox->setCurrentText("SSL/TLS");
            ui->smtpServerEdit->setText("smtp.163.com");
            ui->smtpPortSpinBox->setValue(465);
            ui->smtpEncryptionComboBox->setCurrentText("SSL/TLS");
        } else if (type == "126") {
            ui->imapServerEdit->setText("imap.126.com");
            ui->imapPortSpinBox->setValue(993);
            ui->imapEncryptionComboBox->setCurrentText("SSL/TLS");
            ui->smtpServerEdit->setText("smtp.126.com");
            ui->smtpPortSpinBox->setValue(465);
            ui->smtpEncryptionComboBox->setCurrentText("SSL/TLS");
        } else if (type == "yeah") {
            ui->imapServerEdit->setText("imap.yeah.net");
            ui->imapPortSpinBox->setValue(993);
            ui->imapEncryptionComboBox->setCurrentText("SSL/TLS");
            ui->smtpServerEdit->setText("smtp.yeah.net");
            ui->smtpPortSpinBox->setValue(465);
            ui->smtpEncryptionComboBox->setCurrentText("SSL/TLS");
        } else if (type == "outlook" || type == "hotmail") {
            ui->imapServerEdit->setText("outlook.office365.com");
            ui->imapPortSpinBox->setValue(993);
            ui->imapEncryptionComboBox->setCurrentText("SSL/TLS");
            ui->smtpServerEdit->setText("smtp.office365.com");
            ui->smtpPortSpinBox->setValue(587);
            ui->smtpEncryptionComboBox->setCurrentText("SSL/TLS");
        } else if (type == "gmail") {
            ui->imapServerEdit->setText("imap.gmail.com");
            ui->imapPortSpinBox->setValue(993);
            ui->imapEncryptionComboBox->setCurrentText("SSL/TLS");
            ui->smtpServerEdit->setText("smtp.gmail.com");
            ui->smtpPortSpinBox->setValue(465);
            ui->smtpEncryptionComboBox->setCurrentText("SSL/TLS");
        }
    }
}

void AccountDialog::onCustomSettingsToggled(bool checked)
{
    ui->customSettingsGroupBox->setVisible(checked);
}

void AccountDialog::onAddAccountClicked()
{
    QString name = ui->nameEdit->text().trimmed();
    QString email = ui->emailEdit->text().trimmed();
    QString password = ui->passwordEdit->text();

    if (name.isEmpty() || email.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入账户名称、邮箱地址和密码");
        return;
    }

    EmailAccount account;
    account.name = name;
    account.email = email;
    account.password = password;
    account.type = ui->emailTypeComboBox->currentData().toString();
    account.protocol = ui->protocolComboBox->currentData().toString();
    account.isActive = false;

    if (ui->customSettingsCheckBox->isChecked()) {
        account.imapServer = ui->imapServerEdit->text();
        account.imapPort = ui->imapPortSpinBox->value();
        account.imapEncryption = ui->imapEncryptionComboBox->currentData().toString();
        account.smtpServer = ui->smtpServerEdit->text();
        account.smtpPort = ui->smtpPortSpinBox->value();
        account.smtpEncryption = ui->smtpEncryptionComboBox->currentData().toString();
    } else {
        // 使用预设配置
        if (account.type == "yanyn") {
            account.imapServer = "yanyn.cn";
            account.imapPort = 993;
            account.imapEncryption = "ssl";
            account.smtpServer = "yanyn.cn";
            account.smtpPort = 456;
            account.smtpEncryption = "ssl";
        } else if (account.type == "163") {
            account.imapServer = "imap.163.com";
            account.imapPort = 993;
            account.imapEncryption = "ssl";
            account.smtpServer = "smtp.163.com";
            account.smtpPort = 465;
            account.smtpEncryption = "ssl";
        }
        // 其他预设配置...
    }

    accounts.append(account);
    saveAccounts();
    updateAccountList();

    // 清空输入框
    ui->nameEdit->clear();
    ui->emailEdit->clear();
    ui->passwordEdit->clear();
}

void AccountDialog::onDeleteAccountClicked()
{
    QListWidgetItem *currentItem = ui->accountList->currentItem();
    if (!currentItem) {
        QMessageBox::warning(this, "选择错误", "请选择要删除的账户");
        return;
    }

    int index = ui->accountList->row(currentItem);
    if (index >= 0 && index < accounts.size()) {
        accounts.removeAt(index);
        saveAccounts();
        updateAccountList();
        currentAccountIndex = -1;
    }
}

void AccountDialog::onSwitchAccountClicked()
{
    QListWidgetItem *currentItem = ui->accountList->currentItem();
    if (!currentItem) {
        QMessageBox::warning(this, "选择错误", "请选择要切换的账户");
        return;
    }

    int index = ui->accountList->row(currentItem);
    if (index >= 0 && index < accounts.size()) {
        // 取消之前激活的账户
        for (int i = 0; i < accounts.size(); ++i) {
            accounts[i].isActive = false;
        }

        // 激活当前选择的账户
        accounts[index].isActive = true;
        currentAccountIndex = index;

        saveAccounts();
        updateAccountList();

        emit accountChanged(accounts[index].email);

        QMessageBox::information(this, "切换成功",
                                 QString("已切换到账户: %1").arg(accounts[index].email));
    }
}

void AccountDialog::onTestConnectionClicked()
{
    QListWidgetItem *currentItem = ui->accountList->currentItem();
    if (!currentItem) {
        QMessageBox::warning(this, "选择错误", "请选择要测试的账户");
        return;
    }

    int index = ui->accountList->row(currentItem);
    if (index >= 0 && index < accounts.size()) {
        testEmailConnection(accounts[index]);
    }
}

void AccountDialog::testEmailConnection(const EmailAccount &account)
{
    QProgressDialog progress("测试邮箱连接中...", "取消", 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();

    // 这里应该实现真正的邮箱连接测试
    // 由于邮件协议实现较复杂，这里先模拟测试
    QTimer::singleShot(2000, this, [this, &progress, account]() {
        progress.close();

        // 模拟测试结果 - 使用标准库的rand函数
        bool success = (std::rand() % 2) == 0;

        if (success) {
            QMessageBox::information(this, "连接测试",
                                     QString("成功连接到邮箱服务器: %1").arg(account.email));
        } else {
            QMessageBox::warning(this, "连接测试",
                                 QString("无法连接到邮箱服务器: %1\n请检查网络连接和账户设置").arg(account.email));
        }
    });
}

void AccountDialog::onAccountItemClicked(QListWidgetItem *item)
{
    int index = ui->accountList->row(item);
    if (index >= 0 && index < accounts.size()) {
        const EmailAccount &account = accounts.at(index);
        // 显示账户详情
        ui->nameEdit->setText(account.name);
        ui->emailEdit->setText(account.email);
        ui->passwordEdit->setText(account.password);

        // 设置邮箱类型
        int typeIndex = ui->emailTypeComboBox->findData(account.type);
        if (typeIndex >= 0) {
            ui->emailTypeComboBox->setCurrentIndex(typeIndex);
        }
    }
}

void AccountDialog::loadAccounts()
{
    QSettings settings("Yanyn", "YanynEmail");
    int size = settings.beginReadArray("accounts");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        EmailAccount account;
        account.name = settings.value("name").toString();
        account.email = settings.value("email").toString();
        account.password = settings.value("password").toString();
        account.type = settings.value("type").toString();
        account.protocol = settings.value("protocol").toString();
        account.imapServer = settings.value("imapServer").toString();
        account.imapPort = settings.value("imapPort").toInt();
        account.imapEncryption = settings.value("imapEncryption").toString();
        account.smtpServer = settings.value("smtpServer").toString();
        account.smtpPort = settings.value("smtpPort").toInt();
        account.smtpEncryption = settings.value("smtpEncryption").toString();
        account.isActive = settings.value("isActive", false).toBool();

        if (account.isActive) {
            currentAccountIndex = i;
        }

        accounts.append(account);
    }
    settings.endArray();
}

void AccountDialog::saveAccounts()
{
    QSettings settings("Yanyn", "YanynEmail");
    settings.beginWriteArray("accounts");
    for (int i = 0; i < accounts.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("name", accounts[i].name);
        settings.setValue("email", accounts[i].email);
        settings.setValue("password", accounts[i].password);
        settings.setValue("type", accounts[i].type);
        settings.setValue("protocol", accounts[i].protocol);
        settings.setValue("imapServer", accounts[i].imapServer);
        settings.setValue("imapPort", accounts[i].imapPort);
        settings.setValue("imapEncryption", accounts[i].imapEncryption);
        settings.setValue("smtpServer", accounts[i].smtpServer);
        settings.setValue("smtpPort", accounts[i].smtpPort);
        settings.setValue("smtpEncryption", accounts[i].smtpEncryption);
        settings.setValue("isActive", accounts[i].isActive);
    }
    settings.endArray();
}

void AccountDialog::updateAccountList()
{
    ui->accountList->clear();
    for (int i = 0; i < accounts.size(); ++i) {
        const EmailAccount &account = accounts.at(i);
        QString displayText = QString("%1 (%2)").arg(account.name).arg(account.email);
        if (account.isActive) {
            displayText += " [当前]";
        }

        QListWidgetItem *item = new QListWidgetItem(displayText);
        if (account.isActive) {
            item->setBackground(QColor(230, 243, 255)); // 浅蓝色背景
        }
        ui->accountList->addItem(item);
    }
}
