/**
 * @file main.cpp
 * @brief NIUMA V3.0 程序入口
 *
 * 职责：创建 QApplication、初始化数据库、显示主窗口。
 *
 * 依赖：ui/MainWindow、application/maintenance/StartupUseCase、utils/Logger
 *
 * 文档：coding.md 第二节、task.md T1.1、qt_implementation_guide.md 3.1
 *
 * 启动和退出维护由 StartupUseCase 协调；本入口只配置 QApplication、
 * 提供工作目录选择回调并创建主窗口。
 */

#include <QApplication>
#include <QFileDialog>
#include <QIcon>
#include <QMessageBox>
#include <QTimer>

#include "ui/MainWindow.h"
#include "src/AppVersion.h"
#include "src/SingleInstance.h"
#include "application/maintenance/StartupUseCase.h"
#include "ui/ScanImportDialog.h"
#include "utils/Logger.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("NIUMA");
    app.setApplicationVersion(NIUMA_APP_VERSION);
#ifdef Q_OS_WIN
    // Acquire before logging, migrations, database access or startup dialogs.
    SingleInstance::Guard instance;
    if (!instance.valid()) return 1;
    if (instance.isSecondary()) {
        if (instance.activateExisting()) return 0;
        QMessageBox::information(nullptr, QStringLiteral("NIUMA"),
            QStringLiteral("程序已在运行，但暂时无法切换窗口，请稍后重试。"));
        return 1;
    }
#endif
    // 设置应用程序图标（蓝底白色大写 N，与标题栏图标一致）
    app.setWindowIcon(QIcon(":/icons/app-icon.svg"));

    // 初始化日志（写入 {程序目录}/Data/niuma.log）
    Logger::instance().info(QStringLiteral("NIUMA V%1 启动").arg(NIUMA_APP_VERSION));

    StartupUseCase startup;
    const StartupResult startupResult = startup.prepare([]() {
        return QFileDialog::getExistingDirectory(
            nullptr, QStringLiteral("请选择工作目录的存放位置"),
            QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    });

    // 创建并显示主窗口
    MainWindow mainWindow;
#ifdef Q_OS_WIN
    SetPropW(reinterpret_cast<HWND>(mainWindow.winId()), SingleInstance::windowProperty(),
             reinterpret_cast<HANDLE>(static_cast<quintptr>(GetCurrentProcessId())));
#endif
    mainWindow.show();
    if (startupResult.pendingConflictCount > 0) {
        QTimer::singleShot(0, &mainWindow, [&mainWindow]() {
            ScanImportDialog dialog(&mainWindow);
            QTimer::singleShot(0, &dialog, [&dialog]() {
                QMetaObject::invokeMethod(&dialog, "onScanClicked", Qt::DirectConnection);
            });
            dialog.exec();
            mainWindow.refreshStatusBar();
        });
    }

    // 退出补生成：aboutToQuit 信号触发自动导出 NIUMA_auto.niuma
    QObject::connect(&app, &QApplication::aboutToQuit, [&startup]() {
        Logger::instance().info("程序退出，执行自动备份");
        startup.exportOnExit();
    });

    int ret = app.exec();

    // 程序退出时关闭数据库
    startup.closeDatabase();
    Logger::instance().info(QStringLiteral("NIUMA V%1 退出").arg(NIUMA_APP_VERSION));

    return ret;
}
