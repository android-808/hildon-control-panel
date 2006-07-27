/*
 * This file is part of hildon-control-panel
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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

/**
 * SECTION:cp-grid-item
 * @short_description: Creating grid items used by #CPGrid
 * @see_also: #CPGrid
 *
 * CPGridItem is used to create grid items used by #CPGrid.  The
 * grid item consists of an icon and a label. Based on the displaying
 * mode employed by #CPGrid, the label is justified to the right or
 * the bottom.
 */

/*
 * TODO:
 * - play with libtool to get _-functions private but accesable from grid
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libintl.h>
#define _(String) dgettext(PACKAGE, String)

#include <string.h>

/* GTK includes */
#include <gtk/gtklabel.h>
#include <gtk/gtkicontheme.h>
#include <gtk/gtkimage.h>

/* Osso logg functions */
#include <osso-log.h>

#include "cp-grid-item-private.h"
#include "cp-grid-item.h"


#define CP_GRID_ITEM_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), CP_TYPE_GRID_ITEM, \
                                      CPGridItemPrivate))

typedef struct _CPGridItemPrivate CPGridItemPrivate;


/* Default icon. */
#define DEFAULT_ICON_BASENAME  "qgn_list_gene_unknown_file"
#define CP_GRID_ICON_SIZE      26
#define CP_GRID_EMBLEM_SIZE    16

/* Use some alpha-thing for emblems. */
#define USE_DIRTY_ALPHA

struct _CPGridItemPrivate {
    gchar *icon_basename;
    gint icon_size;
    GtkWidget *icon;

    gchar *emblem_basename;
    gint emblem_size;

    GtkWidget *label;   /* TODO use pango! */

    gint focus_margin;
    gint label_height;
    gint label_icon_margin;
    gint column_margin;
    gint icon_width;
    gint icon_height;
    gint row_height;

    gint pending_icon_size;
    gint pending_emblem_size;
    gint pending_focus_margin;
    gint pending_label_height;
    gint pending_label_icon_margin;
    gint pending_icon_width;
    gint pending_icon_height;

    gboolean selected;
};

enum {
    ACTIVATE,
    LAST_SIGNAL
};

enum{
    PROP_NONE = 0,
    PROP_EMBLEM_TYPE
};

static guint grid_item_signals[LAST_SIGNAL] = { 0 };

/* Prototypes. */
static void cp_grid_item_class_init(CPGridItemClass * klass);
static void cp_grid_item_init(CPGridItem * item);
static gboolean cp_grid_item_expose(GtkWidget * widget,
                                    GdkEventExpose * event);
static void cp_grid_item_size_request(GtkWidget * widget,
                                      GtkRequisition * requisition);
static void cp_grid_item_size_allocate(GtkWidget * widget,
                                       GtkAllocation * allocation);
static void cp_grid_item_forall(GtkContainer * container,
                                gboolean include_int,
                                GtkCallback callback,
                                gpointer callback_data);
static void cp_grid_item_remove(GtkContainer * container,
                                GtkWidget * child);

static void cp_grid_item_finalize(GObject * object);

static void update_icon(CPGridItem * item);
static void set_label_justify(CPGridItem * item);

static void cp_grid_item_set_icon_size(CPGridItem *item,
                                       CPGridItemIconSizeType icon_size);

static void cp_grid_item_set_property(GObject * object,
                                      guint prop_id,
                                      const GValue * value,
                                      GParamSpec * pspec);

static void cp_grid_item_get_property(GObject * object,
                                      guint prop_id, 
				      GValue * value,
                                      GParamSpec * pspec);

/* FIXME: no global variables accord. to code conventions */
static GtkContainerClass *parent_class = NULL;


