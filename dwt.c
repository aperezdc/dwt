/*
 * dwt.c
 * Copyright (C) 2012 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include <gtk/gtk.h>

enum AccelAction
{
    AccelNoAction = 0,
    AccelPaste,
    AccelCopy,
    AccelTerm,
};

static struct
{
    enum AccelAction action;
    guint            accel_key;
    GdkModifierType  accel_mod;
} global_accels [] = {
#include "dwt-accels.h"
};

#include <vte/vte.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>

#define LENGTH_OF(a) (sizeof (a) / sizeof (a[0]))
#define CHECK_FLAGS(_v, _f) (((_v) & (_f)) == (_f))


static const gchar   *opt_workdir = ".";
static const gchar   *opt_command = NULL;
static const gchar   *opt_title   = "dwt";
static const gchar   *opt_font    = "terminus 11";
static       gboolean opt_bold    = FALSE;
static       gint     opt_scroll  = 1024;


static const GOptionEntry option_entries[] =
{
    {
        "command", 'e',
        G_OPTION_FLAG_IN_MAIN,
        G_OPTION_ARG_STRING,
        &opt_command,
        "Execute the argument to this option inside the terminal",
        "COMMAND",
    }, {
        "workdir", 'w',
        G_OPTION_FLAG_IN_MAIN,
        G_OPTION_ARG_STRING,
        &opt_workdir,
        "Set working directory before running the command/shell",
        "PATH",
    }, {
        "font", 'f',
        G_OPTION_FLAG_IN_MAIN,
        G_OPTION_ARG_STRING,
        &opt_font,
        "Font used by the terminal, in FontConfig syntax",
        "FONT",
    }, {
        "title", 't',
        G_OPTION_FLAG_IN_MAIN,
        G_OPTION_ARG_STRING,
        &opt_title,
        "Initial terminal window title",
        "TITLE",
    }, {
        "scrollback", 's',
        G_OPTION_FLAG_IN_MAIN,
        G_OPTION_ARG_INT,
        &opt_scroll,
        "Scrollback buffer size, in bytes (default 1024)",
        "BYTES"
    }, {
        "bold", 'b',
        G_OPTION_FLAG_IN_MAIN,
        G_OPTION_ARG_NONE,
        &opt_bold,
        "Allow using bold fonts",
        NULL,
    }, {
        NULL
    },
};


static gint
accel_index (enum AccelAction action)
{
    guint i;

    for (i = 0; i < LENGTH_OF (global_accels); i++) {
        if (global_accels[i].action == action) {
            return i;
        }
    }

    /* Not found */
    return -1;
}


/*
 * Set of colors as used by GNOME-Terminal for the “Linux” color scheme:
 * http://git.gnome.org/browse/gnome-terminal/tree/src/terminal-profile.c
 */
static const GdkColor color_palette[] =
{
    { 0, 0x0000, 0x0000, 0x0000 },
    { 0, 0xaaaa, 0x0000, 0x0000 },
    { 0, 0x0000, 0xaaaa, 0x0000 },
    { 0, 0xaaaa, 0x5555, 0x0000 },
    { 0, 0x0000, 0x0000, 0xaaaa },
    { 0, 0xaaaa, 0x0000, 0xaaaa },
    { 0, 0x0000, 0xaaaa, 0xaaaa },
    { 0, 0xaaaa, 0xaaaa, 0xaaaa },
    { 0, 0x5555, 0x5555, 0x5555 },
    { 0, 0xffff, 0x5555, 0x5555 },
    { 0, 0x5555, 0xffff, 0x5555 },
    { 0, 0xffff, 0xffff, 0x5555 },
    { 0, 0x5555, 0x5555, 0xffff },
    { 0, 0xffff, 0x5555, 0xffff },
    { 0, 0x5555, 0xffff, 0xffff },
    { 0, 0xffff, 0xffff, 0xffff },
};

/* Use light grey on black */
static const GdkColor color_bg = { 0, 0x0000, 0x0000, 0x0000 };
static const GdkColor color_fg = { 0, 0xdddd, 0xdddd, 0xdddd };

/* Regexp used to match URIs and allow clicking them */
static const gchar uri_regexp[] = "(ftp|http)s?://[-a-zA-Z0-9.?$%&/=_~#.,:;+]*";

/* Characters considered part of a word. Simplifies double-click selection */
static const gchar word_chars[] = "-A-Za-z0-9,./?%&#@_~";


