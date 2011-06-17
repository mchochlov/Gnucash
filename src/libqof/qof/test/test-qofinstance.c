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
    QofInstance *inst;
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
    fixture->inst = g_object_new(QOF_TYPE_INSTANCE, NULL);
}

static void
teardown( Fixture *fixture, gconstpointer pData )
{
    g_object_unref(fixture->inst);
}

static void
test_instance_set_get_book( Fixture *fixture, gconstpointer pData )
{
    QofBook *book;
    
    /* set up */
    book = qof_book_new();
    
    g_assert( QOF_IS_INSTANCE( fixture->inst ) );
    
    g_test_message( "Setting and getting book" );
    qof_instance_set_book( fixture->inst, book );
    g_assert( book == qof_instance_get_book( fixture->inst ) );
    
    g_test_message( "Getting book when instance is null" );
    g_assert( qof_instance_get_book( NULL ) == NULL );
    
    /* Clean up */
    qof_book_destroy( book );
}

static void
test_instance_set_get_guid( Fixture *fixture, gconstpointer pData )
{
    GncGUID *gncGuid;
    
    /* on null instance deprecated getter returns empty guid
     * while instance_get_guid returns null
     */
    g_assert( !qof_instance_get_guid( NULL ) );
    g_assert( qof_entity_get_guid( NULL ) == guid_null() );
    
    /* set up */
    gncGuid = guid_malloc();
    guid_new( gncGuid );
    g_assert( QOF_IS_INSTANCE( fixture->inst ) );
    g_assert( gncGuid );
    
    /* guid already exists after instance init */
    g_test_message( "Setting new guid" );
    g_assert( qof_instance_get_guid( fixture->inst ) );
    g_assert( !guid_equal( gncGuid, qof_instance_get_guid( fixture->inst ) ) );
    qof_instance_set_guid( fixture->inst, gncGuid );
    g_assert( guid_equal( gncGuid, qof_instance_get_guid( fixture->inst ) ) );
    g_assert( guid_equal( gncGuid, qof_entity_get_guid( fixture->inst ) ) );
    
    /* Clean up */
    guid_free( gncGuid );
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
    g_assert( !qof_instance_get_collection( inst ) );
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
    g_assert( !QOF_IS_INSTANCE( inst ) );
    /* set fatal handler */
    g_test_log_set_fatal_handler ( ( GTestLogFatalFunc )fatal_handler, NULL );
    g_assert( qof_instance_get_collection( inst ) == NULL );
    g_assert( g_strrstr( error_message, "assertion `QOF_IS_INSTANCE(ptr)' failed" ) != NULL );
    g_free( error_message );
    
    g_assert( inst->e_type == NULL );
    g_assert( inst->kvp_data == NULL );
    g_assert_cmpint( qof_instance_get_editlevel( inst ), ==, 0 );
    g_assert( g_strrstr( error_message, "assertion `QOF_IS_INSTANCE(ptr)' failed" ) != NULL );
    g_free( error_message );
    
    g_assert( !qof_instance_get_destroying( inst ) );
    g_assert( g_strrstr( error_message, "assertion `QOF_IS_INSTANCE(ptr)' failed" ) != NULL );
    g_free( error_message );
    
    g_assert( !qof_instance_get_dirty_flag( inst ) );
    g_assert( g_strrstr( error_message, "assertion `QOF_IS_INSTANCE(ptr)' failed" ) != NULL );
    g_free( error_message );
}

static void
test_instance_init_data( void )
{
    QofInstance *inst;
    QofIdType test_type = "test type";
    QofBook *book;
    QofCollection *col;
    const GncGUID *gncguid;
    char guid_id_before[GUID_ENCODING_LENGTH + 1];
    char guid_id_after[GUID_ENCODING_LENGTH + 1];
    
    /* set up */
    inst = g_object_new( QOF_TYPE_INSTANCE, NULL );
    g_assert( QOF_IS_INSTANCE( inst ) );
    book = qof_book_new();
    g_assert( QOF_IS_BOOK( book ) );
    /* set fatal handler */
    g_test_log_set_fatal_handler ( ( GTestLogFatalFunc )fatal_handler, NULL );
    
    g_test_message( "Running test with correct initial data" );
    gncguid = qof_instance_get_guid( inst );
    g_assert( gncguid );
    guid_to_string_buff( gncguid, guid_id_before );
    g_assert( qof_instance_get_book( inst ) == NULL );
    g_assert( qof_instance_get_collection( inst ) == NULL );
    /* run init */
    qof_instance_init_data( inst, test_type, book );
    
    g_assert( qof_instance_get_book( inst ) == book );
    guid_to_string_buff( gncguid, guid_id_after );
    g_assert_cmpstr( guid_id_before, !=, guid_id_after );
    g_assert( qof_instance_get_collection( inst ) != NULL );
    col = qof_book_get_collection( book, test_type );
    g_assert( col );
    g_assert( col == qof_instance_get_collection( inst ) );
    g_assert_cmpstr( inst->e_type, ==, test_type );
    g_assert( qof_collection_lookup_entity( qof_instance_get_collection( inst ), gncguid ) == inst );
        
    /* clean up */
    g_object_unref( inst );
    qof_book_destroy( book );
}