/* Private functions */
static void
cp_grid_item_set_property(GObject * object,
                          guint prop_id,
                          const GValue * value, 
			  GParamSpec * pspec)
{
    CPGridItem *item = CP_GRID_ITEM(object);
    CPGridItemPrivate *priv;
  
    priv = CP_GRID_ITEM_GET_PRIVATE(item);
  
    switch (prop_id) {
        case PROP_EMBLEM_TYPE:
            cp_grid_item_set_emblem_type(item, g_value_get_string(value));
        break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
  }
    
}
  
static void
cp_grid_item_get_property(GObject * object,
                          guint prop_id,
                          GValue * value, 
			  GParamSpec * pspec)
{
    CPGridItem *item = CP_GRID_ITEM(object);
    CPGridItemPrivate *priv;
    const gchar *string;

    priv = CP_GRID_ITEM_GET_PRIVATE(item);
    
    switch (prop_id) {
        case PROP_EMBLEM_TYPE:
            string = cp_grid_item_get_emblem_type(item);
            g_value_set_string(value, string);
        break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}


GType
cp_grid_item_get_type(void)
{
    static GType grid_item_type = 0;

    if (!grid_item_type) {
        static const GTypeInfo grid_item_info = {
            sizeof(CPGridItemClass),
            NULL,       /* base_init */
            NULL,       /* base_finalize */
            (GClassInitFunc) cp_grid_item_class_init,
            NULL,       /* class_finalize */
            NULL,       /* class_data */
            sizeof(CPGridItem),
            0,  /* n_preallocs */
            (GInstanceInitFunc) cp_grid_item_init,
        };
        grid_item_type = g_type_register_static(GTK_TYPE_CONTAINER,
                                                "CPGridItem",
                                                &grid_item_info, 0);
    }

    return grid_item_type;
}

static void
cp_grid_item_class_init(CPGridItemClass *klass)
{
    GtkWidgetClass *widget_class;
    GtkContainerClass *container_class;
    GObjectClass *gobject_class;

    widget_class = GTK_WIDGET_CLASS(klass);
    gobject_class = G_OBJECT_CLASS(klass);
    container_class = GTK_CONTAINER_CLASS(klass);

    parent_class = g_type_class_peek_parent(klass);

    g_type_class_add_private(klass, sizeof(CPGridItemPrivate));

    gobject_class->finalize = cp_grid_item_finalize;

    widget_class->expose_event = cp_grid_item_expose;
    widget_class->size_request = cp_grid_item_size_request;
    widget_class->size_allocate = cp_grid_item_size_allocate;

    container_class->forall = cp_grid_item_forall;
    container_class->remove = cp_grid_item_remove;
  
    gobject_class->set_property = cp_grid_item_set_property;
    gobject_class->get_property = cp_grid_item_get_property;
  
    g_object_class_install_property 
      (gobject_class, 
       PROP_EMBLEM_TYPE, 
       g_param_spec_string ("emblem-type",
			    "Emblem Type",
			    "The emblem's basename",
			    NULL,
			    G_PARAM_WRITABLE));

    grid_item_signals[ACTIVATE] = 
        g_signal_new("activate",
                     G_OBJECT_CLASS_TYPE(gobject_class),
                     G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                     G_STRUCT_OFFSET(CPGridItemClass, activate),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);
                     
  
}

static void
cp_grid_item_init(CPGridItem *item)
{
    CPGridItemPrivate *priv;

    priv = CP_GRID_ITEM_GET_PRIVATE(item);

    priv->icon_basename = NULL;
    priv->pending_icon_size = priv->icon_size = CP_GRID_ICON_SIZE;
    priv->icon = NULL;

    priv->emblem_basename = NULL;
    priv->pending_emblem_size = priv->emblem_size = CP_GRID_EMBLEM_SIZE;

    priv->label = NULL;

    priv->selected = FALSE;
    /* FIXME: define magic numbers do not lottery */
    priv->pending_focus_margin = priv->focus_margin = 6;
    priv->pending_label_height = priv->label_height = 30;
    priv->pending_label_icon_margin = priv->label_icon_margin = 6;
    priv->pending_icon_width = priv->icon_width = 64;
    priv->pending_icon_height = priv->icon_height = 54;
    priv->pending_label_height = priv->label_height = 30;
}

/**
 * cp_grid_item_new:
 * @icon_basename:  icon base name
 *
 * Creates a new #CPGridItem.
 *
 * Returns: a new #CPGridItem
 */
GtkWidget *
cp_grid_item_new(const gchar *icon_basename)
{
    CPGridItem *item;
    CPGridItemPrivate *priv;

    item = g_object_new(CP_TYPE_GRID_ITEM, NULL);
    GTK_WIDGET_SET_FLAGS(GTK_WIDGET(item), GTK_CAN_FOCUS);

    priv = CP_GRID_ITEM_GET_PRIVATE(item);
    priv->icon_basename = g_strdup(icon_basename);

    priv->label = gtk_label_new("");
    gtk_widget_set_parent(priv->label, GTK_WIDGET(item));
    gtk_widget_set_name(priv->label, "cp-grid-item-label");

    update_icon(item);
    set_label_justify(item);

    gtk_widget_show(priv->label);

    return GTK_WIDGET(item);
}

/**
 * cp_grid_item_new_with_label:
 * @icon_basename:  icon base name
 * @label:          text label for icon
 *
 * Creates a new #CPGridItem with a specified label for the icon.
 *
 * Returns: a new #CPGridItem
 */
GtkWidget *
cp_grid_item_new_with_label(const gchar *icon_basename,
                            const gchar *label)
{
    CPGridItem *item;
    CPGridItemPrivate *priv;

    item = g_object_new(CP_TYPE_GRID_ITEM, NULL);
    GTK_WIDGET_SET_FLAGS(GTK_WIDGET(item), GTK_CAN_FOCUS);

    priv = CP_GRID_ITEM_GET_PRIVATE(item);
    priv->icon_basename = g_strdup(icon_basename);

    priv->label = gtk_label_new(label != NULL ? label : "");
    gtk_widget_set_name(priv->label, "cp-grid-item-label");
    gtk_widget_set_parent(priv->label, GTK_WIDGET(item));

    update_icon(item);
    set_label_justify(item);

    gtk_widget_show(priv->label);

    return GTK_WIDGET(item);
}


static void
update_icon(CPGridItem *item)
{
    GtkIconTheme *icon_theme;
    GdkPixbuf *icon;
    GdkPixbuf *emblem_icon;
    CPGridItemPrivate *priv;
    GError *error;

    priv = CP_GRID_ITEM_GET_PRIVATE(item);

    if (priv->icon != NULL) {
        if (GTK_WIDGET_VISIBLE(priv->icon))
            gtk_widget_hide(priv->icon);
        gtk_widget_unparent(priv->icon);
    }

    icon_theme = gtk_icon_theme_get_default();

    /* Load icon. Fall to default if loading fails. */
    error = NULL;
    icon = gtk_icon_theme_load_icon(icon_theme,
                                    priv->icon_basename,
                                    priv->icon_size, 0, &error);
    if (icon == NULL) {
        ULOG_WARN("Couldn't load icon \"%s\": %s", 
		  priv->icon_basename, error->message);
        g_error_free(error);

        error = NULL;
        icon = gtk_icon_theme_load_icon(icon_theme,
                                        DEFAULT_ICON_BASENAME,
                                        priv->icon_size, 0, &error);
        if (icon == NULL) {
            ULOG_WARN("Couldn't load default icon: %s!", error->message);
            g_error_free(error);
        }
    }
    priv->icon_width = gdk_pixbuf_get_width(icon);
    priv->icon_height = gdk_pixbuf_get_height(icon);


    /* Load and merge emblem if one is specified. */
    if (priv->emblem_basename != NULL) {
        error = NULL;
        emblem_icon = gtk_icon_theme_load_icon(icon_theme,
                                               priv->emblem_basename,
                                               priv->emblem_size,
                                               0, &error);
        if (emblem_icon == NULL) {
            ULOG_WARN("Couldn't load emblem \"%s\": %s",
                      priv->emblem_basename, error->message);
            g_error_free(error);
        } else {
            gint icon_height;
            gint width, height, y;

#ifdef USE_DIRTY_ALPHA
            GdkPixbuf *tmp;
#endif

            icon_height = gdk_pixbuf_get_height(icon);
            width = MIN(gdk_pixbuf_get_width(emblem_icon),
                        gdk_pixbuf_get_width(icon));
            height = MIN(gdk_pixbuf_get_height(emblem_icon), icon_height);
            y = icon_height - height;
#ifndef USE_DIRTY_ALPHA
            gdk_pixbuf_copy_area(emblem_icon, 0, 0, width, height,
                                 icon, 0, y);
#else
            /* 
             * Using composite to copy emblem to lower left corner creates
             * some garbage on top of emblem. This way it can be avoided.
             */
            tmp = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE,
                                 8, width, icon_height);
            gdk_pixbuf_fill(tmp, 0x00000000);
            gdk_pixbuf_copy_area(emblem_icon, 0, 0, width, height,
                                 tmp, 0, y);
            gdk_pixbuf_composite(tmp, icon,
                                 0, 0, width, icon_height,
                                 0.0, 0.0, 1.0, 1.0,
                                 GDK_INTERP_NEAREST, 255);
            g_object_unref(tmp);
#endif /* ifndef else USE_DIRTY_ALPHA */
            g_object_unref(emblem_icon);
        }
    }

    priv->icon = gtk_image_new_from_pixbuf(icon);
    g_object_unref(icon);

    gtk_widget_set_parent(priv->icon, GTK_WIDGET(item));
    gtk_widget_show(priv->icon);

    gtk_widget_queue_draw(priv->icon);
}

