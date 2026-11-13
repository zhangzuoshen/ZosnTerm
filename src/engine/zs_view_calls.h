#pragma once
#ifdef __cplusplus
extern "C" {
#endif

/* Downward calls issued by the C PuTTY glue (zs_putty_term.c) and
 * implemented on the C++ side (ZSTerminalView). `host` is the
 * ZSTerminalView* bound at zs_make_termwin() time.
 *
 * Colours are passed as scalar components (not structs) to avoid any
 * ABI mismatch between the C and C++ compilers. */

void zs_view_draw_text(void *host, int col, int row,
                       const wchar_t *text, int len,
                       unsigned long attrs, int line_attrs,
                       int fg_enabled, unsigned fg_r, unsigned fg_g, unsigned fg_b,
                       int bg_enabled, unsigned bg_r, unsigned bg_g, unsigned bg_b);

void zs_view_draw_cursor(void *host, int col, int row,
                         const wchar_t *text, int len,
                         unsigned long attrs, int line_attrs,
                         int fg_enabled, unsigned fg_r, unsigned fg_g, unsigned fg_b,
                         int bg_enabled, unsigned bg_r, unsigned bg_g, unsigned bg_b);

void zs_view_refresh(void *host);
void zs_view_set_title(void *host, const char *title);
void zs_view_request_resize(void *host, int w, int h);

/* `colours` is `n * 3` bytes: r,g,b per entry, starting at OSC4 index `start`. */
void zs_view_palette_set(void *host, unsigned start, unsigned n,
                         const unsigned char *colours);

void zs_view_bell(void *host, int mode);

/* Scrollbar state: total lines, current top (0=oldest, total-page=newest), visible page. */
void zs_view_set_scrollbar(void *host, int total, int top, int page);

#ifdef __cplusplus
}
#endif
