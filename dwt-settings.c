/*
 * dwt-settings.c
 * Copyright (C) 2014 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "dwt-settings.h"
#include "dg-util.h"
#include "dirdb.h"

#ifndef DWT_DEFAULT_FONT
#define DWT_DEFAULT_FONT "monospace 11"
#endif /* !DWT_DEFAULT_FONT */


struct _DwtSettingsClass {
    GObjectClass parent_class;
};

struct _DwtSettings {
    GObject  parent;
    DirDB   *dirdb;

    gboolean allow_bold;
    gboolean show_title;
    gboolean no_header_bar;
    guint    scrollback;
    gchar   *font;
};

enum {
    PROP_0,
    PROP_ALLOW_BOLD,
    PROP_SHOW_TITLE,
    PROP_NO_HEADER_BAR,
    PROP_SCROLLBACK,
    PROP_FONT,
    PROP_LAST
};

G_DEFINE_TYPE (DwtSettings, dwt_settings, G_TYPE_OBJECT)


static GParamSpec *properties[PROP_LAST] = { NULL, };


static void
dwt_settings_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
    DwtSettings *settings = DWT_SETTINGS (object);

    switch (prop_id) {
        case PROP_ALLOW_BOLD:
            g_value_set_boolean (value, settings->allow_bold);
            break;
        case PROP_SHOW_TITLE:
            g_value_set_boolean (value, settings->show_title);
            break;
        case PROP_NO_HEADER_BAR:
            g_value_set_boolean (value, settings->no_header_bar);
            break;
        case PROP_SCROLLBACK:
            g_value_set_uint (value, settings->scrollback);
            break;
        case PROP_FONT:
            g_value_set_string (value, settings->font);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
dwt_settings_class_init (DwtSettingsClass *klass)
{
    GObjectClass *g_object_class = G_OBJECT_CLASS (klass);

    properties[PROP_ALLOW_BOLD] =
        g_param_spec_boolean ("allow-bold",
                              "Allow bold fonts",
                              "Whether to allow usage of bold fonts",
                              FALSE,
                              G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
    properties[PROP_SHOW_TITLE] =
        g_param_spec_boolean ("show-title",
                              "Show window titles",
                              "Show title bars on maximized terminal windows",
                              FALSE,
                              G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
    properties[PROP_NO_HEADER_BAR] =
        g_param_spec_boolean ("no-header-bar",
                              "Do not use header bars",
                              "Let the window manager decorate windows instead"
                              " of using client-side header bars",
                              FALSE,
                              G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
    properties[PROP_SCROLLBACK] =
        g_param_spec_uint ("scrollback",
                           "Scrollback size",
                           "Number of lines saved as scrollback buffer",
                           0, G_MAXINT, 0,
                           G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
    properties[PROP_FONT] =
        g_param_spec_string ("font",
                             "Font name",
                             "Name of the terminal font",
                             DWT_DEFAULT_FONT,
                             G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

    g_object_class->get_property = dwt_settings_get_property;
    g_object_class_install_properties (g_object_class,
                                       PROP_LAST,
                                       properties);
}


#define HANDLE_SIMPLE(_type, _idname, _prop)                                                             \
    static void handle_ ## _idname ## _changed (DirDB *dirdb, DirDBEvent event, DwtSettings *settings) { \
        g ## _type new_value;                                                                            \
        switch (event) {                                                                                 \
            case DIRDB_UNSET:                                                                            \
                new_value = g_value_get_ ## _type (g_param_spec_get_default_value (properties[_prop]));  \
                break;                                                                                   \
            case DIRDB_SET:                                                                              \
                new_value = dirdb_read_ ## _type (dirdb, g_param_spec_get_name (properties[_prop]));     \
                break;                                                                                   \
        }                                                                                                \
        if (new_value == settings-> _idname) return;                                                     \
        settings-> _idname = new_value;                                                                  \
        g_object_notify_by_pspec (G_OBJECT (settings), properties[_prop]);                               \
    }

#define HANDLE_STRING(_idname, _prop)                                                                           \
    static void handle_ ## _idname ## _changed (DirDB *dirdb, DirDBEvent event, DwtSettings *settings) {        \
        gchar *new_value;                                                                                       \
        switch (event) {                                                                                        \
            case DIRDB_UNSET:                                                                                   \
                new_value = g_strdup (g_value_get_string (g_param_spec_get_default_value (properties[_prop]))); \
                break;                                                                                          \
            case DIRDB_SET:                                                                                     \
                new_value = dirdb_read_line (dirdb, g_param_spec_get_name (properties[_prop]));                 \
                break;                                                                                          \
        }                                                                                                       \
        if (settings-> _idname && g_str_equal (new_value, settings-> _idname)) {                                \
            g_free (new_value);                                                                                 \
            return;                                                                                             \
        }                                                                                                       \
        g_free (settings-> _idname);                                                                            \
        settings-> _idname = new_value;                                                                         \
    }

HANDLE_SIMPLE (boolean, allow_bold,    PROP_ALLOW_BOLD   )
HANDLE_SIMPLE (boolean, show_title,    PROP_SHOW_TITLE   )
HANDLE_SIMPLE (boolean, no_header_bar, PROP_NO_HEADER_BAR)
HANDLE_SIMPLE (uint,    scrollback,    PROP_SCROLLBACK   )
HANDLE_STRING (         font,          PROP_FONT         )

#undef HANDLE_SIMPLE
#undef HANDLE_STRING


static void
dwt_settings_init (DwtSettings *settings)
{
    dg_lmem gchar *path = g_build_filename (g_get_user_config_dir (), "dwt", NULL);
    settings->dirdb = dirdb_new (path);

    g_signal_connect (settings->dirdb,
                      "changed::allow-bold",
                      G_CALLBACK (handle_allow_bold_changed),
                      settings);
    g_signal_connect (settings->dirdb,
                      "changed::show-title",
                      G_CALLBACK (handle_show_title_changed),
                      settings);
    g_signal_connect (settings->dirdb,
                      "changed::no-header-bar",
                      G_CALLBACK (handle_no_header_bar_changed),
                      settings);
    g_signal_connect (settings->dirdb,
                      "changed::scrollback",
                      G_CALLBACK (handle_scrollback_changed),
                      settings);
    g_signal_connect (settings->dirdb,
                      "changed::font",
                      G_CALLBACK (handle_font_changed),
                      settings);

    /*
     * Read initial settings.
     * FIXME: This is shabby code.
     */
    handle_allow_bold_changed    (settings->dirdb, DIRDB_SET, settings);
    handle_show_title_changed    (settings->dirdb, DIRDB_SET, settings);
    handle_no_header_bar_changed (settings->dirdb, DIRDB_SET, settings);
    handle_scrollback_changed    (settings->dirdb, DIRDB_SET, settings);
    handle_font_changed          (settings->dirdb, DIRDB_SET, settings);
}


static gpointer
dwt_settings_create (gpointer dummy)
{
    return g_object_new (DWT_SETTINGS_TYPE, NULL);
}


DwtSettings*
dwt_settings_get_instance (void)
{
    static GOnce instance_once = G_ONCE_INIT;
    return g_once(&instance_once, dwt_settings_create, NULL);
}


gboolean
dwt_settings_get_allow_bold (DwtSettings *settings)
{
    g_return_val_if_fail (DWT_IS_SETTINGS (settings), FALSE);
    return settings->allow_bold;
}

gboolean
dwt_settings_get_show_title (DwtSettings *settings)
{
    g_return_val_if_fail (DWT_IS_SETTINGS (settings), FALSE);
    return settings->show_title;
}

gboolean
dwt_settings_get_no_header_bar (DwtSettings *settings)
{
  g_return_val_if_fail (DWT_IS_SETTINGS (settings), FALSE);
  return settings->no_header_bar;
}

guint
dwt_settings_get_scrollback (DwtSettings *settings)
{
    g_return_val_if_fail (DWT_IS_SETTINGS (settings), 0);
    return settings->scrollback;
}

const gchar*
dwt_settings_get_font (DwtSettings *settings)
{
    g_return_val_if_fail (DWT_IS_SETTINGS (settings), NULL);
    return settings->font;
}
