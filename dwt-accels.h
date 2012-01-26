/*
 * dwt-accels.h
 * Copyright (C) 2012 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 *
 * Global dwt accelerators (key bindings) are defined as keystroke-action
 * pairs. Keystrokes should be parseable by gtk_accelerator_parse().
 * Available actions:
 *
 *   Copy     - copy the selected text to the X11 clipboard
 *   Paste    - paste the X11 clipboard in the terminal
 */

{ AccelCopy,   GDK_KEY_C,  GDK_CONTROL_MASK },
{ AccelPaste,  GDK_KEY_P,  GDK_CONTROL_MASK },

