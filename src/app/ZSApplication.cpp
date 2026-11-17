#include "ZSApplication.h"

#include "ZSTerminalView.h"
#include "ZSPuTTYBackendAdapter.h"
#include "ZSIConnection.h"
#include "ZSSessionRepository.h"
#include "ZSConfigStore.h"
#include "ZSPlatformPaths.h"

#include <QtGlobal>   // Q_OS_WIN
#include <QSerialPortInfo>
#include <QSerialPort>
#include <QDir>
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

/* 把一组串口参数应用到某会话的引擎上（引擎须已由 registerSessionView 建立）。 */
void ZSApplication::openWith(const QString &port, int baudRate, int dataBits,
                            int stopBits, int parity, int flow,
                            const QString &codepage)
{
    ZSPuTTYBackendAdapter *adapter = activeAdapter();
    if (!adapter) {
        setStatusMessage(QStringLiteral("会话终端未就绪"));
        return;
    }
    if (adapter->isOpen())
        return;   // 已连接则保持，不重复建链

    adapter->setConnectionParameters(port, baudRate, dataBits, stopBits, parity, flow);
    adapter->setCodepage(codepage.isEmpty() ? defaultEncoding() : codepage);

    if (!adapter->open())
        setStatusMessage(
            QStringLiteral("打开串口失败（检查设备 %1 与权限）").arg(port));
    refreshActiveStatus();
}

/* ---------------- M13 多会话独立终端 ---------------- */

ZSPuTTYBackendAdapter *ZSApplication::activeAdapter() const
{
    return m_adapter.data();
}

ZSTerminalView *ZSApplication::activeView() const
{
    return m_view.data();
}

QString ZSApplication::buildSummary(const QString &name) const
{
    const ZSSerialProfile p = m_repo->loadProfile(name);
    if (p.port.isEmpty())
        return QString();
    return QStringLiteral("%1 · %2 %3%4%5")
        .arg(p.port).arg(p.baudRate).arg(p.dataBits)
        .arg(parityChar(p.parity))
        .arg(p.stopBits == 3 ? QStringLiteral("1.5") : QString::number(p.stopBits));
}

void ZSApplication::refreshActiveStatus()
{
    setConnected(activeAdapter() && activeAdapter()->isOpen());
    setConnectionSummary(buildSummary(m_activeSession));
}

void ZSApplication::registerSessionView(const QString &name, QObject *viewObj)
{
    auto *view = qobject_cast<ZSTerminalView *>(viewObj);
    if (!view || name.isEmpty())
        return;

    ZSSessionTerm &ctx = m_terms[name];
    if (ctx.view == view && ctx.adapter)
        return;   // 已注册

    ctx.view = view;
    auto *adapter = new ZSPuTTYBackendAdapter(this);
    adapter->setView(view);
    view->setBackend(adapter);

    // M4：用会话保存的参数预置引擎（未连接时仅配置，不占串口）。
    const ZSSerialProfile p = m_repo->loadProfile(name);
    adapter->setConnectionParameters(p.port, p.baudRate, p.dataBits,
                                     p.stopBits, p.parity, p.flow);
    adapter->setCodepage(p.codepage.isEmpty() ? defaultEncoding() : p.codepage);

    connect(adapter, &ZSPuTTYBackendAdapter::statusMessage,
            this, &ZSApplication::setStatusMessage);
    connect(adapter, &ZSIConnection::stateChanged, this,
            [this, name](ZSIConnection::State s) {
                // 只让当前激活会话驱动全局状态显示；后台会话连/断不影响状态栏。
                if (name != m_activeSession)
                    return;
                QString txt = "未知";
                switch (s) {
                case ZSIConnection::State::Connecting:   txt = "连接中"; break;
                case ZSIConnection::State::Connected:     txt = "已连接"; break;
                case ZSIConnection::State::Disconnected:  txt = "已断开"; break;
                case ZSIConnection::State::Error:         txt = "错误";   break;
                }
                setStatusMessage(QStringLiteral("串口状态：%1").arg(txt));
                setConnected(s == ZSIConnection::State::Connected);
                if (s == ZSIConnection::State::Disconnected)
                    setConnectionSummary(QString());
            });

    ctx.adapter = adapter;
    applySettingsToView(view);

    // 首个注册的会话自动成为激活会话。
    if (m_activeSession.isEmpty())
        setActiveSession(name);
}

