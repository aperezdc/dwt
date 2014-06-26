/*
 * dwt.c
 * Copyright (C) 2012-2014 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include <gtk/gtk.h>

#ifndef DWT_DEFAULT_FONT
#define DWT_DEFAULT_FONT "monospace 11"
#endif /* !DWT_DEFAULT_FONT */

#ifndef DWT_CURSOR_COLOR_FOCUSED
#define DWT_CURSOR_COLOR_FOCUSED "#00cc00"
#endif /* !DWT_CURSOR_COLOR_FOCUSED */

#ifndef DWT_CURSOR_COLOR_UNFOCUSED
#define DWT_CURSOR_COLOR_UNFOCUSED "#999999"
#endif /* !DWT_CURSOR_COLOR_UNFOCUSED */

#ifndef DWT_USE_POPOVER
#define DWT_USE_POPOVER FALSE
#endif /* !DWT_USE_POPOVER */

#ifndef DWT_USE_HEADER_BAR
#define DWT_USE_HEADER_BAR FALSE
#endif /* !DWT_USE_HEADER_BAR */

#ifndef DWT_USE_OVERLAY
#define DWT_USE_OVERLAY FALSE
#endif /* !DWT_USE_OVERLAY */

#define DWT_GRESOURCE(name)  ("/org/perezdecastro/dwt/" name)

#include <vte/vte.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>

#define CHECK_FLAGS(_v, _f) (((_v) & (_f)) == (_f))

static       gboolean opt_showbar = FALSE;
static const gchar   *opt_workdir = ".";
static const gchar   *opt_command = NULL;
static       gchar   *opt_title   = "dwt";
static       gchar   *opt_font    = DWT_DEFAULT_FONT;
static       gboolean opt_bold    = FALSE;
static       gint     opt_scroll  = 1024;


static const gchar osc_cursor_unfocused[] = "]12;" DWT_CURSOR_COLOR_UNFOCUSED "";
static const gchar osc_cursor_focused[]   = "]12;" DWT_CURSOR_COLOR_FOCUSED   "";

/* Last matched text piece. */
static gchar *last_match_text = NULL;


/* Forward declarations. */
static GtkWidget*
create_new_window (GtkApplication *application, const gchar *command);


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
        "title-on-maximize", 'H',
        G_OPTION_FLAG_IN_MAIN,
        G_OPTION_ARG_NONE,
        &opt_showbar,
        "Always show title bar when window is maximized.",
        NULL,
    },
    { NULL }
};


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
    g_assert (opt_font);

    vte_terminal_set_rewrap_on_resize    (vtterm, TRUE);
    vte_terminal_set_scroll_on_keystroke (vtterm, TRUE);
    vte_terminal_set_mouse_autohide      (vtterm, FALSE);
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
                                          G_N_ELEMENTS (color_palette));

    gint match_tag =
        vte_terminal_match_add_gregex (vtterm,
                                       g_regex_new (uri_regexp,
                                                    G_REGEX_CASELESS,
                                                    G_REGEX_MATCH_NOTEMPTY,
                                                    NULL),
                                       0);
    vte_terminal_match_set_cursor_type (vtterm, match_tag, GDK_HAND2);
}


static void
term_beeped (VteTerminal *vtterm, gpointer userdata)
{
    /*
     * Only set the _URGENT hint when the window is not focused. If the
     * window is focused, the user is likely to notice what is happening
     * without the need to call for attention.
     */
    if (!gtk_window_has_toplevel_focus (GTK_WINDOW (userdata)))
        gtk_window_set_urgency_hint (GTK_WINDOW (userdata), TRUE);
}


/*
 * FIXME: A black border still remains when the size hints are set.
 */