static void
test_instance_get_set_slots( Fixture *fixture, gconstpointer pData )
{
    KvpFrame *kvp_frame, *kvp_frame2;
    
    /* set up */
    g_assert( fixture->inst );
    kvp_frame = qof_instance_get_slots( fixture->inst );
    g_assert( kvp_frame );
    
    g_test_message( "Test when kvp frame is the same" );
    qof_instance_set_slots( fixture->inst, kvp_frame );
    g_assert( kvp_frame == qof_instance_get_slots( fixture->inst ) );
    g_assert( qof_instance_get_dirty_flag( fixture->inst ) );

    g_test_message( "Test when kvp frame is not the same" );
    kvp_frame2 = kvp_frame_new();
    g_assert( kvp_frame != kvp_frame2 );
    qof_instance_set_slots( fixture->inst, kvp_frame2 );
    g_assert( kvp_frame2 == qof_instance_get_slots( fixture->inst ) );
    g_assert( qof_instance_get_dirty_flag( fixture->inst ) );
    
    g_test_message( "Test when kvp frame is null" );
    qof_instance_set_slots( fixture->inst, NULL );
    g_assert( NULL == qof_instance_get_slots( fixture->inst ) );
    g_assert( qof_instance_get_dirty_flag( fixture->inst ) );
    
}

static void
test_instance_version_cmp( void )
{
    QofInstance *left, *right;
    int result;
    Timespec timespec_left, timespec_right;
    
    /* set up*/
    left = g_object_new( QOF_TYPE_INSTANCE, NULL );
    right = g_object_new( QOF_TYPE_INSTANCE, NULL );
    
    g_test_message( "Test both null" );
    result = qof_instance_version_cmp( NULL, NULL );
    g_assert_cmpint( result, ==, 0 );
    
    g_test_message( "Test left null" );
    result = qof_instance_version_cmp( NULL, right );
    g_assert_cmpint( result, ==, -1 );
    
    g_test_message( "Test right null" );
    result = qof_instance_version_cmp( left, NULL );
    g_assert_cmpint( result, ==, 1 );
    
    g_test_message( "Test left tv_sec lesser than right" );
    timespec_left.tv_sec = 0;
    timespec_right.tv_sec = 1;
    qof_instance_set_last_update( left, timespec_left );
    qof_instance_set_last_update( right, timespec_right );
    result = qof_instance_version_cmp( left, right );
    g_assert_cmpint( result, ==, -1 );
    
    g_test_message( "Test right tv_sec lesser than left" );
    timespec_left.tv_sec = 1;
    timespec_right.tv_sec = 0;
    qof_instance_set_last_update( left, timespec_left );
    qof_instance_set_last_update( right, timespec_right );
    result = qof_instance_version_cmp( left, right );
    g_assert_cmpint( result, ==, 1 );
    
    g_test_message( "Test left tv_nsec lesser than right" );
    timespec_left.tv_sec = 1;
    timespec_left.tv_nsec = 0;
    timespec_right.tv_sec = 1;
    timespec_right.tv_nsec = 1;
    qof_instance_set_last_update( left, timespec_left );
    qof_instance_set_last_update( right, timespec_right );
    result = qof_instance_version_cmp( left, right );
    g_assert_cmpint( result, ==, -1 );
    
    g_test_message( "Test right tv_sec lesser than left" );
    timespec_left.tv_sec = 1;
    timespec_left.tv_nsec = 1;
    timespec_right.tv_sec = 1;
    timespec_right.tv_nsec = 0;
    qof_instance_set_last_update( left, timespec_left );
    qof_instance_set_last_update( right, timespec_right );
    result = qof_instance_version_cmp( left, right );
    g_assert_cmpint( result, ==, 1 );
    
    g_test_message( "Test both equal" );
    timespec_left.tv_sec = 1;
    timespec_left.tv_nsec = 1;
    timespec_right.tv_sec = 1;
    timespec_right.tv_nsec = 1;
    qof_instance_set_last_update( left, timespec_left );
    qof_instance_set_last_update( right, timespec_right );
    result = qof_instance_version_cmp( left, right );
    g_assert_cmpint( result, ==, 0 );
    
    /* clear */
    g_object_unref( left );
    g_object_unref( right );
}

