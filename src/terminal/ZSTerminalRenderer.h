#pragma once
#include <QObject>
#include <QImage>
#include <QColor>
#include <QFont>

/* Per-view terminal raster. Holds a QImage grid of cols*rows cells and paints
 * text runs / cursor into it using PuTTY's attribute + truecolour data.
 * Pure Qt: no PuTTY dependency. */

struct ZSTrueColour {
    bool fgEnabled = false;
    QColor fg;
    bool bgEnabled = false;
    QColor bg;
};

class ZSTerminalRenderer : public QObject
{
    Q_OBJECT
public:
    explicit ZSTerminalRenderer(QObject *parent = nullptr);

    void setCellMetrics(int cellW, int cellH);
    void setGrid(int cols, int rows);
    int cellWidth() const { return m_cellW; }
    int cellHeight() const { return m_cellH; }
    int cols() const { return m_cols; }
    int rows() const { return m_rows; }
    QSize gridSize() const { return QSize(m_cols, m_rows); }

    void drawTextRun(int col, int row, const wchar_t *text, int len,
                     unsigned long attrs, int lineAttrs, const ZSTrueColour &tc);
    void drawCursor(int col, int row, const wchar_t *text, int len,
                    unsigned long attrs, int lineAttrs, const ZSTrueColour &tc);
    void setPalette(unsigned start, unsigned n, const unsigned char *colours);
    void initDefaultPalette();

    /* ---- M5 外观设置 ---- */
    void setFont(const QFont &font);
    void setDefaultForeground(const QColor &c);
    void setDefaultBackground(const QColor &c);
    void setCursorColor(const QColor &c);
    void setCursorShape(int shape);   // 0=block 1=underline 2=bar

    QImage &image() { return m_img; }
    const QImage &image() const { return m_img; }

    bool hasPendingRefresh() const { return m_pendingRefresh; }
    void clearPendingRefresh() { m_pendingRefresh = false; }

signals:
    void resized();
    void contentChanged();

private:
    void recomputeCellMetrics();

    QColor resolveFg(unsigned long attrs, const ZSTrueColour &tc) const;
    QColor resolveBg(unsigned long attrs, const ZSTrueColour &tc) const;

    QFont m_font;
    int m_cellW = 0;
    int m_cellH = 0;
    int m_cols = 0;
    int m_rows = 0;
    QImage m_img;
    QColor m_palette[262];
    bool m_pendingRefresh = false;
    int m_cursorShape = 0;

    /* Mirror of PuTTY's attribute bit layout (see putty.h). */
    static const unsigned long ATTR_FGMASK  = 0x000001FFU;
    static const unsigned long ATTR_BGMASK  = 0x0003FE00U;
    static const unsigned long ATTR_BGSHIFT = 9;
    static const unsigned long ATTR_BOLD    = 0x00040000U;
    static const unsigned long ATTR_UNDER   = 0x00080000U;
    static const unsigned long ATTR_REVERSE = 0x00100000U;
    static const unsigned long ATTR_BLINK   = 0x00200000U;
};
