#include "settingdialog.h"
#include "ui_settingdialog.h"
#include <QSettings>
#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QFontDatabase>

SettingDialog::SettingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingDialog),
    settings(new QSettings("Yanyn", "YanynEmail"))
{
    ui->setupUi(this);
    setWindowTitle("设置");
    setWindowIcon(QIcon(":/resources/setting.png"));  // 使用设置图标

    // 设置对话框字体
    QString dialogStyle = R"(
        QDialog {
            font-family: "Source Han Sans CN";
        }
        QGroupBox {
            font-family: "Source Han Sans CN";
        }
        QRadioButton {
            font-family: "Source Han Sans CN";
        }
        QCheckBox {
            font-family: "Source Han Sans CN";
        }
        QLabel {
            font-family: "Source Han Sans CN";
        }
        QPushButton {
            font-family: "Source Han Sans CN";
        }
    )";
    setStyleSheet(dialogStyle);

    loadSettings();
}

SettingDialog::~SettingDialog()
{
    delete ui;
    delete settings;
}

void SettingDialog::loadSettings()
{
    QString theme = settings->value("theme", "blue").toString();
    bool autoStart = settings->value("autoStart", false).toBool();
    bool minimizeToTray = settings->value("minimizeToTray", false).toBool();

    if (theme == "dark") {
        ui->darkRadioButton->setChecked(true);
    } else if (theme == "light") {
        ui->lightRadioButton->setChecked(true);
    } else {
        ui->blueRadioButton->setChecked(true);
    }

    ui->autoStartCheckBox->setChecked(autoStart);
    ui->minimizeToTrayCheckBox->setChecked(minimizeToTray);
}

void SettingDialog::saveSettings()
{
    QString theme;
    if (ui->darkRadioButton->isChecked()) {
        theme = "dark";
    } else if (ui->lightRadioButton->isChecked()) {
        theme = "light";
    } else {
        theme = "blue";
    }

    settings->setValue("theme", theme);
    settings->setValue("autoStart", ui->autoStartCheckBox->isChecked());
    settings->setValue("minimizeToTray", ui->minimizeToTrayCheckBox->isChecked());

#ifdef Q_OS_WIN
    // 设置开机自启动
    if (ui->autoStartCheckBox->isChecked()) {
        QSettings reg("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
        reg.setValue("YanynEmail", QDir::toNativeSeparators(QApplication::applicationFilePath()));
    } else {
        QSettings reg("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
        reg.remove("YanynEmail");
    }
#endif
}

QString SettingDialog::getCurrentTheme() const
{
    if (ui->darkRadioButton->isChecked()) {
        return "dark";
    } else if (ui->lightRadioButton->isChecked()) {
        return "light";
    } else {
        return "blue";
    }
}

bool SettingDialog::getAutoStart() const
{
    return ui->autoStartCheckBox->isChecked();
}

bool SettingDialog::getMinimizeToTray() const
{
    return ui->minimizeToTrayCheckBox->isChecked();
}

void SettingDialog::on_buttonBox_accepted()
{
    saveSettings();
    emit themeChanged();
    accept();
}

void SettingDialog::on_buttonBox_rejected()
{
    reject();
}