void
_cp_grid_item_set_label(CPGridItem *item, const gchar *label)
{
    CPGridItemPrivate *priv;

    g_return_if_fail(CP_OBJECT_IS_GRID_ITEM(item));

    priv = CP_GRID_ITEM_GET_PRIVATE(item);
    
    if ( (priv->label == NULL && label == NULL) ||
         (priv->label != NULL && label != NULL &&
          strcmp((char *) priv->label, (char *) label) == 0) ) {
        return;
    }
    
    gtk_label_set_label(GTK_LABEL(priv->label), label);
}

static void
cp_grid_item_set_icon_size(CPGridItem *item, 
		           CPGridItemIconSizeType icon_size)
{
    CPGridItemPrivate *priv;

    g_return_if_fail(CP_OBJECT_IS_GRID_ITEM(item));

    priv = CP_GRID_ITEM_GET_PRIVATE(item);
    if ( priv->pending_icon_size == icon_size ) {
        return;
    }
    priv->pending_icon_size = icon_size;
}


void
_cp_grid_item_set_emblem_size(CPGridItem *item, gint emblem_size)
{
    CPGridItemPrivate *priv;

    g_return_if_fail(CP_OBJECT_IS_GRID_ITEM(item));

    priv = CP_GRID_ITEM_GET_PRIVATE(item);
    if ( priv->pending_emblem_size == emblem_size ) {
        return;
    }
    priv->pending_emblem_size = emblem_size;
}


