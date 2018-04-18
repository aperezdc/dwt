/*
 * dg-settings.c
 * Copyright (C) 2014 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "dg-settings.h"
#include <gio/gio.h>


enum {
  PROP_0,
  PROP_SETTINGS_PATH,
  PROP_SETTINGS_MONITORING_ENABLED,
  PROP_N
};

typedef struct _DgSettingsPrivate DgSettingsPrivate;

struct _DgSettingsPrivate {
  GFile        *settings_path;
  gboolean      monitor_enabled;
  GFileMonitor *monitor;
  GValue      **values;
};

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (DgSettings, dg_settings, G_TYPE_OBJECT)


static GParamSpec *properties[PROP_N] = { NULL, };


static gchar*
read_line (GFile *file)
{
    g_assert (file != NULL);

    g_autoptr(GError) error = NULL;
    g_autoptr(GInputStream) stream = G_INPUT_STREAM (g_file_read (file, NULL, &error));

    if (!stream && error)
        return NULL;

    g_autoptr(GDataInputStream) datain = g_data_input_stream_new (stream);
    gsize read_length = 0;

    return g_data_input_stream_read_line_utf8 (datain,
                                               &read_length,
                                               NULL,
                                               &error);
}


static void
read_boolean (GFile *file, GValue *value)
{
    g_autofree char *line = read_line (file);
    if (line == NULL)
        return;

    g_value_set_boolean (value, g_ascii_strcasecmp ("false", line) != 0);
}


static void
read_uint (GFile *file, GValue *value)
{
    g_autofree char *line = read_line (file);
    if (line == NULL)
        return;

    guint64 uint_value = g_ascii_strtoull (line, NULL, 0);
    if (uint_value <= G_MAXUINT)
        g_value_set_uint (value, uint_value);
}


void
dg_settings__get_property__ (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
    DgSettingsPrivate *priv = dg_settings_get_instance_private (DG_SETTINGS (object));

    g_autoptr(GFile) setting_file = g_file_get_child (priv->settings_path,
                                                      g_param_spec_get_name (pspec));

    /* Use the default value when the file does not exist */
    g_value_copy (g_param_spec_get_default_value (pspec), value);
    if (!g_file_query_exists (setting_file, NULL))
        return;

    /* Try to read the value from the file, converting as appropriate. */
    switch (G_PARAM_SPEC_VALUE_TYPE (pspec)) {
        case G_TYPE_BOOLEAN:
            read_boolean (setting_file, value);
            break;
        case G_TYPE_UINT:
            read_uint (setting_file, value);
            break;
        case G_TYPE_STRING:
            g_value_take_string (value, read_line (setting_file));
            break;
    }
}


static void
write_line (GFile       *file,
            GParamSpec  *pspec,
            const gchar *line)
{
    g_assert (file != NULL);
    g_assert (pspec != NULL);
    g_assert (line != NULL);

    /* TODO: Handle errors */
    g_autoptr(GOutputStream) stream =
        G_OUTPUT_STREAM (g_file_open_readwrite (file, NULL, NULL));
    if (stream == NULL)
        return;

    gsize bytes_written;
    g_output_stream_printf (stream,
                            &bytes_written,
                            NULL,
                            NULL,
                            "%s\n\n%s\n",
                            line,
                            g_param_spec_get_blurb (pspec));

    g_output_stream_close (stream,
                           NULL,
                           NULL);
}


void
dg_settings__set_property__ (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
    DgSettingsPrivate *priv = dg_settings_get_instance_private (DG_SETTINGS (object));

    g_autoptr(GFile) setting_file = g_file_get_child (priv->settings_path,
                                                      g_param_spec_get_name (pspec));

    switch (G_PARAM_SPEC_VALUE_TYPE (pspec)) {
        case G_TYPE_BOOLEAN:
            write_line (setting_file, pspec,
                        g_value_get_boolean (value) ? "true" : "false");
            break;
    }
}


static void
dg_settings_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  DgSettingsPrivate *priv = dg_settings_get_instance_private (DG_SETTINGS (object));

  switch (prop_id) {
    case PROP_SETTINGS_PATH: {
      g_value_take_string (value, g_file_get_path (priv->settings_path));
      break;
    }
    case PROP_SETTINGS_MONITORING_ENABLED:
      g_value_set_boolean (value, priv->monitor_enabled);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}


