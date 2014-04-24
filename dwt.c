/*
 * dwt.c
 * Copyright (C) 2012-2013 Adrian Perez <aperez@igalia.com>
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
    AccelFontNormal,
    AccelFontBigger,
    AccelFontSmaller,
};

static struct
{
    enum AccelAction action;
    guint            accel_key;
    GdkModifierType  accel_mod;
} global_accels [] = {
#include "dwt-config.h"
};

#ifndef DWT_DEFAULT_FONT
#define DWT_DEFAULT_FONT "monospace 11"
#endif /* !DWT_DEFAULT_FONT */

#ifndef DWT_CURSOR_COLOR_FOCUSED
#define DWT_CURSOR_COLOR_FOCUSED "#00cc00"
#endif /* !DWT_CURSOR_COLOR_FOCUSED */

#ifndef DWT_CURSOR_COLOR_UNFOCUSED
#define DWT_CURSOR_COLOR_UNFOCUSED "666666"
#endif /* !DWT_CURSOR_COLOR_UNFOCUSED */

#include <vte/vte.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>

#define LENGTH_OF(a) (sizeof (a) / sizeof (a[0]))
#define CHECK_FLAGS(_v, _f) (((_v) & (_f)) == (_f))

#if DWT_USE_HEADER_BAR
# ifndef DWT_HEADER_BAR_HIDE
#  define DWT_HEADER_BAR_HIDE TRUE
# endif /* !DWT_USE_HEADER_BAR */
static       gboolean opt_hidebar = FALSE;
#endif /* DWT_USE_HEADER_BAR */

static const gchar   *opt_workdir = ".";
static const gchar   *opt_command = NULL;
static       gchar   *opt_title   = "dwt";
static       gchar   *opt_font    = DWT_DEFAULT_FONT;
static       gboolean opt_bold    = FALSE;
static       gint     opt_scroll  = 1024;


static const gchar osc_cursor_unfocused[] = "]12;" DWT_CURSOR_COLOR_UNFOCUSED "";
static const gchar osc_cursor_focused[]   = "]12;" DWT_CURSOR_COLOR_FOCUSED   "";


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
    },
#if DWT_USE_HEADER_BAR
    {
#if !DWT_HEADER_BAR_HIDE
        "no-"
#endif /* !DWT_HEADER_BAR_HIDE */
        "title-on-maximize", 'H',
        G_OPTION_FLAG_IN_MAIN,
        G_OPTION_ARG_NONE,
        &opt_hidebar,
#if DWT_HEADER_BAR_HIDE
        "Show"
#else /*! DWT_HEADER_BAR_HIDE */
        "Hide"
#endif /* DWT_HEADER_BAR_HIDE */
        " title bar when window is maximized.",
        NULL,
    },
#endif /* DWT_USE_HEADER_BAR */
    { NULL }
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

    vte_terminal_feed (vtterm,
                       osc_cursor_focused,
                       sizeof(osc_cursor_focused));
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


/*
 * FIXME: Somehow an empty (black) padding always remains even after setting
 * the sizing hints.
 */
static void
set_window_sizing_increment (VteTerminal *vtterm, GtkWindow *window)
{
    GdkGeometry geometry;
    geometry.height_inc = vte_terminal_get_char_height (vtterm);
    geometry.width_inc = vte_terminal_get_char_width (vtterm);

    GtkBorder padding;
    gtk_style_context_get_padding (gtk_widget_get_style_context (GTK_WIDGET (vtterm)),
                                   gtk_widget_get_state_flags (GTK_WIDGET (vtterm)),
                                   &padding);
    geometry.base_height = padding.top + padding.bottom;
    geometry.base_width = padding.left + padding.right;

    geometry.min_height = geometry.base_height + 3 * geometry.height_inc;
    geometry.min_width = geometry.base_width + 10 * geometry.width_inc;

    gtk_window_set_geometry_hints (window,
                                   GTK_WIDGET (window),
                                   &geometry,
                                   GDK_HINT_MIN_SIZE |
                                   GDK_HINT_BASE_SIZE |
                                   GDK_HINT_RESIZE_INC);
}


