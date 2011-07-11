/********************************************************************
 * test-kvp_frame.c: GLib g_test test suite for kvp_frame.c.		    *
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

static const gchar *suitename = "/qof/kvp_frame";
void test_suite_kvp_frame ( void );

typedef struct
{
    KvpFrame *frame;
} Fixture;

static void
setup( Fixture *fixture, gconstpointer pData )
{
    fixture->frame = kvp_frame_new();
}

static void
teardown( Fixture *fixture, gconstpointer pData )
{
    kvp_frame_delete( fixture->frame );
}

static void
test_kvp_frame_new_delete( void )
{
    KvpFrame *frame;
    Timespec ts;
    GncGUID *guid;
    
    ts.tv_sec = 1;
    ts.tv_nsec = 1;
    guid = guid_malloc();
    guid_new( guid );
    
    frame = kvp_frame_new();
    g_assert( frame );
    g_assert( kvp_frame_is_empty( frame ) );
    
    kvp_frame_set_gint64( frame, "gint64-type", 100 );
    kvp_frame_set_double( frame, "double-type", 3.14159 );
    kvp_frame_set_numeric( frame, "numeric-type", gnc_numeric_zero() );
    kvp_frame_set_timespec( frame, "timespec-type", ts );
    kvp_frame_set_string( frame, "string-type", "abcdefghijklmnop" );
    kvp_frame_set_guid( frame, "guid-type", guid );
    kvp_frame_set_frame( frame, "frame-type", kvp_frame_new() );
    
    g_assert( !kvp_frame_is_empty( frame ) );
    
    kvp_frame_delete( frame );
    g_assert( frame );
    
    guid_free( guid );
}

static void
test_kvp_frame_copy( Fixture *fixture, gconstpointer pData )
{
    KvpFrame *to_copy = NULL;
    gint64 test_gint64, copy_gint64;
    double test_double, copy_double;
    gnc_numeric test_gnc_numeric, copy_gnc_numeric;
    Timespec test_ts, copy_ts;
    const char* test_str, *copy_str;
    GncGUID *test_guid, *copy_guid;
    KvpFrame *test_frame, *copy_frame;
    
    /* init data in source frame */
    test_gint64 = 111;
    test_double = 1.1;
    test_gnc_numeric = gnc_numeric_zero();
    test_ts.tv_sec = 1;
    test_ts.tv_nsec = 1;
    test_str = "abcdefghijklmnop";
    test_guid = guid_malloc();
    guid_new( test_guid );
    test_frame = kvp_frame_new();
    
    g_assert( fixture->frame );
    kvp_frame_set_gint64( fixture->frame, "gint64-type", test_gint64 );
    kvp_frame_set_double( fixture->frame, "double-type", test_double );
    kvp_frame_set_numeric( fixture->frame, "numeric-type", test_gnc_numeric );
    kvp_frame_set_timespec( fixture->frame, "timespec-type", test_ts );
    kvp_frame_set_string( fixture->frame, "string-type", test_str );
    kvp_frame_set_guid( fixture->frame, "guid-type", test_guid );
    kvp_frame_set_frame( fixture->frame, "frame-type", test_frame );
    g_assert( !kvp_frame_is_empty( fixture->frame ) );
    
    g_test_message( "Test frame copy" );
    to_copy = kvp_frame_copy( fixture->frame );
    g_assert( to_copy );
    g_assert( !kvp_frame_is_empty( to_copy ) );
    g_assert( to_copy != fixture->frame );
    g_assert_cmpint( kvp_frame_compare( fixture->frame, to_copy ), ==, 0 );
    copy_gint64 = kvp_frame_get_gint64( to_copy, "gint64-type" );
    g_assert( &copy_gint64 != &test_gint64 );
    g_assert_cmpint( copy_gint64, ==, test_gint64 );
    copy_double = kvp_frame_get_double( to_copy, "double-type" );
    g_assert( &copy_double != &test_double );
    g_assert_cmpfloat( copy_double, ==, test_double );
    copy_gnc_numeric = kvp_frame_get_numeric( to_copy, "numeric-type" );
    g_assert( &copy_gnc_numeric != &test_gnc_numeric );
    g_assert_cmpfloat( copy_gnc_numeric.num, ==, test_gnc_numeric.num );
    g_assert_cmpfloat( copy_gnc_numeric.denom, ==, test_gnc_numeric.denom );
    copy_ts = kvp_frame_get_timespec( to_copy, "timespec-type" );
    g_assert( &copy_ts != &test_ts );
    g_assert_cmpfloat( copy_ts.tv_sec, ==, test_ts.tv_sec );
    g_assert_cmpfloat( copy_ts.tv_nsec, ==, test_ts.tv_nsec );
    copy_str = kvp_frame_get_string( to_copy, "string-type" );
    g_assert( copy_str != test_str );
    g_assert_cmpstr( copy_str, ==, test_str );
    copy_guid = kvp_frame_get_guid( to_copy, "guid-type" );
    g_assert( copy_guid != test_guid );
    g_assert( guid_equal( copy_guid, test_guid ) );
    copy_frame = kvp_frame_get_frame( to_copy, "frame-type");
    g_assert( copy_frame );
    g_assert( kvp_frame_is_empty( copy_frame ) );
    g_assert( copy_frame != test_frame );
    g_assert_cmpint( kvp_frame_compare( copy_frame, test_frame ), ==, 0 );
       
    kvp_frame_delete( to_copy );
    guid_free( test_guid );
}

