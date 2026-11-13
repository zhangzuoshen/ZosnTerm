#include "ZSApplication.h"

#include "ZSTerminalView.h"
#include "ZSPuTTYBackendAdapter.h"
#include "ZSIConnection.h"
#include "ZSSessionRepository.h"
#include "ZSConfigStore.h"

#include <QtGlobal>   // Q_OS_WIN
#include <QSerialPortInfo>
#include <QColor>
#include <QFont>
#include <QFontDatabase>
#include <QDebug>
#include <QVariantMap>
#include <QVariantList>
#include <QLocale>
#include <QVector>
#include <QPair>

ZSApplication::ZSApplication(QObject *parent)
    : QObject(parent)
    , m_statusMessage(QStringLiteral("就绪"))
    , m_repo(new ZSSessionRepository(this))
{
}

QString ZSApplication::statusMessage() const
{
    return m_statusMessage;
}

bool ZSApplication::isConnected() const
{
    return m_connected;
}

QString ZSApplication::connectionSummary() const
{
    return m_connectionSummary;
}

void ZSApplication::setStatusMessage(const QString &msg)
{
    if (m_statusMessage != msg) {
        m_statusMessage = msg;
        emit statusMessageChanged(m_statusMessage);
    }
}

void ZSApplication::setConnected(bool c)
{
    if (m_connected != c) {
        m_connected = c;
        emit connectionStateChanged(m_connected);
    }
}

void ZSApplication::setConnectionSummary(const QString &s)
{
    if (m_connectionSummary != s) {
        m_connectionSummary = s;
        emit connectionSummaryChanged(m_connectionSummary);
    }
}

void ZSApplication::setConfigStore(ZSConfigStore *store)
{
    m_config = store;
}

void ZSApplication::setTerminalView(ZSTerminalView *view)
{
    m_view = view;
    applySettings();   // M5：视图就绪后套用已保存外观
}

QChar ZSApplication::parityChar(int parity)
{
    switch (parity) {
    case 1: return QChar('O');
    case 2: return QChar('E');
    case 3: return QChar('M');
    case 4: return QChar('S');
    default: return QChar('N');
    }
}

QStringList ZSApplication::availablePorts() const
{
    QStringList ports;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const auto &info : infos)
        ports.append(info.portName());
    if (ports.isEmpty()) {
#if defined(Q_OS_WIN)
        ports.append(QStringLiteral("COM1"));
#else
        ports.append(QStringLiteral("/dev/ttyS0"));
#endif
    }
    return ports;
}

void ZSApplication::openWith(const QString &port, int baudRate, int dataBits,
                            int stopBits, int parity, int flow,
                            const QString &codepage)
{
    if (!m_view) {
        setStatusMessage(QStringLiteral("未找到终端视图 (ZSTerminalView)"));
        return;
    }
    if (m_adapter)
        closeSerialSession();

    auto *adapter = new ZSPuTTYBackendAdapter(this);
    adapter->setView(m_view);
    m_view->setBackend(adapter);
    adapter->setConnectionParameters(port, baudRate, dataBits, stopBits, parity, flow);
    adapter->setCodepage(codepage.isEmpty() ? defaultEncoding() : codepage);

    connect(adapter, &ZSPuTTYBackendAdapter::statusMessage,
            this, &ZSApplication::setStatusMessage);
    connect(adapter, &ZSIConnection::stateChanged, this,
            [this](ZSIConnection::State s) {
                const char *txt = "未知";
                switch (s) {
                case ZSIConnection::State::Connecting:   txt = "连接中"; break;
                case ZSIConnection::State::Connected:     txt = "已连接"; break;
                case ZSIConnection::State::Disconnected:  txt = "已断开"; break;
                case ZSIConnection::State::Error:         txt = "错误";   break;
                }
                setStatusMessage(QStringLiteral("串口状态：%1").arg(QString::fromLatin1(txt)));
                setConnected(s == ZSIConnection::State::Connected);
            });

    // M6：连接摘要，供标签页 / 状态栏显示。
    const QString summary = QStringLiteral("%1 · %2 %3%4%5")
        .arg(port).arg(baudRate).arg(dataBits)
        .arg(parityChar(parity))
        .arg(stopBits == 3 ? QStringLiteral("1.5") : QString::number(stopBits));
    setConnectionSummary(summary);

    m_adapter = adapter;

    if (!adapter->open()) {
        setStatusMessage(
            QStringLiteral("打开串口失败（检查设备 %1 与权限）").arg(port));
        setConnectionSummary(QString());
    }
}

