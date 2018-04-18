/*
 * dwt-settings.c
 * Copyright (C) 2014 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "dg-settings.h"

#ifndef DWT_DEFAULT_FONT
#define DWT_DEFAULT_FONT "monospace 11"
#endif /* !DWT_DEFAULT_FONT */


DG_SETTINGS_CLASS_DECLARE (DwtSettings, dwt_settings)
DG_SETTINGS_CLASS_DEFINE  (DwtSettings, dwt_settings)

DG_SETTINGS_BOOLEAN    ("allow-bold",
                        "Allow bold fonts",
                        "Whether to allow usage of bold fonts."
                        " Note that some type faces may not include"
                        " a bold variant, and a substitute font which"
                        " does not match well with the selected font"
                        " might be chosen, or the base font may be"
                        " emboldened to create a “fake bold” variant.",
                        TRUE);

DG_SETTINGS_BOOLEAN    ("mouse-autohide",
                        "Autohide mouse pointer",
                        "Whether to automatically hide the mouse cursor"
                        " on key press when the cursor is over a terminal"
                        " window. To show the cursor again, move the mouse.",
                        TRUE);

DG_SETTINGS_BOOLEAN    ("audible-bell",
                        "Audible terminal bell",
                        "Whether the terminal bell is audible (i.e. produces"
                        " sound.",
                        FALSE);

DG_SETTINGS_BOOLEAN    ("show-title",
                        "Show window titles",
                        "Show title bars for maximized terminal windows.",
                        FALSE);

DG_SETTINGS_BOOLEAN    ("update-title",
                        "Update window titles",
                        "Allow updating the titles of windows using the"
                        " same escape sequence used by XTerm.",
                        TRUE);

DG_SETTINGS_BOOLEAN    ("no-header-bar",
                        "Do not use header bars",
                        "Let the window manager decorate windows instead."
                        " of using client-side header bars.",
                        FALSE);

DG_SETTINGS_UINT_RANGE ("scrollback",
                        "Scrollback size",
                        "Number of lines saved as scrollback buffer.",
                        0, 0, 10000);

DG_SETTINGS_STRING     ("font",
                        "Font name",
                        "Name of the terminal font.",
                        DWT_DEFAULT_FONT);

DG_SETTINGS_STRING     ("command",
                        "Command to run",
                        "Command to run instead of the user shell, if"
                        " flag '-e' is not specified during invocation.",
                        NULL);

DG_SETTINGS_STRING     ("title",
                        "Default window title",
                        "Initial title to show on terminal windows, if not"
                        " changed using the XTerm title change escape"
                        " sequence.",
                        "dwt");

DG_SETTINGS_STRING     ("icon",
                        "Terminal window icon",
                        "Name of the icon used for terminal windows, or"
                        " path to the file containing the image to be used"
                        " as icon.",
                        "terminal");

DG_SETTINGS_STRING     ("cursor-color",
                        "Cursor color",
                        "Color of the cursor in active terminal windows,"
                        " in #RRGGBB hex format.",
                        "#AA0033");

DG_SETTINGS_STRING     ("background-color",
                        "Background color",
                        "Terminal background color. Overrides the color"
                        " defined in the active theme.",
                        NULL);

DG_SETTINGS_STRING     ("foreground-color",
                        "Foreground color",
                        "Terminal foreground color. Overrides the color"
                        " defined in the active theme.",
                        NULL);

DG_SETTINGS_STRING     ("theme",
                        "Color theme",
                        "Name of the built-in color theme to be used for the"
                        " foreground, background, and the 16 basic terminal"
                        " colors.",
                        NULL);

DG_SETTINGS_CLASS_END


static gpointer
dwt_settings_create (gpointer dummy)
{
    g_autofree char *path = g_build_filename (g_get_user_config_dir (),
                                              g_get_prgname (),
                                              NULL);
    return g_object_new (dwt_settings_get_type (),
                         "settings-path", path,
                         "settings-monitoring-enabled", TRUE,
                         NULL);
}


DwtSettings*
dwt_settings_get_instance (void)
{
    static GOnce instance_once = G_ONCE_INIT;
    return g_once (&instance_once, dwt_settings_create, NULL);
}