static void
term_char_size_changed (VteTerminal *vtterm,
                        guint        width,
                        guint        height,
                        gpointer     userdata)
{
    GdkGeometry geometry;
    geometry.height_inc = height;
    geometry.width_inc = width;

    GtkBorder padding;
    gtk_style_context_get_padding (gtk_widget_get_style_context (GTK_WIDGET (vtterm)),
                                   gtk_widget_get_state_flags (GTK_WIDGET (vtterm)),
                                   &padding);
    geometry.base_height = padding.top + padding.bottom;
    geometry.base_width = padding.left + padding.right;

    geometry.min_height = geometry.base_height + 3 * geometry.height_inc;
    geometry.min_width = geometry.base_width + 10 * geometry.width_inc;

    gtk_window_set_geometry_hints (GTK_WINDOW (userdata),
                                   GTK_WIDGET (vtterm),
                                   &geometry,
                                   GDK_HINT_MIN_SIZE |
                                   GDK_HINT_BASE_SIZE |
                                   GDK_HINT_RESIZE_INC);
    gtk_widget_queue_resize (GTK_WIDGET (vtterm));
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


#if DWT_USE_POPOVER
static gboolean
popover_idle_closed_tick (gpointer userdata)
{
    gtk_widget_grab_focus (GTK_WIDGET (userdata));
    return FALSE; /* Do no re-arm (run once) */
}

static void
popover_closed (GtkPopover  *popover,
                VteTerminal *vtterm)
{
    /* XXX: Grabbing the focus right away does not work, must do later. */
    g_idle_add (popover_idle_closed_tick, vtterm);
}

static GtkWidget*
setup_popover (VteTerminal *vtterm)
{
    GtkWidget *popover = gtk_popover_new (GTK_WIDGET (vtterm));
    g_signal_connect (G_OBJECT (popover), "closed",
                      G_CALLBACK (popover_closed), vtterm);

    GtkBuilder *builder = gtk_builder_new_from_resource (DWT_GRESOURCE ("menus.xml"));
    gtk_popover_bind_model (GTK_POPOVER (popover),
                            G_MENU_MODEL (gtk_builder_get_object (builder, "popover-menu")),
                            NULL);
    g_object_unref (builder);
    return popover;
}
#else /* !DWT_USE_POPOVER */
# define setup_popover(_vtterm) NULL
#endif /* DWT_USE_POPOVER */


static gboolean
term_mouse_button_released (VteTerminal    *vtterm,
                            GdkEventButton *event,
                            gpointer        userdata)
{
    g_free (last_match_text);
    last_match_text = NULL;

    glong row = (glong) (event->y) / vte_terminal_get_char_height (vtterm);
    glong col = (glong) (event->x) / vte_terminal_get_char_width  (vtterm);

    gint match_tag;
    gchar* match = vte_terminal_match_check (vtterm, col, row, &match_tag);

    if (match && event->button == 1 && CHECK_FLAGS (event->state, GDK_CONTROL_MASK)) {
        GError *gerror = NULL;
        if (!gtk_show_uri (NULL, match, event->time, &gerror)) {
            g_printerr ("Could not open URL: %s\n", gerror->message);
            g_error_free (gerror);
        }
        g_free (match);
        return TRUE;
    }


#if DWT_USE_POPOVER
    if (event->button == 3 && userdata != NULL) {
        GdkRectangle rect;
        rect.height = vte_terminal_get_char_height (vtterm);
        rect.width = vte_terminal_get_char_width (vtterm);
        rect.y = rect.height * row;
        rect.x = rect.width * col;
        gtk_popover_set_pointing_to (GTK_POPOVER (userdata), &rect);

        GActionMap *actions = G_ACTION_MAP (gtk_widget_get_ancestor (GTK_WIDGET (vtterm),
                                                                     GTK_TYPE_WINDOW));
        g_simple_action_set_enabled (G_SIMPLE_ACTION (g_action_map_lookup_action (actions,
                                                                                  "copy")),
                                    vte_terminal_get_has_selection (vtterm));
        g_simple_action_set_enabled (G_SIMPLE_ACTION (g_action_map_lookup_action (actions,
                                                                                  "open-url")),
                                     match != NULL);
        g_simple_action_set_enabled (G_SIMPLE_ACTION (g_action_map_lookup_action (actions,
                                                                                  "copy-url")),
                                     match != NULL);
        if (match) {
            last_match_text = match;
            match = NULL;
        }

        gtk_widget_show_all (GTK_WIDGET (userdata));

        return TRUE;
    }
#endif /* DWT_USE_POPOVER */

    return FALSE;
}


#if DWT_USE_HEADER_BAR || DWT_USE_OVERLAY
static gboolean
beeped_revealer_timeout (gpointer userdata)
{
    gtk_revealer_set_reveal_child (GTK_REVEALER (userdata), FALSE);
    return FALSE; /* Do no re-arm (run once) */
}
#endif /* DWT_USE_HEADER_BAR || DWT_USE_OVERLAY */

#if DWT_USE_HEADER_BAR
static void
header_bar_term_beeped (VteTerminal *vtterm,
                        GtkRevealer *revealer)
{
    /* If already shown, do nothing. */
    if (gtk_revealer_get_reveal_child (revealer))
        return;

    GtkWindow *window = GTK_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (vtterm),
                                                             GTK_TYPE_WINDOW));
    if (!gtk_window_has_toplevel_focus (window))
        return;

    gtk_revealer_set_reveal_child (revealer, TRUE);
    g_timeout_add_seconds (2, beeped_revealer_timeout, revealer);
}

