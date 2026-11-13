#include "ZSTerminalView.h"
#include "ZSPuTTYBackendAdapter.h"
#include "zs_view_calls.h"

#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QGuiApplication>
#include <QtMath>

static const qreal kScrollbarWidth = 12.0;

ZSTerminalView::ZSTerminalView(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setRenderTarget(QQuickPaintedItem::FramebufferObject);
    setAntialiasing(false);
    setAcceptedMouseButtons(Qt::AllButtons);
    setFocus(true);
    connect(&m_renderer, &ZSTerminalRenderer::resized, this, &ZSTerminalView::onRendererResized);
    connect(&m_renderer, &ZSTerminalRenderer::contentChanged, this, &ZSTerminalView::onContentChanged);

    /* Seed the grid from whatever size we have right now; geometryChange() and
     * paint() will keep it in sync with the window thereafter. */
    syncGridToGeometry();
}

ZSTerminalView::~ZSTerminalView() = default;

void ZSTerminalView::onRendererResized()
{
    /* NOTE: do NOT setImplicitWidth/Height here. The terminal must always fill
     * the area the layout gives it (Layout.fillWidth/Height: true); feeding the
     * rendered image size back as the implicit size can make the layout shrink
     * the item to a 80x24-sized box, which is exactly the "small region" bug. */
    update();
}

void ZSTerminalView::onContentChanged()
{
    update();
}

void ZSTerminalView::paint(QPainter *painter)
{
    /* Defensive backstop: ensure the raster matches the current item size on
     * every frame. geometryChange() normally keeps it in sync, but this guards
     * against the first geometry change firing before layout settled, or
     * platforms where geometryChange arrives with a stale size. */
    syncGridToGeometry();

    const QImage &img = m_renderer.image();
    if (img.isNull()) {
        painter->fillRect(boundingRect(), QColor(0, 0, 0));
        return;
    }
    painter->drawImage(QPoint(0, 0), img);
    drawScrollbar(painter);
}

void ZSTerminalView::setBackend(ZSPuTTYBackendAdapter *b)
{
    m_backend = b;
}

void ZSTerminalView::configureGrid(int /*cols*/, int /*rows*/)
{
    /* The real grid is derived from the item's pixel geometry so the terminal
     * always matches the window size; see syncGridToGeometry(). */
    syncGridToGeometry();
}

void ZSTerminalView::syncGridToGeometry()
{
    const qreal w = width();
    const qreal h = height();
    if (w <= 0 || h <= 0)
        return;
    const int cellW = m_renderer.cellWidth();
    const int cellH = m_renderer.cellHeight();
    if (cellW <= 0 || cellH <= 0)
        return;

    /* ceil() so the raster always *covers* the whole item (no uncovered strip
     * at the right/bottom edge). The renderer image is cols*cellW which is >=
     * the item width; the tiny overflow is clipped by the item bounds. */
    const int cols = qMax(1, static_cast<int>(qCeil(w / cellW)));
    const int rows = qMax(1, static_cast<int>(qCeil(h / cellH)));

    if (cols == m_renderer.cols() && rows == m_renderer.rows())
        return;

    m_renderer.setGrid(cols, rows);
    if (m_backend)
        m_backend->resizeGrid(cols, rows);
}

void ZSTerminalView::geometryChange(const QRectF &newGeometry,
                                    const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    Q_UNUSED(oldGeometry);
    syncGridToGeometry();
}

void ZSTerminalView::focusInEvent(QFocusEvent *event)
{
    QQuickPaintedItem::focusInEvent(event);
    setFocus(true);
}

void ZSTerminalView::mousePressEvent(QMouseEvent *event)
{
    QQuickPaintedItem::mousePressEvent(event);
    if (isInScrollbar(event->position())) {
        m_scrolling = true;
        scrollTo(scrollTopFromPos(event->position().y()));
        event->accept();
        return;
    }
    forceActiveFocus();
    event->accept();
}

void ZSTerminalView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_scrolling) {
        scrollTo(scrollTopFromPos(event->position().y()));
        event->accept();
        return;
    }
    QQuickPaintedItem::mouseMoveEvent(event);
}

void ZSTerminalView::mouseReleaseEvent(QMouseEvent *event)
{
    m_scrolling = false;
    QQuickPaintedItem::mouseReleaseEvent(event);
}

void ZSTerminalView::wheelEvent(QWheelEvent *event)
{
    if (m_scrollTotal > m_scrollPage && m_scrollPage > 0) {
        const int lines = 3;                       // 每次滚动 3 行
        const int dir = (event->angleDelta().y() > 0) ? -1 : 1;  // 上滚=看更旧历史
        scrollTo(m_scrollTop + dir * lines);
        event->accept();
        return;
    }
    QQuickPaintedItem::wheelEvent(event);
}

void ZSTerminalView::setScrollbarInfo(int total, int top, int page)
{
    m_scrollTotal = total;
    m_scrollTop = top;
    m_scrollPage = page;
    update();
}

bool ZSTerminalView::isInScrollbar(const QPointF &p) const
{
    return p.x() >= width() - kScrollbarWidth && p.x() <= width();
}

qreal ZSTerminalView::scrollbarTrackHeight() const
{
    return height();
}

qreal ZSTerminalView::scrollbarThumbHeight() const
{
    if (m_scrollTotal <= 0 || m_scrollPage <= 0)
        return 0;
    const qreal h = scrollbarTrackHeight();
    qreal thumb = h * (qreal)m_scrollPage / (qreal)m_scrollTotal;
    return qMax(thumb, 24.0);
}

