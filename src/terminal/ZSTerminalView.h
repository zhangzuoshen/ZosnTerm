#pragma once
#include <QQuickPaintedItem>
#include <QPointer>
#include "ZSTerminalRenderer.h"

class ZSPuTTYBackendAdapter; /* forward decl; includes kept out of the header */

/* Terminal view: a QQuickPaintedItem that owns a ZSTerminalRenderer and is the
 * host object bound to a PuTTY TermWin (via zs_make_termwin). The PuTTY glue
 * calls back into the extern "C" zs_view_* functions (defined in this .cpp) with
 * `host` == this object.
 *
 * Input: grabs keyboard focus on mouse press so typing always reaches the
 * terminal (after interacting with the toolbar/comboboxes, focus returns here).
 * Resize: recomputes the terminal grid (cols/rows) from the item's pixel
 * geometry whenever the window is resized, so the terminal fills the area and
 * reflows like MobaXterm. */
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

    /* Size the terminal grid (columns/rows). Called by the backend on open().
     * The actual grid is derived from the current item geometry so the
     * terminal always matches the window size. */
    Q_INVOKABLE void configureGrid(int cols, int rows);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void geometryChange(const QRectF &newGeometry,
                        const QRectF &oldGeometry) override;

private slots:
    void onRendererResized();
    void onContentChanged();

private:
    /* Recompute cols/rows from the item's current pixel size and resize both
     * the renderer image and the PuTTY terminal. No-op if unchanged. */
    void syncGridToGeometry();

    QByteArray keyToBytes(QKeyEvent *event) const;

    ZSTerminalRenderer m_renderer;
    QPointer<ZSPuTTYBackendAdapter> m_backend;
};