static void
test_kvp_frame_set_foo( Fixture *fixture, gconstpointer pData )
{
    gnc_numeric test_gnc_numeric, copy_gnc_numeric;
    Timespec test_ts, copy_ts;
    GncGUID *test_guid, *copy_guid;
    
    test_gnc_numeric = gnc_numeric_zero();
    test_ts.tv_sec = 1;
    test_ts.tv_nsec = 1;
    test_guid = guid_malloc();
    guid_new( test_guid );
    
    g_assert( fixture->frame );
    g_assert( kvp_frame_is_empty( fixture->frame ) );
  
    g_test_message( "Test gint64 setup and replace, test frame is created" );
    g_assert( kvp_frame_get_frame( fixture->frame, "/test" ) == NULL );
    kvp_frame_set_gint64( fixture->frame, "/test/gint64", 1 );
    g_assert( kvp_frame_get_frame( fixture->frame, "/test" ) != NULL );
    g_assert_cmpint( kvp_frame_get_gint64( fixture->frame, "/test/gint64" ), ==, 1 );
    kvp_frame_set_gint64( fixture->frame, "/test/gint64", 5 );
    g_assert_cmpint( kvp_frame_get_gint64( fixture->frame, "/test/gint64" ), ==, 5 );
    
    g_test_message( "Test double setup and replace, test2 frame is created" );
    g_assert( kvp_frame_get_frame( fixture->frame, "/test2" ) == NULL );
    kvp_frame_set_double( fixture->frame, "/test2/double", 1.1 );
    g_assert( kvp_frame_get_frame( fixture->frame, "/test2" ) != NULL );
    g_assert_cmpfloat( kvp_frame_get_double( fixture->frame, "/test2/double" ), ==, 1.1 );
    kvp_frame_set_double( fixture->frame, "/test2/double", 5.5 );
    g_assert_cmpfloat( kvp_frame_get_double( fixture->frame, "/test2/double" ), ==, 5.5 );
    
    g_test_message( "Test double setup and replace, test3 frame is created" );
    g_assert( kvp_frame_get_frame( fixture->frame, "/test3" ) == NULL );
    kvp_frame_set_numeric( fixture->frame, "/test3/numeric", test_gnc_numeric );
    g_assert( kvp_frame_get_frame( fixture->frame, "/test3" ) != NULL );
    copy_gnc_numeric = kvp_frame_get_numeric( fixture->frame, "/test3/numeric" );
    g_assert_cmpint( copy_gnc_numeric.num, ==, 0 );
    g_assert_cmpint( copy_gnc_numeric.denom, ==, 1 );
    test_gnc_numeric.num = 2;
    test_gnc_numeric.denom = 3;
    kvp_frame_set_numeric( fixture->frame, "/test3/numeric", test_gnc_numeric );
    copy_gnc_numeric = kvp_frame_get_numeric( fixture->frame, "/test3/numeric" );
    g_assert_cmpint( copy_gnc_numeric.num, ==, 2 );
    g_assert_cmpint( copy_gnc_numeric.denom, ==, 3 );
    
    g_test_message( "Test timespec setup and replace, test4 frame is created" );
    g_assert( kvp_frame_get_frame( fixture->frame, "/test4" ) == NULL );
    kvp_frame_set_timespec( fixture->frame, "/test4/timespec", test_ts );
    g_assert( kvp_frame_get_frame( fixture->frame, "/test4" ) != NULL );
    copy_ts = kvp_frame_get_timespec( fixture->frame, "/test4/timespec" );
    g_assert_cmpint( copy_ts.tv_sec, ==, 1 );
    g_assert_cmpint( copy_ts.tv_nsec, ==, 1 );
    test_ts.tv_sec = 7;
    test_ts.tv_nsec = 13;
    kvp_frame_set_timespec( fixture->frame, "/test4/timespec", test_ts );
    copy_ts = kvp_frame_get_timespec( fixture->frame, "/test4/timespec" );
    g_assert_cmpint( copy_ts.tv_sec, ==, 7 );
    g_assert_cmpint( copy_ts.tv_nsec, ==, 13 );

    g_test_message( "Test string setup and replace, test5 frame is created" );
    g_assert( kvp_frame_get_frame( fixture->frame, "/test5" ) == NULL );
    kvp_frame_set_string( fixture->frame, "/test5/string", "one string" );
    g_assert( kvp_frame_get_frame( fixture->frame, "/test5" ) != NULL );
    g_assert_cmpstr( kvp_frame_get_string( fixture->frame, "/test5/string" ), ==, "one string" );
    kvp_frame_set_string( fixture->frame, "/test5/string", "another string" );
    g_assert_cmpstr( kvp_frame_get_string( fixture->frame, "/test5/string" ), ==, "another string" );
 
    g_test_message( "Test guid setup and replace, test6 frame is created" );
    g_assert( kvp_frame_get_frame( fixture->frame, "/test6" ) == NULL );
    kvp_frame_set_guid( fixture->frame, "/test6/guid", test_guid );
    g_assert( kvp_frame_get_frame( fixture->frame, "/test6" ) != NULL );
    copy_guid = kvp_frame_get_guid( fixture->frame, "/test6/guid" );
    g_assert( guid_equal( copy_guid, test_guid ) );
    kvp_frame_set_guid( fixture->frame, "/test6/guid", guid_null() );
    copy_guid = kvp_frame_get_guid( fixture->frame, "/test6/guid" );
    g_assert( guid_equal( copy_guid, guid_null() ) );
    
    g_test_message( "Test frame setup and replace, test7 frame is created" );
    g_assert( kvp_frame_get_frame( fixture->frame, "/test7" ) == NULL );
    kvp_frame_set_frame( fixture->frame, "/test7", kvp_frame_new() );
    g_assert( kvp_frame_get_frame( fixture->frame, "/test7" ) != NULL );
    kvp_frame_set_frame( fixture->frame, "/test7", NULL );
    g_assert( kvp_frame_get_frame( fixture->frame, "/test7" ) == NULL );
}

