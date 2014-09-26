/*
 * test-settings.c
 * Copyright (C) 2014 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "../dg-settings.h"


DG_SETTINGS_CLASS_DECLARE (TestSettings, test_settings)
DG_SETTINGS_CLASS_DEFINE (TestSettings, test_settings)
  g_printerr ("%s\n", __func__);
  DG_SETTINGS_BOOLEAN ("foo", "Foo", "Foo", FALSE);
  DG_SETTINGS_STRING  ("bar", "Bar", "Bar", "BAR");
DG_SETTINGS_CLASS_END


static void
test_settings_read (void)
{
  TestSettings *settings = test_settings_new ("/tmp/test-settings", FALSE);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/settings/read", test_settings_read);
  return g_test_run ();
}

