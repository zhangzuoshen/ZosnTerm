#pragma once
#include "ZSIConnection.h"
#include "zs_putty_term.h"   /* opaque void* handles only; no PuTTY types */

#include <QSerialPort>
#include <QTimer>
#include <QPointer>

class ZSTerminalView; /* forward decl */

/* Drives the embedded PuTTY terminal-emulation engine for a serial connection.
 *
 * Lifecycle (open):
 *   zs_conf_new -> zs_do_defaults -> zs_configure_serial
 *   -> zs_ucs_new -> zs_init_ucs
 *   -> zs_make_termwin(view) -> zs_term_init -> zs_term_pwron -> zs_term_size
 *   -> QSerialPort open; bytes read -> zs_term_data; QTimer -> zs_tick.
 *
 * The serial transport is provided by Qt (QSerialPort); we do NOT use PuTTY's
 * own serial backend. */
class ZSPuTTYBackendAdapter : public ZSIConnection
{
    Q_OBJECT
public:
    explicit ZSPuTTYBackendAdapter(QObject *parent = nullptr);
    ~ZSPuTTYBackendAdapter() override;

    bool open() override;
    void close() override;
    bool isOpen() const override { return m_open; }

    /* Wire the view that hosts this terminal's TermWin. Must be called before
     * open(). */
    void setView(ZSTerminalView *view) { m_view = view; }
    ZSTerminalView *view() const { return m_view; }

    /* Serial parameters (mirror PuTTY SER_PAR / SER_FLOW families via ZS_* macros). */
    void setConnectionParameters(const QString &portName, int baudRate,
                                 int dataBits = 8, int stopBits = 1,
                                 int parity = ZS_PAR_NONE,
                                 int flow = ZS_FLOW_NONE);
    void setGrid(int cols, int rows) { m_cols = cols; m_rows = rows; }
    void resizeGrid(int cols, int rows);

    /* Keyboard / output bytes to send to the serial port. */
    Q_INVOKABLE void sendData(const QByteArray &data);

signals:
    void statusMessage(const QString &msg);

private slots:
    void onReadyRead();
    void onTick();

private:
    bool applySerialSettings();

    ZSConf      m_conf = nullptr;
    ZSUCS       m_ucs = nullptr;
    ZSTerminal  m_term = nullptr;
    ZSTermWin   m_termwin = nullptr;

    QSerialPort  m_port;
    QTimer       m_timer;

    QPointer<ZSTerminalView> m_view;

    QString  m_portName;
    int      m_baudRate = 9600;
    int      m_dataBits = 8;
    int      m_stopBits = 1;
    int      m_parity = ZS_PAR_NONE;
    int      m_flow = ZS_FLOW_NONE;
    int      m_cols = 80;
    int      m_rows = 24;

    bool     m_open = false;
};
