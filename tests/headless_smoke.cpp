/* ZosnTerm M2 headless smoke test.
 *
 * Proves the embedded PuTTY terminal-emulation engine drives the draw
 * callbacks (zs_view_draw_text) end-to-end, with NO Qt / GUI dependency.
 * Provides its own zs_view_* stubs (matching zs_view_calls.h) and links
 * only against the putty_engine static library. */

#include <cstdio>
#include <cstring>
#include <cwchar>
#include <string>
#include <vector>

#include "zs_putty_term.h"
#include "zs_view_calls.h"

struct ZSDrawRecord {
    int col;
    int row;
    unsigned long attrs;
    std::string text;
};

static std::vector<ZSDrawRecord> g_draws;

static std::string narrow(const wchar_t *text, int len)
{
    if (!text || len <= 0)
        return std::string();
    std::wstring w(text, len);
    return std::string(w.begin(), w.end());
}

extern "C" void zs_view_draw_text(void *host, int col, int row,
    const wchar_t *text, int len, unsigned long attrs, int lineAttrs,
    int fgE, unsigned fgR, unsigned fgG, unsigned fgB,
    int bgE, unsigned bgR, unsigned bgG, unsigned bgB)
{
    (void)host; (void)lineAttrs;
    (void)fgE; (void)fgR; (void)fgG; (void)fgB;
    (void)bgE; (void)bgR; (void)bgG; (void)bgB;
    g_draws.push_back({col, row, attrs, narrow(text, len)});
}

extern "C" void zs_view_draw_cursor(void *host, int col, int row,
    const wchar_t *text, int len, unsigned long attrs, int lineAttrs,
    int fgE, unsigned fgR, unsigned fgG, unsigned fgB,
    int bgE, unsigned bgR, unsigned bgG, unsigned bgB)
{
    (void)host; (void)col; (void)row; (void)text; (void)len; (void)attrs; (void)lineAttrs;
    (void)fgE; (void)fgR; (void)fgG; (void)fgB;
    (void)bgE; (void)bgR; (void)bgG; (void)bgB;
}

extern "C" void zs_view_refresh(void *host)        { (void)host; }
extern "C" void zs_view_set_title(void *host, const char *t) { (void)host; (void)t; }
extern "C" void zs_view_request_resize(void *host, int w, int h) { (void)host; (void)w; (void)h; }
extern "C" void zs_view_palette_set(void *host, unsigned s, unsigned n, const unsigned char *c)
{ (void)host; (void)s; (void)n; (void)c; }
extern "C" void zs_view_bell(void *host, int m)    { (void)host; (void)m; }
extern "C" void zs_view_set_scrollbar(void *host, int total, int top, int page)
{ (void)host; (void)total; (void)top; (void)page; }

static void pump(ZSTerminal term)
{
    for (int i = 0; i < 8; ++i)
        zs_tick(term);
}

int main()
{
    ZSConf conf = zs_conf_new();
    zs_do_defaults(conf);
    zs_configure_serial(conf, "/dev/null", 9600, 8, 1, ZS_PAR_NONE,
                        ZS_FLOW_NONE, "xterm", "UTF-8");
    ZSUCS ucs = zs_ucs_new();
    zs_init_ucs(conf, ucs);
    ZSTermWin tw = zs_make_termwin(nullptr);
    ZSTerminal term = zs_term_init(conf, ucs, tw);
    zs_term_size(term, 24, 80, 0);
    zs_term_pwron(term, 1);
    zs_term_set_trust(term, 0);

    /* --- Case 1: plain text lands at cell (0,0) --- */
    const char *hello = "hello\r\n";
    zs_term_data(term, hello, (unsigned long)strlen(hello));
    pump(term);

    bool helloAtOrigin = false;
    for (const auto &d : g_draws) {
        if (d.row == 0 && d.col == 0 && d.text.rfind("hello", 0) == 0)
            helloAtOrigin = true;
    }

    const size_t case1Count = g_draws.size();

    /* --- Case 2: SGR colour attribute reaches the draw callback ---
     * ATTR_FGMASK == 0x1FF; ANSI red is palette index 1. PuTTY paints whole
     * rows, so "RED" may be followed by padding blanks in the same run. */
    g_draws.clear();
    const char *ansi = "\033[31mRED\033[0m";
    zs_term_data(term, ansi, (unsigned long)strlen(ansi));
    pump(term);

    bool sawRedRun = false;
    for (const auto &d : g_draws) {
        if ((d.attrs & 0x1FFUL) == 1UL && d.text.find("RED") != std::string::npos)
            sawRedRun = true;
    }

    printf("[smoke] case1 draw callbacks      = %zu\n", case1Count);
    printf("[smoke] case1 'hello' at (0,0)    = %s\n", helloAtOrigin ? "yes" : "no");
    printf("[smoke] case2 SGR red run present = %s\n", sawRedRun ? "yes" : "no");

    zs_term_free(term);
    zs_free_termwin(tw);
    zs_ucs_free(ucs);
    zs_conf_free(conf);

    const bool ok = helloAtOrigin && sawRedRun;
    printf(ok ? "[smoke] PASS: emulation -> draw callback chain verified\n"
              : "[smoke] FAIL: draw callback chain incomplete\n");
    return ok ? 0 : 1;
}
