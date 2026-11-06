/* ZosnTerm PuTTY engine: front-end symbols the engine references at link
 * time but which we (not PuTTY) must supply, because we are not building
 * PuTTY's own GUI / platform layer. Compiled as plain C, includes putty.h. */

#include "putty.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

const char *const appname = "ZosnTerm";

void modalfatalbox(const char *p, ...)
{
    va_list ap;
    fprintf(stderr, "ZosnTerm FATAL: ");
    va_start(ap, p);
    vfprintf(stderr, p, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(1);
}

/* We drive the terminal directly (no PuTTY backends), so the backend list is
 * empty. n_ui_backends is referenced by config.c's (unused) GUI dialog code,
 * which is dropped by --gc-sections. */
const struct BackendVtable *const backends[] = { NULL };
const size_t n_ui_backends = 0;

char *platform_default_s(const char *name)
{
    (void)name;
    return NULL;
}

bool platform_default_b(const char *name, bool def)
{
    (void)name;
    return def;
}

int platform_default_i(const char *name, int def)
{
    (void)name;
    return def;
}

FontSpec *platform_default_fontspec(const char *name)
{
    (void)name;
    return fontspec_new_default();
}

Filename *platform_default_filename(const char *name)
{
    (void)name;
    return filename_from_str("");
}

/* Called by the timing code when the next timer changes. We drive timers
 * ourselves from a Qt timer, so this is a no-op. */
void timer_change_notify(unsigned long next)
{
    (void)next;
}

/* Millisecond monotonic clock, used by run_timers() via GETTICKCOUNT. */
unsigned long getticks(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

/* storage.c's get_setting() reads X resources via x_get_default; we have no X
 * server in this embedded engine, so always report "no default". Declaration
 * lives in unix/platform.h (char *x_get_default(const char *key);). */
char *x_get_default(const char *key)
{
    (void)key;
    return NULL;
}

/* Build-version identifier, normally supplied by the platform main program.
 * Defined here as a static placeholder for the embedded engine. */
const char commitid[] = "ZosnTerm-embedded";

/* ---- Embedded-engine stubs for session-storage helpers we never use ----
 * We never load SSH host-certificate sessions in this serial terminal, so the
 * cert/host_ca machinery is stubbed. Declarations come from putty.h. */
struct host_ca *host_ca_new(void) { return NULL; }

void nonfatal(const char *fmt, ...)
{
    va_list ap;
    fprintf(stderr, "ZosnTerm nonfatal: ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}
