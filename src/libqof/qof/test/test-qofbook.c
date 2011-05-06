/********************************************************************
 * test_qofbook.c: GLib g_test test suite for qofbook.		    *
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
#include <string.h>
#include <glib.h>
#include "qof.h"
#include "qofbook-p.h"

static const gchar *suitename = "/qof/qofbook";
void test_suite_qofbook ( void );

typedef struct
{
    QofBook *book;
} Fixture;

static void
setup( Fixture *fixture, gconstpointer pData )
{
    fixture->book = qof_book_new();
}

static void
teardown( Fixture *fixture, gconstpointer pData )
{
    qof_book_destroy( fixture->book );
}

static gboolean
handle_faults ( const char * log_domain, GLogLevelFlags log_level, const gchar *msg, gpointer user_data)
{
    return FALSE;
}

static void
test_book_readonly( Fixture *fixture, gconstpointer pData )
{
    g_assert( fixture->book != NULL );
    g_assert( !qof_book_is_readonly( fixture->book ) );
    qof_book_mark_readonly( fixture->book );
    g_assert( qof_book_is_readonly( fixture->book ) );
}
static void
test_book_validate_counter( void )
{
    gchar *r;
    g_test_bug("644036");

    /* Test for detection of missing format conversion */
    r = qof_book_validate_counter_format("This string is missing the conversion specifier");
    g_assert(r);
    if (r && g_test_verbose())
    {
        g_test_message("Counter format validation correctly failed: %s", r);
    }
    g_free(r);

    /* Test the usual Linux/Unix G_GINT64_FORMAT */
    r = qof_book_validate_counter_format_internal("Test - %li", "li");
    if (r && g_test_verbose())
    {
        g_test_message("Counter format validation erroneously failed: %s", r);
    }
    g_assert(r == NULL);
    g_free(r);

    /* Test the Windows G_GINT64_FORMAT */
    r = qof_book_validate_counter_format_internal("Test - %I64i", "I64i");
    if (r && g_test_verbose())
    {
        g_test_message("Counter format validation erroneously failed: %s", r);
    }
    g_assert(r == NULL);
    g_free(r);

    /* Test the system's GINT64_FORMAT */
    r = qof_book_validate_counter_format("Test - %" G_GINT64_FORMAT);
    if (r && g_test_verbose())
    {
        g_test_message("Counter format validation erroneously failed: %s", r);
    }
    g_assert(r == NULL);
    g_free(r);

    /* Test an erroneous Windows G_GINT64_FORMAT */
    r = qof_book_validate_counter_format_internal("Test - %li", "I64i");
    if (r && g_test_verbose())
    {
        g_test_message("Counter format validation correctly failed: %s", r);
    }
    g_assert(r);
    g_free(r);

    /* Test an erroneous Linux G_GINT64_FORMAT */
    r = qof_book_validate_counter_format_internal("Test - %I64i", "li");
    if (r && g_test_verbose())
    {
        g_test_message("Counter format validation correctly failed: %s", r);
    }
    g_assert(r);
    g_free(r);
}

static void
test_book_get_string_option( Fixture *fixture, gconstpointer pData )
{
    const char *opt_name = "Option Name";
    const char *opt_value = "Option Value";
    const char *opt_name_notset = "Not Set";
    g_assert( fixture->book != NULL );
    qof_book_set_string_option( fixture->book, opt_name, opt_value);
    g_assert_cmpstr( qof_book_get_string_option( fixture->book, opt_name ), ==, opt_value);
    g_assert_cmpstr( qof_book_get_string_option( fixture->book, opt_name_notset ), ==, NULL );
}

static void
test_book_set_string_option( Fixture *fixture, gconstpointer pData )
{
    const char *opt_name = "Option Name";
    const char *opt_value = "Option Value";
    g_assert( fixture->book != NULL );
    qof_book_set_string_option( fixture->book, opt_name, opt_value);
    g_assert( qof_book_not_saved( fixture->book ) );
}

static void
test_book_not_saved( Fixture *fixture, gconstpointer pData )
{
    const char *opt_name = "Option Name";
    const char *opt_value = "Option Value";
    g_assert( fixture->book != NULL );
    g_assert( !qof_book_not_saved( fixture->book ) );
    qof_book_set_string_option( fixture->book, opt_name, opt_value );
    g_assert( qof_book_not_saved( fixture->book ) );
    qof_book_mark_saved( fixture->book );
    g_assert( !qof_book_not_saved( fixture->book ) );
    qof_book_mark_dirty( fixture-> book );
    g_assert( qof_book_not_saved( fixture->book ) );
}

static void
test_book_mark_saved( Fixture *fixture, gconstpointer pData )
{
    time_t dirty_time, clean_time;
    
    qof_book_mark_dirty( fixture-> book );
    g_assert( qof_book_not_saved( fixture->book ) );
    dirty_time = qof_book_get_dirty_time( fixture->book );
    qof_book_mark_saved( fixture->book );
    clean_time = qof_book_get_dirty_time( fixture->book );
    g_assert( !qof_book_not_saved( fixture->book ) );
    g_assert( dirty_time != clean_time );
    g_assert( clean_time == 0);
}

static void
test_book_get_counter( Fixture *fixture, gconstpointer pData )
{
    const char *counter_name = "Counter name";
    gint64 counter;
    
    /* need this as long as we have fatal warnings enabled */
    g_test_log_set_fatal_handler ( ( GTestLogFatalFunc )handle_faults, NULL );
    
    counter = qof_book_get_counter( NULL, counter_name );
    g_assert_cmpint( counter, ==, -1 );
    
    counter = qof_book_get_counter( fixture->book, NULL );
    g_assert_cmpint( counter, ==, -1 );
    
    counter = qof_book_get_counter( fixture->book, '\0' );
    g_assert_cmpint( counter, ==, -1 );
    
    counter = qof_book_get_counter( fixture->book, counter_name );
    g_assert_cmpint( counter, ==, 0 );
    
    qof_book_increment_and_format_counter( fixture->book, counter_name );
    counter = qof_book_get_counter( fixture->book, counter_name );
    g_assert_cmpint( counter, ==, 1 );
}

void
test_suite_qofbook ( void )
{
    g_test_add( suitename, Fixture, NULL, setup, test_book_readonly, teardown );
    g_test_add_func( suitename, test_book_validate_counter );
    g_test_add( suitename, Fixture, NULL, setup, test_book_get_string_option, teardown );
    g_test_add( suitename, Fixture, NULL, setup, test_book_set_string_option, teardown );
    g_test_add( suitename, Fixture, NULL, setup, test_book_not_saved, teardown );
    g_test_add( suitename, Fixture, NULL, setup, test_book_mark_saved, teardown );
    g_test_add( suitename, Fixture, NULL, setup, test_book_get_counter, teardown );
}
