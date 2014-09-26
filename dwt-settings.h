/*
 * dwt-settings.h
 * Copyright (C) 2014 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef DWT_SETTINGS_H
#define DWT_SETTINGS_H

#include <glib-object.h>

G_BEGIN_DECLS

#define DWT_SETTINGS_TYPE     (dwt_settings_get_type ())
#define DWT_SETTINGS(obj)     (G_TYPE_CHECK_INSTANCE_CAST ((obj), DWT_SETTINGS_TYPE, DwtSettings))
#define DWT_IS_SETTINGS(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DWT_SETTINGS_TYPE))

typedef struct _DwtSettingsClass DwtSettingsClass;
typedef struct _DwtSettings      DwtSettings;

GType        dwt_settings_get_type          (void);
DwtSettings* dwt_settings_get_instance      (void);

gboolean     dwt_settings_get_allow_bold    (DwtSettings *settings);
gboolean     dwt_settings_get_show_title    (DwtSettings *settings);
gboolean     dwt_settings_get_no_header_bar (DwtSettings *settings);
guint        dwt_settings_get_scrollback    (DwtSettings *settings);
const gchar* dwt_settings_get_font          (DwtSettings *settings);

G_END_DECLS

#endif /* !DWT_SETTINGS_H */