static void
test_instance_get_set_dirty( Fixture *fixture, gconstpointer pData )
{
    QofIdType type = "test type";
    QofCollection *col;
    
    /* setup */
    col = qof_collection_new ( type );
    qof_instance_set_collection( fixture->inst, col );
    g_assert( qof_instance_get_collection( fixture->inst ) );
    
    g_test_message( "Test get_dirty on empty instance returns false" );
    g_assert( qof_instance_get_dirty( NULL ) == FALSE );
    
    g_test_message( "Test dirty in normal mode" );
    g_assert( !qof_get_alt_dirty_mode() ); 
    g_assert( !qof_instance_get_dirty_flag( fixture->inst ) );
    g_assert( !qof_collection_is_dirty( col ) );
    g_assert( !qof_instance_get_dirty( fixture->inst ) );
    qof_instance_set_dirty( fixture->inst );
    g_assert( qof_instance_get_dirty_flag( fixture->inst ) );
    g_assert( qof_collection_is_dirty( col ) );
    g_assert( qof_instance_get_dirty( fixture->inst ) );
    
    
    g_test_message( "Test dirty in alternate mode" );
    qof_set_alt_dirty_mode ( TRUE );
    /* restore */
    qof_collection_mark_clean( col );
    qof_instance_set_dirty_flag( fixture->inst, FALSE );
    
    g_assert( qof_get_alt_dirty_mode() ); 
    g_assert( !qof_instance_get_dirty_flag( fixture->inst ) );
    g_assert( !qof_collection_is_dirty( col ) );
    g_assert( !qof_instance_get_dirty( fixture->inst ) );
    qof_instance_set_dirty( fixture->inst );
    g_assert( qof_instance_get_dirty_flag( fixture->inst ) );
    g_assert( !qof_collection_is_dirty( col ) );
    g_assert( qof_instance_get_dirty( fixture->inst ) );
    
    /* clean up */
    qof_instance_set_collection( fixture->inst, NULL );
    qof_collection_destroy( col );
}

static void
test_instance_gemini_and_lookup( void )
{
    QofBook *to_book, *from_book;
    KvpFrame *to_book_frame, *from_book_frame;
    
    /* setup books */
    to_book = qof_book_new();
    from_book = qof_book_new();
    g_assert( QOF_IS_BOOK( to_book ) );
    g_assert( QOF_IS_BOOK( from_book ) );
    g_assert( to_book != from_book );
    g_assert( to_book == qof_instance_get_book( QOF_INSTANCE( to_book ) ) );
    g_assert( from_book == qof_instance_get_book( QOF_INSTANCE( from_book ) ) );
    
    g_test_message( "Test instances lookup has instance and book null protection" );
    g_assert( qof_instance_lookup_twin( NULL, from_book ) == NULL );
    g_assert( qof_instance_lookup_twin( QOF_INSTANCE( from_book ), NULL ) == NULL );
    
    g_test_message( "Test instances lookup which are not geminis should return null" );
    g_assert( qof_instance_lookup_twin( QOF_INSTANCE( to_book ), from_book ) == NULL );
    g_assert( qof_instance_lookup_twin( QOF_INSTANCE( from_book ), to_book ) == NULL );
    
    g_test_message( "Test instances with the same book are not paired" );
    qof_instance_gemini (&to_book->inst, (QofInstance *) &to_book->inst);
    g_assert( qof_instance_lookup_twin( QOF_INSTANCE( to_book ), to_book ) == NULL );
    g_assert( qof_instance_lookup_twin( QOF_INSTANCE( from_book ), from_book ) == NULL );
    
    g_test_message( "Test instances with different books are paired" );
    qof_instance_gemini (&to_book->inst, (QofInstance *) &from_book->inst);
    g_assert( qof_instance_lookup_twin( QOF_INSTANCE( to_book ), from_book ) == &from_book->inst );
    g_assert( qof_instance_lookup_twin( QOF_INSTANCE( from_book ), to_book ) == &to_book->inst );
      
    /* destroy books */
    qof_book_destroy( to_book );
    qof_book_destroy( from_book );
}

void
test_suite_qofinstance ( void )
{
    GNC_TEST_ADD( suitename, "set get book", Fixture, NULL, setup, test_instance_set_get_book, teardown );
    GNC_TEST_ADD( suitename, "set get guid", Fixture, NULL, setup, test_instance_set_get_guid, teardown );
    GNC_TEST_ADD_FUNC( suitename, "instance new and destroy", test_instance_new_destroy );
    GNC_TEST_ADD_FUNC( suitename, "init data", test_instance_init_data );
    GNC_TEST_ADD( suitename, "get set slots", Fixture, NULL, setup, test_instance_get_set_slots, teardown );
    GNC_TEST_ADD_FUNC( suitename, "version compare", test_instance_version_cmp );
    GNC_TEST_ADD( suitename, "get set dirty", Fixture, NULL, setup, test_instance_get_set_dirty, teardown );
    GNC_TEST_ADD_FUNC( suitename, "gemini creation and lookup", test_instance_gemini_and_lookup );
}
