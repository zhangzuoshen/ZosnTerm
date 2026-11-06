#include "ZSTerminalRenderer.h"
#include <QPainter>
#include <QFontMetricsF>
#include <QFontDatabase>

ZSTerminalRenderer::ZSTerminalRenderer(QObject *parent)
    : QObject(parent)
{
    m_font = QFont(QStringLiteral("DejaVu Sans Mono"));
    m_font.setStyleHint(QFont::Monospace);
    m_font.setFixedPitch(true);
    m_font.setKerning(false);
    m_font.setPixelSize(14);

    QFontMetricsF fm(m_font);
    m_cellH = qMax(1, qRound(fm.height()));
    m_cellW = qMax(1, qRound(fm.horizontalAdvance(QLatin1Char('M'))));

    initDefaultPalette();
    setGrid(80, 24);
}

void ZSTerminalRenderer::setCellMetrics(int cellW, int cellH)
{
    if (cellW > 0) m_cellW = cellW;
    if (cellH > 0) m_cellH = cellH;
}

void ZSTerminalRenderer::setGrid(int cols, int rows)
{
    if (cols < 1) cols = 1;
    if (rows < 1) rows = 1;
    m_cols = cols;
    m_rows = rows;
    m_img = QImage(cols * m_cellW, rows * m_cellH, QImage::Format_RGB32);
    m_img.fill(m_palette[258]); /* default background */
    emit resized();
}

QColor ZSTerminalRenderer::resolveFg(unsigned long attrs, const ZSTrueColour &tc) const
{
    if (tc.fgEnabled) return tc.fg;
    unsigned idx = (unsigned)(attrs & ATTR_FGMASK);
    if (idx < 262) return m_palette[idx];
    return m_palette[256];
}

QColor ZSTerminalRenderer::resolveBg(unsigned long attrs, const ZSTrueColour &tc) const
{
    if (tc.bgEnabled) return tc.bg;
    unsigned idx = (unsigned)((attrs & ATTR_BGMASK) >> ATTR_BGSHIFT);
    if (idx < 262) return m_palette[idx];
    return m_palette[258];
}

void ZSTerminalRenderer::drawTextRun(int col, int row, const wchar_t *text, int len,
                                     unsigned long attrs, int lineAttrs, const ZSTrueColour &tc)
{
    Q_UNUSED(lineAttrs);
    if (m_cellW <= 0 || m_cellH <= 0 || len <= 0) return;
    QColor fg = resolveFg(attrs, tc);
    QColor bg = resolveBg(attrs, tc);
    if (attrs & ATTR_REVERSE) qSwap(fg, bg);

    const int x = col * m_cellW;
    const int y = row * m_cellH;
    QPainter p(&m_img);
    p.setFont(m_font);
    p.fillRect(x, y, len * m_cellW, m_cellH, bg);
    p.setPen(fg);
    if (attrs & ATTR_BOLD) {
        QFont f = m_font;
        f.setBold(true);
        p.setFont(f);
    }
    const QString s = QString::fromWCharArray(text, len);
    p.drawText(x, y, len * m_cellW, m_cellH,
               Qt::AlignLeft | Qt::AlignTop | Qt::TextSingleLine, s);
    if (attrs & ATTR_UNDER) {
        p.drawLine(x, y + m_cellH - 1, x + len * m_cellW - 1, y + m_cellH - 1);
    }
    m_pendingRefresh = true;
    emit contentChanged();
}

void ZSTerminalRenderer::drawCursor(int col, int row, const wchar_t *text, int len,
                                    unsigned long attrs, int lineAttrs, const ZSTrueColour &tc)
{
    Q_UNUSED(lineAttrs);
    if (m_cellW <= 0 || m_cellH <= 0 || len <= 0) return;
    QColor fg = resolveFg(attrs, tc);
    QColor bg = resolveBg(attrs, tc);
    if (attrs & ATTR_REVERSE) qSwap(fg, bg);

    const int x = col * m_cellW;
    const int y = row * m_cellH;
    QPainter p(&m_img);
    p.setFont(m_font);
    p.fillRect(x, y, len * m_cellW, m_cellH, fg);
    p.setPen(bg);
    const QString s = QString::fromWCharArray(text, len);
    p.drawText(x, y, len * m_cellW, m_cellH,
               Qt::AlignLeft | Qt::AlignTop | Qt::TextSingleLine, s);
    m_pendingRefresh = true;
    emit contentChanged();
}

void ZSTerminalRenderer::setPalette(unsigned start, unsigned n, const unsigned char *colours)
{
    for (unsigned i = 0; i < n && (start + i) < 262; ++i) {
        m_palette[start + i] = QColor(colours[i * 3], colours[i * 3 + 1], colours[i * 3 + 2]);
    }
    m_pendingRefresh = true;
    emit contentChanged();
}

void ZSTerminalRenderer::initDefaultPalette()
{
    const QColor ansi[16] = {
        QColor(0,0,0),     QColor(170,0,0),   QColor(0,170,0),   QColor(170,85,0),
        QColor(0,0,170),   QColor(170,0,170), QColor(0,170,170), QColor(170,170,170),
        QColor(85,85,85),  QColor(255,85,85), QColor(85,255,85), QColor(255,255,85),
        QColor(85,85,255), QColor(255,85,255),QColor(85,255,255),QColor(255,255,255)
    };
    for (int i = 0; i < 16; ++i) m_palette[i] = ansi[i];

    for (int i = 0; i < 216; ++i) {
        int r = (i / 36) % 6, g = (i / 6) % 6, b = i % 6;
        auto v = [](int x){ return x == 0 ? 0 : 55 + x * 40; };
        m_palette[16 + i] = QColor(v(r), v(g), v(b));
    }
    for (int i = 0; i < 24; ++i) {
        int v = 8 + i * 10;
        m_palette[232 + i] = QColor(v, v, v);
    }
    m_palette[256] = QColor(192,192,192); /* default fg */
    m_palette[257] = QColor(255,255,255); /* default bold fg */
    m_palette[258] = QColor(0,0,0);       /* default bg */
    m_palette[259] = QColor(0,0,0);       /* default bold bg */
    m_palette[260] = QColor(0,0,0);       /* cursor fg */
    m_palette[261] = QColor(192,192,192); /* cursor bg */
}
