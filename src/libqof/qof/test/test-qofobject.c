/********************************************************************
 * test-qofobject.c: GLib g_test test suite for qofobject.c.		    *
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
********************************************************************/
#include "config.h"
#include <string.h>
#include <glib.h>
#include "test-stuff.h"
#include "qof.h"
#include "qofobject-p.h"

static const gchar *suitename = "/qof/qofobject";
void test_suite_qofobject ( void );

typedef struct
{
    QofObject *qofobject;
} Fixture;

static QofObject*
new_object( QofIdType e_type, const char *type_label)
{
    QofObject *object = NULL;
    
    object = g_new0( QofObject, 1 );
    g_assert( object );
    object->interface_version = QOF_OBJECT_VERSION;
    object->e_type = e_type;
    object->type_label = type_label;
    return object;
}

static void
setup( Fixture *fixture, gconstpointer pData )
{
    qof_object_initialize();
    fixture->qofobject = new_object( "my type object", "object desc" );
}

static void
teardown( Fixture *fixture, gconstpointer pData )
{
    g_free( fixture->qofobject );
    qof_object_shutdown();
}

/*
 * TESTS 
 */
static struct
{
    GList *books;
    guint call_count;
} book_begin_struct;

static void
mock_book_begin( QofBook *book )
{
    g_assert( book );
    g_assert( book == book_begin_struct.books->data );
    book_begin_struct.books = book_begin_struct.books->next;
    book_begin_struct.call_count++;
}

static void
test_qof_object_register( Fixture *fixture, gconstpointer pData )
{
    GList *books = NULL;
    gint32 list_length = g_test_rand_int_range( 0, 5 );
    int i;
    QofObject *simple_object = NULL;    
    
    for (i = 0; i < list_length; i++ )
    {
	QofBook *book = qof_book_new();
	g_assert( book );
	books = g_list_prepend ( books, book );
	g_assert_cmpint( g_list_length( books ), ==, (i + 1) );
    }
    g_assert_cmpint( list_length, ==, g_list_length( books ) );
    
    g_test_message( "Test null check" );
    g_assert( qof_object_register( NULL ) == FALSE );
    
    g_test_message( "Test new object register with book_begin specified" );
    fixture->qofobject->book_begin = mock_book_begin;
    book_begin_struct.books = books;
    book_begin_struct.call_count = 0;
    g_assert( qof_object_register( fixture->qofobject ) == TRUE );
    g_assert( qof_object_lookup( "my type object" ) == fixture->qofobject );
    g_assert_cmpint( book_begin_struct.call_count, ==, list_length );
    
    g_test_message( "Test registering the same object one more time" );
    book_begin_struct.call_count = 0;
    g_assert( qof_object_register( fixture->qofobject ) == FALSE );
    g_assert( qof_object_lookup( "my type object" ) == fixture->qofobject );
    g_assert_cmpint( book_begin_struct.call_count, ==, 0 );
    
    g_test_message( "Test new object register without book_begin specified" );
    simple_object = new_object( "my type simple", "simple desc" );
    g_assert( qof_object_register( simple_object ) == TRUE );
    g_assert( qof_object_lookup( "my type simple" ) == simple_object );
    g_assert_cmpint( book_begin_struct.call_count, ==, 0 );
    
    g_test_message( "Test register simple object one more time" );
    g_assert( qof_object_register( simple_object ) == FALSE );
    g_assert( qof_object_lookup( "my type simple" ) == simple_object );
      
    g_test_message( "Test book begin is called only one time when object is registered" );
    simple_object->book_begin = mock_book_begin;
    book_begin_struct.books = books;
    book_begin_struct.call_count = 0;
    g_assert( qof_object_register( simple_object ) == FALSE );
    g_assert_cmpint( book_begin_struct.call_count, ==, 0 );
    
    g_list_foreach( books, (GFunc) qof_book_destroy, NULL );
    g_list_free( books );
    g_free( simple_object );
}

static void
test_qof_object_lookup( Fixture *fixture, gconstpointer pData )
{
    g_test_message( "Test null check" );
    g_assert( qof_object_lookup( NULL ) == NULL );
    
    g_test_message( "Test existing object lookup" );
    g_assert( qof_object_register( fixture->qofobject ) == TRUE );
    g_assert( qof_object_lookup( "my type object" ) == fixture->qofobject );
    
    g_test_message( "Test non existing object lookup" );
    g_assert( qof_object_lookup( "anytype" ) == NULL );
}

void
test_suite_qofobject (void)
{
    GNC_TEST_ADD( suitename, "qof object register", Fixture, NULL, setup, test_qof_object_register, teardown );
    GNC_TEST_ADD( suitename, "qof object lookup", Fixture, NULL, setup, test_qof_object_lookup, teardown );
}