static void
test_kvp_frame_get_frame_slash( Fixture *fixture, gconstpointer pData )
{
    KvpFrame *result_frame = NULL;
    /* Mostly testing static routine kvp_frmae_get_frame_slash_trash */
    g_assert( fixture->frame );
    
    g_test_message( "Test path with one slash same frame should be returned" );
    result_frame = kvp_frame_get_frame_slash( fixture->frame, "/" );
    g_assert( result_frame );
    g_assert( result_frame == fixture->frame );
    
    g_test_message( "Test path with trailing slash same frame should be returned" );
    result_frame = kvp_frame_get_frame_slash( fixture->frame, "/////" );
    g_assert( result_frame );
    g_assert( result_frame == fixture->frame );
    
    g_test_message( "Test new frame is created" );
    result_frame = kvp_frame_get_frame_slash( fixture->frame, "/test" );
    g_assert( result_frame );
    g_assert( result_frame != fixture->frame );
    g_assert( result_frame == kvp_frame_get_frame( fixture->frame, "/test" ) );
    
    g_test_message( "Test trailing slashes are ignored and frame created" );
    result_frame = kvp_frame_get_frame_slash( fixture->frame, "////test2/////" );
    g_assert( result_frame );
    g_assert( result_frame != fixture->frame );
    g_assert( result_frame == kvp_frame_get_frame( fixture->frame, "/test2" ) );
    g_assert( result_frame != kvp_frame_get_frame( fixture->frame, "/test" ) );
    
    g_test_message( "Test frames are created along the path if not exist and last frame is returned" );
    result_frame = kvp_frame_get_frame_slash( fixture->frame, "////test3/////test4//////" );
    g_assert( result_frame );
    g_assert( result_frame != fixture->frame );
    g_assert( result_frame != kvp_frame_get_frame( fixture->frame, "/test2" ) );
    g_assert( result_frame != kvp_frame_get_frame( fixture->frame, "/test" ) );
    g_assert( kvp_frame_get_frame( fixture->frame, "/test3" ) != NULL );
    g_assert( result_frame != kvp_frame_get_frame( fixture->frame, "/test3" ) );
    g_assert( result_frame == kvp_frame_get_frame( fixture->frame, "/test3/test4" ) );
    
    g_test_message( "Test existing frame is returned" );
    g_assert( result_frame == kvp_frame_get_frame_slash( fixture->frame, "////test3/////test4//////" ) );
}

