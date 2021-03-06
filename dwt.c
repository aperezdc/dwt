/*
 * dwt.c
 * Copyright (C) 2012-2014 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#define PCRE2_CODE_UNIT_WIDTH 8

#define DWT_GRESOURCE(name)  ("/org/perezdecastro/dwt/" name)

#include "dwt-settings.h"
#include <gtk/gtk.h>
#include <gio/gvfs.h>
#include <pcre2.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <vte/vte.h>

#ifdef GDK_WINDOWING_X11
# include <gdk/gdkx.h>
#endif

#define CHECK_FLAGS(_v, _f) (((_v) & (_f)) == (_f))
#define NDIGITS10(_t) (sizeof (_t) * __CHAR_BIT__ * 3 / 10)

/* Last matched text piece. */
static gchar *last_match_text = NULL;

/* Default font size */
static gint default_font_size = 0;


/* Forward declarations. */
static GtkWidget*
create_new_window (GtkApplication *application,
                   GVariantDict   *options);


static const GOptionEntry option_entries[] =
{
    {
        "command", 'e',
        G_OPTION_FLAG_IN_MAIN,
        G_OPTION_ARG_STRING,
        NULL,
        "Execute the argument to this option inside the terminal",
        "COMMAND",
    }, {
        "workdir", 'w',
        G_OPTION_FLAG_IN_MAIN,
        G_OPTION_ARG_STRING,
        NULL,
        "Set working directory before running the command/shell",
        "PATH",
    }, {
        "font", 'f',
        G_OPTION_FLAG_IN_MAIN,
        G_OPTION_ARG_STRING,
        NULL,
        "Font used by the terminal, in FontConfig syntax",
        "FONT",
    }, {
        "theme", 'T',
        G_OPTION_FLAG_IN_MAIN,
        G_OPTION_ARG_STRING,
        NULL,
        "Choose a built-in color theme",
        "NAME",
    }, {
        "title", 't',
        G_OPTION_FLAG_IN_MAIN,
        G_OPTION_ARG_STRING,
        NULL,
        "Initial terminal window title",
        "TITLE",
    }, {
        "no-title-updates", 'U',
        G_OPTION_FLAG_IN_MAIN,
        G_OPTION_ARG_NONE,
        NULL,
        "Do not update window title automatically",
        NULL,
    }, {
        "scrollback", 's',
        G_OPTION_FLAG_IN_MAIN,
        G_OPTION_ARG_INT,
        NULL,
        "Scrollback buffer size, in bytes (default 1024)",
        "BYTES"
    }, {
        "bold", 'b',
        G_OPTION_FLAG_IN_MAIN,
        G_OPTION_ARG_NONE,
        NULL,
        "Allow using bold fonts",
        NULL,
    }, {
        "title-on-maximize", 'H',
        G_OPTION_FLAG_IN_MAIN,
        G_OPTION_ARG_NONE,
        NULL,
        "Always show title bar when window is maximized.",
        NULL,
    }, {
        "no-header-bar", 'N',
        G_OPTION_FLAG_IN_MAIN,
        G_OPTION_ARG_NONE,
        NULL,
        "Disable header bars in terminal windows (use window manager decorations)",
        NULL,
    },
    { NULL }
};


typedef struct {
    const gchar *name;
    GdkRGBA fg, bg;
    GdkRGBA colors[16];
} Theme;


/* The first element is the "default" theme. */
static const Theme themes[] = {
#include "themes/linux.h"
#include "themes/jellybeans.h"
#include "themes/monokai.h"
#include "themes/molokai.h"
#include "themes/solarized.h"
#include "themes/thayer.h"
#include "themes/wombat.h"
#include "themes/zenburn.h"
};


static GdkRGBA cursor_active   = {   0, 0.75,   0, 1 };
static GdkRGBA cursor_inactive = { 0.6,  0.6, 0.6, 1 };


/* Regexp used to match URIs and allow clicking them */
static const gchar uri_regexp[] = "(ftp|http)s?://[-@a-zA-Z0-9.?$%&/=_~#.,:;+]*";
static const gchar image_regex_string[] = "/[^/]+\\.(png|jpg|jpeg|gif|webp)$";