static void
setup_header_bar (GtkWidget   *window,
                  VteTerminal *vtterm)
{
    /*
     * Using the default GtkHeaderBar title/subtitle widget makes the bar
     * too thick to look nice for a terminal, so set a custom widget.
     */
    GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);

    GtkWidget *label = gtk_label_new (opt_title);
    g_object_bind_property (G_OBJECT (vtterm), "window-title",
                            G_OBJECT (label), "label",
                            G_BINDING_DEFAULT);

    gtk_widget_set_margin_top (label, 5);
    gtk_widget_set_margin_bottom (label, 5);
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    gtk_window_set_titlebar (GTK_WINDOW (window), hbox);

    /* Hide the header bar when the window is maximized. */
    if (!opt_showbar) {
        g_object_bind_property (G_OBJECT (window), "is-maximized",
                                G_OBJECT (hbox), "visible",
                                G_BINDING_INVERT_BOOLEAN);
    }
}
#endif /* DWT_USE_HEADER_BAR */


static void
window_has_toplevel_focus_notified (GObject    *object,
                                    GParamSpec *pspec,
                                    gpointer    userdata)
{
    if (gtk_window_has_toplevel_focus (GTK_WINDOW (object))) {
        vte_terminal_feed (VTE_TERMINAL (userdata),
                           osc_cursor_focused,
                           sizeof (osc_cursor_focused));
        /* Clear the _URGENT hint when the window gets activated. */
        gtk_window_set_urgency_hint (GTK_WINDOW (object), FALSE);
    } else {
        vte_terminal_feed (VTE_TERMINAL (userdata),
                           osc_cursor_unfocused,
                           sizeof (osc_cursor_unfocused));
    }
}


static void
term_child_exited (VteTerminal *vtterm,
                   gpointer     userdata)
{
    /*
     * Destroy the window when the terminal child is exited. Note that this
     * will fire the "delete-event" signal, and its handler already takes
     * care of deregistering the window in the GtkApplication.
     */
    gtk_widget_destroy (GTK_WIDGET (userdata));
}


static VteTerminal*
window_get_term_widget (GtkWindow *window)
{
    GtkWidget *widget = gtk_bin_get_child (GTK_BIN (window));
    if (GTK_IS_BIN (widget))
        widget = gtk_bin_get_child (GTK_BIN (widget));
    return VTE_TERMINAL (widget);
}


