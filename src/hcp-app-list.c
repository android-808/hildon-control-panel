/*
 * This file is part of hildon-control-panel
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Author: Lucas Rocha <lucas.rocha@nokia.com>
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <string.h>

#include <osso-log.h>
#include <libosso.h>
#include <hildon-base-lib/hildon-base-dnotify.h>

#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include <glib/gi18n.h>

#include "hcp-app-list.h"
#include "hcp-app.h"
#include "hcp-config-keys.h"

#define HCP_APP_LIST_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HCP_TYPE_APP_LIST, HCPAppListPrivate))

G_DEFINE_TYPE (HCPAppList, hcp_app_list, G_TYPE_OBJECT);

typedef enum
{
  HCP_APP_LIST_SIGNAL_UPDATED,
  HCP_APP_LIST_SIGNALS
} HCPAppListSignals;

static gint hcp_app_list_signals[HCP_APP_LIST_SIGNALS];

enum
{
  HCP_APP_LIST_PROP_APPS = 1,
  HCP_APP_LIST_PROP_CATEGORIES,
};

struct _HCPAppListPrivate 
{
  GHashTable *apps;
  GSList     *categories;
};

#define HCP_SEPARATOR_DEFAULT _("copa_ia_extras")

/* Delay to wait from callback to actually reading 
 * the entries in msecs */
#define HCP_DIR_READ_DELAY 500

static int callback_pending = 0;

/* Called by dnotify_callback_f after a timeout (to prevent exessive
   callbacks. */
static gboolean 
hcp_dnotify_reread_desktop_entries (HCPAppList *al)
{
  callback_pending = 0;

  /* Re-read the item list from .desktop files */
  hcp_app_list_update (al);

  g_signal_emit (G_OBJECT (al), 
                 hcp_app_list_signals[HCP_APP_LIST_SIGNAL_UPDATED], 
                 0, NULL);

  return FALSE; 
}

static void 
hcp_dnotify_callback_f (char *path, HCPAppList *al)
{
  if (!callback_pending) 
  {
    callback_pending = 1;
    g_timeout_add (HCP_DIR_READ_DELAY,
                  (GSourceFunc) hcp_dnotify_reread_desktop_entries, al);
  }
}

static int 
hcp_init_dnotify (HCPAppList *al, const gchar *path)
{
  hildon_return_t ret;

  ret = hildon_dnotify_handler_init ();

  if (ret != HILDON_OK)
  {
    return ret;
  }
  
  ret = hildon_dnotify_set_cb ((hildon_dnotify_cb_f *) hcp_dnotify_callback_f,
                               (gchar *) path,
                               al);

  if (ret != HILDON_OK)
  {
      return ret;
  }

  return HILDON_OK;
}

static void
hcp_app_list_get_configured_categories (HCPAppList *al)
{
  GConfClient *client = NULL;
  GSList *group_names = NULL;
  GSList *group_names_i = NULL;
  GSList *group_ids = NULL;
  GSList *group_ids_i = NULL;
  GError *error = NULL;

  client = gconf_client_get_default ();
  
  if (client)
  {
    /* Get the group names */
    group_names = gconf_client_get_list (client, 
    		HCP_GCONF_GROUPS_KEY,
    		GCONF_VALUE_STRING, &error);

    if (error)
    {
      ULOG_ERR (error->message);
      g_error_free (error);
      g_object_unref (client);

      goto cleanup;
    }

    /* Get the group ids */
    group_ids = gconf_client_get_list (client, 
    		HCP_GCONF_GROUP_IDS_KEY,
    		GCONF_VALUE_STRING, &error);
    
    if (error)
    {
      ULOG_ERR (error->message);
      g_error_free (error);
      g_object_unref (client);

      goto cleanup;
    }

    g_object_unref (client);
  }

  group_names_i = group_names;
  group_ids_i = group_ids;

  while (group_ids_i && group_names_i)
  {
    HCPCategory *category = g_new0 (HCPCategory, 1);

    category->id = (gchar *) group_ids_i->data;
    category->name = (gchar *) group_names_i->data;

    al->priv->categories = g_slist_append (al->priv->categories, category);

    group_ids_i = g_slist_next (group_ids_i);
    group_names_i = g_slist_next (group_names_i);
  }

cleanup:
  if (group_names)
    g_slist_free (group_names);

  if (group_ids)
    g_slist_free (group_ids);
}

