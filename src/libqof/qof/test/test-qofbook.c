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
#include "qofbookslots.h"
#include "test-stuff.h"

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

/* mock dirty callback function */
static void
mock_dirty_cb (QofBook *book, gboolean dirty, gpointer user_data)
{
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

static void
test_book_get_counter_format ( Fixture *fixture, gconstpointer pData )
{
    const char *counter_name = "Counter name";
    const char *counter_name_not_set = "Counter name not set";
    gchar *r;
    gint64 counter;
    
    /* need this as long as we have fatal warnings enabled */
    g_test_log_set_fatal_handler ( ( GTestLogFatalFunc )handle_faults, NULL );
    
    g_test_message( "Testing counter format when book is null" );
    r = qof_book_get_counter_format( NULL, counter_name );
    g_assert_cmpstr( r, ==, NULL );
    
    g_test_message( "Testing counter format when counter name is null" );
    r = qof_book_get_counter_format( fixture->book, NULL );
    g_assert_cmpstr( r, ==, NULL );
    
    g_test_message( "Testing counter format when counter name is empty string" );
    r = qof_book_get_counter_format( fixture->book, '\0' );
    g_assert_cmpstr( r, ==, NULL );
    
    g_test_message( "Testing counter format with existing counter" );
    counter = qof_book_get_counter( fixture->book, counter_name );
    r = qof_book_get_counter_format( fixture->book, counter_name );
    g_assert_cmpstr( r, ==, "%.6" G_GINT64_FORMAT);
    
    g_test_message( "Testing counter format for default value" );
    r = qof_book_get_counter_format( fixture->book, counter_name );
    g_assert_cmpstr( r, ==, "%.6" G_GINT64_FORMAT);
}

static void
test_book_increment_and_format_counter ( Fixture *fixture, gconstpointer pData )
{
    const char *counter_name = "Counter name";
    gchar *format;
    gchar *r;
    gint64 counter;
    
    /* need this as long as we have fatal warnings enabled */
    g_test_log_set_fatal_handler ( ( GTestLogFatalFunc )handle_faults, NULL );
    
    g_test_message( "Testing increment and format when book is null" );
    r = qof_book_increment_and_format_counter( NULL, counter_name );
    g_assert_cmpstr( r, ==, NULL );
    g_free( r );
    
    g_test_message( "Testing increment and format when counter name is null" );
    r = qof_book_increment_and_format_counter( fixture->book, NULL );
    g_assert_cmpstr( r, ==, NULL );
    g_free( r );
    
    g_test_message( "Testing increment and format when counter name is empty string" );
    r = qof_book_increment_and_format_counter( fixture->book, '\0' );
    g_assert_cmpstr( r, ==, NULL );
    g_free( r );
    
    g_test_message( "Testing increment and format with new counter" );
    r = qof_book_increment_and_format_counter( fixture->book, counter_name );
    counter = qof_book_get_counter( fixture->book, counter_name );
    format = qof_book_get_counter_format( fixture->book, counter_name );
    g_assert_cmpint( counter, ==, 1 );
    g_assert( qof_book_not_saved( fixture->book ) );
    g_assert_cmpstr( r, ==, g_strdup_printf( format, counter ));
    g_free( r );
    
    g_test_message( "Testing increment and format with existing counter" );
    r = qof_book_increment_and_format_counter( fixture->book, counter_name );
    counter = qof_book_get_counter( fixture->book, counter_name );
    format = qof_book_get_counter_format( fixture->book, counter_name );
    g_assert_cmpint( counter, ==, 2 );
    g_assert_cmpstr( r, ==, g_strdup_printf( format, counter ));
    g_free( r );
}

static void
test_book_kvp_changed( Fixture *fixture, gconstpointer pData )
{
    g_test_message( "Testing book is marked dirty after kvp_changed" );
    g_assert( !qof_book_not_saved( fixture->book ) );
    qof_book_kvp_changed( fixture->book );
    g_assert( qof_book_not_saved( fixture->book ) );
}

static void
test_book_use_trading_accounts( Fixture *fixture, gconstpointer pData )
{
    const char *slot_path;
    
    /* create correct slot path */
    slot_path = (const char *) g_strconcat( KVP_OPTION_PATH, "/", OPTION_SECTION_ACCOUNTS, "/", OPTION_NAME_TRADING_ACCOUNTS, NULL );
    g_assert( slot_path != NULL );
  
    g_test_message( "Testing when no trading accounts are used" );
    g_assert( qof_book_use_trading_accounts( fixture-> book ) == FALSE );
    
    g_test_message( "Testing with incorrect slot path and correct value - t" );
    qof_book_set_string_option( fixture->book, OPTION_NAME_TRADING_ACCOUNTS, "t" );
    g_assert( qof_book_use_trading_accounts( fixture-> book ) == FALSE );
    
    g_test_message( "Testing with existing trading accounts set to true - t" );
    qof_book_set_string_option( fixture->book, slot_path, "t" );
    g_assert( qof_book_use_trading_accounts( fixture-> book ) == TRUE );
    
    g_test_message( "Testing with existing trading accounts and incorrect value - tt" );
    qof_book_set_string_option( fixture->book, slot_path, "tt" );
    g_assert( qof_book_use_trading_accounts( fixture-> book ) == FALSE );
    
}

static void
test_book_mark_dirty( Fixture *fixture, gconstpointer pData )
{
    QofBook *_empty = NULL;
    time_t before, after;
  
    g_test_message( "Testing when book is NULL" );
    qof_book_mark_dirty( _empty );
    g_assert( _empty == NULL );
    
    g_test_message( "Testing when book is not dirty and dirty_cb is null" );
    g_assert_cmpint( qof_book_get_dirty_time( fixture->book ), ==, 0);
    g_assert( fixture->book->dirty_cb == NULL );
    g_assert( qof_book_not_saved( fixture->book ) == FALSE );
    qof_book_mark_dirty( fixture->book );
    g_assert_cmpint( qof_book_get_dirty_time( fixture->book ), !=, 0);
    g_assert( fixture->book->dirty_cb == NULL );
    g_assert( qof_book_not_saved( fixture->book ) == TRUE );
    
    g_test_message( "Testing when book is not dirty and dirty_cb is not null" );
    qof_book_mark_saved( fixture->book );
    qof_book_set_dirty_cb( fixture->book, mock_dirty_cb, NULL );
    g_assert( fixture->book->dirty_cb != NULL );
    g_assert_cmpint( qof_book_get_dirty_time( fixture->book ), ==, 0);
    g_assert( qof_book_not_saved( fixture->book ) == FALSE );
    qof_book_mark_dirty( fixture->book );
    g_assert_cmpint( qof_book_get_dirty_time( fixture->book ), !=, 0);
    g_assert( fixture->book->dirty_cb != NULL );
    g_assert( qof_book_not_saved( fixture->book ) == TRUE );
    
    g_test_message( "Testing when book is dirty" );
    g_assert( qof_book_not_saved( fixture->book ) == TRUE );
    before = qof_book_get_dirty_time( fixture->book );
    qof_book_mark_dirty( fixture->book );
    g_assert( qof_book_not_saved( fixture->book ) == TRUE );
    after = qof_book_get_dirty_time( fixture->book );
    g_assert_cmpint( before, ==, after );
}

static void
test_book_get_dirty_time( Fixture *fixture, gconstpointer pData )
{
    time_t temp_time;
    
    g_test_message( "Testing time on saved book = 0" );
    g_assert( qof_book_not_saved( fixture->book ) == FALSE );
    g_assert_cmpint( qof_book_get_dirty_time( fixture->book ), ==, 0);
    
    g_test_message( "Testing time on dirty book > 0" );
    qof_book_mark_dirty( fixture->book );
    g_assert_cmpint( qof_book_get_dirty_time( fixture->book ), >, 0);
}

static void
test_book_set_dirty_cb( Fixture *fixture, gconstpointer pData )
{
    g_test_message( "Testing when callback is previously not set" );
    g_assert( fixture->book->dirty_cb == NULL );
    qof_book_set_dirty_cb( fixture->book, mock_dirty_cb, NULL );
    g_assert( fixture->book->dirty_cb == mock_dirty_cb );
    g_assert( fixture->book->dirty_data == NULL );
    
    /* need this as long as we have fatal warnings enabled */
    g_test_log_set_fatal_handler ( ( GTestLogFatalFunc )handle_faults, NULL );
    
    g_test_message( "Testing when callback was previously set" );
    g_assert( fixture->book->dirty_cb != NULL );
    qof_book_set_dirty_cb( fixture->book, NULL, NULL );
    g_assert( fixture->book->dirty_cb == NULL );
    g_assert( fixture->book->dirty_data == NULL );
}

void
test_suite_qofbook ( void )
{
    GNC_TEST_ADD( suitename, "readonly", Fixture, NULL, setup, test_book_readonly, teardown );
    GNC_TEST_ADD_FUNC( suitename, "validate counter", test_book_validate_counter );
    GNC_TEST_ADD( suitename, "get string option", Fixture, NULL, setup, test_book_get_string_option, teardown );
    GNC_TEST_ADD( suitename, "set string option", Fixture, NULL, setup, test_book_set_string_option, teardown );
    GNC_TEST_ADD( suitename, "not saved", Fixture, NULL, setup, test_book_not_saved, teardown );
    GNC_TEST_ADD( suitename, "mark saved", Fixture, NULL, setup, test_book_mark_saved, teardown );
    GNC_TEST_ADD( suitename, "get counter", Fixture, NULL, setup, test_book_get_counter, teardown );
    GNC_TEST_ADD( suitename, "get counter format", Fixture, NULL, setup, test_book_get_counter_format, teardown );
    GNC_TEST_ADD( suitename, "increment and format counter", Fixture, NULL, setup, test_book_increment_and_format_counter, teardown );
    GNC_TEST_ADD( suitename, "kvp changed", Fixture, NULL, setup, test_book_kvp_changed, teardown );
    GNC_TEST_ADD( suitename, "use trading accounts", Fixture, NULL, setup, test_book_use_trading_accounts, teardown );
    GNC_TEST_ADD( suitename, "mark dirty", Fixture, NULL, setup, test_book_mark_dirty, teardown );
    GNC_TEST_ADD( suitename, "dirty time", Fixture, NULL, setup, test_book_get_dirty_time, teardown );
    GNC_TEST_ADD( suitename, "set dirty callback", Fixture, NULL, setup, test_book_set_dirty_cb, teardown );
}