static void
font_size_action_ativated (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       userdata)
{
    VteTerminal *vtterm = window_get_term_widget (GTK_WINDOW (userdata));
    const gint modifier = g_variant_get_int32 (parameter);

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
        return;
    }
}


static void
paste_action_activated (GSimpleAction *action,
                        GVariant      *parameter,
                        gpointer       userdata)
{
    vte_terminal_paste_clipboard (window_get_term_widget (GTK_WINDOW (userdata)));
}


static void
copy_action_activated (GSimpleAction *action,
                       GVariant      *parameter,
                       gpointer       userdata)
{
    VteTerminal *vtterm = window_get_term_widget (GTK_WINDOW (userdata));
    vte_terminal_copy_clipboard (vtterm);
    vte_terminal_copy_primary (vtterm);
}


static void
new_terminal_action_activated (GSimpleAction *action,
                               GVariant      *parameter,
                               gpointer       userdata)
{
    create_new_window (GTK_APPLICATION (userdata), NULL);
}


static void
about_action_activated (GSimpleAction *action,
                       GVariant      *parameter,
                       gpointer       userdata)
{
    static const gchar* authors[] = {
        "Adrián Pérez de Castro",
        NULL,
    };

    gtk_show_about_dialog (NULL,
                           "authors",        authors,
                           "logo-icon-name", "terminal",
                           "license-type",   GTK_LICENSE_MIT_X11,
                           "comments",       "A sleek terminal emulator",
                           "website",        "https://github.com/aperezdc/dwt",
                           NULL);
}


static void
quit_action_activated (GSimpleAction *action,
                       GVariant      *parameter,
                       gpointer       userdata)
{
    g_application_quit (G_APPLICATION (userdata));
}


static void
copy_url_action_activated (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       userdata)
{
    gtk_clipboard_set_text (gtk_clipboard_get (GDK_SELECTION_PRIMARY),
                            last_match_text, -1);
    gtk_clipboard_set_text (gtk_clipboard_get (GDK_SELECTION_CLIPBOARD),
                            last_match_text, -1);

    g_free (last_match_text);
    last_match_text = NULL;
}


static void
open_url_action_activated (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       userdata)
{
    GError *gerror = NULL;
    if (!gtk_show_uri (NULL, last_match_text, GDK_CURRENT_TIME, &gerror)) {
        g_printerr ("Could not open URL: %s\n", gerror->message);
        g_error_free (gerror);
    }

    g_free (last_match_text);
    last_match_text = NULL;
}


static const GActionEntry win_actions[] = {
    { "font-reset",   font_size_action_ativated,  "i",  "0", NULL },
    { "font-bigger",  font_size_action_ativated,  "i",  "1", NULL },
    { "font-smaller", font_size_action_ativated,  "i", "-1", NULL },
    { "copy",         copy_action_activated,     NULL, NULL, NULL },
    { "paste",        paste_action_activated,    NULL, NULL, NULL },
    { "copy-url",     copy_url_action_activated, NULL, NULL, NULL },
    { "open-url",     open_url_action_activated, NULL, NULL, NULL },
};

static const GActionEntry app_actions[] = {
    { "new-terminal", new_terminal_action_activated, NULL, NULL, NULL },
    { "about",        about_action_activated,        NULL, NULL, NULL },
    { "quit",         quit_action_activated,         NULL, NULL, NULL },
};


#if DWT_USE_OVERLAY
static void
overlay_term_beeped (GtkWidget   *vtterm,
                     GtkRevealer *revealer)
{
    GtkWindow *window = GTK_WINDOW (gtk_widget_get_ancestor (vtterm,
                                                             GTK_TYPE_WINDOW));

    if (gtk_window_get_hide_titlebar_when_maximized (window) &&
        gtk_window_is_maximized (window))
    {
        gtk_revealer_set_reveal_child (revealer, TRUE);
        g_timeout_add_seconds (1, beeped_revealer_timeout, revealer);
    }
}
#endif /* DWT_USE_OVERLAY */


