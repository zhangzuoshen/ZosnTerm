#include "ZSPlatformPaths.h"

#include <QStandardPaths>
#include <QDir>

QString ZSPlatformPaths::appConfigDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
}

QString ZSPlatformPaths::logDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
           + QStringLiteral("/logs");
}

QString ZSPlatformPaths::sessionStorePath()
{
    return appConfigDir() + QStringLiteral("/sessions.json");
}

QString ZSPlatformPaths::configPath()
{
    return appConfigDir() + QStringLiteral("/config.json");
}

QString ZSPlatformPaths::cacheDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
}
