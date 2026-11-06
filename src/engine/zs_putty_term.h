#pragma once
#ifdef __cplusplus
extern "C" {
#endif

/* ZosnTerm PuTTY engine: opaque C API boundary.
 *
 * C++ translation units (ZSTerminalView, ZSPuTTYBackendAdapter) include ONLY
 * this header. They never see any PuTTY type, because putty.h is NOT
 * C++-safe (it uses the C++ keyword "private" as a parameter name and relies
 * on implicit int->enum conversions). All PuTTY state is hidden behind void*.
 */

typedef void *ZSConf;       /* struct Conf */
typedef void *ZSUCS;        /* struct unicode_data */
typedef void *ZSTerminal;   /* struct Terminal */
typedef void *ZSTermWin;    /* struct TermWin */

/* Serial parity / flow values (mirror PuTTY SER_PAR_* / SER_FLOW_*). */
#define ZS_PAR_NONE    0
#define ZS_PAR_ODD     1
#define ZS_PAR_EVEN    2
#define ZS_PAR_MARK    3
#define ZS_PAR_SPACE   4

#define ZS_FLOW_NONE     0
#define ZS_FLOW_XONXOFF 1
#define ZS_FLOW_RTSCTS   2
#define ZS_FLOW_DSRDTR   3

/* ---- Config / unicode lifecycle ---- */
ZSConf      zs_conf_new(void);
void        zs_conf_free(ZSConf conf);
void        zs_do_defaults(ZSConf conf);
void        zs_configure_serial(ZSConf conf,
                                const char *serial_line, int speed,
                                int databits, int stopbits, int parity,
                                int flow, const char *termtype);
ZSUCS       zs_ucs_new(void);
void        zs_ucs_free(ZSUCS ucs);
void        zs_init_ucs(ZSConf conf, ZSUCS ucs);

/* ---- Terminal lifecycle ---- */
ZSTerminal  zs_term_init(ZSConf conf, ZSUCS ucs, ZSTermWin termwin);
void        zs_term_free(ZSTerminal term);
void        zs_term_size(ZSTerminal term, int rows, int cols, int savelines);
void        zs_term_data(ZSTerminal term, const void *data, unsigned long len);
void        zs_term_update(ZSTerminal term);
void        zs_term_pwron(ZSTerminal term, int on);
void        zs_term_set_trust(ZSTerminal term, int trusted);

/* Drive timers + toplevel callbacks + paint. Call from a Qt timer tick. */
void        zs_tick(ZSTerminal term);

/* TermWin (front-end drawing surface) bound to a host object (the view). */
ZSTermWin   zs_make_termwin(void *host);
void        zs_free_termwin(ZSTermWin termwin);

#ifdef __cplusplus
}
#endif
