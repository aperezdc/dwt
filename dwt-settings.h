/*
 * dwt-settings.h
 * Copyright (C) 2014 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef DWT_SETTINGS_H
#define DWT_SETTINGS_H

#include "dg-settings.h"

G_BEGIN_DECLS

DG_SETTINGS_CLASS_DECLARE (DwtSettings, dwt_settings)

DwtSettings* dwt_settings_get_instance (void);

G_END_DECLS

#endif /* !DWT_SETTINGS_H */
