/*
 * dg-util.h
 * Copyright (C) 2014 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef DG_UTIL_H
#define DG_UTIL_H

#ifdef __GNUC__
# define dg_lmem __attribute__ ((cleanup (dg_lmem_cleanup)))
# define dg_lobj __attribute__ ((cleanup (dg_lobj_cleanup)))
# define dg_lerr __attribute__ ((cleanup (dg_lerr_cleanup)))
# include <glib-object.h>

static inline void
dg_lmem_cleanup (void *ptr)
{
    gpointer **location = ptr;
    if (location && *location) {
        g_free (*location);
        *location = NULL;
    }
}

static inline void
dg_lobj_cleanup (void *ptr)
{
    GObject **location = ptr;
    if (location && *location) {
        g_object_unref (*location);
        *location = NULL;
    }
}

static inline void
dg_lerr_cleanup (void *ptr)
{
    GError **location = ptr;
    if (location && *location) {
        g_error_free (*location);
        *location = NULL;
    }
}

#else
# define dg_lmem - GCC or Clang is needed for dg_lmem to work
# define dg_lobj - GCC or Clang is needed for dg_lobj to work
# define dg_lerr - GCC or Clang is needed for dg_lerr to work
#endif

#define DG_LENGTH_OF(_array) \
    (sizeof (_array) / sizeof (_array)[0])

#endif /* !DG_UTIL_H */
