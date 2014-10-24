/*
 * dg-settings.h
 * Copyright (C) 2014 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef DG_SETTINGS_H
#define DG_SETTINGS_H

#include <glib-object.h>

G_BEGIN_DECLS

#define DG_SETTINGS_TYPE     (dg_settings_get_type ())
#define DG_SETTINGS(obj)     (G_TYPE_CHECK_INSTANCE_CAST ((obj), DG_SETTINGS_TYPE, DgSettings))
#define DG_IS_SETTINGS(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DG_SETTINGS_TYPE))

typedef struct _DgSettingsClass DgSettingsClass;
typedef struct _DgSettings      DgSettings;

struct _DgSettingsClass {
  GObjectClass parent_class;
};

struct _DgSettings {
  GObject parent;
};

GType dg_settings_get_type (void);

void dg_settings__get_property__ (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec);
void dg_settings__constructed__  (GObject    *object);

#define DG_SETTING__FLAG \
  (1 << G_PARAM_USER_SHIFT)
#define DG_SETTING_FLAGS \
  (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | DG_SETTING__FLAG)


#define DG_SETTINGS_BOOLEAN(_name, _nick, _desc, _default)         \
  g_object_class_install_property (G_OBJECT_CLASS (klass),          \
                                   ++klass->prop_id,                 \
                                   g_param_spec_boolean ((_name),     \
                                                         (_nick),      \
                                                         (_desc),       \
                                                         (_default),     \
                                                         DG_SETTING_FLAGS))

#define DG_SETTINGS_STRING(_name, _nick, _desc, _default)         \
  g_object_class_install_property (G_OBJECT_CLASS (klass),         \
                                   ++klass->prop_id,                \
                                   g_param_spec_string ((_name),     \
                                                        (_nick),      \
                                                        (_desc),       \
                                                        (_default),     \
                                                        DG_SETTING_FLAGS))

#define DG_SETTINGS_UINT_RANGE(_name, _nick, _desc, _default, _min, _max) \
    g_object_class_install_property (G_OBJECT_CLASS (klass),              \
                                     ++klass->prop_id,                    \
                                     g_param_spec_uint ((_name),          \
                                                        (_nick),          \
                                                        (_desc),          \
                                                        (_min),           \
                                                        (_max),           \
                                                        (_default),       \
                                                        DG_SETTING_FLAGS))

#define DG_SETTINGS_UINT(_name, _nick, _desc, _default) \
    DG_SETTINGS_UINT_RANGE ((_name), (_nick), (_desc), (_default), 0, G_MAXUINT)

#define DG_SETTINGS_CLASS_DECLARE(_T, _t)                               \
  typedef struct _ ## _T ## Class _T ## Class;                          \
  typedef struct _ ## _T _T;                                            \
  GType _t ## _get_type (void);                                         \
  static inline _T* _t ## _new(const gchar *path, gboolean monitor) {   \
    return g_object_new (_t ## _get_type(), "settings-path", path,      \
                         "settings-monitoring-enabled", monitor, NULL); }

#define DG_SETTINGS_CLASS_DEFINE(_T, _t)                                    \
  struct _ ## _T ## Class { DgSettingsClass parent_class; guint prop_id; }; \
  struct _ ## _T { DgSettings parent; };                                    \
  static void _t ## _init (_T *object) { }                                  \
  G_DEFINE_TYPE (_T, _t, DG_SETTINGS_TYPE)                                  \
  static void _t ## _class_init (_T ## Class *klass) {                      \
    G_OBJECT_CLASS (klass)->get_property = dg_settings__get_property__;     \
    G_OBJECT_CLASS (klass)->constructed  = dg_settings__constructed__;
#define DG_SETTINGS_CLASS_END }

G_END_DECLS

#endif /* !DG_SETTINGS_H */