static void
hcp_app_list_init (HCPAppList *al)
{
  al->priv = HCP_APP_LIST_GET_PRIVATE (al);

  HCPCategory *extras_category = g_new0 (HCPCategory, 1);

  al->priv->apps = g_hash_table_new (g_str_hash, g_str_equal);

  hcp_app_list_get_configured_categories (al);
  
  /* Add the default category as the last one */
  extras_category->id   = g_strdup ("");
  extras_category->name = g_strdup (HCP_SEPARATOR_DEFAULT);

  al->priv->categories = g_slist_append (al->priv->categories, extras_category);

  hcp_init_dnotify (al, CONTROLPANEL_ENTRY_DIR);
}

static void
hcp_app_list_empty_category (HCPCategory* category)
{
  g_slist_free (category->apps);
  category->apps = NULL;
}

static gboolean
hcp_app_list_free_app (gchar *plugin, HCPApp *app)
{
  g_object_unref (app);

  return TRUE;
}

static void
hcp_app_list_finalize (GObject *object)
{
  HCPAppList *al;
  HCPAppListPrivate *priv;
  
  g_return_if_fail (object != NULL);
  g_return_if_fail (HCP_IS_APP_LIST (object));

  al = HCP_APP_LIST (object);
  priv = al->priv;

  if (priv->apps != NULL) 
  {
    g_hash_table_foreach_remove (priv->apps, (GHRFunc) hcp_app_list_free_app, NULL);
    g_hash_table_destroy (priv->apps);

    g_slist_foreach (priv->categories, (GFunc) hcp_app_list_empty_category, NULL);
  }
}