void
_cp_grid_item_set_focus_margin(CPGridItem *item,
                               const gint focus_margin)
{
    CPGridItemPrivate *priv;

    g_return_if_fail(CP_OBJECT_IS_GRID_ITEM(item));

    priv = CP_GRID_ITEM_GET_PRIVATE(item);
    if ( priv->pending_focus_margin == focus_margin ) {
        return;
    }
    priv->pending_focus_margin = focus_margin;
}


void
_cp_grid_item_set_label_height(CPGridItem *item,
                               const gint label_height)
{
    CPGridItemPrivate *priv;

    g_return_if_fail(CP_OBJECT_IS_GRID_ITEM(item));

    priv = CP_GRID_ITEM_GET_PRIVATE(item);
    if ( priv->pending_label_height == label_height ) {
        return;
    }
    priv->pending_label_height = label_height;
}


void
_cp_grid_item_set_label_icon_margin(CPGridItem *item,
                                    const gint label_icon_margin)
{
    CPGridItemPrivate *priv;

    g_return_if_fail(CP_OBJECT_IS_GRID_ITEM(item));

    priv = CP_GRID_ITEM_GET_PRIVATE(item);
    if ( priv->pending_label_icon_margin == label_icon_margin ) {
        return;
    }
    priv->pending_label_icon_margin = label_icon_margin;
}