static GRegex *image_regex = NULL;


static const Theme* const
find_theme (const gchar *name)
{
    for (size_t i = 0; i < G_N_ELEMENTS (themes); i++) {
        if (g_str_equal (name, themes[i].name)) {
            return &themes[i];
        }
    }
    return NULL;
}


#define SWAP(t, a, b)             \
    do {                          \
        t tmp_ ## __LINE__ = (a); \
        (a) = (b);                \
        (b) = tmp_ ## __LINE__;   \
    } while (0)

static void
configure_term_widget (VteTerminal  *vtterm,
                       GVariantDict *options)
{
    /* Pick default settings from the settings... */
    g_autofree char *opt_font = NULL;
    g_autofree char *opt_theme = NULL;
    g_autofree char *opt_fgcolor = NULL;
    g_autofree char *opt_bgcolor = NULL;
    gboolean opt_bold;
    guint opt_scroll;
    DwtSettings *settings = dwt_settings_get_instance ();

    g_object_get (settings,
                  "font", &opt_font,
                  "theme", &opt_theme,
                  "allow-bold", &opt_bold,
                  "scrollback", &opt_scroll,
                  "foreground-color", &opt_fgcolor,
                  "background-color", &opt_bgcolor,
                  NULL);

    /*
     * This ensures that properties are updated for the terminal whenever they
     * change in the configuration files.
     *
     * TODO: For now this is done only for those properties which cannot be
     *       overriden using command line flags.
     */
    static const struct {
        const gchar  *setting_name;
        const gchar  *property_name;
        GBindingFlags bind_flags;
    } property_bind_map[] = {
        { "mouse-autohide", "pointer-autohide", G_BINDING_SYNC_CREATE },
        { "audible-bell",   "audible-bell",     G_BINDING_SYNC_CREATE },
    };
    for (guint i = 0; i < G_N_ELEMENTS (property_bind_map); i++) {
        g_object_bind_property (settings,
                                property_bind_map[i].setting_name,
                                G_OBJECT (vtterm),
                                property_bind_map[i].property_name,
                                property_bind_map[i].bind_flags);
    }

    /* ...and allow command line options to override them. */
    if (options) {
        g_autofree char *cmd_font = NULL;
        g_variant_dict_lookup (options, "font", "s", &cmd_font);
        if (cmd_font) SWAP (gchar*, cmd_font, opt_font);

        g_autofree char *cmd_theme = NULL;
        g_variant_dict_lookup (options, "theme", "s", &cmd_theme);
        if (cmd_theme) SWAP (gchar*, cmd_theme, opt_theme);

        g_variant_dict_lookup (options, "allow-bold", "b", &opt_bold);
        g_variant_dict_lookup (options, "scrollback", "u", &opt_scroll);
    }

    PangoFontDescription *fontd = pango_font_description_from_string (opt_font);
    if (fontd) {
      if (!pango_font_description_get_family (fontd))
        pango_font_description_set_family_static (fontd, "monospace");
      if (!pango_font_description_get_size (fontd))
        pango_font_description_set_size (fontd, 12 * PANGO_SCALE);
      vte_terminal_set_font (vtterm, fontd);
      pango_font_description_free (fontd);
      fontd = NULL;
    }

    const Theme *theme = &themes[1];
    if (opt_theme) {
        theme = find_theme (opt_theme);
        if (!theme) {
            g_printerr ("No such theme '%s', using default (linux)\n", opt_theme);
            theme = &themes[1];
        }
    }

    GdkRGBA fgcolor, bgcolor;
    if (!(opt_fgcolor && gdk_rgba_parse (&fgcolor, opt_fgcolor)))
        fgcolor = theme->fg;
    if (!(opt_bgcolor && gdk_rgba_parse (&bgcolor, opt_bgcolor)))
        bgcolor = theme->bg;

    vte_terminal_set_rewrap_on_resize    (vtterm, TRUE);
    vte_terminal_set_scroll_on_keystroke (vtterm, TRUE);
    vte_terminal_set_audible_bell        (vtterm, FALSE);
    vte_terminal_set_scroll_on_output    (vtterm, FALSE);
    vte_terminal_set_allow_bold          (vtterm, opt_bold);
    vte_terminal_set_scrollback_lines    (vtterm, opt_scroll);
    vte_terminal_set_cursor_blink_mode   (vtterm, VTE_CURSOR_BLINK_OFF);
    vte_terminal_set_cursor_shape        (vtterm, VTE_CURSOR_SHAPE_BLOCK);
    vte_terminal_set_colors              (vtterm,
                                          &fgcolor,
                                          &bgcolor,
                                          theme->colors,
                                          G_N_ELEMENTS (theme->colors));

    g_autoptr(GError) error = NULL;
    g_autoptr(VteRegex) regex =
        vte_regex_new_for_match (uri_regexp, -1,
                                 PCRE2_CASELESS | PCRE2_MULTILINE,
                                 &error);
    if (regex) {
        if (!vte_regex_jit (regex, PCRE2_JIT_COMPLETE, &error))
            g_warning ("Could not JIT-compile URI regex: %s", error->message);
        int tag = vte_terminal_match_add_regex (vtterm, regex, PCRE2_NOTEMPTY);
        vte_terminal_match_set_cursor_type (vtterm, tag, GDK_HAND2);
    } else {
        g_critical ("Could not compile URI regex: %s", error->message);
    }
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

    g_autoptr(GtkBuilder) builder = gtk_builder_new_from_resource (DWT_GRESOURCE ("menus.xml"));
    gtk_popover_bind_model (GTK_POPOVER (popover),
                            G_MENU_MODEL (gtk_builder_get_object (builder, "popover-menu")),
                            NULL);
    return popover;
}