static void
test_kvp_frame_get_slot_path( Fixture *fixture, gconstpointer pData )
{
    KvpValue *result_value = NULL ;
  
    g_assert( fixture->frame );
    g_assert( kvp_frame_is_empty( fixture->frame ) );
    
    g_test_message( "Test with non existing path should return NULL" );
    result_value = kvp_frame_get_slot_path( fixture->frame, "test", "test2", NULL );
    g_assert( !result_value );
    
    g_test_message( "Test with existing value set to current frame" );
    kvp_frame_set_gint64( fixture->frame, "/test", 1 );
    result_value = kvp_frame_get_slot_path( fixture->frame, "test", NULL );
    g_assert( result_value );
    g_assert( kvp_value_get_type( result_value ) == KVP_TYPE_GINT64 );
    g_assert_cmpint( kvp_value_get_gint64( result_value ), ==, 1 );
    
    g_test_message( "Test should return null as test is not a frame" );
    kvp_frame_set_gint64( fixture->frame, "/test/test2", 2 );
    result_value = kvp_frame_get_slot_path( fixture->frame, "test", "test2", NULL );
    g_assert( !result_value );
    
    g_test_message( "Test should return last value in the path" );
    kvp_frame_set_gint64( fixture->frame, "/test2/test3", 2 );
    result_value = kvp_frame_get_slot_path( fixture->frame, "test2", "test3", NULL );
    g_assert( result_value );
    g_assert( kvp_value_get_type( result_value ) == KVP_TYPE_GINT64 );
    g_assert_cmpint( kvp_value_get_gint64( result_value ), ==, 2 );
    
    g_test_message( "Test should return null as last value in the path does not exist" );
    result_value = kvp_frame_get_slot_path( fixture->frame, "test2", "test3", "test4", NULL );
    g_assert( !result_value );
}