void
_cp_grid_item_set_icon_height(CPGridItem *item,
                              const gint icon_height)
{
    CPGridItemPrivate *priv;

    g_return_if_fail(CP_OBJECT_IS_GRID_ITEM(item));

    priv = CP_GRID_ITEM_GET_PRIVATE(item);
    if ( priv->pending_icon_height == icon_height ) {
        return;
    }
    priv->pending_icon_height = icon_height;
}


void
_cp_grid_item_set_icon_width(CPGridItem *item,
                             const gint icon_width)
{
    CPGridItemPrivate *priv;

    g_return_if_fail(CP_OBJECT_IS_GRID_ITEM(item));

    cp_grid_item_set_icon_size(item, icon_width);

    priv = CP_GRID_ITEM_GET_PRIVATE(item);
    if ( priv->pending_icon_width == icon_width ) {
        return;
    }
    priv->pending_icon_width = icon_width;
}


static void
set_label_justify(CPGridItem *item)
{
    CPGridItemPrivate *priv;

    priv = CP_GRID_ITEM_GET_PRIVATE(item);

    if ( priv->label != NULL ) {
        gtk_misc_set_alignment(GTK_MISC(priv->label), 0.0, 0.5);
    }
}

static void
cp_grid_item_remove(GtkContainer *container, GtkWidget *child)
{
    CPGridItem *item;
    CPGridItemPrivate *priv;

    item = CP_GRID_ITEM(container);
    priv = CP_GRID_ITEM_GET_PRIVATE(item);

    g_return_if_fail(GTK_IS_WIDGET(child));
    g_return_if_fail(child == priv->label || child == priv->icon);

    if (child == priv->label) {
        gtk_widget_unparent(child);
        priv->label = NULL;
    } else if (child == priv->icon) {
        gtk_widget_unparent(child);
        priv->icon = NULL;
    }
}

static gboolean
cp_grid_item_expose(GtkWidget *widget, GdkEventExpose *event)
{
    CPGridItem *item;
    CPGridItemPrivate *priv;
    GdkRectangle clip;
    GtkWidget *focused;	     

    g_return_val_if_fail(widget, FALSE);
    g_return_val_if_fail(CP_OBJECT_IS_GRID_ITEM(widget), FALSE);
    g_return_val_if_fail(event, FALSE);

    item = CP_GRID_ITEM(widget);
    priv = CP_GRID_ITEM_GET_PRIVATE(item);

    if ( priv->label == NULL && priv->icon == NULL ) {
        return FALSE;
    }
    if ( GTK_WIDGET_HAS_FOCUS(GTK_WIDGET(item)) ) {
        
        if (priv->label != NULL) {
            focused = priv->label;
        } else {
            focused = priv->icon;
        }

	/* Determine the coordinates and size of clip */ 
        clip.x = widget->allocation.x;
        clip.y = widget->allocation.y;
        clip.width = widget->allocation.width;
        clip.height = widget->allocation.height;

	/* Build painting box for the exposure event */
        gtk_paint_box(focused->style,
                      widget->window,
                      GTK_STATE_SELECTED,
                      GTK_SHADOW_NONE,
                      &clip, focused, "selected",
                      clip.x, clip.y, clip.width, clip.height);
    }

    /* 
     * Items are not exposed unless they are visible.
     * -> No need to "optimize" by checking if they need exposing.
     */
    gtk_container_propagate_expose(GTK_CONTAINER(widget),
                                   priv->icon, event);
    gtk_container_propagate_expose(GTK_CONTAINER(widget),
                                   priv->label, event);
    return TRUE;
}


