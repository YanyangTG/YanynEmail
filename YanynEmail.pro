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
    # Windows Socket 库
    LIBS += -lws2_32 -lwsock32 -lcrypt32

    # 明确指定库文件路径以避免链接错误
    LIBS += $$MY_PWD/libs/mailio/libs/libmailio.dll.a
    LIBS += $$MY_PWD/libs/openssl/libs/libssl.dll.a
    LIBS += $$MY_PWD/libs/openssl/libs/libcrypto.dll.a
}

# 禁用 Boost ASIO 弃用警告
QMAKE_CXXFLAGS += -DBOOST_ASIO_DISABLE_DEPRECATION_WARNINGS

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
