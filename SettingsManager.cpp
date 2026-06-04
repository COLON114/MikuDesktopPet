#include "SettingsManager.h"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
    , settings("MikuPet", "MikuWidget")
{
}

void SettingsManager::saveWindowPosition(const QPoint &pos)
{
    settings.setValue("posX", pos.x());
    settings.setValue("posY", pos.y());
}

QPoint SettingsManager::loadWindowPosition()
{
    int x = settings.value("posX", -1).toInt();
    int y = settings.value("posY", -1).toInt();
    
    if (x >= 0 && y >= 0) {
        return QPoint(x, y);
    }
    return QPoint();  // 返回空点，表示使用默认位置
}

void SettingsManager::setAutoStart(bool enable)
{
    QSettings regSettings(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        QSettings::NativeFormat
    );
    
    if (enable) {
        QString appPath = QCoreApplication::applicationFilePath();
        regSettings.setValue("MikuPet", appPath.replace('/', '\\'));
    } else {
        regSettings.remove("MikuPet");
    }
}

bool SettingsManager::isAutoStartEnabled()
{
    QSettings regSettings(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        QSettings::NativeFormat
    );
    return regSettings.contains("MikuPet");
}