#include "ZSPuTTYBackendAdapter.h"
#include "ZSTerminalView.h"

#include <QDebug>
#include <QSerialPort>
#include <QGuiApplication>
#include <QClipboard>
#include <cstdlib>
#include <string>

ZSPuTTYBackendAdapter::ZSPuTTYBackendAdapter(QObject *parent)
    : ZSIConnection(parent)
{
    m_port.setParent(this);
    m_timer.setParent(this);
    m_timer.setSingleShot(false);
}

ZSPuTTYBackendAdapter::~ZSPuTTYBackendAdapter()
{
    close();
}

void ZSPuTTYBackendAdapter::setConnectionParameters(const QString &portName,
                                                   int baudRate, int dataBits,
                                                   int stopBits, int parity, int flow)
{
    m_portName = portName;
    m_baudRate = baudRate;
    m_dataBits = dataBits;
    m_stopBits = stopBits;
    m_parity = parity;
    m_flow = flow;
}

bool ZSPuTTYBackendAdapter::applySerialSettings()
{
    m_port.setPortName(m_portName);
    if (!m_port.setBaudRate(static_cast<qint32>(m_baudRate))) {
        emit errorMessage(QStringLiteral("无法设置波特率 %1").arg(m_baudRate));
        return false;
    }
    switch (m_dataBits) {
    case 5: m_port.setDataBits(QSerialPort::Data5); break;
    case 6: m_port.setDataBits(QSerialPort::Data6); break;
    case 7: m_port.setDataBits(QSerialPort::Data7); break;
    default: m_port.setDataBits(QSerialPort::Data8); break;
    }
    m_port.setStopBits(m_stopBits == 2 ? QSerialPort::TwoStop
                       : (m_stopBits == 1 ? QSerialPort::OneStop
                                          : QSerialPort::OneAndHalfStop));
    switch (m_parity) {
    case ZS_PAR_ODD:   m_port.setParity(QSerialPort::OddParity); break;
    case ZS_PAR_EVEN:  m_port.setParity(QSerialPort::EvenParity); break;
    case ZS_PAR_MARK:  m_port.setParity(QSerialPort::MarkParity); break;
    case ZS_PAR_SPACE: m_port.setParity(QSerialPort::SpaceParity); break;
    default:           m_port.setParity(QSerialPort::NoParity); break;
    }
    switch (m_flow) {
    case ZS_FLOW_XONXOFF: m_port.setFlowControl(QSerialPort::SoftwareControl); break;
    case ZS_FLOW_RTSCTS:   m_port.setFlowControl(QSerialPort::HardwareControl); break;
    default:               m_port.setFlowControl(QSerialPort::NoFlowControl); break;
    }
    return true;
}

bool ZSPuTTYBackendAdapter::open()
{
    if (m_open)
        return true;

    if (!m_view) {
        emit errorMessage(QStringLiteral("未绑定终端视图 (ZSTerminalView)"));
        emit stateChanged(State::Error);
        return false;
    }

    /* --- PuTTY engine setup --- */
    m_conf = zs_conf_new();
    zs_do_defaults(m_conf);
    zs_configure_serial(m_conf, m_portName.toUtf8().constData(), m_baudRate,
                        m_dataBits, m_stopBits, m_parity, m_flow, "xterm",
                        m_codepage.toUtf8().constData());
    m_ucs = zs_ucs_new();
    zs_init_ucs(m_conf, m_ucs);

    m_termwin = zs_make_termwin(static_cast<void *>(m_view));
    m_term = zs_term_init(m_conf, m_ucs, m_termwin);
    m_view->configureGrid(m_cols, m_rows);
    zs_term_size(m_term, m_rows, m_cols, 0);
    zs_term_pwron(m_term, 1);
    zs_term_set_trust(m_term, 0);

    /* --- Serial port --- */
    if (!applySerialSettings()) {
        close();
        emit stateChanged(State::Error);
        return false;
    }
    if (!m_port.open(QIODevice::ReadWrite)) {
        emit errorMessage(QStringLiteral("无法打开串口 %1: %2")
                             .arg(m_portName, m_port.errorString()));
        close();
        emit stateChanged(State::Error);
        return false;
    }
    connect(&m_port, &QSerialPort::readyRead, this, &ZSPuTTYBackendAdapter::onReadyRead);

    /* --- Tick timer (timers + repaint) --- */
    connect(&m_timer, &QTimer::timeout, this, &ZSPuTTYBackendAdapter::onTick);
    m_timer.start(30);

    m_open = true;
    emit statusMessage(QStringLiteral("已连接 %1 @ %2 8N1").arg(m_portName).arg(m_baudRate));
    emit stateChanged(State::Connected);
    return true;
}