static void
font_size (VteTerminal *vtterm, GtkWindow *window, gint modifier)
{
    const PangoFontDescription *fontd = vte_terminal_get_font (vtterm);
    gint old_size = pango_font_description_get_size (fontd);
    PangoFontDescription *new_fontd;
    gint new_size;

    switch (modifier) {
      case 0:
        vte_terminal_set_font_from_string (vtterm, opt_font);
        break;

      case 1:
      case -1:
        new_size = old_size + modifier * PANGO_SCALE;
        new_fontd = pango_font_description_copy_static (fontd);
        pango_font_description_set_size (new_fontd, new_size);
        vte_terminal_set_font (vtterm, new_fontd);
        pango_font_description_free (new_fontd);
        break;

      default:
        g_printerr ("%s: invalid modifier '%i'", __func__, modifier);
    }

    if (new_size != old_size)
        set_window_sizing_increment (vtterm, window);
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


static void
spawn_terminal (VteTerminal *vtterm)
{
    const gchar *curdir_uri = vte_terminal_get_current_directory_uri(vtterm);
    const gchar *curdir_path = NULL;

    gchar *argv[] = {
        "dwt",
        "-t", opt_title,
        "-f", opt_font,
        opt_bold ? "-b" : NULL,
        NULL
    };

    /*
     * TODO: Use entries in /proc to get current directory of the child
     * shell and our own location by reading /proc/self/exe
     */

    if (curdir_uri && g_str_has_prefix (curdir_uri, "file://"))
        curdir_path = curdir_uri + 7;

    GPid pid;
    GError *error = NULL;
    g_spawn_async (curdir_path,
                   argv,
                   NULL,
                   G_SPAWN_SEARCH_PATH,
                   NULL,
                   NULL,
                   &pid,
                   &error);
}


static gboolean
handle_key_press (GtkWidget   *widget,
                  GdkEventKey *event,
                  gpointer     userdata)
{
    g_assert (VTE_IS_TERMINAL (widget));
    g_assert (GTK_IS_WINDOW (userdata));
    g_assert (event);

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
                  vte_terminal_paste_clipboard (VTE_TERMINAL (widget)));

    HANDLE_ACCEL (AccelCopy,
                  vte_terminal_copy_clipboard (VTE_TERMINAL (widget));
                  vte_terminal_copy_primary (VTE_TERMINAL (widget)));

    HANDLE_ACCEL (AccelTerm,
                  spawn_terminal (VTE_TERMINAL (widget)));

    HANDLE_ACCEL (AccelFontNormal,
                  font_size (VTE_TERMINAL (widget), GTK_WINDOW (userdata),  0));
    HANDLE_ACCEL (AccelFontBigger,
                  font_size (VTE_TERMINAL (widget), GTK_WINDOW (userdata), +1));
    HANDLE_ACCEL (AccelFontSmaller,
                  font_size (VTE_TERMINAL (widget), GTK_WINDOW (userdata), -1));
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


#if DWT_USE_HEADER_BAR
static void
set_header_bar_title (VteTerminal *vtterm, gpointer userdata)
{
    gtk_label_set_text (GTK_LABEL (userdata),
                        vte_terminal_get_window_title (vtterm));
}


static gboolean
header_bar_close_released (GtkWidget *widget,
                           GdkEvent  *event,
                           gpointer   userdata)
{
    gtk_main_quit ();
    return TRUE;
}


static void
toggle_window_maximized (GObject    *object,
                         GParamSpec *pspec,
                         gpointer    userdata)
{
    GtkWidget *header = GTK_WIDGET (userdata);

    if (gtk_window_is_maximized (GTK_WINDOW (object))) {
        gtk_widget_hide (header);
    }
    else {
        gtk_widget_show (header);
    }
}

static void
setup_header_bar (GtkWidget *window, GtkWidget *vtterm)
{
    GtkWidget *header = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *widget = gtk_label_new (opt_title);
    GtkWidget *bin = gtk_alignment_new (0.5, 0.5, 1, 1);

    gtk_alignment_set_padding (GTK_ALIGNMENT (bin), 3, 4, 12, 12);
    gtk_container_add (GTK_CONTAINER (bin), widget);

    /*
     * Pick the title changes and propagate them to the title label.
     */
    g_signal_connect (G_OBJECT (vtterm), "window-title-changed",
                      G_CALLBACK (set_header_bar_title),
                      (gpointer) widget);

    gtk_box_pack_start (GTK_BOX (header), bin, TRUE, TRUE, 0);

    /* Close button */
    widget = gtk_image_new_from_icon_name ("window-close-symbolic",
                                           GTK_ICON_SIZE_BUTTON);
    gtk_widget_add_events (widget,
                           GDK_BUTTON_PRESS_MASK |
                           GDK_BUTTON_RELEASE_MASK |
                           GDK_ENTER_NOTIFY_MASK |
                           GDK_LEAVE_NOTIFY_MASK);
    gtk_widget_set_margin_start (widget, 5);
    gtk_widget_set_margin_end (widget, 5);

    bin = gtk_event_box_new ();
    gtk_container_add (GTK_CONTAINER (bin), widget);
    g_signal_connect (G_OBJECT (bin), "button-release-event",
                      G_CALLBACK (header_bar_close_released), NULL);
    gtk_box_pack_end (GTK_BOX (header), bin, FALSE, FALSE, 0);

    gtk_window_set_titlebar (GTK_WINDOW (window), header);

    /*
     * Hide the header bar when the window is maximized.
     */
#if DWT_HEADER_BAR_HIDE
    if (!opt_hidebar)
#else /* !DWT_HEADER_BAR_HIDE */
    if (opt_hidebar)
#endif /* DWT_HEADER_BAR_HIDE */
    {
        g_signal_connect (G_OBJECT (window), "notify::is-maximized",
                          G_CALLBACK (toggle_window_maximized),
                          header);
    }
}
#endif /* DWT_USE_HEADER_BAR */


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

    /* Prefer a dark theme variant, if available */
    g_object_set(gtk_settings_get_default(),
                 "gtk-application-prefer-dark-theme",
                 TRUE, NULL);

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_icon_name (GTK_WINDOW (window), "terminal");
    gtk_window_set_title (GTK_WINDOW (window), opt_title);
    gtk_window_set_has_resize_grip (GTK_WINDOW (window), FALSE);
    gtk_window_set_hide_titlebar_when_maximized (GTK_WINDOW (window), TRUE);

    vtterm = vte_terminal_new ();
    configure_term_widget (VTE_TERMINAL (vtterm));
    set_window_sizing_increment (VTE_TERMINAL (vtterm), GTK_WINDOW (window));

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
    g_signal_connect (G_OBJECT (vtterm), "key-press-event",
                      G_CALLBACK (handle_key_press),
                      window);

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

#if DWT_USE_HEADER_BAR
    setup_header_bar (window, vtterm);
#endif /* DWT_USE_HEADER_BAR */

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

