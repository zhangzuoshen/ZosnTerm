#include "ZSTerminalView.h"
#include "ZSPuTTYBackendAdapter.h"
#include "zs_view_calls.h"

#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QGuiApplication>

ZSTerminalView::ZSTerminalView(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setRenderTarget(QQuickPaintedItem::FramebufferObject);
    setAntialiasing(false);
    setAcceptedMouseButtons(Qt::AllButtons);
    setFocus(true);
    connect(&m_renderer, &ZSTerminalRenderer::resized, this, &ZSTerminalView::onRendererResized);
    connect(&m_renderer, &ZSTerminalRenderer::contentChanged, this, &ZSTerminalView::onContentChanged);
    onRendererResized();
}

ZSTerminalView::~ZSTerminalView() = default;

void ZSTerminalView::onRendererResized()
{
    setImplicitWidth(m_renderer.image().width());
    setImplicitHeight(m_renderer.image().height());
    update();
}

void ZSTerminalView::onContentChanged()
{
    update();
}

void ZSTerminalView::paint(QPainter *painter)
{
    const QImage &img = m_renderer.image();
    if (img.isNull()) {
        painter->fillRect(boundingRect(), QColor(0, 0, 0));
        return;
    }
    painter->drawImage(QPoint(0, 0), img);
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
    const int cols = qMax(1, static_cast<int>(w / cellW));
    const int rows = qMax(1, static_cast<int>(h / cellH));
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
    forceActiveFocus();
    event->accept();
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

ZSPuTTYBackendAdapter *ZSTerminalView::backend() const
{
    return m_backend;
}