static void
cp_grid_item_size_request(GtkWidget *widget, GtkRequisition *requisition)
{
    CPGridItem *item;
    CPGridItemPrivate *priv;
    GtkRequisition label_req;
    gint label_margin;

    item = CP_GRID_ITEM(widget);
    priv = CP_GRID_ITEM_GET_PRIVATE(item);

    label_margin = priv->focus_margin;

    gtk_widget_size_request(priv->icon, requisition);
    gtk_widget_size_request(priv->label, &label_req);

    requisition->width += label_req.width + label_margin;
    requisition->height = MAX(requisition->height, label_req.height);
}

static void
cp_grid_item_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
    CPGridItem *item;
    CPGridItemPrivate *priv;
    GtkRequisition l_req;
    GtkAllocation i_alloc, l_alloc;

    g_return_if_fail(widget);
    g_return_if_fail(allocation);

    item = CP_GRID_ITEM(widget);
    priv = CP_GRID_ITEM_GET_PRIVATE(item);
    widget->allocation = *allocation;

    /* If creating label and icon failed, don't show a thing... */
    if (priv->label == NULL && priv->icon == NULL) {
        return;
    }
    if (priv->label != NULL) {
        gtk_widget_get_child_requisition(priv->label, &l_req);
    } else {
        l_req.width = l_req.height = 0;
    }

    /* Determine icon and label allocation based on label position */
    i_alloc.x = allocation->x + priv->focus_margin;
    i_alloc.y = allocation->y; /* +
        (priv->label_height - priv->icon_height) / 2;*/

    if (priv->label != NULL) {
        l_alloc.x = allocation->x + priv->focus_margin +
            priv->icon_width + priv->label_icon_margin;
        l_alloc.y = allocation->y;
        l_alloc.width = allocation->width - priv->focus_margin * 2 -
            priv->label_icon_margin - priv->icon_width;
        l_alloc.height = priv->label_height;
    }

    if (i_alloc.y < allocation->y) {
        i_alloc.height -= i_alloc.height - allocation->height;
        i_alloc.y = allocation->y;
    }
    if (i_alloc.y + i_alloc.height > allocation->y + allocation->height) {
        i_alloc.height-= i_alloc.y + i_alloc.height -
            allocation->y - allocation->height;
    }
      

    i_alloc.width = priv->icon_width;
    i_alloc.height = priv->icon_height;

    if (priv->label != NULL) {
        gtk_widget_size_allocate(priv->label, &l_alloc);
    }
    if (priv->icon != NULL) {
        gtk_widget_size_allocate(priv->icon, &i_alloc);
    }
}

static void
cp_grid_item_forall(GtkContainer *container,
                    gboolean include_int,
                    GtkCallback callback,
                    gpointer callback_data)
{
    CPGridItem *item;
    CPGridItemPrivate *priv;

    g_return_if_fail(container);
    g_return_if_fail(callback);

    item = CP_GRID_ITEM(container);
    priv = CP_GRID_ITEM_GET_PRIVATE(item);

    /* Connect callback functions to the item */
    if (priv->icon != NULL) {
        (*callback) (priv->icon, callback_data);
    }
    if (priv->label != NULL) {
        (*callback) (priv->label, callback_data);
    }
}

