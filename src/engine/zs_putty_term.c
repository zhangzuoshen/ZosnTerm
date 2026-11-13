/* ZosnTerm PuTTY engine glue.
 *
 * THIS IS THE ONLY TRANSLATION UNIT THAT INCLUDES putty.h.
 * All PuTTY types are confined here; everything else talks to us through
 * the opaque void* API in zs_putty_term.h. */

#include "putty.h"
#include "terminal.h"
#include "zs_putty_term.h"
#include "zs_view_calls.h"

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <wchar.h>

/* editing-op forward decl (terminal.h is intentionally not included in this TU) */
void term_clrsb(Terminal *);

/* Scrollback lines retained for the on-screen scrollbar. */
#define ZS_SAVELINES 2000

/* Our TermWin wrapper. TermWin must be the first member so we can recover the
 * host pointer via offsetof(container_of). */
struct ZSTermWin {
    TermWin base;
    void *host;
};

#define ZS_CONTAINER_OF(ptr, type, field) \
    ((type *)((char *)(ptr) - offsetof(type, field)))

/* ------------------------------------------------------------------ */
/* TermWinVtable methods                                               */
/* ------------------------------------------------------------------ */

static bool zs_tw_setup_draw_ctx(TermWin *win)
{
    (void)win;
    return true; /* we always have a draw context (Qt renderer) */
}

static void zs_tw_free_draw_ctx(TermWin *win) { (void)win; }

static void zs_tw_draw_text(TermWin *win, int x, int y, wchar_t *text, int len,
                           unsigned long attrs, int line_attrs, truecolour tc)
{
    struct ZSTermWin *z = ZS_CONTAINER_OF(win, struct ZSTermWin, base);
    zs_view_draw_text(z->host, x, y, text, len, attrs, line_attrs,
                      tc.fg.enabled, tc.fg.r, tc.fg.g, tc.fg.b,
                      tc.bg.enabled, tc.bg.r, tc.bg.g, tc.bg.b);
}

static void zs_tw_draw_cursor(TermWin *win, int x, int y, wchar_t *text, int len,
                             unsigned long attrs, int line_attrs, truecolour tc)
{
    struct ZSTermWin *z = ZS_CONTAINER_OF(win, struct ZSTermWin, base);
    zs_view_draw_cursor(z->host, x, y, text, len, attrs, line_attrs,
                        tc.fg.enabled, tc.fg.r, tc.fg.g, tc.fg.b,
                        tc.bg.enabled, tc.bg.r, tc.bg.g, tc.bg.b);
}

static void zs_tw_draw_trust_sigil(TermWin *win, int x, int y)
{
    (void)win; (void)x; (void)y;
}

static int zs_tw_char_width(TermWin *win, int uc)
{
    (void)win;
    /* PuTTY's own implementation is portable; the C runtime wcwidth() is
     * missing from some Windows toolchains (MSVCRT / older MinGW). */
    int w = mk_wcwidth((unsigned int)uc);
    if (w < 0) w = 1;
    return w;
}

static void zs_tw_set_cursor_pos(TermWin *win, int x, int y) { (void)win;(void)x;(void)y; }
static void zs_tw_set_raw_mouse_mode(TermWin *win, bool e) { (void)win;(void)e; }
static void zs_tw_set_raw_mouse_mode_pointer(TermWin *win, bool e) { (void)win;(void)e; }
static void zs_tw_set_scrollbar(TermWin *win, int t, int s, int p)
{
    struct ZSTermWin *z = ZS_CONTAINER_OF(win, struct ZSTermWin, base);
    zs_view_set_scrollbar(z->host, t, s, p);
}

static void zs_tw_bell(TermWin *win, int mode)
{
    struct ZSTermWin *z = ZS_CONTAINER_OF(win, struct ZSTermWin, base);
    zs_view_bell(z->host, mode);
}

static void zs_tw_clip_write(TermWin *win, int cb, wchar_t *text, int *attrs,
                            truecolour *colours, int len, bool desel)
{
    (void)attrs;(void)colours;(void)desel;
    (void)cb;
    struct ZSTermWin *z = ZS_CONTAINER_OF(win, struct ZSTermWin, base);
    zs_view_set_clipboard(z->host, text, len);
}

static void zs_tw_clip_request_paste(TermWin *win, int cb) { (void)win;(void)cb; }

static void zs_tw_refresh(TermWin *win)
{
    struct ZSTermWin *z = ZS_CONTAINER_OF(win, struct ZSTermWin, base);
    zs_view_refresh(z->host);
}

static void zs_tw_request_resize(TermWin *win, int w, int h)
{
    struct ZSTermWin *z = ZS_CONTAINER_OF(win, struct ZSTermWin, base);
    zs_view_request_resize(z->host, w, h);
}

static void zs_tw_set_title(TermWin *win, const char *title, int codepage)
{
    struct ZSTermWin *z = ZS_CONTAINER_OF(win, struct ZSTermWin, base);
    (void)codepage;
    zs_view_set_title(z->host, title);
}

