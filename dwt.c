/*
 * xdwmterm.c
 * Copyright (C) 2012 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include <gtk/gtk.h>
#include <vte/vte.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>


static void
configure_term_widget (VteTerminal *vtterm)
{
    g_assert (vtterm);
    vte_terminal_set_default_colors      (vtterm);
    vte_terminal_set_scrollback_lines    (vtterm, 4096);
    vte_terminal_set_mouse_autohide      (vtterm, TRUE);
    vte_terminal_set_allow_bold          (vtterm, TRUE);
    vte_terminal_set_visible_bell        (vtterm, FALSE);
    vte_terminal_set_audible_bell        (vtterm, FALSE);
    vte_terminal_set_scroll_on_output    (vtterm, FALSE);
    vte_terminal_set_scroll_on_keystroke (vtterm, FALSE);
    vte_terminal_set_font_from_string    (vtterm, "unifont 12");
    vte_terminal_set_cursor_blink_mode   (vtterm, VTE_CURSOR_BLINK_OFF);
    vte_terminal_set_cursor_shape        (vtterm, VTE_CURSOR_SHAPE_BLOCK);
}


static void
set_urgent (VteTerminal *vtterm, gpointer userdata)
{
    gtk_window_set_urgency_hint (GTK_WINDOW (userdata), 1);
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


int
main (int argc, char *argv[])
{
    char *cmd_argv[] = {
        guess_shell (),
        NULL,
    };

    GtkWidget *window = NULL;
    GtkWidget *vtterm = NULL;
    GError    *gerror = NULL;

    gtk_init (&argc, &argv);

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "dwt");

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

    if (!vte_terminal_fork_command_full (VTE_TERMINAL (vtterm),
                                         VTE_PTY_DEFAULT,
                                         g_get_home_dir (),
                                         cmd_argv,
                                         NULL,
                                         0,
                                         NULL,
                                         NULL,
                                         NULL,
                                         &gerror))
    {
        g_printerr ("Could not spawn shell: %s\n", gerror->message);
        g_error_free (gerror);
        gerror = NULL;
    }

    gtk_container_add (GTK_CONTAINER (window), vtterm);
    gtk_widget_show_all (window);

    gtk_main ();
    return EXIT_SUCCESS;
}

