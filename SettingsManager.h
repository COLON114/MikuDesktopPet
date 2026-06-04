#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>
#include <QSettings>
#include <QPoint>

class SettingsManager : public QObject
{
    Q_OBJECT

public:
    explicit SettingsManager(QObject *parent = nullptr);

    // 保存和读取窗口位置
    void saveWindowPosition(const QPoint &pos);
    QPoint loadWindowPosition();

    // 开机自启
    void setAutoStart(bool enable);
    bool isAutoStartEnabled();

private:
    QSettings settings;
};

#endif