static void
image_popover_closed (GtkWidget *popover,
                      gpointer   userdata)
{
	gtk_widget_destroy (popover);
}


static void
image_pixbuf_loaded (GInputStream *stream,
                     GAsyncResult *result,
                     GtkWidget    *popover)
{
	g_autoptr(GError) error = NULL;
    g_autoptr(GdkPixbuf) pixbuf = gdk_pixbuf_new_from_stream_finish (result,
                                                                     &error);
	if (!pixbuf || error) {
		g_printerr ("Could not decode image from pixbuf: %s", error->message);
		return;
	}

    gtk_container_add (GTK_CONTAINER (popover),
                       gtk_image_new_from_pixbuf (pixbuf));
	g_signal_connect (popover, "closed",
                      G_CALLBACK (image_popover_closed), NULL);
	gtk_widget_show_all (popover);
}


static void
image_file_opened (GFile        *file,
                   GAsyncResult *result,
                   GtkWidget    *popover)
{
	g_autoptr(GError) error = NULL;
    g_autoptr(GFileInputStream) stream = g_file_read_finish (file, result, &error);
	if (!stream || error) {
		g_autofree char *uri = g_file_get_uri (file);
		g_printerr ("Could not open URL '%s': %s", uri, error->message);
		return;
	}

	gdk_pixbuf_new_from_stream_at_scale_async (G_INPUT_STREAM (stream),
                                               500, 500, TRUE,
                                               NULL,
                                               (GAsyncReadyCallback) image_pixbuf_loaded,
                                               popover);
}


static GtkWidget*
make_popover_for_image_url (VteTerminal *vtterm,
							const gchar *uri)
{
	g_assert (vtterm);
	g_assert (uri);

	GtkWidget *popover = gtk_popover_new (GTK_WIDGET (vtterm));
	g_autoptr(GVfs) gvfs = g_vfs_get_default ();
	g_autoptr(GFile) file = g_vfs_get_file_for_uri (gvfs, uri);
	g_file_read_async (file,
                       G_PRIORITY_DEFAULT,
                       NULL,
                       (GAsyncReadyCallback) image_file_opened,
                       popover);
	return popover;
}


