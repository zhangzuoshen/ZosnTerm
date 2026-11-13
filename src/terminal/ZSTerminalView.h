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
 * reflows like MobaXterm.
 * Scroll: PuTTY keeps a scrollback buffer; we expose it through an on-screen
 * scrollbar (wheel to scroll back, drag the thumb to seek) driven by the
 * zs_view_set_scrollbar callback. */
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

    /* Called by the PuTTY glue (zs_view_set_scrollbar) on every repaint to
     * report the scrollback range/position so we can draw + drive a scrollbar.
     * total = scrollback+visible lines; top = current top (0 = oldest
     * scrollback, total-page = newest); page = visible rows. */
    void setScrollbarInfo(int total, int top, int page);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
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

    /* Scrollbar geometry (track is a thin strip on the right edge). */
    bool isInScrollbar(const QPointF &p) const;
    qreal scrollbarTrackHeight() const;
    qreal scrollbarThumbHeight() const;
    int   scrollTopFromPos(qreal y) const;
    void  scrollTo(int top);
    void  drawScrollbar(QPainter *painter);

    ZSTerminalRenderer m_renderer;
    QPointer<ZSPuTTYBackendAdapter> m_backend;

    /* Scrollback state reported by PuTTY. */
    int  m_scrollTotal = 0;   // total lines (scrollback + visible)
    int  m_scrollTop  = 0;    // current top (0 = oldest, total-page = newest)
    int  m_scrollPage = 0;    // visible rows
public:
    /* Right-click context menu + find support (driven from QML). */
    void scrollToBottom();
    void scrollToAbsLine(int absLine);
    void setFindHighlight(int absLine, int col, int len);
    void clearFindHighlight();

signals:
    void contextMenuRequested(qreal x, qreal y);

private:
    void drawFindHighlight(QPainter *painter);

    bool m_scrolling = false;  // user is dragging the scrollbar thumb

    /* Find highlight: an absolute line (0 = oldest scrollback) plus the
     * character column/length of the current match, in terminal cells. */
    bool m_findActive = false;
    int  m_findAbsLine = -1;
    int  m_findCol = 0;
    int  m_findLen = 0;
};
