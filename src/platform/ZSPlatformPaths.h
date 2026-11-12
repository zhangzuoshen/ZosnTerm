#pragma once

#include <QString>

// 跨平台路径抽象（配置/日志/会话存储/缓存）。
class ZSPlatformPaths
{
public:
    static QString appConfigDir();                       // ~/.config/ZosnTerm (Linux) / AppData (Win)
    static QString logDir();                             // 上述路径 + /logs
    static QString sessionStorePath();                   // 上述路径 + /sessions.json
    static QString configPath();                        // 上述路径 + /config.json (M5 全局设置)
    static QString cacheDir();
};
