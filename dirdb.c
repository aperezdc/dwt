/*
 * dirdb.c
 * Copyright (C) 2014 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "dirdb.h"
#include "dg-util.h"
#include <gio/gio.h>


struct _DirDBClass {
    GObjectClass parent_class;
};

struct _DirDB {
    GObject       parent;
    GFile        *base_path;
    GFileMonitor *monitor;
};

enum {
    PROP_0,
    PROP_BASE_PATH,
    PROP_LAST
};

enum {
    CHANGED,
    LAST_SIGNAL
};

G_DEFINE_TYPE (DirDB, dirdb, G_TYPE_OBJECT)

static guint signals[LAST_SIGNAL] = { 0, };
static GParamSpec *properties[PROP_LAST] = { NULL, };


static void
dirdb_set_property (GObject      *object,
                    guint         prop_id,
                    const GValue *value,
                    GParamSpec   *param_spec)
{
    DirDB *dirdb = DIRDB (object);

    switch (prop_id) {
        case PROP_BASE_PATH:
            g_assert (!dirdb->base_path);
            dirdb->base_path = g_file_new_for_path (g_value_get_string (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, param_spec);
    }
}


static void
dirdb_get_property (GObject    *object,
                    guint       prop_id,
                    GValue     *value,
                    GParamSpec *param_spec)
{
    DirDB *dirdb = DIRDB (object);

    switch (prop_id) {
        case PROP_BASE_PATH: {
            dg_lmem gchar *path = g_file_get_path (dirdb->base_path);
            g_value_set_string (value, path);
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, param_spec);
    }
}


static void
dirdb_finalize (GObject *object)
{
    DirDB *dirdb = DIRDB (object);
    g_object_unref (dirdb->monitor);
    g_object_unref (dirdb->base_path);
    G_OBJECT_CLASS (dirdb_parent_class)->finalize (object);
}


static void
dirdb_init (DirDB *dirdb)
{
}


static GQuark
dirdb_gquark_from_file (GFile *file)
{
    g_assert (file != NULL);
    dg_lmem gchar *basename = g_file_get_basename (file);
    return g_quark_from_string (basename);
}


static void
dirdb_monitor_changed (GFileMonitor     *monitor,
                       GFile            *file,
                       GFile            *other_file,
                       GFileMonitorEvent event,
                       DirDB            *dirdb)
{
    g_assert (monitor == dirdb->monitor);

    switch (event) {
        case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
        case G_FILE_MONITOR_EVENT_CREATED:
            g_signal_emit (dirdb,
                           CHANGED,
                           dirdb_gquark_from_file (file),
                           DIRDB_SET);
            break;

        case G_FILE_MONITOR_EVENT_MOVED:
            g_signal_emit (dirdb,
                           CHANGED,
                           dirdb_gquark_from_file (other_file),
                           DIRDB_SET);
            /* fall-through */
        case G_FILE_MONITOR_EVENT_DELETED:
            g_signal_emit (dirdb,
                           CHANGED,
                           dirdb_gquark_from_file (file),
                           DIRDB_UNSET);
            break;

        case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
            /* TODO: Emit when readability of the file changes. */
            break;

        case G_FILE_MONITOR_EVENT_CHANGED:
            /* This is ignored, CHANGES_DONE_HINT is used above. */
            break;

        case G_FILE_MONITOR_EVENT_PRE_UNMOUNT:
        case G_FILE_MONITOR_EVENT_UNMOUNTED: {
            dg_lmem gchar *path = g_file_get_path (dirdb->base_path);
            g_warning ("DirDB(%s): Location is about to be unmounted. "
                       "Expect unexpected behavior (pun intended).\n",
                       path);
            break;
        }
    }
}