int ZSTerminalView::scrollTopFromPos(qreal y) const
{
    const qreal h = scrollbarTrackHeight();
    const qreal thumb = scrollbarThumbHeight();
    const qreal maxScroll = qMax(0, m_scrollTotal - m_scrollPage);
    if (maxScroll <= 0)
        return 0;
    qreal f = (y - thumb / 2.0) / (h - thumb);
    f = qBound(0.0, f, 1.0);
    return qRound(f * maxScroll);
}

void ZSTerminalView::scrollTo(int top)
{
    const int maxScroll = qMax(0, m_scrollTotal - m_scrollPage);
    top = qBound(0, top, maxScroll);
    if (top == m_scrollTop)
        return;
    m_scrollTop = top;
    if (m_backend)
        m_backend->scrollTo(m_scrollTop, m_scrollTotal, m_scrollPage);
    update();
}

void ZSTerminalView::drawScrollbar(QPainter *painter)
{
    if (m_scrollTotal <= m_scrollPage || m_scrollPage <= 0)
        return;
    const qreal w = width();
    const qreal h = height();
    const qreal x = w - kScrollbarWidth;

    /* Track */
    painter->fillRect(QRectF(x, 0, kScrollbarWidth, h), QColor(40, 40, 40, 180));

    const qreal thumb = scrollbarThumbHeight();
    const qreal maxScroll = qMax(0, m_scrollTotal - m_scrollPage);
    const qreal f = (maxScroll > 0) ? (qreal)m_scrollTop / (qreal)maxScroll : 0.0;
    const qreal y = f * (h - thumb);

    painter->fillRect(QRectF(x + 2, y, kScrollbarWidth - 4, thumb),
                      QColor(150, 150, 150, 220));
}

QByteArray ZSTerminalView::keyToBytes(QKeyEvent *e) const
{
    switch (e->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:   return QByteArray("\r");
    case Qt::Key_Backspace: return QByteArray("\b");
    case Qt::Key_Tab:     return QByteArray("\t");
    case Qt::Key_Escape:  return QByteArray("\x1b");
    case Qt::Key_Up:      return QByteArray("\x1b[A");
    case Qt::Key_Down:    return QByteArray("\x1b[B");
    case Qt::Key_Right:   return QByteArray("\x1b[C");
    case Qt::Key_Left:    return QByteArray("\x1b[D");
    case Qt::Key_Home:    return QByteArray("\x1b[H");
    case Qt::Key_End:     return QByteArray("\x1b[F");
    default: break;
    }
    if (!e->text().isEmpty()) {
        /* e->text() already carries the final byte(s): shifted chars, Ctrl-C
         * (-> 0x03), etc. */
        return e->text().toUtf8();
    }
    return QByteArray();
}

void ZSTerminalView::keyPressEvent(QKeyEvent *event)
{
    QByteArray bytes = keyToBytes(event);
    if (!bytes.isEmpty() && m_backend) {
        m_backend->sendData(bytes);
        event->accept();
    } else {
        event->ignore();
    }
}

void ZSTerminalView::keyReleaseEvent(QKeyEvent *event)
{
    event->ignore();
}

/* ------------------------------------------------------------------ */
/* extern "C" view callbacks invoked by zs_putty_term.c                */
/* ------------------------------------------------------------------ */

extern "C" void zs_view_draw_text(void *host, int col, int row,
    const wchar_t *text, int len, unsigned long attrs, int lineAttrs,
    int fgE, unsigned fgR, unsigned fgG, unsigned fgB,
    int bgE, unsigned bgR, unsigned bgG, unsigned bgB)
{
    auto *v = static_cast<ZSTerminalView *>(host);
    if (!v) return;
    ZSTrueColour tc;
    if (fgE) { tc.fgEnabled = true; tc.fg = QColor(fgR, fgG, fgB); }
    if (bgE) { tc.bgEnabled = true; tc.bg = QColor(bgR, bgG, bgB); }
    v->renderer()->drawTextRun(col, row, text, len, attrs, lineAttrs, tc);
}

extern "C" void zs_view_draw_cursor(void *host, int col, int row,
    const wchar_t *text, int len, unsigned long attrs, int lineAttrs,
    int fgE, unsigned fgR, unsigned fgG, unsigned fgB,
    int bgE, unsigned bgR, unsigned bgG, unsigned bgB)
{
    auto *v = static_cast<ZSTerminalView *>(host);
    if (!v) return;
    ZSTrueColour tc;
    if (fgE) { tc.fgEnabled = true; tc.fg = QColor(fgR, fgG, fgB); }
    if (bgE) { tc.bgEnabled = true; tc.bg = QColor(bgR, bgG, bgB); }
    v->renderer()->drawCursor(col, row, text, len, attrs, lineAttrs, tc);
}

extern "C" void zs_view_refresh(void *host)
{
    auto *v = static_cast<ZSTerminalView *>(host);
    if (v) v->update();
}

extern "C" void zs_view_set_title(void *host, const char *title)
{
    (void)host; (void)title;
    /* M2: title is not surfaced yet. */
}

extern "C" void zs_view_request_resize(void *host, int w, int h)
{
    (void)host; (void)w; (void)h;
    /* M2: we fix the grid ourselves; ignore backend resize requests. */
}

extern "C" void zs_view_palette_set(void *host, unsigned start, unsigned n,
                                    const unsigned char *colours)
{
    auto *v = static_cast<ZSTerminalView *>(host);
    if (v) v->renderer()->setPalette(start, n, colours);
}

extern "C" void zs_view_bell(void *host, int mode)
{
    (void)host; (void)mode;
    /* M2: no audio bell. */
}

extern "C" void zs_view_set_scrollbar(void *host, int total, int top, int page)
{
    auto *v = static_cast<ZSTerminalView *>(host);
    if (v) v->setScrollbarInfo(total, top, page);
}

ZSPuTTYBackendAdapter *ZSTerminalView::backend() const
{
    return m_backend;
}
