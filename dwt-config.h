/*
 * dwt-config.h
 * Copyright (C) 2012-2013 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

/*
 * Default font name (uses fontconfig)
 */


/* #define DWT_DEFAULT_FONT "monospace 11" */

/*
 * Cursor colors.
 */

/* #define DWT_CURSOR_COLOR_FOCUSED   "#00cc00" */
/* #define DWT_CURSOR_COLOR_UNFOCUSED "#666666" */

/*
 * Whether to use a GtkHeaderBar for window titles. By default the
 * header bar will be hidden for maximized windows.
 * Note that this feature is experimental.
 */
#define DWT_USE_HEADER_BAR  0
#define DWT_HEADER_BAR_HIDE TRUE

/*
 * Global dwt accelerators (key bindings) are defined as keystroke-action
 * pairs. Keystrokes should be parseable by gtk_accelerator_parse().
 * Available actions:
 *
 *   Copy        - copy the selected text to the X11 clipboard
 *   Paste       - paste the X11 clipboard in the terminal
 *   Term        - launch new dwt terminal instance
 *   FontNormal  - reset font to initial size
 *   FontBigger  - increase font size
 *   FontSmaller - decrease font size
 */

{ AccelFontNormal,  GDK_KEY_0,     GDK_SUPER_MASK },
{ AccelFontBigger,  GDK_KEY_plus,  GDK_SUPER_MASK },
{ AccelFontSmaller, GDK_KEY_minus, GDK_SUPER_MASK },
{ AccelCopy,        GDK_KEY_C,     GDK_CONTROL_MASK | GDK_SHIFT_MASK },
{ AccelPaste,       GDK_KEY_P,     GDK_CONTROL_MASK | GDK_SHIFT_MASK },
{ AccelTerm,        GDK_KEY_N,     GDK_CONTROL_MASK | GDK_SHIFT_MASK },


