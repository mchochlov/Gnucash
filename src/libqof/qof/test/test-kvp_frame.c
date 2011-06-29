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

void
test_suite_kvp_frame( void )
{
    GNC_TEST_ADD_FUNC( suitename, "kvp frame new and delete", test_kvp_frame_new_delete );
    //GNC_TEST_ADD( suitename, kvp hash func, Fixture, NULL, test_kvp_hash_func,  teardown );
}