void ZSApplication::connectSerial(const QString &port, int baudRate, int dataBits,
                                 int stopBits, int parity, int flow,
                                 const QString &codepage)
{
    // M4：记住本次使用的配置，供下次启动自动恢复。
    ZSSerialProfile last;
    last.port = port;
    last.baudRate = baudRate;
    last.dataBits = dataBits;
    last.stopBits = stopBits;
    last.parity = parity;
    last.flow = flow;
    last.codepage = codepage;
    m_repo->setLastProfile(last);

    openWith(port, baudRate, dataBits, stopBits, parity, flow, codepage);
}

void ZSApplication::newSerialSession()
{
#if defined(Q_OS_WIN)
    const QString defaultPort = QStringLiteral("COM1");
#else
    const QString defaultPort = QStringLiteral("/dev/ttyS0");
#endif
    openWith(defaultPort, 9600, 8, 1, ZS_PAR_NONE, ZS_FLOW_NONE);
}

void ZSApplication::closeSerialSession()
{
    if (m_adapter) {
        m_adapter->close();
        m_adapter->deleteLater();
        m_adapter = nullptr;
    }
    setConnected(false);
    setConnectionSummary(QString());
}

/* ---------------- M4 会话管理 ---------------- */

QStringList ZSApplication::sessionNames() const
{
    return m_repo->profileNames();
}

QVariantList ZSApplication::sessionList() const
{
    QVariantList out;
    const QStringList names = m_repo->profileNames();
    for (const QString &nm : names) {
        ZSSerialProfile p = m_repo->loadProfile(nm);
        QVariantMap m;
        m.insert(QStringLiteral("name"), nm);
        m.insert(QStringLiteral("port"), p.port);
        m.insert(QStringLiteral("baudRate"), p.baudRate);
        m.insert(QStringLiteral("dataBits"), p.dataBits);
        m.insert(QStringLiteral("stopBits"), p.stopBits);
        m.insert(QStringLiteral("parity"), p.parity);
        m.insert(QStringLiteral("flow"), p.flow);
        out.append(m);
    }
    return out;
}

void ZSApplication::saveSession(const QString &name, const QString &port,
                               int baudRate, int dataBits, int stopBits,
                               int parity, int flow, const QString &codepage)
{
    QString nm = name.trimmed();
    if (nm.isEmpty()) {
        setStatusMessage(QStringLiteral("会话名不能为空"));
        return;
    }
    ZSSerialProfile p;
    p.port = port;
    p.baudRate = baudRate;
    p.dataBits = dataBits;
    p.stopBits = stopBits;
    p.parity = parity;
    p.flow = flow;
    p.codepage = codepage;
    m_repo->saveProfile(nm, p);
    setStatusMessage(QStringLiteral("已保存会话：%1").arg(nm));
}

void ZSApplication::loadSession(const QString &name)
{
    ZSSerialProfile p = m_repo->loadProfile(name);
    if (p.port.isEmpty()) {
        setStatusMessage(QStringLiteral("未找到会话：%1").arg(name));
        return;
    }
    emit connectionParamsApplied(p.port, p.baudRate, p.dataBits,
                                 p.stopBits, p.parity, p.flow, p.codepage);
}

void ZSApplication::deleteSession(const QString &name)
{
    if (m_repo->deleteProfile(name))
        setStatusMessage(QStringLiteral("已删除会话：%1").arg(name));
    else
        setStatusMessage(QStringLiteral("未找到会话：%1").arg(name));
}

void ZSApplication::openSession(const QString &name)
{
    ZSSerialProfile p = m_repo->loadProfile(name);
    if (p.port.isEmpty()) {
        setStatusMessage(QStringLiteral("未找到会话：%1").arg(name));
        return;
    }
    // 更新上次配置并立即连接（MobaXTerm 双击会话的行为）。
    m_repo->setLastProfile(p);
    openWith(p.port, p.baudRate, p.dataBits, p.stopBits, p.parity, p.flow,
            p.codepage.isEmpty() ? defaultEncoding() : p.codepage);
}

QVariantMap ZSApplication::lastParams() const
{
    ZSSerialProfile p = m_repo->lastProfile();
    QVariantMap m;
    m.insert(QStringLiteral("port"), p.port);
    m.insert(QStringLiteral("baudRate"), p.baudRate);
    m.insert(QStringLiteral("dataBits"), p.dataBits);
    m.insert(QStringLiteral("stopBits"), p.stopBits);
    m.insert(QStringLiteral("parity"), p.parity);
    m.insert(QStringLiteral("flow"), p.flow);
    m.insert(QStringLiteral("codepage"), p.codepage);
    return m;
}

/* ---------------- M5 外观/字体/主题设置 ---------------- */