static gboolean
term_mouse_button_released (VteTerminal    *vtterm,
                            GdkEventButton *event,
                            gpointer        userdata)
{
    g_clear_pointer (&last_match_text, g_free);

    int match_tag;
    g_autofree char *match =
        vte_terminal_match_check_event (vtterm, (GdkEvent*) event, &match_tag);

    const long col = event->x / vte_terminal_get_char_width (vtterm);
    const long row = event->y / vte_terminal_get_char_height (vtterm);

    GtkWindow *window = GTK_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (vtterm),
                                                             GTK_TYPE_WINDOW));

    if (match && event->button == 1) {
		if (CHECK_FLAGS (event->state, GDK_CONTROL_MASK)) {
			g_autoptr(GError) error = NULL;
			if (!gtk_show_uri_on_window (window, match, event->time, &error))
				g_printerr ("Could not open URL: %s\n", error->message);
			return FALSE;
		} else if (g_regex_match (image_regex, match, 0, NULL)) {
			/* Show picture in a popover */
			GdkRectangle rect;
			rect.height = vte_terminal_get_char_height (vtterm);
			rect.width = vte_terminal_get_char_width (vtterm);
			rect.y = rect.height * row;
			rect.x = rect.width * col;

			GtkWidget* popover = make_popover_for_image_url (vtterm, match);
			gtk_popover_set_pointing_to (GTK_POPOVER (popover), &rect);
			return FALSE;
		}
    }


    if (event->button == 3 && userdata != NULL) {
        GdkRectangle rect;
        rect.height = vte_terminal_get_char_height (vtterm);
        rect.width = vte_terminal_get_char_width (vtterm);
        rect.y = rect.height * row;
        rect.x = rect.width * col;
        gtk_popover_set_pointing_to (GTK_POPOVER (userdata), &rect);

        GActionMap *actions = G_ACTION_MAP (window);
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

    return FALSE;
}

static gboolean
beeped_revealer_timeout (gpointer userdata)
{
    GtkWindow *window =
            GTK_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (userdata),
                                                 GTK_TYPE_WINDOW));
    if (!gtk_window_has_toplevel_focus (window))
        return TRUE;

    gtk_revealer_set_reveal_child (GTK_REVEALER (userdata), FALSE);
    return FALSE;  /* Do not re-arm timer, run only once */
}

static void
header_bar_term_beeped (VteTerminal *vtterm,
                        GtkRevealer *revealer)
{
    /* If already shown, do nothing */
    if (gtk_revealer_get_reveal_child (revealer))
        return;

    gtk_revealer_set_reveal_child (revealer, TRUE);
    g_timeout_add_seconds (3, beeped_revealer_timeout, revealer);
}

static void
setup_header_bar (GtkWidget   *window,
                  VteTerminal *vtterm,
                  gboolean     show_maximized_title)
{
    /*
     * Using the default GtkHeaderBar title/subtitle widget makes the bar
     * too thick to look nice for a terminal, so set a custom widget.
     */
    const gchar *title = gtk_window_get_title (GTK_WINDOW (window));
    GtkWidget *label = gtk_label_new (title ? title : "dwt");
    g_object_bind_property (G_OBJECT (vtterm), "window-title",
                            G_OBJECT (label), "label",
                            G_BINDING_DEFAULT);

    GtkWidget *header = gtk_header_bar_new ();
    gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (header), TRUE);
    gtk_header_bar_set_has_subtitle (GTK_HEADER_BAR (header), FALSE);
    gtk_header_bar_set_custom_title (GTK_HEADER_BAR (header), label);

    GtkWidget *button = gtk_button_new_from_icon_name ("tab-new-symbolic",
                                                       GTK_ICON_SIZE_BUTTON);
    gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
    gtk_actionable_set_action_name (GTK_ACTIONABLE (button), "app.new-terminal");
    gtk_header_bar_pack_start (GTK_HEADER_BAR (header), button);

    GtkWidget *revealer = gtk_revealer_new ();
    gtk_container_add (GTK_CONTAINER (revealer),
                       gtk_image_new_from_icon_name ("software-update-urgent-symbolic",
                                                     GTK_ICON_SIZE_BUTTON));
    gtk_revealer_set_transition_duration (GTK_REVEALER (revealer), 500);
    gtk_revealer_set_transition_type (GTK_REVEALER (revealer),
                                      GTK_REVEALER_TRANSITION_TYPE_CROSSFADE);
    gtk_header_bar_pack_end (GTK_HEADER_BAR (header), revealer);

    g_signal_connect (G_OBJECT (vtterm), "bell",
                      G_CALLBACK (header_bar_term_beeped), revealer);
    g_object_bind_property (G_OBJECT (window), "urgency-hint",
                            G_OBJECT (revealer), "reveal-child",
                            G_BINDING_DEFAULT);

    gtk_window_set_titlebar (GTK_WINDOW (window), header);

    /* Hide the header bar when the window is maximized. */
    if (!show_maximized_title) {
        g_object_bind_property (G_OBJECT (window), "is-maximized",
                                G_OBJECT (header), "visible",
                                G_BINDING_INVERT_BOOLEAN);
    }
}