void ZSApplication::setActiveSession(const QString &name)
{
    if (name == m_activeSession && m_adapter)
        return;
    auto it = m_terms.find(name);
    if (it == m_terms.end())
        return;

    m_activeSession = name;
    m_view = it.value().view;
    m_adapter = it.value().adapter;
    refreshActiveStatus();
    clearFind();   // 切换会话清掉旧会话的查找高亮
}

QString ZSApplication::activeSession() const
{
    return m_activeSession;
}

void ZSApplication::connectSession(const QString &name)
{
    auto it = m_terms.find(name);
    if (it == m_terms.end()) {
        setStatusMessage(QStringLiteral("会话终端未就绪：%1").arg(name));
        return;
    }
    ZSPuTTYBackendAdapter *adapter = it.value().adapter.data();
    if (!adapter) {
        setStatusMessage(QStringLiteral("会话引擎未就绪：%1").arg(name));
        return;
    }
    // 切为激活会话，使状态显示与输入都指向它。
    setActiveSession(name);

    const ZSSerialProfile p = m_repo->loadProfile(name);
    if (p.port.isEmpty()) {
        setStatusMessage(QStringLiteral("未找到会话：%1").arg(name));
        return;
    }
    if (adapter->isOpen())
        return;   // 保持已有连接，重复右键连接不重连

    adapter->setConnectionParameters(p.port, p.baudRate, p.dataBits,
                                     p.stopBits, p.parity, p.flow);
    adapter->setCodepage(p.codepage.isEmpty() ? defaultEncoding() : p.codepage);
    m_repo->setLastProfile(p);
    if (!adapter->open())
        setStatusMessage(
            QStringLiteral("打开串口失败（检查设备 %1 与权限）").arg(p.port));
    refreshActiveStatus();
}

void ZSApplication::closeSession(const QString &name)
{
    auto it = m_terms.find(name);
    if (it == m_terms.end())
        return;
    ZSPuTTYBackendAdapter *adapter = it.value().adapter.data();
    if (adapter && adapter->isOpen())
        adapter->close();
    if (name == m_activeSession) {
        setConnected(false);
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
    closeSession(m_activeSession);
}

/* ---------------- M4 会话管理 ---------------- */

QStringList ZSApplication::sessionNames() const
{
    return m_repo->profileNames();
}

QString ZSApplication::sessionStorePath() const
{
    return ZSPlatformPaths::sessionStorePath();
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
        m.insert(QStringLiteral("codepage"), p.codepage);
        m.insert(QStringLiteral("showTimestamp"), p.showTimestamp);
        m.insert(QStringLiteral("hexDisplay"), p.hexDisplay);
        m.insert(QStringLiteral("localEcho"), p.localEcho);
        m.insert(QStringLiteral("autoScroll"), p.autoScroll);
        m.insert(QStringLiteral("autoReconnect"), p.autoReconnect);
        m.insert(QStringLiteral("crlf"), p.crlf);
        m.insert(QStringLiteral("logEnabled"), p.logEnabled);
        m.insert(QStringLiteral("logPath"), p.logPath);
        out.append(m);
    }
    return out;
}

