/*
 * dirdb.h
 * Copyright (C) 2014 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef DIRDB_H
#define DIRDB_H

#include <glib-object.h>

G_BEGIN_DECLS

#define DIRDB_TYPE     (dirdb_get_type ())
#define DIRDB(obj)     (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIRDB_TYPE, DirDB))
#define IS_DIRDB(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIRDB_TYPE))

typedef struct _DirDBClass DirDBClass;
typedef struct _DirDB      DirDB;

typedef enum {
    DIRDB_SET,
    DIRDB_UNSET,
} DirDBEvent;

GType        dirdb_get_type      (void);
DirDB*       dirdb_new           (const gchar *base_path);
gchar*       dirdb_get_base_path (DirDB *dirdb);
gboolean     dirdb_read_boolean  (DirDB *dirdb, const gchar *name);
guint        dirdb_read_uint     (DirDB *dirdb, const gchar *name);
gchar*       dirdb_read_line     (DirDB *dirdb, const gchar *name);

G_END_DECLS

#endif /* !DIRDB_H */