static void
window_has_toplevel_focus_notified (GObject    *object,
                                    GParamSpec *pspec,
                                    gpointer    userdata)
{
    if (gtk_window_has_toplevel_focus (GTK_WINDOW (object))) {
        vte_terminal_set_color_cursor (VTE_TERMINAL (userdata),
                                       &cursor_active);
        /* Clear the _URGENT hint when the window gets activated. */
        gtk_window_set_urgency_hint (GTK_WINDOW (object), FALSE);
    } else {
        vte_terminal_set_color_cursor (VTE_TERMINAL (userdata),
                                       &cursor_inactive);
    }
}


static void
term_child_exited (VteTerminal *vtterm,
                   gint         status,
                   gpointer     userdata)
{
    /*
     * Destroy the window when the terminal child is exited. Note that this
     * will fire the "delete-event" signal, and its handler already takes
     * care of deregistering the window in the GtkApplication.
     */
    gtk_window_close (GTK_WINDOW (userdata));
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
    if (default_font_size == 0)
      default_font_size = old_size;
    PangoFontDescription *new_fontd;
    gint new_size;

    switch (modifier) {
      case 0:
        old_size = default_font_size;
        /* fall-through */
      case 1:
      case -1:
        new_size = old_size + modifier * PANGO_SCALE;
        new_fontd = pango_font_description_copy_static (fontd);
        pango_font_description_set_size (new_fontd, new_size);
        vte_terminal_set_font (vtterm, new_fontd);
        break;

      default:
        g_printerr ("%s: invalid modifier '%i'", __func__, modifier);
        return;
    }

    pango_font_description_free (new_fontd);
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
    vte_terminal_copy_clipboard_format (vtterm, VTE_FORMAT_TEXT);
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
    g_autoptr(GError) error = NULL;
    if (!gtk_show_uri_on_window (GTK_WINDOW (userdata),
                                 last_match_text,
                                 GDK_CURRENT_TIME,
                                 &error))
        g_warning ("Could not open URL: %s", error->message);
    g_clear_pointer (&last_match_text, g_free);
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


static void
on_child_spawned (G_GNUC_UNUSED VteTerminal *vtterm,
                  GPid pid, GError *error, void *userdata)
{
    if (pid == -1) {
        // Error: report and close window.
        g_assert_nonnull (error);
        gtk_window_close (GTK_WINDOW (userdata));
        g_error ("Cannot spawn child process: %s", error->message);
    } else {
        g_assert_null (error);
    }
}


static GtkWidget*
create_new_window (GtkApplication *application,
                   GVariantDict   *options)
{
    g_autofree char *command = NULL;
    g_autofree char *title = NULL;
    gboolean opt_show_title;
    gboolean opt_update_title;
    gboolean opt_no_headerbar;

    g_object_get (dwt_settings_get_instance (),
                  "show-title", &opt_show_title,
                  "update-title", &opt_update_title,
                  "no-header-bar", &opt_no_headerbar,
                  "command", &command,
                  "title", &title,
                  NULL);

    const gchar *opt_command = command;
    const gchar *opt_title   = title;
    const gchar *opt_workdir = NULL;

    if (options) {
        gboolean opt_no_auto_title = FALSE;
        g_variant_dict_lookup (options, "title-on-maximize", "b", &opt_show_title);
        g_variant_dict_lookup (options, "no-header-bar", "b", &opt_no_headerbar);
        g_variant_dict_lookup (options, "no-auto-title", "b", &opt_no_auto_title);
        g_variant_dict_lookup (options, "workdir", "&s", &opt_workdir);
        g_variant_dict_lookup (options, "command", "&s", &opt_command);
        g_variant_dict_lookup (options, "title",   "&s", &opt_title);
        if (opt_no_auto_title)
            opt_update_title = FALSE;
    }
    if (!opt_workdir) opt_workdir = g_get_home_dir ();
    if (!opt_command) opt_command = guess_shell ();

    /*
     * Title either comes from the default value of the "title" setting,
     * or from the command line flag, but should never be NULL at this
     * point.
     */
    g_assert (opt_title);

    g_autoptr(GError) error = NULL;
    gint command_argv_len = 0;
    gchar **command_argv = NULL;

    if (!g_shell_parse_argv (opt_command,
                             &command_argv_len,
                             &command_argv,
                             &error))
    {
        g_printerr ("%s: coult not parse command: %s\n",
                    __func__, error->message);
        return NULL;
    }

    GtkWidget *window = gtk_application_window_new (application);
    gtk_widget_set_visual (window,
                           gdk_screen_get_system_visual (gtk_widget_get_screen (window)));
    gtk_application_window_set_show_menubar (GTK_APPLICATION_WINDOW (window),
                                             FALSE);
    gtk_window_set_title (GTK_WINDOW (window), opt_title);
    gtk_window_set_hide_titlebar_when_maximized (GTK_WINDOW (window),
                                                 !opt_show_title);

    g_action_map_add_action_entries (G_ACTION_MAP (window), win_actions,
                                     G_N_ELEMENTS (win_actions), window);

    VteTerminal *vtterm = VTE_TERMINAL (vte_terminal_new ());
    configure_term_widget (vtterm, options);
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
    g_signal_connect (G_OBJECT (vtterm), "bell",
                      G_CALLBACK (term_beeped), window);
    g_signal_connect (G_OBJECT (vtterm), "button-release-event",
                      G_CALLBACK (term_mouse_button_released),
                      setup_popover (vtterm));

    /*
     * Propagate title changes to the window.
     */
    if (opt_update_title)
        g_object_bind_property (G_OBJECT (vtterm), "window-title",
                                G_OBJECT (window), "title",
                                G_BINDING_DEFAULT);

    if (!opt_no_headerbar)
        setup_header_bar (window, vtterm, opt_show_title);

    gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (vtterm));
    gtk_widget_set_receives_default (GTK_WIDGET (vtterm), TRUE);

    /* We need to realize and show the window for it to have a valid XID */
    gtk_widget_show_all (window);

    gchar **command_env = g_get_environ ();
#ifdef GDK_WINDOWING_X11
    if (GDK_IS_X11_SCREEN (gtk_widget_get_screen (window))) {
        GdkWindow *gdk_window = gtk_widget_get_window (window);
        if (gdk_window) {
            gchar window_id[NDIGITS10(unsigned long)];
            snprintf (window_id,
                      sizeof (window_id),
                      "%lu",
                      GDK_WINDOW_XID (gdk_window));
            command_env = g_environ_setenv (command_env,
                                            "WINDOWID",
                                            window_id,
                                            TRUE);
        } else {
            g_printerr ("No window, cannot set $WINDOWID!\n");
        }
    }
#endif /* GDK_WINDOWING_X11 */

    vte_terminal_spawn_async (VTE_TERMINAL (vtterm),
                              VTE_PTY_DEFAULT,
                              opt_workdir,
                              command_argv,
                              command_env,
                              G_SPAWN_SEARCH_PATH,
                              NULL,
                              NULL,
                              NULL,
                              -1,
                              NULL,
                              on_child_spawned,
                              window);
    return window;
}