static GtkWidget*
create_new_window (GtkApplication *application,
                   const gchar    *command)
{
    if (!command)
        command = guess_shell ();

    GError *gerror = NULL;
    gint command_argv_len = 0;
    gchar **command_argv = NULL;

    if (!g_shell_parse_argv (command,
                             &command_argv_len,
                             &command_argv,
                             &gerror))
    {
        g_printerr ("%s: coult not parse command: %s\n",
                    __func__, gerror->message);
        g_error_free (gerror);
        return NULL;
    }

    GtkWidget *window = gtk_application_window_new (application);
    gtk_window_set_title (GTK_WINDOW (window), opt_title);
    gtk_window_set_has_resize_grip (GTK_WINDOW (window), FALSE);
    gtk_window_set_hide_titlebar_when_maximized (GTK_WINDOW (window),
                                                 !opt_showbar);

    g_action_map_add_action_entries (G_ACTION_MAP (window), win_actions,
                                     G_N_ELEMENTS (win_actions), window);

    VteTerminal *vtterm = VTE_TERMINAL (vte_terminal_new ());
    configure_term_widget (vtterm);
    term_char_size_changed (vtterm,
                            vte_terminal_get_char_width (vtterm),
                            vte_terminal_get_char_height (vtterm),
                            window);

    g_signal_connect (G_OBJECT (window), "notify::has-toplevel-focus",
                      G_CALLBACK (window_has_toplevel_focus_notified),
                      vtterm);

    g_signal_connect (G_OBJECT (vtterm), "char-size-changed",
                      G_CALLBACK (term_char_size_changed), window);
    g_signal_connect (G_OBJECT (vtterm), "child-exited",
                      G_CALLBACK (term_child_exited), window);
    g_signal_connect (G_OBJECT (vtterm), "beep",
                      G_CALLBACK (term_beeped), window);
    g_signal_connect (G_OBJECT (vtterm), "button-release-event",
                      G_CALLBACK (term_mouse_button_released),
                      setup_popover (vtterm));

    /*
     * Propagate title changes to the window.
     */
    g_object_bind_property (G_OBJECT (vtterm), "window-title",
                            G_OBJECT (window), "title",
                            G_BINDING_DEFAULT);

#if DWT_USE_HEADER_BAR
    setup_header_bar (window, vtterm);
#endif /* DWT_USE_HEADER_BAR */

#if DWT_USE_OVERLAY
    GtkWidget *overlay = gtk_overlay_new ();
    gtk_container_add (GTK_CONTAINER (overlay), GTK_WIDGET (vtterm));
    gtk_container_add (GTK_CONTAINER (window), overlay);

    GtkWidget *icon = gtk_image_new_from_icon_name ("software-update-urgent-symbolic",
                                                    GTK_ICON_SIZE_DIALOG);
    gtk_widget_set_margin_top (icon, 12);
    gtk_widget_set_margin_end (icon, 12);
    gtk_widget_set_margin_start (icon, 12);
    gtk_widget_set_margin_bottom (icon, 12);
    gtk_widget_set_halign (icon, GTK_ALIGN_END);
    gtk_widget_set_valign (icon, GTK_ALIGN_START);

    GtkWidget *revealer = gtk_revealer_new ();
    gtk_container_add (GTK_CONTAINER (revealer), icon);
    gtk_revealer_set_transition_type (GTK_REVEALER (revealer),
                                      GTK_REVEALER_TRANSITION_TYPE_CROSSFADE);
    gtk_revealer_set_transition_duration (GTK_REVEALER (revealer), 250);
    g_signal_connect (G_OBJECT (vtterm), "beep",
                      G_CALLBACK (overlay_term_beeped), revealer);
    gtk_overlay_add_overlay (GTK_OVERLAY (overlay), revealer);
#else /* !DWT_USE_OVERLAY */
    gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (vtterm));