static void
test_kvp_frame_get_slot_path_gslist( Fixture *fixture, gconstpointer pData )
{
    /* similar to previous test except path is passed as GSList*/
    GSList *path_list = NULL;
    KvpValue *result_value = NULL ;
  
    g_assert( fixture->frame );
    g_assert( kvp_frame_is_empty( fixture->frame ) );
    
    g_test_message( "Test with non existing path should return NULL" );
    path_list = g_slist_append (path_list, "test");
    path_list = g_slist_append (path_list, "test2");
    result_value = kvp_frame_get_slot_path_gslist( fixture->frame, path_list );
    g_assert( !result_value );
    
    g_test_message( "Test with existing value set to current frame" );
    path_list = g_slist_remove( path_list, "test2" );
    kvp_frame_set_gint64( fixture->frame, "/test", 1 );
    result_value = kvp_frame_get_slot_path_gslist( fixture->frame, path_list );
    g_assert( result_value );
    g_assert( kvp_value_get_type( result_value ) == KVP_TYPE_GINT64 );
    g_assert_cmpint( kvp_value_get_gint64( result_value ), ==, 1 );
    
    g_test_message( "Test should return null as test is not a frame" );
    path_list = g_slist_append (path_list, "test2");
    kvp_frame_set_gint64( fixture->frame, "/test/test2", 2 );
    result_value = kvp_frame_get_slot_path_gslist( fixture->frame, path_list );
    g_assert( !result_value );
    
    g_test_message( "Test should return last value in the path" );
    path_list = g_slist_remove( path_list, "test" );
    path_list = g_slist_append (path_list, "test3");
    kvp_frame_set_gint64( fixture->frame, "/test2/test3", 2 );
    result_value = kvp_frame_get_slot_path_gslist( fixture->frame, path_list );
    g_assert( result_value );
    g_assert( kvp_value_get_type( result_value ) == KVP_TYPE_GINT64 );
    g_assert_cmpint( kvp_value_get_gint64( result_value ), ==, 2 );
    
    g_test_message( "Test should return null as last value in the path does not exist" );
    path_list = g_slist_append (path_list, "test4");
    result_value = kvp_frame_get_slot_path_gslist( fixture->frame, path_list );
    g_assert( !result_value );
    g_slist_free( path_list );
}

static void
test_kvp_frame_add_frame_nc( Fixture *fixture, gconstpointer pData )
{
    /* basically we test static function kvp_frame_add_value_nc 
     * if path not exist it's created
     * if gslist exist on the path new value added to the list
     * if any other value exist it's converted to gslist and newvalue added
     */
    KvpFrame *test_frame = NULL, 
	     *test_frame2 = NULL,
	     *test_frame3 = NULL,
	     *result_frame = NULL;
    KvpValue *result_value = NULL;
    GList *result_list = NULL;
    
    g_assert( fixture->frame );
    g_assert( kvp_frame_is_empty( fixture->frame ) );
    
    g_test_message( "Test when path does not exist it is created" );
    result_frame = kvp_frame_get_frame( fixture->frame, "/test/test2/test3" );
    g_assert( !result_frame );
    test_frame = kvp_frame_new();
    kvp_frame_add_frame_nc( fixture->frame, "/test/test2/test3", test_frame );
    result_frame = kvp_frame_get_frame( fixture->frame, "/test/test2/test3" );
    g_assert( result_frame );
    g_assert( result_frame == test_frame ); /* no copying done */
    result_frame = kvp_frame_get_frame( fixture->frame, "/test/test2" );
    g_assert( result_frame != test_frame );
    result_frame = kvp_frame_get_frame( fixture->frame, "/test" );
    g_assert( result_frame != test_frame );
    
    g_test_message( "Test when value exist on the path it's converted to bag and new value added" );
    test_frame2 = kvp_frame_new();
    kvp_frame_add_frame_nc( fixture->frame, "/test/test2/test3", test_frame2 );
    result_value = kvp_frame_get_value( fixture->frame, "/test/test2/test3" );
    result_list = kvp_value_get_glist( result_value );
    g_assert( result_list );
    g_assert_cmpint( g_list_length( result_list ), ==, 2 );
    result_value = g_list_first( result_list )->data;
    g_assert( result_value );
    g_assert( kvp_value_get_type( result_value ) == KVP_TYPE_FRAME );
    g_assert( kvp_value_get_frame( result_value ) == test_frame );
    result_value = g_list_next( result_list )->data;
    g_assert( result_value );
    g_assert( kvp_value_get_type( result_value ) == KVP_TYPE_FRAME );
    g_assert( kvp_value_get_frame( result_value ) == test_frame2 );
    
    g_test_message( "Test when bag exists on the path new values are added to it" );
    test_frame3 = kvp_frame_new();
    kvp_frame_add_frame_nc( fixture->frame, "/test/test2/test3", test_frame3 );
    result_value = kvp_frame_get_value( fixture->frame, "/test/test2/test3" );
    g_assert( result_list == kvp_value_get_glist( result_value ) ); /* same list used */
    g_assert_cmpint( g_list_length( result_list ), ==, 3 );
    result_value = g_list_first( result_list )->data;
    g_assert( result_value );
    g_assert( kvp_value_get_type( result_value ) == KVP_TYPE_FRAME );
    g_assert( kvp_value_get_frame( result_value ) == test_frame );
    result_value = g_list_next( result_list )->data;
    g_assert( result_value );
    g_assert( kvp_value_get_type( result_value ) == KVP_TYPE_FRAME );
    g_assert( kvp_value_get_frame( result_value ) == test_frame2 );
    result_value = g_list_last( result_list )->data;
    g_assert( result_value );
    g_assert( kvp_value_get_type( result_value ) == KVP_TYPE_FRAME );
    g_assert( kvp_value_get_frame( result_value ) == test_frame3 );
}