static void
configure_term_widget (VteTerminal *vtterm)
{
    gint match_tag;

    g_assert (vtterm);
    g_assert (opt_font);

    vte_terminal_set_mouse_autohide      (vtterm, TRUE);
    vte_terminal_set_scroll_on_keystroke (vtterm, TRUE);
    vte_terminal_set_visible_bell        (vtterm, FALSE);
    vte_terminal_set_audible_bell        (vtterm, FALSE);
    vte_terminal_set_scroll_on_output    (vtterm, FALSE);
    vte_terminal_set_font_from_string    (vtterm, opt_font);
    vte_terminal_set_allow_bold          (vtterm, opt_bold);
    vte_terminal_set_scrollback_lines    (vtterm, opt_scroll);
    vte_terminal_set_word_chars          (vtterm, word_chars);
    vte_terminal_set_cursor_blink_mode   (vtterm, VTE_CURSOR_BLINK_OFF);
    vte_terminal_set_cursor_shape        (vtterm, VTE_CURSOR_SHAPE_BLOCK);
    vte_terminal_set_colors              (vtterm,
                                          &color_fg,
                                          &color_bg,
                                          color_palette,
                                          LENGTH_OF (color_palette));

    match_tag = vte_terminal_match_add_gregex (vtterm,
                                               g_regex_new (uri_regexp,
                                                            G_REGEX_CASELESS,
                                                            G_REGEX_MATCH_NOTEMPTY,
                                                            NULL),
                                               0);

    vte_terminal_match_set_cursor_type (vtterm, match_tag, GDK_HAND2);
}


static void
set_urgent (VteTerminal *vtterm, gpointer userdata)
{
    gtk_window_set_urgency_hint (GTK_WINDOW (userdata), TRUE);
}


static void
set_title (VteTerminal *vtterm, gpointer userdata)
{
    gtk_window_set_title (GTK_WINDOW (userdata),
                          vte_terminal_get_window_title (vtterm));
}


static char*
guess_shell (void)
{
    char *shell = getenv ("SHELL");
    if (!shell) {
        struct passwd *pw = getpwuid (getuid ());
        shell = (pw) ? pw->pw_shell : "/bin/sh";
    }
    /* Return a copy */
    return g_strdup (shell);
}


static gboolean
handle_key_press (GtkWidget *widget,
                  GdkEventKey *event,
                  gpointer userdata)
{
    g_assert (widget);
    g_assert (event);
    g_assert (userdata);
    g_assert (VTE_IS_TERMINAL (userdata));

    if (event->type != GDK_KEY_PRESS) {
        return FALSE;
    }

#define HANDLE_ACCEL(_action, _expr) \
    do { \
        static gint idx = -2; \
        if (idx == -2) idx = accel_index (_action); \
        if (idx >= 0 && event->keyval == global_accels[idx].accel_key && \
            CHECK_FLAGS (event->state, global_accels[idx].accel_mod)) { \
            _expr; \
            return TRUE; \
        } \
    } while (0)

    HANDLE_ACCEL (AccelPaste,
                  vte_terminal_paste_clipboard (VTE_TERMINAL (userdata)));

    HANDLE_ACCEL (AccelCopy,
                  vte_terminal_copy_clipboard (VTE_TERMINAL (userdata));
                  vte_terminal_copy_primary (VTE_TERMINAL (userdata)));

#undef HANDLE_ACCEL

    return FALSE;
}


static const gchar*
guess_browser (void)
{
    static gchar *browser = NULL;

    if (!browser) {
        if (g_getenv ("BROWSER")) {
            browser = g_strdup (g_getenv ("BROWSER"));
        }
        else {
            browser = g_find_program_in_path ("xdg-open");
            if (!browser) {
                browser = g_find_program_in_path ("gnome-open");
                if (!browser) {
                    browser = g_find_program_in_path ("exo-open");
                    if (!browser) {
                        browser = g_find_program_in_path ("firefox");
                    }
                }
            }
        }
    }

    return browser;
}


