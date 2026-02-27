#ifndef SETTINGDIALOG_H
#define SETTINGDIALOG_H

#include <QDialog>
#include <QSettings>

namespace Ui {
class SettingDialog;
}

class SettingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingDialog(QWidget *parent = nullptr);
    ~SettingDialog();

    QString getCurrentTheme() const;
    bool getAutoStart() const;
    bool getMinimizeToTray() const;

signals:
    void themeChanged();

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

private:
    Ui::SettingDialog *ui;
    QSettings *settings;

    void loadSettings();
    void saveSettings();
};

#endif // SETTINGDIALOG_H