static void
test_kvp_value_copy( void )
{
    KvpValue *gint64_orig_value, *gint64_copy_value;
    KvpValue *double_orig_value, *double_copy_value;
    KvpValue *numeric_orig_value, *numeric_copy_value;
    KvpValue *string_orig_value, *string_copy_value;
    KvpValue *guid_orig_value, *guid_copy_value;
    KvpValue *timespec_orig_value, *timespec_copy_value;
    KvpValue *glist_orig_value, *glist_copy_value;
    KvpValue *frame_orig_value, *frame_copy_value;
 
    /* data init */
    gnc_numeric gnc_numeric_orig, gnc_numeric_copy;
    GncGUID *guid_orig, *guid_copy;
    Timespec ts_orig, ts_copy;
    GList *list_orig, *list_copy;
    KvpFrame *frame_orig, *frame_copy;
    
    gnc_numeric_orig = gnc_numeric_zero();
    guid_orig = guid_malloc();
    guid_new( guid_orig );
    ts_orig.tv_sec = 1;
    ts_orig.tv_nsec = 1;
    list_orig = NULL;
    list_orig = g_list_append( list_orig, kvp_value_new_string( "abcdefghijklmnop" ) );
    frame_orig = kvp_frame_new();
    
    g_test_message( "Test creates original values and checks copies of them" );
    gint64_orig_value = kvp_value_new_gint64( 2 );
    double_orig_value = kvp_value_new_double( 3.3 );
    numeric_orig_value = kvp_value_new_gnc_numeric( gnc_numeric_orig );
    string_orig_value = kvp_value_new_string( "abcdefghijklmnop" );
    guid_orig_value = kvp_value_new_guid( guid_orig );
    timespec_orig_value = kvp_value_new_timespec( ts_orig );
    glist_orig_value = kvp_value_new_glist( list_orig );
    frame_orig_value = kvp_value_new_frame( frame_orig );
    g_assert( gint64_orig_value && double_orig_value && numeric_orig_value && string_orig_value 
	      && guid_orig_value && timespec_orig_value && glist_orig_value && frame_orig_value );
    
    /* copy values */
    gint64_copy_value = kvp_value_copy( gint64_orig_value );
    g_assert( gint64_copy_value );
    g_assert( gint64_copy_value != gint64_orig_value );
    g_assert( kvp_value_get_type( gint64_copy_value ) == KVP_TYPE_GINT64 );
    g_assert_cmpint( kvp_value_get_gint64( gint64_copy_value ), ==, 2 );
    
    double_copy_value = kvp_value_copy( double_orig_value );
    g_assert( double_copy_value );
    g_assert( double_copy_value != double_orig_value );
    g_assert( kvp_value_get_type( double_copy_value ) == KVP_TYPE_DOUBLE );
    g_assert_cmpfloat( kvp_value_get_double( double_copy_value ), ==, 3.3 );
    
    numeric_copy_value = kvp_value_copy( numeric_orig_value );
    g_assert( numeric_copy_value );
    g_assert( numeric_copy_value != numeric_orig_value );
    g_assert( kvp_value_get_type( numeric_copy_value ) == KVP_TYPE_NUMERIC );
    gnc_numeric_copy = kvp_value_get_numeric( numeric_copy_value );
    g_assert_cmpfloat( gnc_numeric_copy.num, ==, gnc_numeric_orig.num );
    g_assert_cmpfloat( gnc_numeric_copy.denom, ==, gnc_numeric_orig.denom );
    
    string_copy_value = kvp_value_copy( string_orig_value );
    g_assert( string_copy_value );
    g_assert( string_copy_value != string_orig_value );
    g_assert( kvp_value_get_type( string_copy_value ) == KVP_TYPE_STRING );
    g_assert_cmpstr( kvp_value_get_string( string_copy_value ), ==, "abcdefghijklmnop" );
    
    guid_copy_value = kvp_value_copy( guid_orig_value );
    g_assert( guid_copy_value );
    g_assert( guid_copy_value != guid_orig_value );
    g_assert( kvp_value_get_type( guid_copy_value ) == KVP_TYPE_GUID );
    guid_copy = kvp_value_get_guid( guid_copy_value );
    g_assert( guid_orig != guid_copy );
    g_assert( guid_equal( guid_orig, guid_copy ) );
    
    timespec_copy_value = kvp_value_copy( timespec_orig_value );
    g_assert( timespec_copy_value );
    g_assert( timespec_copy_value != timespec_orig_value );
    g_assert( kvp_value_get_type( timespec_copy_value ) == KVP_TYPE_TIMESPEC );
    ts_copy = kvp_value_get_timespec( timespec_copy_value );
    g_assert_cmpfloat( ts_copy.tv_sec, ==, ts_orig.tv_sec );
    g_assert_cmpfloat( ts_copy.tv_nsec, ==, ts_orig.tv_nsec );
    
    glist_copy_value = kvp_value_copy( glist_orig_value );
    g_assert( glist_copy_value );
    g_assert( glist_copy_value != glist_orig_value );
    g_assert( kvp_value_get_type( glist_copy_value ) == KVP_TYPE_GLIST );
    list_copy = kvp_value_get_glist( glist_copy_value );
    g_assert( list_copy != list_orig );
    g_assert_cmpint( g_list_length( list_copy ), ==, g_list_length( list_orig ) );
    g_assert_cmpint( kvp_glist_compare( list_orig, list_copy ), ==, 0 );
    
    frame_copy_value = kvp_value_copy( frame_orig_value );
    g_assert( frame_copy_value );
    g_assert( frame_copy_value != frame_orig_value );
    g_assert( kvp_value_get_type( frame_copy_value ) == KVP_TYPE_FRAME );
    frame_copy = kvp_value_get_frame( frame_copy_value );
    g_assert_cmpint( kvp_frame_compare( frame_orig, frame_copy ), ==, 0 );
    
    /* destroy objects */
    kvp_value_delete( gint64_orig_value );
    kvp_value_delete( double_orig_value );
    kvp_value_delete( numeric_orig_value );
    kvp_value_delete( string_orig_value );
    kvp_value_delete( guid_orig_value );
    kvp_value_delete( timespec_orig_value );
    kvp_value_delete( glist_orig_value );
    kvp_value_delete( frame_orig_value );
    
    kvp_value_delete( gint64_copy_value );
    kvp_value_delete( double_copy_value );
    kvp_value_delete( numeric_copy_value );
    kvp_value_delete( string_copy_value );
    kvp_value_delete( guid_copy_value );
    kvp_value_delete( timespec_copy_value );
    kvp_value_delete( glist_copy_value );
    kvp_value_delete( frame_copy_value );
}

void
test_suite_kvp_frame( void )
{
    GNC_TEST_ADD_FUNC( suitename, "kvp frame new and delete", test_kvp_frame_new_delete );
    GNC_TEST_ADD( suitename, "kvp frame copy", Fixture, NULL, setup, test_kvp_frame_copy, teardown );
    GNC_TEST_ADD( suitename, "kvp frame set foo", Fixture, NULL, setup, test_kvp_frame_set_foo, teardown );
    GNC_TEST_ADD( suitename, "kvp frame get frame slash", Fixture, NULL, setup, test_kvp_frame_get_frame_slash, teardown );
    GNC_TEST_ADD( suitename, "kvp frame get slot path", Fixture, NULL, setup, test_kvp_frame_get_slot_path, teardown );
    GNC_TEST_ADD( suitename, "kvp frame get slot path gslist", Fixture, NULL, setup, test_kvp_frame_get_slot_path_gslist, teardown );
    GNC_TEST_ADD( suitename, "kvp frame add frame nc", Fixture, NULL, setup, test_kvp_frame_add_frame_nc, teardown );
    GNC_TEST_ADD_FUNC( suitename, "kvp value copy", test_kvp_value_copy );
}