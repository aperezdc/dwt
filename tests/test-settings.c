/*
 * test-settings.c
 * Copyright (C) 2014 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "../dg-settings.h"
#include "../dg-util.h"
#include <glib.h>
#include <gio/gio.h>
#include <glib/gstdio.h>
#include <unistd.h>
#include <string.h>


DG_SETTINGS_CLASS_DECLARE (TestSettings, test_settings)
DG_SETTINGS_CLASS_DEFINE (TestSettings, test_settings)
  DG_SETTINGS_BOOLEAN ("foo", "Foo", "Foo", FALSE);
  DG_SETTINGS_UINT    ("baz", "Baz", "Baz", 12345);
  DG_SETTINGS_STRING  ("bar", "Bar", "Bar", "BAR");
DG_SETTINGS_CLASS_END


static void
delete_settings_dir (gpointer userdata)
{
    dg_lmem gchar *dir_path = userdata;
    GDir *dir = g_dir_open (dir_path, 0, NULL);
    const gchar *name;

    while ((name = g_dir_read_name (dir))) {
        dg_lmem gchar *file_path = g_build_filename (dir_path, name, NULL);
        g_remove (file_path);
    }
    g_dir_close (dir);
}


static const gchar*
temporary_settings_dir (void)
{
    gchar *template = g_build_filename (g_get_tmp_dir (),
                                        "test-settings-XXXXXX",
                                        NULL);
    g_test_queue_destroy (delete_settings_dir, template);
    return g_mkdtemp (template);
}


static void
test_settings_read_defaults (void)
{
    TestSettings *settings =
        test_settings_new (temporary_settings_dir (), FALSE);
    g_test_queue_unref (settings);

    gboolean bool_value;
    gchar* string_value = NULL;
    guint uint_value = 0;

    g_object_get (G_OBJECT (settings),
                  "foo", &bool_value,
                  "bar", &string_value,
                  "baz", &uint_value,
                  NULL);
    g_test_queue_free (string_value);

    g_assert_false (bool_value);
    g_assert_cmpstr (string_value, ==, "BAR");
    g_assert_cmpuint (uint_value, ==, 12345);
}


static void
populate_setting (const gchar *settings_path,
                  const gchar *setting_name,
                  const gchar *value_as_string)
{
    dg_lmem gchar *path = g_build_filename (settings_path, setting_name, NULL);
    dg_lobj GFile *file = g_file_new_for_path (path);
    g_file_replace_contents (file,
                             value_as_string,
                             strlen (value_as_string),
                             NULL,
                             FALSE,
                             G_FILE_CREATE_NONE,
                             NULL,
                             NULL,
                             NULL);
}


static void
test_settings_read (void)
{
    const gchar* settings_path = temporary_settings_dir ();
    populate_setting (settings_path, "foo", "TRUE");
    populate_setting (settings_path, "bar", "Kitteh sez: meow!");
    populate_setting (settings_path, "baz", "42");

    TestSettings *settings = test_settings_new (settings_path, FALSE);
    g_test_queue_unref (settings);

    gboolean bool_value;
    gchar* string_value = NULL;
    guint uint_value = 0;

    g_object_get (G_OBJECT (settings),
                  "foo", &bool_value,
                  "bar", &string_value,
                  "baz", &uint_value,
                  NULL);
    g_test_queue_free (string_value);

    g_assert_true (bool_value);
    g_assert_cmpstr (string_value, ==, "Kitteh sez: meow!");
    g_assert_cmpuint (uint_value, ==, 42);
}


int
main (int argc, char *argv[])
{
    g_test_init (&argc, &argv, NULL);
    g_test_add_func ("/settings/read-defaults", test_settings_read_defaults);
    g_test_add_func ("/settings/read", test_settings_read);
    return g_test_run ();
}