void ZSApplication::saveSession(const QString &name, const QString &port,
                               int baudRate, int dataBits, int stopBits,
                               int parity, int flow, const QString &codepage,
                               const QVariantMap &extras)
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
    p.showTimestamp = extras.value(QStringLiteral("showTimestamp"), p.showTimestamp).toBool();
    p.hexDisplay    = extras.value(QStringLiteral("hexDisplay"), p.hexDisplay).toBool();
    p.localEcho     = extras.value(QStringLiteral("localEcho"), p.localEcho).toBool();
    p.autoScroll    = extras.value(QStringLiteral("autoScroll"), p.autoScroll).toBool();
    p.autoReconnect = extras.value(QStringLiteral("autoReconnect"), p.autoReconnect).toBool();
    p.crlf          = extras.value(QStringLiteral("crlf"), p.crlf).toBool();
    p.logEnabled    = extras.value(QStringLiteral("logEnabled"), p.logEnabled).toBool();
    p.logPath       = extras.value(QStringLiteral("logPath"), p.logPath).toString();
    m_repo->saveProfile(nm, p);
    setStatusMessage(QStringLiteral("已保存会话：%1").arg(nm));
}

QString ZSApplication::testSerial(const QString &port, int baudRate, int dataBits,
                                  int stopBits, int parity, int flow) const
{
    if (port.trimmed().isEmpty())
        return QStringLiteral("请先选择串口");
    QSerialPort sp(port.trimmed());
    sp.setBaudRate(baudRate);
    sp.setDataBits(static_cast<QSerialPort::DataBits>(dataBits));
    sp.setStopBits(stopBits == 2 ? QSerialPort::TwoStop
                   : stopBits == 3 ? QSerialPort::OneAndHalfStop
                                   : QSerialPort::OneStop);
    sp.setParity(parity == 1 ? QSerialPort::OddParity
                : parity == 2 ? QSerialPort::EvenParity
                : parity == 3 ? QSerialPort::MarkParity
                : parity == 4 ? QSerialPort::SpaceParity
                              : QSerialPort::NoParity);
    sp.setFlowControl(flow == 1 ? QSerialPort::SoftwareControl
                    : flow == 2 ? QSerialPort::HardwareControl
                                : QSerialPort::NoFlowControl);
    if (!sp.open(QIODevice::ReadWrite))
        return QStringLiteral("打开失败：%1").arg(sp.errorString());
    sp.close();
    return QString();
}

QString ZSApplication::defaultLogPath() const
{
    return QDir::homePath() + QStringLiteral("/ZosnTerm/logs");
}

QVariantMap ZSApplication::sessionConfig(const QString &name) const
{
    ZSSerialProfile p = m_repo->loadProfile(name);
    QVariantMap m;
    m.insert(QStringLiteral("name"), name);
    m.insert(QStringLiteral("port"), p.port);
    m.insert(QStringLiteral("baudRate"), p.baudRate);
    m.insert(QStringLiteral("dataBits"), p.dataBits);
    m.insert(QStringLiteral("stopBits"), p.stopBits);
    m.insert(QStringLiteral("parity"), p.parity);
    m.insert(QStringLiteral("flow"), p.flow);
    m.insert(QStringLiteral("codepage"), p.codepage);
    m.insert(QStringLiteral("showTimestamp"), p.showTimestamp);
    m.insert(QStringLiteral("hexDisplay"), p.hexDisplay);
    m.insert(QStringLiteral("localEcho"), p.localEcho);
    m.insert(QStringLiteral("autoScroll"), p.autoScroll);
    m.insert(QStringLiteral("autoReconnect"), p.autoReconnect);
    m.insert(QStringLiteral("crlf"), p.crlf);
    m.insert(QStringLiteral("logEnabled"), p.logEnabled);
    m.insert(QStringLiteral("logPath"), p.logPath);
    return m;
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
    if (m_repo->deleteProfile(name)) {
        // 先断开并移除该会话引擎，再清上下文。
        auto it = m_terms.find(name);
        if (it != m_terms.end()) {
            ZSPuTTYBackendAdapter *adapter = it.value().adapter.data();
            if (adapter && adapter->isOpen())
                adapter->close();
            if (adapter)
                adapter->deleteLater();
            m_terms.remove(name);
        }
        if (m_activeSession == name) {
            m_activeSession.clear();
            m_view = nullptr;
            m_adapter = nullptr;
            refreshActiveStatus();
        }
        setStatusMessage(QStringLiteral("已删除会话：%1").arg(name));
    } else {
        setStatusMessage(QStringLiteral("未找到会话：%1").arg(name));
    }
}