static void
app_started (GApplication *application, gpointer userdata)
{
    /* Set default icon, and preferred theme variant. */
    g_autofree char *cursor_color = NULL;
    g_autofree char *icon = NULL;
    g_object_get (dwt_settings_get_instance (),
                  "cursor-color", &cursor_color,
                  "icon", &icon,
                  NULL);
    gtk_window_set_default_icon_name (icon);
    g_object_set(gtk_settings_get_default(),
                 "gtk-application-prefer-dark-theme",
                 TRUE, NULL);

    if (cursor_color) {
        gdk_rgba_parse (&cursor_active, cursor_color);
        memcpy (&cursor_inactive, &cursor_active, sizeof (GdkRGBA));
        cursor_inactive.red   = 0.5 * cursor_active.red;
        cursor_inactive.green = 0.5 * cursor_active.green;
        cursor_inactive.blue  = 0.5 * cursor_active.blue;
        cursor_inactive.alpha = 0.5 * cursor_active.alpha;
    }

	image_regex = g_regex_new (image_regex_string, G_REGEX_CASELESS | G_REGEX_OPTIMIZE, 0, NULL);
	g_assert (image_regex);

    g_action_map_add_action_entries (G_ACTION_MAP (application), app_actions,
                                     G_N_ELEMENTS (app_actions), application);

    g_autoptr(GtkBuilder) builder = gtk_builder_new_from_resource (DWT_GRESOURCE ("menus.xml"));
    gtk_application_set_app_menu (GTK_APPLICATION (application),
        G_MENU_MODEL (gtk_builder_get_object (builder, "app-menu")));

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


static void
app_shutdown (GApplication *application, gpointer userdata)
{
	g_regex_unref (image_regex);
}


static gint
app_command_line_received (GApplication            *application,
                           GApplicationCommandLine *cmdline)
{
    g_application_hold (G_APPLICATION (application));
    GVariantDict *options = g_application_command_line_get_options_dict (cmdline);

    g_autofree char *opt_theme = NULL;
    if (g_variant_dict_lookup (options, "theme", "s", &opt_theme) && g_str_equal (opt_theme, "list")) {
        for (size_t i = 0; i < G_N_ELEMENTS (themes); i++) {
            g_print ("%s\n", themes[i].name);
        }
    } else {
        create_new_window (GTK_APPLICATION (application), options);
    }
    g_variant_dict_unref (options);
    g_application_release (application);
    return 0;
}


static const gchar*
get_application_id (const gchar* argv0) {
    static const gchar *app_id = NULL;
    if (!app_id) {
        if (!(app_id = g_getenv ("DWT_APPLICATION_ID"))) {
            app_id = "org.perezdecastro.dwt";
            const gchar* slash_position = strrchr (argv0, '/');
            if (slash_position)
                argv0 = slash_position + 1;
            if (!g_str_equal ("dwt", argv0))
                app_id = g_strdup_printf ("%s.%s", app_id, argv0);
            g_set_prgname (app_id + G_N_ELEMENTS ("org.perezdecastro.") - 1);
        }
    }
    return (g_str_equal ("none", app_id) ||
            g_getenv ("DWT_SINGLE_WINDOW_PROCESS") != NULL)
        ? NULL : app_id;
}


int
main (int argc, char *argv[])
{
    g_autoptr(GtkApplication) application =
        gtk_application_new (get_application_id (argv[0]),
                             G_APPLICATION_HANDLES_COMMAND_LINE);

    g_application_add_main_option_entries (G_APPLICATION (application),
                                           option_entries);

    g_signal_connect (G_OBJECT (application), "startup",
                      G_CALLBACK (app_started), NULL);
    g_signal_connect (G_OBJECT (application), "shutdown",
                      G_CALLBACK (app_shutdown), NULL);
    g_signal_connect (G_OBJECT (application), "command-line",
                      G_CALLBACK (app_command_line_received), NULL);

    return g_application_run (G_APPLICATION (application), argc, argv);
}

