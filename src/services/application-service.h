#pragma once

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define FOOBAR_TYPE_APPLICATION_ITEM    foobar_application_item_get_type( )
#define FOOBAR_TYPE_APPLICATION_SERVICE foobar_application_service_get_type( )

G_DECLARE_FINAL_TYPE( FoobarApplicationItem, foobar_application_item, FOOBAR, APPLICATION_ITEM, GObject )

gchar const* foobar_application_item_get_id         ( FoobarApplicationItem* self );
gchar const* foobar_application_item_get_name       ( FoobarApplicationItem* self );
gchar const* foobar_application_item_get_description( FoobarApplicationItem* self );
gchar const* foobar_application_item_get_executable ( FoobarApplicationItem* self );
gchar const* foobar_application_item_get_categories ( FoobarApplicationItem* self );
GIcon*       foobar_application_item_get_icon       ( FoobarApplicationItem* self );
gint64       foobar_application_item_get_frequency  ( FoobarApplicationItem* self );
gboolean     foobar_application_item_match          ( FoobarApplicationItem* self,
                                                      gchar const* const*    terms );
void         foobar_application_item_launch         ( FoobarApplicationItem* self );

G_DECLARE_FINAL_TYPE( FoobarApplicationService, foobar_application_service, FOOBAR, APPLICATION_SERVICE, GObject )

FoobarApplicationService* foobar_application_service_new      ( void );
GListModel*               foobar_application_service_get_items( FoobarApplicationService* self );

G_END_DECLS
