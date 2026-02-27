#include "mainwindow.h"
#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QDebug>
#include <QScreen>
#include <QStandardPaths>
#include <QDir>
#include <QFile>

int main(int argc, char *argv[])
{
    qDebug() << "=== 应用程序启动 ===";
    qDebug() << "argc:" << argc;
    qDebug() << "工作目录:" << QDir::currentPath();

    QApplication app(argc, argv);

    qDebug() << "QApplication 创建成功";
    qDebug() << "屏幕数量:" << app.screens().size();
    qDebug() << "主屏幕可用几何:" << app.primaryScreen()->availableGeometry();

    // 设置应用程序属性
    app.setApplicationName("Yanyn Email");
    app.setApplicationVersion("26.1");
    app.setOrganizationName("Yanyn");

    qDebug() << ("应用程序属性设置完成");

    // 检查资源文件
    qDebug() << "检查资源文件:";
    qDebug() << "  icon.png 存在:" << QFile::exists(":/resources/icon.png");
    qDebug() << "  字体文件存在:" << QFile::exists(":/resources/SourceHanSansCN-Regular.ttf");

    // 先不设置图标，避免资源加载问题
    // app.setWindowIcon(QIcon(":/resources/icon.png"));

    // 暂时禁用自定义字体加载进行测试
    /*
    // 加载思源黑体字体
    int fontId = QFontDatabase::addApplicationFont(":/resources/SourceHanSansCN-Regular.ttf");
    if (fontId != -1) {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontId);
        if (!fontFamilies.isEmpty()) {
            QFont font(fontFamilies.at(0));
            font.setPointSize(9);
            app.setFont(font);
            qDebug() << "自定义字体加载成功:" << fontFamilies.at(0);
        }
    } else {
        qDebug() << "自定义字体加载失败，使用系统默认字体";
        // 如果加载失败，使用系统默认字体但设置为无衬线字体
        QFont font("Microsoft YaHei UI", 9);  // 备用字体
        app.setFont(font);
    }
    */

    // 使用简单字体测试
    QFont defaultFont("Microsoft YaHei", 9);
    app.setFont(defaultFont);
    qDebug() << "设置默认字体:" << defaultFont.family();

    qDebug() << "准备创建主窗口";
    qDebug() << "MainWindow 类大小:" << sizeof(MainWindow);

    MainWindow window;
    qDebug() << "主窗口创建完成";
    qDebug() << "窗口指针地址:" << &window;
    qDebug() << "窗口父对象:" << window.parent();
    qDebug() << "窗口窗口标志:" << window.windowFlags();
    qDebug() << "窗口属性:" << window.windowModality();

    // 检查窗口属性
    qDebug() << "窗口是否可见:" << window.isVisible();
    qDebug() << "窗口是否隐藏:" << window.isHidden();
    qDebug() << "窗口几何位置:" << window.geometry();
    qDebug() << "窗口框架几何:" << window.frameGeometry();
    qDebug() << "窗口最小尺寸:" << window.minimumSize();
    qDebug() << "窗口最大尺寸:" << window.maximumSize();

    qDebug() << "调用 show() 方法";
    window.show();

    qDebug() << "show() 调用后:";
    qDebug() << "  窗口是否可见:" << window.isVisible();
    qDebug() << "  窗口是否隐藏:" << window.isHidden();
    qDebug() << "  窗口几何位置:" << window.geometry();
    qDebug() << "  窗口激活状态:" << window.isActiveWindow();

    // 强制窗口到前台
    qDebug() << "尝试将窗口置于前台";
    window.raise();
    window.activateWindow();

    qDebug() << "进入事件循环";
    qDebug() << "应用程序即将开始运行";

    int result = app.exec();
    qDebug() << "应用程序退出，返回码:" << result;

    return result;
}