void ZSApplication::applySettings()
{
    if (!m_view || !m_config)
        return;
    auto *r = m_view->renderer();

    const QString fam = m_config->value(QStringLiteral("appearance/fontFamily"),
                                        QStringLiteral("DejaVu Sans Mono")).toString();
    const int size = m_config->value(QStringLiteral("appearance/fontSize"), 14).toInt();
    const QColor fg(m_config->value(QStringLiteral("appearance/foreground"),
                                    QStringLiteral("#c0c0c0")).toString());
    const QColor bg(m_config->value(QStringLiteral("appearance/background"),
                                    QStringLiteral("#000000")).toString());
    const QColor cur(m_config->value(QStringLiteral("appearance/cursorColor"),
                                     QStringLiteral("#c0c0c0")).toString());
    const int shape = m_config->value(QStringLiteral("appearance/cursorShape"), 0).toInt();

    QFont f(fam);
    f.setPixelSize(size);
    r->setFont(f);
    r->setDefaultForeground(fg);
    r->setDefaultBackground(bg);
    r->setCursorColor(cur);
    r->setCursorShape(shape);
}

QStringList ZSApplication::fontFamilies() const
{
    QFontDatabase db;
    const QStringList all = db.families();
    QStringList mono;
    for (const QString &fam : all)
        if (db.isFixedPitch(fam))
            mono.append(fam);
    if (mono.isEmpty())
        mono = all;
    mono.sort();
    return mono;
}

QVariantList ZSApplication::availableEncodings() const
{
    static const QVector<QPair<QString, QString>> list = {
        {QStringLiteral("UTF-8"),          QStringLiteral("UTF-8")},
        {QStringLiteral("GBK (中文)"),      QStringLiteral("GBK")},
        {QStringLiteral("GB2312"),         QStringLiteral("GB2312")},
        {QStringLiteral("Latin-1"),       QStringLiteral("ISO-8859-1")},
        {QStringLiteral("CP1252 (西欧)"),  QStringLiteral("CP1252")}
    };
    QVariantList out;
    for (const auto &e : list) {
        QVariantMap m;
        m.insert(QStringLiteral("label"), e.first);
        m.insert(QStringLiteral("cp"), e.second);
        out.append(m);
    }
    return out;
}

QString ZSApplication::defaultEncoding() const
{
    return (QLocale::system().language() == QLocale::Chinese)
               ? QStringLiteral("GBK") : QStringLiteral("UTF-8");
}

/* ---------------- 右键菜单：编辑操作 ---------------- */

void ZSApplication::selectAll()
{
    if (m_adapter) m_adapter->selectAll();
}

void ZSApplication::copySelection()
{
    if (m_adapter) m_adapter->copySelection();
}

void ZSApplication::paste()
{
    if (m_adapter) m_adapter->paste();
}

void ZSApplication::clearScreen()
{
    if (m_adapter) m_adapter->clearScreen();
}

/* ---------------- 查找 ---------------- */

int ZSApplication::findText(const QString &query)
{
    m_findQuery = query;
    m_findText = m_adapter ? m_adapter->extractFullText() : QString();
    m_findOffsets.clear();
    m_findIndex = 0;
    m_findCount = 0;
    if (!query.isEmpty() && !m_findText.isEmpty()) {
        const QString hay = m_findText.toLower();
        const QString needle = query.toLower();
        int from = 0;
        int idx;
        while ((idx = hay.indexOf(needle, from)) >= 0) {
            m_findOffsets.append(idx);
            from = idx + needle.length();
        }
        m_findCount = m_findOffsets.size();
    }
    if (m_findCount > 0)
        applyFindMatch(0);
    else if (m_view)
        m_view->clearFindHighlight();
    return m_findCount;
}

int ZSApplication::findNext()
{
    if (m_findCount <= 0)
        return 0;
    m_findIndex = (m_findIndex + 1) % m_findCount;
    applyFindMatch(m_findIndex);
    return m_findCount;
}

int ZSApplication::findPrev()
{
    if (m_findCount <= 0)
        return 0;
    m_findIndex = (m_findIndex - 1 + m_findCount) % m_findCount;
    applyFindMatch(m_findIndex);
    return m_findCount;
}

void ZSApplication::clearFind()
{
    m_findText.clear();
    m_findQuery.clear();
    m_findOffsets.clear();
    m_findCount = 0;
    m_findIndex = 0;
    if (m_view)
        m_view->clearFindHighlight();
}

void ZSApplication::applyFindMatch(int index)
{
    if (!m_view || index < 0 || index >= m_findCount)
        return;
    const int off = m_findOffsets[index];
    const int lineStart = m_findText.lastIndexOf(QLatin1Char('\n'), off - 1) + 1;
    const int col = off - lineStart;
    const int absLine = m_findText.left(off).count(QLatin1Char('\n'));
    const int len = m_findQuery.length();
    m_view->setFindHighlight(absLine, col, len);
    m_view->scrollToAbsLine(absLine);
}