void ZSPuTTYBackendAdapter::close()
{
    if (!m_open && !m_term)
        return;
    m_timer.stop();
    if (m_port.isOpen())
        m_port.close();
    if (m_term)    { zs_term_free(m_term);    m_term = nullptr; }
    if (m_termwin) { zs_free_termwin(m_termwin); m_termwin = nullptr; }
    if (m_ucs)     { zs_ucs_free(m_ucs);     m_ucs = nullptr; }
    if (m_conf)    { zs_conf_free(m_conf);    m_conf = nullptr; }
    m_open = false;
    emit statusMessage(QStringLiteral("已断开"));
    emit stateChanged(State::Disconnected);
}

void ZSPuTTYBackendAdapter::onReadyRead()
{
    if (!m_open)
        return;
    const QByteArray data = m_port.readAll();
    if (data.isEmpty())
        return;
    zs_term_data(m_term, data.constData(), (unsigned long)data.size());
    zs_tick(m_term);
    if (m_view)
        m_view->update();
}

void ZSPuTTYBackendAdapter::onTick()
{
    if (!m_open)
        return;
    zs_tick(m_term);
    if (m_view)
        m_view->update();
}

void ZSPuTTYBackendAdapter::sendData(const QByteArray &data)
{
    if (m_open && m_port.isOpen())
        m_port.write(data);
}

void ZSPuTTYBackendAdapter::scrollTo(int top, int total, int page)
{
    if (!m_open || !m_term)
        return;
    zs_term_scroll_to(m_term, top, total, page);
    zs_tick(m_term);
    if (m_view)
        m_view->update();
}

void ZSPuTTYBackendAdapter::resizeGrid(int cols, int rows)
{
    m_cols = qMax(1, cols);
    m_rows = qMax(1, rows);
    if (m_open && m_term)
        zs_term_size(m_term, m_rows, m_cols, 0);
}

void ZSPuTTYBackendAdapter::selectAll()
{
    if (m_open && m_term)
        zs_term_select_all(m_term);
}

void ZSPuTTYBackendAdapter::copySelection()
{
    if (m_open && m_term)
        zs_term_copy(m_term);
}

void ZSPuTTYBackendAdapter::paste()
{
    if (!m_open || !m_term)
        return;
    const QString clip = QGuiApplication::clipboard()->text();
    if (clip.isEmpty())
        return;
    std::wstring w = clip.toStdWString();
    zs_term_paste(m_term, w.data(), w.size());
    zs_tick(m_term);
    if (m_view)
        m_view->update();
}

void ZSPuTTYBackendAdapter::clearScreen()
{
    if (!m_open || !m_term)
        return;
    zs_term_clear(m_term);
    zs_tick(m_term);
    if (m_view) {
        m_view->scrollToBottom();
        m_view->update();
    }
}

QString ZSPuTTYBackendAdapter::extractFullText()
{
    if (!m_open || !m_term)
        return QString();
    wchar_t *buf = nullptr;
    size_t len = 0;
    zs_term_find_text(m_term, &buf, &len);
    if (!buf)
        return QString();
    QString s = QString::fromWCharArray(buf, (int)len);
    free(buf);
    return s;
}