static gboolean
handle_mouse_press (VteTerminal *vtterm,
                    GdkEventButton *event,
                    gpointer userdata)
{
    gchar *match = NULL;
    glong col, row;
    gint match_tag;

    g_assert (vtterm);
    g_assert (event);

    if (event->type != GDK_BUTTON_PRESS)
        return FALSE;

    row = (glong) (event->y) / vte_terminal_get_char_height (vtterm);
    col = (glong) (event->x) / vte_terminal_get_char_width  (vtterm);

    if ((match = vte_terminal_match_check (vtterm, col, row, &match_tag)) != NULL) {
        if (event->button == 1 && CHECK_FLAGS (event->state, GDK_CONTROL_MASK)) {
            GError *error = NULL;
            gchar *cmdline[] = {
                (gchar*) guess_browser (),
                match,
                NULL
            };

            if (!cmdline[0]) {
                g_printerr ("Could not determine browser to use.\n");
            }
            else if (!g_spawn_async (NULL,
                                     cmdline,
                                     NULL,
                                     G_SPAWN_SEARCH_PATH,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &error))
            {
                g_printerr ("Could not launch browser: %s", error->message);
                g_error_free (error);
            }
        }
        g_free (match);
        return TRUE;
    }

    return FALSE;
}


int
main (int argc, char *argv[])
{
    GOptionContext *optctx = NULL;

    GtkWidget *window = NULL;
    GtkWidget *vtterm = NULL;
    GError    *gerror = NULL;

    gchar **command = NULL;
    gint    cmdlen = 0;

    optctx = g_option_context_new ("[-e command]");

    g_option_context_set_help_enabled (optctx, TRUE);
    g_option_context_add_main_entries (optctx, option_entries, NULL);
    g_option_context_add_group (optctx, gtk_get_option_group (TRUE));

    if (!g_option_context_parse (optctx, &argc, &argv, &gerror)) {
        g_printerr ("%s: could not parse command line: %s\n",
                    argv[0], gerror->message);
        g_error_free (gerror);
        exit (EXIT_FAILURE);
    }

    g_option_context_free (optctx);
    optctx = NULL;

    if (!opt_command)
        opt_command = guess_shell ();

    if (!g_shell_parse_argv (opt_command, &cmdlen, &command, &gerror)) {
        g_printerr ("%s: coult not parse command: %s\n",
                    argv[0], gerror->message);
        g_error_free (gerror);
        exit (EXIT_FAILURE);
    }

    gtk_init (&argc, &argv);

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_icon_name (GTK_WINDOW (window), "terminal");
    gtk_window_set_title (GTK_WINDOW (window), opt_title);

    vtterm = vte_terminal_new ();
    configure_term_widget (VTE_TERMINAL (vtterm));

    /*
     * Exit when the child process is exited, or when the window is closed.
     */
    g_signal_connect (G_OBJECT (window), "delete-event",
                      G_CALLBACK (gtk_main_quit),
                      NULL);

    g_signal_connect (G_OBJECT (vtterm), "child-exited",
                      G_CALLBACK (gtk_main_quit),
                      NULL);

    /*
     * Handle keyboard shortcuts.
     */
    g_signal_connect (G_OBJECT (window), "key-press-event",
                      G_CALLBACK (handle_key_press),
                      vtterm);

    /*
     * Handles clicks un URIs
     */
    g_signal_connect (G_OBJECT (vtterm), "button-press-event",
                      G_CALLBACK (handle_mouse_press),
                      NULL);

    /*
     * Transform terminal beeps in _URGENT hints for the window.
     */
    g_signal_connect (G_OBJECT (vtterm), "beep",
                      G_CALLBACK (set_urgent),
                      (gpointer) window);

    /*
     * Pick the title changes and propagate them to the window.
     */
    g_signal_connect (G_OBJECT (vtterm), "window-title-changed",
                      G_CALLBACK (set_title),
                      (gpointer) window);

    g_assert (opt_workdir);

    if (!vte_terminal_fork_command_full (VTE_TERMINAL (vtterm),
                                         VTE_PTY_DEFAULT,
                                         opt_workdir,
                                         command,
                                         NULL,
                                         G_SPAWN_SEARCH_PATH,
                                         NULL,
                                         NULL,
                                         NULL,
                                         &gerror))
    {
        g_printerr ("%s: could not spawn shell: %s\n",
                    argv[0], gerror->message);
        g_error_free (gerror);
        exit (EXIT_FAILURE);
    }

    gtk_container_add (GTK_CONTAINER (window), vtterm);
    gtk_widget_show_all (window);

    gtk_main ();
    return EXIT_SUCCESS;
}

