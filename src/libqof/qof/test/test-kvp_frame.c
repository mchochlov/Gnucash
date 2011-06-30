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

void
test_suite_kvp_frame( void )
{
    GNC_TEST_ADD_FUNC( suitename, "kvp frame new and delete", test_kvp_frame_new_delete );
    GNC_TEST_ADD( suitename, "kvp frame copy", Fixture, NULL, setup, test_kvp_frame_copy, teardown );
}