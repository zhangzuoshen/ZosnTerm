/* ZosnTerm PuTTY engine: front-end symbols the engine references at link
 * time but which we (not PuTTY) must supply, because we are not building
 * PuTTY's own GUI / platform layer. Compiled as plain C, includes putty.h.
 *
 * Platform policy: intentionally ZosnTerm-side only. PuTTY's platform
 * runtime is replaced by the vendored stubs/no-*.c modules, so this file
 * contains the ONLY platform-dependent code in the whole engine. */

/* On Windows, windows.h MUST be included before putty.h: unix/platform.h
 * defines BYTE / WCHAR as macros, which would corrupt the Win32 typedefs
 * ("typedef unsigned char unsigned char") if the order was reversed.
 * WIN32_LEAN_AND_MEAN / NOMINMAX keep Win32 macro pollution to a minimum. */
#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
   /* unix/platform.h defines CP_UTF8 as CS_UTF8; drop the Win32 65001
    * definition first so the compiler does not warn about redefinition. */
#  undef CP_UTF8
#endif

#include "putty.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifndef _WIN32
#  include <time.h>
#endif

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

/* Millisecond clock, used by run_timers() via GETTICKCOUNT (declared in
 * unix/platform.h, which we use on ALL platforms as a pure type/declaration
 * header). clock_gettime is POSIX; GetTickCount is Win32. */
unsigned long getticks(void)
{
#ifdef _WIN32
    return (unsigned long)GetTickCount();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
}

/* X-resource lookup, declared in unix/platform.h. Only unix/storage.c used
 * it; with stubs/no-storage.c nothing calls it, but keep the definition so
 * the symbol is satisfied on either platform. */
char *x_get_default(const char *key)
{
    (void)key;
    return NULL;
}

/* Build-version identifier, normally supplied by the platform main program.
 * Defined here as a static placeholder for the embedded engine. */
const char commitid[] = "ZosnTerm-embedded";

/* ---- Minimal platform stubs -------------------------------------------
 * We construct Conf directly and never touch disk-backed sessions, so the
 * POSIX-only unix/utils/{get_username,make_dir_path}.c are replaced here.
 * host_ca_new() and all settings storage APIs come from stubs/no-storage.c. */
char *get_username(void)
{
    return dupstr("user");
}

char *make_dir_path(const char *path, mode_t mode)
{
    (void)path;
    (void)mode;
    return NULL;
}

void nonfatal(const char *fmt, ...)
{
    va_list ap;
    fprintf(stderr, "ZosnTerm nonfatal: ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}
