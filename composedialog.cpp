#include "composedialog.h"
#include "ui_composedialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>

ComposeDialog::ComposeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ComposeDialog)
{
    ui->setupUi(this);

    setWindowTitle("撰写邮件");
    setWindowIcon(QIcon(":/resources/send.png"));  // 修正路径
    setMinimumSize(600, 500);

    // 设置圆角样式
    setStyleSheet(R"(
        QDialog {
            background: white;
            border-radius: 12px;
            font-family: "Source Han Sans CN";
        }
        QLineEdit, QTextEdit {
            border-radius: 6px;
            border: 1px solid #e0e0e0;
            padding: 8px;
            font-family: "Source Han Sans CN";
        }
        QPushButton {
            border-radius: 6px;
            padding: 8px 16px;
            font-family: "Source Han Sans CN";
            border: none;
        }
        QPushButton#sendButton {
            background: #0078d4;
            color: white;
        }
        QPushButton#sendButton:hover {
            background: #106ebe;
        }
        QPushButton#cancelButton {
            background: #6c757d;
            color: white;
        }
        QPushButton#cancelButton:hover {
            background: #5a6268;
        }
        QPushButton#attachmentButton {
            background: #28a745;
            color: white;
        }
        QPushButton#attachmentButton:hover {
            background: #218838;
        }
        QPushButton#removeButton {
            background: #dc3545;
            color: white;
            padding: 4px 8px;
            font-size: 12px;
        }
        QPushButton#removeButton:hover {
            background: #c82333;
        }
        QListWidget {
            border-radius: 6px;
            border: 1px solid #e0e0e0;
        }
        QListWidget::item {
            padding: 8px;
            border-bottom: 1px solid #f0f0f0;
        }
        QListWidget::item:last {
            border-bottom: none;
        }
        QGroupBox {
            border-radius: 8px;
            border: 1px solid #e0e0e0;
            margin-top: 10px;
            padding-top: 10px;
        }
    )");

    // 设置按钮对象名用于样式表
    ui->sendButton->setObjectName("sendButton");
    ui->cancelButton->setObjectName("cancelButton");
    ui->addAttachmentButton->setObjectName("attachmentButton");

    // 连接信号槽
    connect(ui->addAttachmentButton, &QPushButton::clicked, this, &ComposeDialog::onAddAttachmentClicked);
    connect(ui->sendButton, &QPushButton::clicked, this, &ComposeDialog::onSendClicked);
    connect(ui->cancelButton, &QPushButton::clicked, this, &ComposeDialog::onCancelClicked);
    connect(ui->htmlRadioButton, &QRadioButton::toggled, this, &ComposeDialog::onFormatToggled);

    // 默认选择纯文本格式
    ui->plainRadioButton->setChecked(true);
    onFormatToggled(false);
}

ComposeDialog::~ComposeDialog()
{
    delete ui;
}

void ComposeDialog::setReplyMode(const QString &replyTo, const QString &originalSubject)
{
    // 自动填写收件人
    ui->toEdit->setText(replyTo);

    // 自动设置标题（添加回复前缀）
    QString replySubject = originalSubject;
    if (!replySubject.startsWith("回复:")) {
        replySubject = "回复: " + replySubject;
    }
    ui->subjectEdit->setText(replySubject);

    // 设置光标到内容编辑框
    ui->contentEdit->setFocus();

    // 在内容开头添加引用
    QString replyContent = "\n\n----------------------------------------\n";
    replyContent += "原始邮件:\n";
    replyContent += "发件人: " + replyTo + "\n";
    replyContent += "主题: " + originalSubject + "\n";
    replyContent += "时间: " + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + "\n";
    replyContent += "----------------------------------------\n\n";

    ui->contentEdit->setPlainText(replyContent);
}

