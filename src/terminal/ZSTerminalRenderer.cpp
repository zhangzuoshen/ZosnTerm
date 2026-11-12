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

void ZSTerminalRenderer::recomputeCellMetrics()
{
    QFontMetricsF fm(m_font);
    m_cellH = qMax(1, qRound(fm.height()));
    m_cellW = qMax(1, qRound(fm.horizontalAdvance(QLatin1Char('M'))));
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
    Q_UNUSED(attrs);
    Q_UNUSED(tc);
    if (m_cellW <= 0 || m_cellH <= 0 || len <= 0) return;

    const int x = col * m_cellW;
    const int y = row * m_cellH;
    QColor cursorCol = m_palette[260];   // 光标填充色
    QColor cursorText = m_palette[261];  // 光标上文字色

    QPainter p(&m_img);
    p.setFont(m_font);

    if (m_cursorShape == 1) {
        // 下划线光标
        const int h = qMax(2, m_cellH / 10);
        p.fillRect(x, y + m_cellH - h, len * m_cellW, h, cursorCol);
    } else if (m_cursorShape == 2) {
        // 竖条光标
        const int w = qMax(2, m_cellW / 8);
        p.fillRect(x, y, w, m_cellH, cursorCol);
    } else {
        // 块状光标（默认）：用光标色填充整格，文字反相显示在光标上
        p.fillRect(x, y, len * m_cellW, m_cellH, cursorCol);
        p.setPen(cursorText);
        const QString s = QString::fromWCharArray(text, len);
        p.drawText(x, y, len * m_cellW, m_cellH,
                   Qt::AlignLeft | Qt::AlignTop | Qt::TextSingleLine, s);
    }
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
    m_palette[260] = QColor(192,192,192); /* cursor fg */
    m_palette[261] = QColor(0,0,0);       /* cursor bg */
}

/* ---------------- M5 外观设置 ---------------- */

void ZSTerminalRenderer::setFont(const QFont &font)
{
    m_font = font;
    m_font.setStyleHint(QFont::Monospace);
    m_font.setFixedPitch(true);
    m_font.setKerning(false);
    recomputeCellMetrics();
    if (m_cols > 0 && m_rows > 0) {
        m_img = QImage(m_cols * m_cellW, m_rows * m_cellH, QImage::Format_RGB32);
        m_img.fill(m_palette[258]);
        m_pendingRefresh = true;
        emit resized();
        emit contentChanged();
    }
}

void ZSTerminalRenderer::setDefaultForeground(const QColor &c)
{
    m_palette[256] = c;
    m_palette[257] = c;
    m_pendingRefresh = true;
    emit contentChanged();
}

void ZSTerminalRenderer::setDefaultBackground(const QColor &c)
{
    m_palette[258] = c;
    m_palette[259] = c;
    if (!m_img.isNull()) {
        m_img.fill(c);
        m_pendingRefresh = true;
        emit contentChanged();
    }
}

void ZSTerminalRenderer::setCursorColor(const QColor &c)
{
    m_palette[260] = c;
    m_palette[261] = c;
    m_pendingRefresh = true;
    emit contentChanged();
}

void ZSTerminalRenderer::setCursorShape(int shape)
{
    m_cursorShape = qBound(0, shape, 2);
    m_pendingRefresh = true;
    emit contentChanged();
}
