#ifndef COMPOSEDIALOG_H
#define COMPOSEDIALOG_H

#include <QDialog>
#include "accountdialog.h"

namespace Ui {
class ComposeDialog;
}

class ComposeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ComposeDialog(QWidget *parent = nullptr);
    ~ComposeDialog();

    // 设置回复模式，自动填写收件人和标题
    void setReplyMode(const QString &replyTo, const QString &originalSubject);

    // 获取邮件内容
    Email getEmail() const;
    bool isHtml() const;

private slots:
    void onAddAttachmentClicked();
    // 移除下面这行
    // void onRemoveAttachmentClicked();
    void onSendClicked();
    void onCancelClicked();
    void onFormatToggled(bool html);

private:
    Ui::ComposeDialog *ui;
    QStringList attachments;
};

#endif // COMPOSEDIALOG_H