void ZSApplication::openSession(const QString &name)
{
    connectSession(name);
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
    m.insert(QStringLiteral("showTimestamp"), p.showTimestamp);
    m.insert(QStringLiteral("hexDisplay"), p.hexDisplay);
    m.insert(QStringLiteral("localEcho"), p.localEcho);
    m.insert(QStringLiteral("autoScroll"), p.autoScroll);
    m.insert(QStringLiteral("autoReconnect"), p.autoReconnect);
    m.insert(QStringLiteral("crlf"), p.crlf);
    m.insert(QStringLiteral("logEnabled"), p.logEnabled);
    m.insert(QStringLiteral("logPath"), p.logPath);
    return m;
}

/* ---------------- M5 外观/字体/主题设置 ---------------- */

void ZSApplication::applySettingsToView(ZSTerminalView *view) const
{
    if (!view || !m_config)
        return;
    auto *r = view->renderer();

    const QString fam = m_config->value(QStringLiteral("appearance/fontFamily"),
                                        QStringLiteral("DejaVu Sans Mono")).toString();
    const int size = m_config->value(QStringLiteral("appearance/fontSize"), 14).toInt();
    const QColor fg(m_config->value(QStringLiteral("appearance/foreground"),
                                    QStringLiteral("#1f2329")).toString());
    const QColor bg(m_config->value(QStringLiteral("appearance/background"),
                                    QStringLiteral("#ffffff")).toString());
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

void ZSApplication::applySettings()
{
    for (auto it = m_terms.constBegin(); it != m_terms.constEnd(); ++it)
        applySettingsToView(it.value().view.data());
    // 兼容：若通过 setTerminalView 注入过独立视图也应用。
    if (m_view)
        applySettingsToView(m_view.data());
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

void ZSApplication::cancelSelection()
{
    if (m_adapter) m_adapter->cancelSelection();
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
    QVector<ZSFindMatch> matches;
    matches.reserve(m_findCount);
    int currentAbsLine = -1;
    for (int i = 0; i < m_findCount; ++i) {
        const int off = m_findOffsets.at(i);
        const int lineStart = m_findText.lastIndexOf(QLatin1Char('\n'), off - 1) + 1;
        ZSFindMatch fm;
        fm.absLine = m_findText.left(off).count(QLatin1Char('\n'));
        fm.col = off - lineStart;
        fm.len = m_findQuery.length();
        matches.append(fm);
        if (i == index) currentAbsLine = fm.absLine;
    }
    m_view->setFindMatches(matches, index);
    m_view->scrollToAbsLine(currentAbsLine);
}


/* ---- M15 底部输入栏发送 ---- */
void ZSApplication::sendToActiveSession(const QString &text, bool hex)
{
    if (!m_adapter || !m_adapter->isOpen() || text.isEmpty())
        return;
    QByteArray payload;
    if (hex) {
        QString clean;
        for (int i = 0; i < text.size(); ++i) {
            if (!text.at(i).isSpace())
                clean.append(text.at(i));
        }
        if (clean.size() % 2 != 0) {
            setStatusMessage(QStringLiteral("HEX 数据长度必须为偶数"));
            return;
        }
        bool ok = true;
        for (int i = 0; i < clean.size(); i += 2) {
            int v = clean.mid(i, 2).toInt(&ok, 16);
            if (!ok || v < 0 || v > 255) { ok = false; break; }
            payload.append(static_cast<char>(v));
        }
        if (!ok) {
            setStatusMessage(QStringLiteral("HEX 格式无效"));
            return;
        }
    } else {
        payload = text.toUtf8();
    }
    m_adapter->sendData(payload);
}