static void zs_tw_set_icon_title(TermWin *win, const char *t, int cp)
{
    (void)win;(void)t;(void)cp;
}
static void zs_tw_set_minimised(TermWin *win, bool m) { (void)win;(void)m; }
static void zs_tw_set_maximised(TermWin *win, bool m) { (void)win;(void)m; }
static void zs_tw_move(TermWin *win, int x, int y) { (void)win;(void)x;(void)y; }
static void zs_tw_set_zorder(TermWin *win, bool top) { (void)win;(void)top; }

static void zs_tw_palette_set(TermWin *win, unsigned start, unsigned ncolours,
                              const rgb *colours)
{
    struct ZSTermWin *z = ZS_CONTAINER_OF(win, struct ZSTermWin, base);
    unsigned char buf[OSC4_NCOLOURS * 3];
    unsigned n = ncolours;
    if (n > OSC4_NCOLOURS) n = OSC4_NCOLOURS;
    for (unsigned i = 0; i < n; i++) {
        buf[i*3+0] = colours[i].r;
        buf[i*3+1] = colours[i].g;
        buf[i*3+2] = colours[i].b;
    }
    zs_view_palette_set(z->host, start, n, buf);
}

static void zs_tw_palette_get_overrides(TermWin *win, Terminal *term)
{
    (void)win;(void)term;
}

static void zs_tw_unthrottle(TermWin *win, size_t bufsize) { (void)win;(void)bufsize; }

static const TermWinVtable zs_termwin_vt = {
    .setup_draw_ctx = zs_tw_setup_draw_ctx,
    .draw_text = zs_tw_draw_text,
    .draw_cursor = zs_tw_draw_cursor,
    .draw_trust_sigil = zs_tw_draw_trust_sigil,
    .char_width = zs_tw_char_width,
    .free_draw_ctx = zs_tw_free_draw_ctx,
    .set_cursor_pos = zs_tw_set_cursor_pos,
    .set_raw_mouse_mode = zs_tw_set_raw_mouse_mode,
    .set_raw_mouse_mode_pointer = zs_tw_set_raw_mouse_mode_pointer,
    .set_scrollbar = zs_tw_set_scrollbar,
    .bell = zs_tw_bell,
    .clip_write = zs_tw_clip_write,
    .clip_request_paste = zs_tw_clip_request_paste,
    .refresh = zs_tw_refresh,
    .request_resize = zs_tw_request_resize,
    .set_title = zs_tw_set_title,
    .set_icon_title = zs_tw_set_icon_title,
    .set_minimised = zs_tw_set_minimised,
    .set_maximised = zs_tw_set_maximised,
    .move = zs_tw_move,
    .set_zorder = zs_tw_set_zorder,
    .palette_set = zs_tw_palette_set,
    .palette_get_overrides = zs_tw_palette_get_overrides,
    .unthrottle = zs_tw_unthrottle,
};

/* ------------------------------------------------------------------ */
/* Opaque wrappers (the public C API used by C++ code)                 */
/* ------------------------------------------------------------------ */

ZSConf zs_conf_new(void) { return (ZSConf)conf_new(); }
void zs_conf_free(ZSConf conf) { conf_free((Conf *)conf); }
void zs_do_defaults(ZSConf conf) { do_defaults(NULL, (Conf *)conf); }

void zs_configure_serial(ZSConf conf, const char *serial_line, int speed,
                         int databits, int stopbits, int parity, int flow,
                         const char *termtype, const char *codepage)
{
    Conf *c = (Conf *)conf;
    conf_set_str(c, CONF_serline, serial_line ? serial_line : "");
    conf_set_int(c, CONF_serspeed, speed);
    conf_set_int(c, CONF_serdatabits, databits);
    conf_set_int(c, CONF_serstopbits, stopbits);
    conf_set_int(c, CONF_serparity, parity);
    conf_set_int(c, CONF_serflow, flow);
    conf_set_str(c, CONF_termtype, termtype ? termtype : "xterm");
    conf_set_str(c, CONF_line_codepage,
                 (codepage && *codepage) ? codepage : "UTF-8");
    conf_set_int(c, CONF_width, 80);
    conf_set_int(c, CONF_height, 24);
}

ZSUCS zs_ucs_new(void)
{
    struct unicode_data *u = snew(struct unicode_data);
    memset(u, 0, sizeof(*u));
    return (ZSUCS)u;
}
void zs_ucs_free(ZSUCS ucs) { sfree((struct unicode_data *)ucs); }

void zs_init_ucs(ZSConf conf, ZSUCS ucs)
{
    init_ucs_generic((Conf *)conf, (struct unicode_data *)ucs);
}

