#pragma once

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define FOOBAR_TYPE_QUICK_ANSWER         foobar_quick_answer_get_type( )
#define FOOBAR_TYPE_QUICK_ANSWER_SERVICE foobar_quick_answer_service_get_type( )

G_DECLARE_FINAL_TYPE( FoobarQuickAnswer, foobar_quick_answer, FOOBAR, QUICK_ANSWER, GObject )

gchar const* foobar_quick_answer_get_value( FoobarQuickAnswer* self );

G_DECLARE_FINAL_TYPE( FoobarQuickAnswerService, foobar_quick_answer_service, FOOBAR, QUICK_ANSWER_SERVICE, GObject )

FoobarQuickAnswerService* foobar_quick_answer_service_new      ( void );
FoobarQuickAnswer*        foobar_quick_answer_service_query    ( FoobarQuickAnswerService* self,
                                                                 gchar const*              query );

G_END_DECLS