Email ComposeDialog::getEmail() const
{
    Email email;
    email.sender = ""; // 将从当前账户获取
    email.subject = ui->subjectEdit->text();

    if (ui->htmlRadioButton->isChecked()) {
        email.content = ui->contentEdit->toHtml();
        email.isHtml = true;
    } else {
        email.content = ui->contentEdit->toPlainText();
        email.isHtml = false;
    }

    email.attachments = attachments;
    return email;
}

void ComposeDialog::onAddAttachmentClicked()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(
        this,
        "选择附件",
        QDir::homePath(),
        "所有文件 (*.*)"
        );

    if (!fileNames.isEmpty()) {
        for (const QString &fileName : fileNames) {
            QFileInfo fileInfo(fileName);
            QString displayName = fileInfo.fileName();

            if (!attachments.contains(fileName)) {
                attachments.append(fileName);

                // 添加到附件列表显示
                QListWidgetItem *item = new QListWidgetItem(displayName);
                item->setData(Qt::UserRole, fileName);

                // 创建移除按钮
                QWidget *widget = new QWidget();
                QHBoxLayout *layout = new QHBoxLayout(widget);
                layout->setContentsMargins(5, 2, 5, 2);

                QLabel *label = new QLabel(displayName);
                label->setStyleSheet("color: #333;");
                QPushButton *removeButton = new QPushButton("移除");
                removeButton->setObjectName("removeButton");
                removeButton->setFixedSize(50, 25);

                layout->addWidget(label);
                layout->addStretch();
                layout->addWidget(removeButton);

                widget->setLayout(layout);

                // 连接移除按钮
                connect(removeButton, &QPushButton::clicked, [this, fileName, item]() {
                    attachments.removeAll(fileName);
                    delete ui->attachmentsList->takeItem(ui->attachmentsList->row(item));
                });

                item->setSizeHint(widget->sizeHint());
                ui->attachmentsList->addItem(item);
                ui->attachmentsList->setItemWidget(item, widget);
            }
        }

        // 更新附件计数
        ui->attachmentsLabel->setText(QString("附件 (%1)").arg(attachments.size()));
    }
}

void ComposeDialog::onSendClicked()
{
    // 验证输入
    if (ui->toEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入收件人邮箱地址");
        ui->toEdit->setFocus();
        return;
    }

    if (ui->subjectEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入邮件主题");
        ui->subjectEdit->setFocus();
        return;
    }

    if (ui->contentEdit->toPlainText().trimmed().isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入邮件内容");
        ui->contentEdit->setFocus();
        return;
    }

    // 验证邮箱格式（简单验证）
    QString toEmail = ui->toEdit->text().trimmed();
    if (!toEmail.contains('@') || !toEmail.contains('.')) {
        QMessageBox::warning(this, "输入错误", "请输入有效的邮箱地址");
        ui->toEdit->setFocus();
        return;
    }

    // 检查附件大小（示例：限制总大小10MB）
    qint64 totalSize = 0;
    for (const QString &filePath : attachments) {
        QFileInfo fileInfo(filePath);
        totalSize += fileInfo.size();
    }

    if (totalSize > 10 * 1024 * 1024) { // 10MB
        QMessageBox::warning(this, "附件过大", "附件总大小不能超过10MB");
        return;
    }

    // 发送邮件
    accept();
}

void ComposeDialog::onCancelClicked()
{
    reject();
}

void ComposeDialog::onFormatToggled(bool html)
{
    if (html) {
        // 切换到HTML模式
        ui->contentEdit->setAcceptRichText(true);
        ui->contentEdit->setHtml(ui->contentEdit->toHtml());

        // 添加简单的HTML工具栏（可选）
        // 这里可以添加粗体、斜体等按钮
    } else {
        // 切换到纯文本模式
        ui->contentEdit->setAcceptRichText(false);
        ui->contentEdit->setPlainText(ui->contentEdit->toPlainText());
    }
}

bool ComposeDialog::isHtml() const
{
    return ui->htmlRadioButton->isChecked();
}
