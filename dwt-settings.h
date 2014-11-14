/*
 * dwt-settings.h
 * Copyright (C) 2014 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef DWT_SETTINGS_H
#define DWT_SETTINGS_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct _DwtSettings DwtSettings;

DwtSettings* dwt_settings_get_instance (void);

G_END_DECLS

#endif /* !DWT_SETTINGS_H */