ZSTerminal zs_term_init(ZSConf conf, ZSUCS ucs, ZSTermWin termwin)
{
    Terminal *t = term_init((Conf *)conf, (struct unicode_data *)ucs,
                            (TermWin *)termwin);

    /* term_init() leaves rows/cols == -1.  With a real drawing frontend
     * (setup_draw_ctx returns true) the term_update() inside term_pwron()
     * would run do_paint(), whose snewn(term->cols, termchar) then calls
     * out_of_memory() on -1.  Establish a sane grid immediately so no
     * caller can ever paint an unsized terminal. */
    int rows = conf_get_int((Conf *)conf, CONF_height);
    int cols = conf_get_int((Conf *)conf, CONF_width);
    if (rows < 1) rows = 24;
    if (cols < 1) cols = 80;
    term_size(t, rows, cols, ZS_SAVELINES);

    return (ZSTerminal)t;
}
void zs_term_free(ZSTerminal term) { term_free((Terminal *)term); }

void zs_term_size(ZSTerminal term, int rows, int cols, int savelines)
{
    (void)savelines;
    term_size((Terminal *)term, rows, cols, ZS_SAVELINES);
}

void zs_term_data(ZSTerminal term, const void *data, unsigned long len)
{
    term_data((Terminal *)term, data, (size_t)len);
}

void zs_term_update(ZSTerminal term) { term_update((Terminal *)term); }
void zs_term_pwron(ZSTerminal term, int on) { term_pwron((Terminal *)term, on != 0); }
void zs_term_set_trust(ZSTerminal term, int trusted)
{
    term_set_trust_status((Terminal *)term, trusted != 0);
}

void zs_tick(ZSTerminal term)
{
    unsigned long next = 0;
    run_timers(getticks(), &next);
    run_toplevel_callbacks();
    term_update((Terminal *)term);
}

void zs_term_scroll_to(ZSTerminal term, int top, int total, int page)
{
    int nscroll = total - page;
    int disptop = top - nscroll;
    term_scroll((Terminal *)term, -1, disptop);
}

ZSTermWin zs_make_termwin(void *host)
{
    struct ZSTermWin *z = snew(struct ZSTermWin);
    memset(z, 0, sizeof(*z));
    z->base.vt = &zs_termwin_vt;
    z->host = host;
    return (ZSTermWin)z;
}

void zs_free_termwin(ZSTermWin termwin)
{
    sfree((struct ZSTermWin *)termwin);
}

/* ------------------------------------------------------------------ */
/* Editing operations                                                  */
/* ------------------------------------------------------------------ */

/* Select the entire scrollback + screen. Mirrors term_copyall's range.
 * terminal.h (which defines `pos` and the selection enums) is intentionally
 * NOT included here, so we assign the selstart/selend members directly and use
 * the known enum integer values:
 *   selstate: NO_SELECTION=0 ABOUT_TO=1 DRAGGING=2 SELECTED=3
 *   seltype : LEXICOGRAPHIC=0 RECTANGULAR=1
 *   selmode : SM_CHAR=0 SM_WORD=1 SM_LINE=2 */
void zs_term_select_all(ZSTerminal term)
{
    Terminal *t = (Terminal *)term;
    if (!t) return;
    t->selstart.x = 0;
    t->selstart.y = -ZS_SAVELINES;
    t->selend.x = t->cols;
    t->selend.y = t->rows + ZS_SAVELINES - 1;
    t->seltype = 0;   /* LEXICOGRAPHIC */
    t->selmode = 0;   /* SM_CHAR */
    t->selstate = 3;  /* SELECTED */
    term_update(t);
}

/* Copy the whole terminal buffer to the system clipboard. */
void zs_term_copy(ZSTerminal term)
{
    Terminal *t = (Terminal *)term;
    if (!t) return;
    int cb = CLIP_LOCAL;
    term_copyall(t, &cb, 1);   /* populates t->last_selected_text */
    struct ZSTermWin *z = ZS_CONTAINER_OF(t->win, struct ZSTermWin, base);
    zs_view_set_clipboard(z->host, t->last_selected_text, (int)t->last_selected_len);
}

/* Paste text from the system clipboard into the terminal input. */
void zs_term_paste(ZSTerminal term, const wchar_t *data, size_t len)
{
    Terminal *t = (Terminal *)term;
    if (!t || !data || len == 0) return;
    term_do_paste(t, data, len);
}

/* Clear scrollback + reset the on-screen terminal (PuTTY's own reset). */
void zs_term_clear(ZSTerminal term)
{
    Terminal *t = (Terminal *)term;
    if (!t) return;
    term_clrsb(t);
    term_pwron(t, true);
}

/* Extract the full terminal text (scrollback + screen) as a NUL-terminated
 * wchar_t buffer allocated with malloc(); the caller must free() it. */
void zs_term_find_text(ZSTerminal term, wchar_t **out, size_t *out_len)
{
    Terminal *t = (Terminal *)term;
    *out = NULL;
    *out_len = 0;
    if (!t) return;
    int cb = CLIP_LOCAL;
    term_copyall(t, &cb, 1);   /* populates t->last_selected_text */
    size_t n = t->last_selected_len;
    wchar_t *buf = (wchar_t *)malloc((n + 1) * sizeof(wchar_t));
    if (!buf) return;
    for (size_t i = 0; i < n; i++)
        buf[i] = t->last_selected_text[i];
    buf[n] = 0;
    *out = buf;
    *out_len = n;
}
