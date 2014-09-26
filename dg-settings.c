/*
 * dg-settings.c
 * Copyright (C) 2014 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "dg-settings.h"
#include "dg-util.h"
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


void
dg_settings__get_property__ (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  g_printerr ("%s, id=%u\n", __func__, prop_id);
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
      dg_lmem gchar* path = g_file_get_path (priv->settings_path);
      g_value_set_string (value, path);
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
                             DgSettingsPrivate *priv)
{
  g_assert (monitor == priv->monitor);

  /* TODO */
}


void
dg_settings__constructed__ (GObject *object)
{
  guint n_properties = 0;
  GParamSpec **props = g_object_class_list_properties (G_OBJECT_GET_CLASS (object),
                                                       &n_properties);
  g_printerr ("%s, %u properties\n", __func__, n_properties);

  guint n_settings = 0;
  for (guint i = 0; i < n_properties; i++) {
    g_printerr ("  - %s%s\n",
                g_param_spec_get_name (props[i]),
                (props[i]->flags & DG_SETTING__FLAG) ? " [setting]" : "");
    if (props[i]->flags & DG_SETTING__FLAG)
      n_properties++;
  }

  DgSettingsPrivate *priv = dg_settings_get_instance_private (DG_SETTINGS (object));

  g_assert (priv->settings_path);
  if (priv->monitor_enabled) {
    dg_lerr GError *error = NULL;
    priv->monitor = g_file_monitor_directory (priv->settings_path,
                                              G_FILE_MONITOR_NONE,
                                              NULL,
                                              &error);
    if (priv->monitor) {
      g_file_monitor_set_rate_limit (priv->monitor, 500);
      g_signal_connect (priv->monitor,
                        "changed",
                        G_CALLBACK (dg_settings_monitor_changed),
                        priv);
    } else {
      dg_lmem gchar* path = g_file_get_path (priv->settings_path);
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
  g_printerr ("%s\n", __func__);
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