static void
dirdb_constructed (GObject *object)
{
    DirDB *dirdb = DIRDB (object);

    g_assert (dirdb->base_path);
    dirdb->monitor = g_file_monitor_directory (dirdb->base_path,
                                               G_FILE_MONITOR_NONE,
                                               NULL,
                                               NULL);
    g_file_monitor_set_rate_limit (dirdb->monitor, 500);
    g_signal_connect (dirdb->monitor,
                      "changed",
                      G_CALLBACK (dirdb_monitor_changed),
                      dirdb);
}


static void
dirdb_class_init (DirDBClass *klass)
{
    GObjectClass *g_object_class = G_OBJECT_CLASS (klass);
    g_object_class->set_property = dirdb_set_property;
    g_object_class->get_property = dirdb_get_property;
    g_object_class->constructed  = dirdb_constructed;
    g_object_class->finalize     = dirdb_finalize;

    /**
     * DirDB:base-path:
     *
     * The base path of the #DirDB.
     */
    properties[PROP_BASE_PATH] =
        g_param_spec_string ("base-path",
                             "Base path",
                             "The base path of the directory database",
                             NULL,
                             G_PARAM_READWRITE |
                             G_PARAM_CONSTRUCT_ONLY |
                             G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (g_object_class,
                                       PROP_LAST,
                                       properties);

    /**
     * DirDB::changed:
     * @dirdb: the #DirDB
     * @change_type: a #DirDBEvent value
     *
     * This signal is emitted when a change is detected in the database
     * directory.
     */
    signals[CHANGED] = g_signal_new ("changed",
                                     G_TYPE_FROM_CLASS (g_object_class),
                                     G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                                     0,
                                     0,
                                     0,
                                     g_cclosure_marshal_VOID__UINT,
                                     G_TYPE_NONE,
                                     1,
                                     G_TYPE_UINT);
}


/**
 * dirdb_new:
 * @base_path: a string
 *
 * Creates a new #DirDB at the given @base_path.
 *
 * Returns: the newly created #DirDB
 */
DirDB*
dirdb_new (const gchar *base_path)
{
    g_return_val_if_fail (base_path, NULL);
    return g_object_new (DIRDB_TYPE, "base-path", base_path, NULL);
}


/**
 * dirdb_get_base_path:
 * @dirdb: a #DirDB
 *
 * Obtains the base path of the #DirDB.
 *
 * Returns: a string
 */
gchar*
dirdb_get_base_path (DirDB *dirdb)
{
    g_return_val_if_fail (IS_DIRDB (dirdb), NULL);
    return g_file_get_path (dirdb->base_path);
}


gboolean
dirdb_read_boolean  (DirDB       *dirdb,
                     const gchar *name)
{
    g_return_val_if_fail (IS_DIRDB (dirdb), FALSE);
    dg_lmem gchar *line = dirdb_read_line (dirdb, name);
    return line && g_ascii_strcasecmp ("false", line) != 0;
}

guint
dirdb_read_uint (DirDB       *dirdb,
                 const gchar *name)
{
    g_return_val_if_fail (IS_DIRDB (dirdb), 0);

    dg_lmem gchar *line = dirdb_read_line (dirdb, name);

    if (line == NULL)
        return G_MAXUINT;

    guint64 value = g_ascii_strtoull (line, NULL, 0);
    return (value >= G_MAXUINT) ? G_MAXUINT : value;
}

gchar*
dirdb_read_line (DirDB       *dirdb,
                 const gchar *name)
{
    g_return_val_if_fail (IS_DIRDB (dirdb), NULL);

    dg_lerr GError *error = NULL;
    dg_lobj GFile *file = g_file_get_child (dirdb->base_path, name);
    dg_lobj GInputStream *stream = G_INPUT_STREAM (g_file_read (file, NULL, &error));

    if (!stream && error)
        return NULL;

    dg_lobj GDataInputStream *datain = g_data_input_stream_new (stream);
    gsize read_length = 0;

    return g_data_input_stream_read_line_utf8 (datain,
                                               &read_length,
                                               NULL,
                                               &error);

}
