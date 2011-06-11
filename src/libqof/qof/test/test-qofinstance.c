/********************************************************************
 * test_qofinstance.c: GLib g_test test suite for qofinstance.	    *
 * Copyright 2011 John Ralls <jralls@ceridwen.us>		    *
 *                                                                  *
 * This program is free software; you can redistribute it and/or    *
 * modify it under the terms of the GNU General Public License as   *
 * published by the Free Software Foundation; either version 2 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU General Public License for more details.                     *
 *                                                                  *
 * You should have received a copy of the GNU General Public License*
 * along with this program; if not, contact:                        *
 *                                                                  *
 * Free Software Foundation           Voice:  +1-617-542-5942       *
 * 51 Franklin Street, Fifth Floor    Fax:    +1-617-542-2652       *
 * Boston, MA  02110-1301,  USA       gnu@gnu.org                   *
\********************************************************************/
#include "config.h"
#include <glib.h>
#include "qof.h"
#include "test-stuff.h"

static const gchar *suitename = "/qof/qofinstance";
void test_suite_qofinstance ( void );
static gchar* error_message;

typedef struct
{
    GObject parent_instance;
} MockObject;

typedef struct
{
    GObjectClass parent_class;
} MockObjectClass;

#define MOCK_TYPE_OBJECT ( mock_object_get_type() )

GType mock_object_get_type( void );

GType
mock_object_get_type ( void )
{
    static GType type = 0;
    static const GTypeInfo info = {
	sizeof( MockObjectClass ),
	NULL,
	NULL,
	(GClassInitFunc) NULL,
	NULL,
	NULL,
	sizeof( MockObject ),
	0,
	(GInstanceInitFunc) NULL
    };
    type = g_type_register_static( G_TYPE_OBJECT, "MockObject", &info, 0);
    return type;
}

typedef struct
{
    QofInstance *instance;
} Fixture;

/* use g_free on error_message after this function been called */
static gboolean
fatal_handler ( const char * log_domain, 
		GLogLevelFlags log_level, 
		const gchar *msg, 
		gpointer user_data)
{
    error_message = (gchar *) g_strdup( msg );
    return FALSE;
}

static void
setup( Fixture *fixture, gconstpointer pData )
{
    fixture->instance = g_object_new(QOF_TYPE_INSTANCE, NULL);
    qof_instance_set_book( fixture->instance, qof_book_new() );
}

static void
teardown( Fixture *fixture, gconstpointer pData )
{
    qof_book_destroy( qof_instance_get_book( fixture->instance ) );
    g_object_unref(fixture->instance);
}

static void
test_book_readonly( Fixture *fixture, gconstpointer pData )
{
    QofBook *book = qof_instance_get_book( fixture->instance );
    g_assert( !qof_book_is_readonly( book ) );
    qof_book_mark_readonly( book );
    g_assert( qof_book_is_readonly( book ) );
}

static void
test_instance_set_get_book( void )
{
    QofInstance *inst;
    QofBook *book;
    
    /* set up */
    book = qof_book_new();
    inst = g_object_new(QOF_TYPE_INSTANCE, NULL);
    
    g_assert( QOF_IS_INSTANCE( inst ) );
    
    g_test_message( "Setting and getting book" );
    qof_instance_set_book( inst, book );
    g_assert( book == qof_instance_get_book( inst ) );
    
    g_test_message( "Getting book when instance is null" );
    g_assert( qof_instance_get_book( NULL ) == NULL );
    
    /* Clean up */
    qof_book_destroy( book );
    g_object_unref( inst );
}

static void
test_instance_set_get_guid( void )
{
    QofInstance *inst;
    GncGUID *gncGuid;
    
    /* on null instance deprecated getter returns empty guid
     * while instance_get_guid returns null
     */
    g_assert( !qof_instance_get_guid( NULL ) );
    g_assert( qof_entity_get_guid( NULL ) == guid_null() );
    
    /* set up */
    gncGuid = guid_malloc();
    guid_new( gncGuid );
    inst = g_object_new(QOF_TYPE_INSTANCE, NULL);    
    g_assert( QOF_IS_INSTANCE( inst ) );
    g_assert( gncGuid );
    
    /* guid already exists after instance init */
    g_test_message( "Setting new guid" );
    g_assert( qof_instance_get_guid( inst ) );
    g_assert( !guid_equal( gncGuid, qof_instance_get_guid( inst ) ) );
    qof_instance_set_guid( inst, gncGuid );
    g_assert( guid_equal( gncGuid, qof_instance_get_guid( inst ) ) );
    g_assert( guid_equal( gncGuid, qof_entity_get_guid( inst ) ) );
    
    /* Clean up */
    guid_free( gncGuid );
    g_object_unref( inst );
}

static void
test_instance_new_destroy( void )
{
    /* qofinstance var */
    QofInstance *inst;
    QofInstanceClass *klass;
    /* test var */
    Timespec timespec_priv;
  
    g_test_message( "Testing qofinstance object initialization" );
    inst = g_object_new(QOF_TYPE_INSTANCE, NULL);
    g_assert( QOF_IS_INSTANCE( inst ) );
    /* test class fields */
    klass = QOF_INSTANCE_GET_CLASS( inst );
    g_assert( QOF_IS_INSTANCE_CLASS( klass ) );
    g_assert( klass->get_display_name == NULL );
    g_assert( klass->refers_to_object == NULL );
    g_assert( klass->get_typed_referring_object_list == NULL );
    /* testing initial values */
    g_assert( qof_instance_get_guid( inst ) );
    g_assert( qof_instance_get_collection( inst ) );
    g_assert( qof_instance_get_book( inst ) == NULL );
    g_assert( inst->kvp_data );
    timespec_priv = qof_instance_get_last_update( inst );
    g_assert_cmpint( timespec_priv.tv_sec, ==, 0 );
    g_assert_cmpint( timespec_priv.tv_nsec, ==, -1 );
    g_assert_cmpint( qof_instance_get_editlevel( inst ), ==, 0 );
    g_assert( !qof_instance_get_destroying( inst ) );
    g_assert( !qof_instance_get_dirty_flag( inst ) );
    g_assert( qof_instance_get_infant( inst ) );
    g_assert_cmpint( qof_instance_get_version( inst ), ==, 0 );
    g_assert_cmpint( qof_instance_get_version_check( inst ), ==, 0 );
    g_assert_cmpint( qof_instance_get_idata( inst ), ==, 0 );
    
    g_test_message( "Testing object destruction" );
    g_object_unref( inst );
    /* test fields were deinitialized */
    g_assert( inst );
    g_assert( qof_instance_get_collection( inst ) == NULL );
    g_assert( inst->e_type == NULL );
    g_assert( inst->kvp_data == NULL );
    g_assert_cmpint( qof_instance_get_editlevel( inst ), ==, 0 );
    g_assert( !qof_instance_get_destroying( inst ) );
    g_assert( !qof_instance_get_dirty_flag( inst ) );
    
}

void
test_suite_qofinstance ( void )
{
    GNC_TEST_ADD( suitename, "book readonly", Fixture, NULL, setup, test_book_readonly, teardown );
    GNC_TEST_ADD_FUNC( suitename, "set get book", test_instance_set_get_book );
    GNC_TEST_ADD_FUNC( suitename, "set get guid", test_instance_set_get_guid );
    GNC_TEST_ADD_FUNC( suitename, "instance new and destroy", test_instance_set_get_guid );
}