static void
cp_grid_item_finalize(GObject *object)
{
    CPGridItem *item;
    CPGridItemPrivate *priv;

    item = CP_GRID_ITEM(object);
    priv = CP_GRID_ITEM_GET_PRIVATE(item);

    g_free(priv->icon_basename);
    if (priv->emblem_basename != NULL) {
        g_free(priv->emblem_basename);
    }

    G_OBJECT_CLASS(parent_class)->finalize(object);
}

#if 0
static int cp_time_get_font_width(GtkWidget * widget)
{
    PangoContext *context;
    PangoFontMetrics *metrics;
    gint digit_width;

    context = gtk_widget_get_pango_context(widget);
    metrics = pango_context_get_metrics(context,
                                        widget->style->font_desc,
                                        pango_context_get_language
                                        (context));

    digit_width = pango_font_metrics_get_approximate_digit_width(metrics);
    digit_width = PANGO_PIXELS(digit_width);

    pango_font_metrics_unref(metrics);

    return digit_width;
}
#endif


/**
 * cp_grid_item_set_emblem_type:
 * @item:               #CPGridItem
 * @emblem_basename:    emblem's basename
 *
 * Sets item emblem type.
 */
void
cp_grid_item_set_emblem_type(CPGridItem *item,
                             const gchar *emblem_basename)
{
    CPGridItemPrivate *priv;

    g_return_if_fail(CP_OBJECT_IS_GRID_ITEM(item));

    priv = CP_GRID_ITEM_GET_PRIVATE(item);

    if (priv->emblem_basename != NULL) {
        g_free(priv->emblem_basename);
    }

    priv->emblem_basename = g_strdup(emblem_basename);

    update_icon(item);

    g_object_notify (G_OBJECT (item), "emblem-type");
}

/**
 * cp_grid_item_get_emblem_type:
 * @item:   #CPGridItem
 *
 * Returns: emblem's basename. Must not be changed or freed.
 */
const gchar *
cp_grid_item_get_emblem_type(CPGridItem *item)
{
    g_return_val_if_fail(CP_OBJECT_IS_GRID_ITEM(item), NULL);

    return CP_GRID_ITEM_GET_PRIVATE(item)->emblem_basename;
}



void
_cp_grid_item_done_updating_settings(CPGridItem *item)
{
    gboolean need_update_icon;
    gboolean need_resize;

    CPGridItemPrivate *priv;
    g_return_if_fail(CP_OBJECT_IS_GRID_ITEM(item));
    priv = CP_GRID_ITEM_GET_PRIVATE(item);

    need_update_icon = need_resize = FALSE;
    
    if (priv->pending_icon_size != priv->icon_size) {
        if (priv->pending_icon_size > 0) {
            priv->icon_size = priv->pending_icon_size;
        } else {
            priv->icon_size = 1;
        }
        need_update_icon = TRUE;
    }
    if (priv->pending_emblem_size != priv->emblem_size) {
        priv->emblem_size = priv->pending_emblem_size;
        need_update_icon = TRUE;
    }
    /*
     * grid will take care of this
     *
    if (priv->pending_focus_margin != priv->focus_margin) {
        priv->focus_margin = priv->pending_focus_margin;
        need_resize = TRUE;
    }
    if (priv->pending_label_height != priv->label_height) {
        priv->label_height = priv->pending_label_height;
        need_resize = TRUE;
    }
    if (priv->pending_label_icon_margin != priv->label_icon_margin) {
        priv->label_icon_margin = priv->pending_label_icon_margin;
        need_resize = TRUE;
    }
    if (priv->pending_icon_height != priv->icon_height) {
        priv->icon_height = priv->pending_icon_height;
        need_resize = TRUE;
    }
    if (priv->pending_icon_width != priv->icon_width) {
        priv->icon_width = priv->pending_icon_width;
        need_resize = TRUE;
    }
     */

    if (need_update_icon == TRUE) {
        update_icon(CP_GRID_ITEM(item));
    }
    /*
    if (need_resize == TRUE) {
        gtk_widget_queue_resize(GTK_WIDGET(item));
    }
    */
}