static void
dg_settings_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  DgSettingsPrivate *priv = dg_settings_get_instance_private (DG_SETTINGS (object));

  switch (prop_id) {
    case PROP_SETTINGS_PATH:
      g_assert (!priv->settings_path);
      priv->settings_path = g_file_new_for_path (g_value_get_string (value));
      break;
    case PROP_SETTINGS_MONITORING_ENABLED:
      priv->monitor_enabled = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}


static void
dg_settings_finalize (GObject *object)
{
  DgSettingsPrivate *priv = dg_settings_get_instance_private (DG_SETTINGS (object));
  if (priv->monitor_enabled)
      g_object_unref (priv->monitor);
  g_object_unref (priv->settings_path);
  G_OBJECT_CLASS (dg_settings_parent_class)->finalize (object);
}


static void
dg_settings_init (DgSettings *settings)
{
}


static void
dg_settings_monitor_changed (GFileMonitor      *monitor,
                             GFile             *file,
                             GFile             *other_file,
                             GFileMonitorEvent  event,
                             DgSettings        *settings)
{
    switch (event) {
        case G_FILE_MONITOR_EVENT_CHANGED:
        case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
        case G_FILE_MONITOR_EVENT_PRE_UNMOUNT:
        case G_FILE_MONITOR_EVENT_UNMOUNTED:
        case G_FILE_MONITOR_EVENT_MOVED:
        case G_FILE_MONITOR_EVENT_RENAMED:
        case G_FILE_MONITOR_EVENT_MOVED_IN:
        case G_FILE_MONITOR_EVENT_MOVED_OUT:
        case G_FILE_MONITOR_EVENT_CREATED:
            return;

        case G_FILE_MONITOR_EVENT_DELETED:
        case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT: {
            g_autofree char *filename = g_file_get_basename (file);
            GParamSpec *pspec =
                g_object_class_find_property (G_OBJECT_GET_CLASS (settings),
                                              filename);
            if (pspec) g_object_notify_by_pspec (G_OBJECT (settings), pspec);
        }
    }
}


void
dg_settings__constructed__ (GObject *object)
{
  DgSettingsPrivate *priv = dg_settings_get_instance_private (DG_SETTINGS (object));

  g_assert (priv->settings_path);
  if (priv->monitor_enabled) {
    g_autoptr(GError) error = NULL;
    priv->monitor = g_file_monitor_directory (priv->settings_path,
                                              G_FILE_MONITOR_NONE,
                                              NULL,
                                              &error);
    if (priv->monitor) {
      g_file_monitor_set_rate_limit (priv->monitor, 500);
      g_signal_connect (priv->monitor,
                        "changed",
                        G_CALLBACK (dg_settings_monitor_changed),
                        object);
    } else {
      g_autofree char *path = g_file_get_path (priv->settings_path);
      g_warning ("DgSettings: cannot initialize monitoring for directory '%s'\n", path);
      if (error) g_warning ("DgSettings: %s\n", error->message);

      g_warning ("DgSettings: trying to continue anyway with monitoring disabled\n");
      priv->monitor_enabled = FALSE;
    }
  }
}


static void
dg_settings_class_init (DgSettingsClass *klass)
{
  GObjectClass *g_object_class = G_OBJECT_CLASS (klass);
  g_object_class->get_property = dg_settings_get_property;
  g_object_class->set_property = dg_settings_set_property;
  g_object_class->finalize     = dg_settings_finalize;

  properties[PROP_SETTINGS_PATH] =
    g_param_spec_string ("settings-path",
                         "Settings path",
                         "Directory where the configuration files reside",
                         NULL,
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_STATIC_STRINGS);

  properties[PROP_SETTINGS_MONITORING_ENABLED] =
    g_param_spec_boolean ("settings-monitoring-enabled",
                          "Settings monitoring enabled",
                          "Whether changes to files backing up settings will be monitored",
                          FALSE,
                          G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (g_object_class, PROP_N, properties);
}