static void
hcp_app_list_get_property (GObject    *gobject,
                           guint      prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{

  HCPAppList *al = HCP_APP_LIST (gobject);

  switch (prop_id)
  {
    case HCP_APP_LIST_PROP_APPS:
      g_value_set_pointer (value, al->priv->apps);
      break;

    case HCP_APP_LIST_PROP_CATEGORIES:
      g_value_set_pointer (value, al->priv->categories);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
hcp_app_list_class_init (HCPAppListClass *class)
{
  GObjectClass *g_object_class = (GObjectClass *) class;
  
  g_object_class->finalize = hcp_app_list_finalize;

  g_object_class->get_property = hcp_app_list_get_property;

  hcp_app_list_signals[HCP_APP_LIST_SIGNAL_UPDATED] =
        g_signal_new ("updated",
                      G_OBJECT_CLASS_TYPE (g_object_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (HCPAppListClass,updated),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

  g_object_class_install_property (g_object_class,
                                   HCP_APP_LIST_PROP_APPS,
                                   g_param_spec_pointer ("apps",
                                                         "Apps",
                                                         "Set app list",
                                                         G_PARAM_READABLE));

  g_object_class_install_property (g_object_class,
                                   HCP_APP_LIST_PROP_CATEGORIES,
                                   g_param_spec_pointer ("categories",
                                                         "Categories",
                                                         "Set categories list",
                                                         G_PARAM_READABLE));

  g_type_class_add_private (g_object_class, sizeof (HCPAppListPrivate));
}
 
static void 
hcp_app_list_read_desktop_entries (const gchar *dir_path, GHashTable *entries)
{
  GDir *dir;
  GError *error = NULL;
  const char *filename;
  GKeyFile *keyfile;

  ULOG_DEBUG("hcp-app-list:read_desktop_entries");

  g_return_if_fail (dir_path && entries);

  dir = g_dir_open(dir_path, 0, &error);

  if (!dir)
  {
    ULOG_ERR (error->message);
    g_error_free (error);
    return;
  }

  keyfile = g_key_file_new ();

  while ((filename = g_dir_read_name (dir)))
  {
    error = NULL;
    gchar *desktop_path = NULL;
    gchar *name = NULL;
    gchar *plugin = NULL;
    gchar *icon = NULL;
    gchar *category = NULL;

    desktop_path = g_build_filename (dir_path, filename, NULL);

    g_printerr ("Reading: %s\n", desktop_path);

    g_key_file_load_from_file (keyfile,
                               desktop_path,
                               G_KEY_FILE_NONE,
                               &error);

    g_free (desktop_path);

    if (error)
    {
      ULOG_ERR (error->message);
      g_error_free (error);
      continue;
    }

    name = g_key_file_get_locale_string (keyfile,
            HCP_DESKTOP_GROUP,
            HCP_DESKTOP_KEY_NAME,
            NULL /* current locale */,
            &error);

    g_printerr ("  + Name: %s\n", name);

    if (error)
    {
      ULOG_ERR (error->message);
      g_error_free (error);
      continue;
    }

    plugin = g_key_file_get_string (keyfile,
            HCP_DESKTOP_GROUP,
            HCP_DESKTOP_KEY_PLUGIN,
            &error);

    g_printerr ("  + Plugin: %s\n", plugin);

    if (error)
    {
      ULOG_ERR (error->message);
      g_error_free (error);
      continue;
    }

    icon = g_key_file_get_string (keyfile,
            HCP_DESKTOP_GROUP,
            HCP_DESKTOP_KEY_ICON,
            &error);

    g_printerr ("  + Icon: %s\n", icon);

    if (error)
    {
      ULOG_WARN (error->message);
      g_error_free (error);
      error = NULL;
    }
    

    category = g_key_file_get_string (keyfile,
            HCP_DESKTOP_GROUP,
            HCP_DESKTOP_KEY_CATEGORY,
            &error);

    g_printerr ("  + Category: %s\n", category);

    if (error)
    {
      ULOG_WARN (error->message);
      g_error_free (error);
      error = NULL;
    }

    {
      GObject *app = hcp_app_new ();

      g_object_set (G_OBJECT (app), 
                    "name", name,
                    "plugin", plugin,
                    "icon", icon,
                    "category", category,
                    NULL); 

      g_hash_table_insert (entries, plugin, app);
    }

    g_free (name);
    g_free (plugin);
    g_free (icon);
    g_free (category);
  }

  g_key_file_free (keyfile);
  g_dir_close (dir);
}

static gint
hcp_app_list_find_category (gpointer _category, gpointer _app)
{
  HCPCategory *category = (HCPCategory *) _category;
  HCPApp *app = (HCPApp *) _app;
  gchar *category_id = NULL;

  g_object_get (G_OBJECT (app),
                "category", &category_id,
                NULL);

  if (category_id && category->id &&
      !strcmp (category_id, category->id))
  {
    g_printerr ("App found in category %s\n", category_id);
    g_free (category_id);
    return 0;
  }

  g_free (category_id);

  return 1;
}

static void
hcp_app_list_sort_by_category (gpointer key, gpointer value, gpointer _al)
{
  HCPAppList *al = (HCPAppList *) _al;
  GSList *category_item = NULL;
  HCPApp *app = (HCPApp *) value;
  HCPCategory *category; 

  /* Find a category for this applet */
  category_item = g_slist_find_custom (al->priv->categories,
                                       app,
                                       (GCompareFunc) hcp_app_list_find_category);

  if (category_item)
  {
    category = (HCPCategory *) category_item->data;
  }
  else
  {
    /* If category doesn't exist or wasn't matched,
     * add to the default one (Extra) */
    category = g_slist_last (al->priv->categories)->data;
  }
      
  category->apps = g_slist_insert_sorted (
                                  category->apps,
                                  app,
                                  (GCompareFunc) hcp_app_sort_func);
}

void
hcp_app_list_update (HCPAppList *al)
{
  /* Clean the previous list */
  g_hash_table_foreach_remove (al->priv->apps, (GHRFunc) hcp_app_list_free_app, NULL);

  /* Read all the entries */
  hcp_app_list_read_desktop_entries (CONTROLPANEL_ENTRY_DIR, al->priv->apps);

  /* Place them is the relevant category */
  g_slist_foreach (al->priv->categories, (GFunc) hcp_app_list_empty_category, NULL);
  g_hash_table_foreach (al->priv->apps,
                        (GHFunc) hcp_app_list_sort_by_category,
                        al);
}

GObject *
hcp_app_list_new ()
{
  GObject *al = g_object_new (HCP_TYPE_APP_LIST, NULL);

  return al;
}