#endif

    gtk_widget_set_receives_default (GTK_WIDGET (vtterm), TRUE);

    g_assert (opt_workdir);
    if (!vte_terminal_fork_command_full (VTE_TERMINAL (vtterm),
                                         VTE_PTY_DEFAULT,
                                         opt_workdir,
                                         command_argv,
                                         NULL,
                                         G_SPAWN_SEARCH_PATH,
                                         NULL,
                                         NULL,
                                         NULL,
                                         &gerror))
    {
        g_printerr ("%s: could not spawn shell: %s\n",
                    __func__, gerror->message);
        g_error_free (gerror);
        gtk_widget_destroy (window);
        return NULL;
    }

    gtk_widget_show_all (window);
    return window;
}


static void
app_started (GApplication *application)
{
    /* Set default icon, and preferred theme variant. */
    gtk_window_set_default_icon_name ("terminal");
    g_object_set(gtk_settings_get_default(),
                 "gtk-application-prefer-dark-theme",
                 TRUE, NULL);

    g_action_map_add_action_entries (G_ACTION_MAP (application), app_actions,
                                     G_N_ELEMENTS (app_actions), application);

    GtkBuilder *builder = gtk_builder_new_from_resource (DWT_GRESOURCE ("menus.xml"));
    gtk_application_set_app_menu (GTK_APPLICATION (application),
        G_MENU_MODEL (gtk_builder_get_object (builder, "app-menu")));
    g_object_unref (builder);

    const struct {
        const gchar *action;
        const gchar *accel;
        GVariant    *param;
    } accel_map[] = {
        { "app.new-terminal", "<Ctrl><Shift>n", NULL                     },
        { "win.font-reset",   "<Super>0",       g_variant_new_int32 (+0) },
        { "win.font-bigger",  "<Super>plus",    g_variant_new_int32 (+1) },
        { "win.font-smaller", "<Super>minus",   g_variant_new_int32 (-1) },
        { "win.copy",         "<Ctrl><Shift>c", NULL                     },
        { "win.paste",        "<Ctrl><Shift>p", NULL                     },
    };

    for (guint i = 0; i < G_N_ELEMENTS (accel_map); i++) {
        gtk_application_add_accelerator (GTK_APPLICATION (application),
                                         accel_map[i].accel,
                                         accel_map[i].action,
                                         accel_map[i].param);
    }
}


static gint
app_command_line_received (GApplication            *application,
                           GApplicationCommandLine *cmdline)
{
    g_application_hold (G_APPLICATION (application));

    gint argc = 0;
    gchar **argv = g_application_command_line_get_arguments (cmdline, &argc);

    /* TODO: pass options */
    create_new_window (GTK_APPLICATION (application), NULL);

    g_strfreev (argv);
    g_application_release (application);
    return 0;
}


static const gchar*
get_appplication_id (void) {
    static const gchar *app_id = NULL;
    if (!app_id) {
        if (!(app_id = g_getenv ("DWT_APPLICATION_ID")))
            app_id = "org.perezdecastro.dwt";
    }
    return g_str_equal ("none", app_id) ? NULL : app_id;
}


int
main (int argc, char *argv[])
{
    GtkApplication *application =
        gtk_application_new (get_appplication_id (),
                             G_APPLICATION_HANDLES_COMMAND_LINE);

    g_application_add_main_option_entries (G_APPLICATION (application),
                                           option_entries);

    g_signal_connect (G_OBJECT (application), "startup",
                      G_CALLBACK (app_started), NULL);
    g_signal_connect (G_OBJECT (application), "command-line",
                      G_CALLBACK (app_command_line_received), NULL);

    gint status = g_application_run (G_APPLICATION (application), argc, argv);
    g_object_unref (application);
    return status;
}

