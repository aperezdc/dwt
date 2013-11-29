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

/* #define DWT_CURSOR_COLOR "#00cc00" */


/*
 * Global dwt accelerators (key bindings) are defined as keystroke-action
 * pairs. Keystrokes should be parseable by gtk_accelerator_parse().
 * Available actions:
 *
 *   Copy     - copy the selected text to the X11 clipboard
 *   Paste    - paste the X11 clipboard in the terminal
 */

{ AccelCopy,   GDK_KEY_C, GDK_CONTROL_MASK },
{ AccelPaste,  GDK_KEY_P, GDK_CONTROL_MASK },
{ AccelTerm,   GDK_KEY_N, GDK_CONTROL_MASK },

