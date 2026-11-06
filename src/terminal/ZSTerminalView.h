#pragma once
#include <QQuickPaintedItem>
#include <QPointer>
#include "ZSTerminalRenderer.h"

class ZSPuTTYBackendAdapter; /* forward decl; includes kept out of the header */

/* Terminal view: a QQuickPaintedItem that owns a ZSTerminalRenderer and is the
 * host object bound to a PuTTY TermWin (via zs_make_termwin). The PuTTY glue
 * calls back into the extern "C" zs_view_* functions (defined in this .cpp) with
 * `host` == this object. */
class ZSTerminalView : public QQuickPaintedItem
{
    Q_OBJECT
public:
    explicit ZSTerminalView(QQuickItem *parent = nullptr);
    ~ZSTerminalView() override;

    void paint(QPainter *painter) override;

    ZSTerminalRenderer *renderer() { return &m_renderer; }

    void setBackend(ZSPuTTYBackendAdapter *b);
    ZSPuTTYBackendAdapter *backend() const;

    /* Size the terminal grid (columns/rows). Called by the backend on open(). */
    Q_INVOKABLE void configureGrid(int cols, int rows);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;

private slots:
    void onRendererResized();
    void onContentChanged();

private:
    QByteArray keyToBytes(QKeyEvent *event) const;

    ZSTerminalRenderer m_renderer;
    QPointer<ZSPuTTYBackendAdapter> m_backend;
};
