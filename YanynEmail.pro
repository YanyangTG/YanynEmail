QT += core gui widgets network

CONFIG += c++17

TARGET = YanynEmail
TEMPLATE = app

# 定义项目根目录
MY_PWD = $$PWD

# mailio 配置
INCLUDEPATH += $$MY_PWD/libs/mailio/include
LIBS += -L$$MY_PWD/libs/mailio/libs -lmailio
DEPENDPATH += $$MY_PWD/libs/mailio/include

# Boost 配置
INCLUDEPATH += $$MY_PWD/libs/boost/include

# OpenSSL 配置
INCLUDEPATH += $$MY_PWD/libs/openssl/include
LIBS += -L$$MY_PWD/libs/openssl/libs -lssl -lcrypto

# Windows 平台特定的库链接
win32 {
    # Windows Socket 库（这些系统库应该能找到）
    LIBS += -lws2_32 -lwsock32 -lcrypt32

    # 移除找不到的库链接，让 mailio 使用内置实现
    # LIBS -= -lssl -lcrypto -lboost_system -lboost_date_time
}

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    settingdialog.cpp \
    trayicon.cpp \
    accountdialog.cpp \
    emailclient.cpp \
    composedialog.cpp

HEADERS += \
    mainwindow.h \
    settingdialog.h \
    trayicon.h \
    accountdialog.h \
    emailclient.h \
    composedialog.h

FORMS += \
    mainwindow.ui \
    settingdialog.ui \
    accountdialog.ui \
    composedialog.ui

RESOURCES += resources.qrc
