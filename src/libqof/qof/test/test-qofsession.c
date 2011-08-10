/********************************************************************
 * test_qofsession.c: GLib g_test test suite for qofsession.        *
 * Copyright 2011 John Ralls <jralls@ceridwen.us>                   *
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
#include "test-stuff.h"
#include "qof.h"
#include "qofbackend-p.h"
#include "qofsession-p.h"
#include "qofclass-p.h"

static const gchar *suitename = "/qof/qofsession";
void test_suite_qofsession ( void );

extern void ( *p_qof_instance_foreach_copy )( gpointer data, gpointer user_data );

extern void init_static_qofsession_pointers( void );

typedef struct
{
    QofSession *session;
} Fixture;

static void
safe_sync( QofBackend *be, QofBook *book )
{
    qof_backend_set_error( be, ERR_BACKEND_DATA_CORRUPT );
    qof_backend_set_message( be, "Just Kidding!" );
}

static void
percentage_fn ( const char* message, double percent )
{
    g_print( "%s %f complete", message, percent );
}

static void
setup( Fixture *fixture, gconstpointer pData )
{
    fixture->session = qof_session_new();
    fixture->session->backend = g_new0( QofBackend, 1 );
}

static void
teardown( Fixture *fixture, gconstpointer pData )
{
    qof_session_destroy( fixture->session );
}

static void
test_session_safe_save( Fixture *fixture, gconstpointer pData )
{
    fixture->session->backend->safe_sync = safe_sync;
    qof_session_safe_save( fixture->session, percentage_fn );
    g_assert_cmpint( ERR_BACKEND_DATA_CORRUPT, == ,
                     qof_session_get_error( fixture->session ));
    g_assert( NULL == qof_session_get_url( fixture->session ));
}

static struct
{
    QofInstance *from;
    QofInstance *to;
    QofInstance *col_inst;
    QofParam *param;
    gboolean getter_called;
    gboolean setter_called;
    QofType type;
    gpointer data;
    QofBook *book;
} foreach_copy_struct;

static gpointer 
mock_getter( gpointer object, const QofParam *param)
{
    static Timespec ts_getter;
    static gnc_numeric numeric_getter;
    static gint32 gint32_getter;
    static gint64 gint64_getter;
    static double double_getter;
    static gboolean gboolean_getter;
    GncGUID *guid;
    KvpFrame *frame;
    
    g_assert( object );
    g_assert( param );
    g_assert( foreach_copy_struct.from == (QofInstance*) object );
    g_assert( foreach_copy_struct.param == param );
    foreach_copy_struct.type = param->param_type;
    if ( safe_strcmp( param->param_type, QOF_TYPE_STRING ) == 0 )
    {
	foreach_copy_struct.getter_called = TRUE;
	return (gpointer) g_strdup( "string_type" );
    }
    if ( safe_strcmp( param->param_type, QOF_TYPE_DATE ) == 0 )
    {
	foreach_copy_struct.getter_called = TRUE;
	ts_getter.tv_nsec = 3;
	ts_getter.tv_sec = 7;
	return ( gpointer ) &ts_getter;
    }
    if ( safe_strcmp( param->param_type, QOF_TYPE_NUMERIC ) == 0 )
    {
	foreach_copy_struct.getter_called = TRUE;
	numeric_getter.num = 5;
	numeric_getter.denom = 6;
	return (gpointer) &numeric_getter;
    }
    if ( safe_strcmp( param->param_type, QOF_TYPE_GUID ) == 0 )
    {
	foreach_copy_struct.getter_called = TRUE;
	guid = g_new0( GncGUID, 1 );
	foreach_copy_struct.data = (gpointer) guid;
	return (gpointer) guid;
    }
    if ( safe_strcmp( param->param_type, QOF_TYPE_INT32 ) == 0 )
    {
	foreach_copy_struct.getter_called = TRUE;
	gint32_getter = 5;
	return ( gpointer ) &gint32_getter;
    }
    if ( safe_strcmp( param->param_type, QOF_TYPE_INT64 ) == 0 )
    {
	foreach_copy_struct.getter_called = TRUE;
	gint64_getter = 10;
	return ( gpointer ) &gint64_getter;    
    }
    if ( safe_strcmp( param->param_type, QOF_TYPE_DOUBLE ) == 0 )
    {
	foreach_copy_struct.getter_called = TRUE;
	double_getter = 10.5;
	return ( gpointer ) &double_getter;    
    }
    if ( safe_strcmp( param->param_type, QOF_TYPE_BOOLEAN ) == 0 )
    {
	foreach_copy_struct.getter_called = TRUE;
	gboolean_getter = TRUE;
	return ( gpointer ) &gboolean_getter;
    }
    if ( safe_strcmp( param->param_type, QOF_TYPE_KVP ) == 0 )
    {
	foreach_copy_struct.getter_called = TRUE;
	frame = kvp_frame_new();
	foreach_copy_struct.data = (gpointer) frame;
	return (gpointer) frame;
    }
    if ( safe_strcmp( param->param_type, QOF_TYPE_CHAR ) == 0 )
    {
	foreach_copy_struct.getter_called = TRUE;
	return (gpointer) g_strdup( "gchar_type" );
    }
    if ( safe_strcmp( param->param_type, QOF_TYPE_COLLECT ) == 0 )
    {
	foreach_copy_struct.getter_called = TRUE;
	foreach_copy_struct.col_inst = g_object_new( QOF_TYPE_INSTANCE, NULL );
	/* this will create col of col_type and insert inst there */
	qof_instance_init_data( foreach_copy_struct.col_inst, "col_type", foreach_copy_struct.book );
	foreach_copy_struct.data = (gpointer) qof_book_get_collection( foreach_copy_struct.book, "col_type" );
	return (gpointer) qof_book_get_collection( foreach_copy_struct.book, "col_type" );
    }
    if ( safe_strcmp( param->param_type, QOF_TYPE_CHAR ) == 0 )
    {
	foreach_copy_struct.getter_called = TRUE;
	return (gpointer) g_strdup( "gchar_type" );
    }
    if ( safe_strcmp( param->param_type, "non_registered_type" ) == 0 )
    {
	foreach_copy_struct.getter_called = TRUE;
	foreach_copy_struct.data = (gpointer) g_object_new( QOF_TYPE_INSTANCE, NULL );
	qof_instance_init_data( ( QofInstance* ) foreach_copy_struct.data, "non_reg_type", foreach_copy_struct.book );
	return foreach_copy_struct.data;
    }
    /* should not reach this place */
    g_assert( FALSE );
    return NULL;
}

static void
mock_setter( gpointer inst, gpointer data )
{
    Timespec *ts;
    gnc_numeric *numeric_setter;
    gint32 *gint_32;
    gint64 *gint_64;
    double *double_setter;
    gboolean *gboolean_setter;
    
    g_assert( inst );
    g_assert( data );
    g_assert( foreach_copy_struct.to == ( QofInstance* ) inst );
    if ( safe_strcmp( foreach_copy_struct.type, QOF_TYPE_STRING ) == 0 )
    {
	g_assert_cmpstr( "string_type", ==, ( const char* ) data );
	foreach_copy_struct.setter_called = TRUE;
	g_free( data );
	return;
    }
    if ( safe_strcmp( foreach_copy_struct.type, QOF_TYPE_DATE ) == 0 )
    {
	ts = ( Timespec* ) data;
	g_assert_cmpint( ts->tv_nsec, ==, 3 );
	g_assert_cmpint( ts->tv_sec, ==, 7 );
	foreach_copy_struct.setter_called = TRUE;
	return;
    }
    if ( safe_strcmp( foreach_copy_struct.type, QOF_TYPE_NUMERIC ) == 0 )
    {
	numeric_setter = ( gnc_numeric* ) data;
	g_assert_cmpint( numeric_setter->num, ==, 5 );
	g_assert_cmpint( numeric_setter->denom, ==, 6 );
	foreach_copy_struct.setter_called = TRUE;
	return;
    }
    if ( safe_strcmp( foreach_copy_struct.type, QOF_TYPE_GUID ) == 0 )
    {
	g_assert( guid_equal( ( GncGUID* ) foreach_copy_struct.data, ( GncGUID* ) data ) );
	foreach_copy_struct.setter_called = TRUE;
	g_free( data );
	return;
    }
    if ( safe_strcmp( foreach_copy_struct.type, QOF_TYPE_INT32 ) == 0 )
    {
	gint_32 = ( gint32* ) data;
	g_assert_cmpint( *gint_32, ==, 5 );
	foreach_copy_struct.setter_called = TRUE;
	return;
    }
    if ( safe_strcmp( foreach_copy_struct.type, QOF_TYPE_INT64 ) == 0 )
    {
	gint_64 = ( gint64* ) data;
	g_assert_cmpint( *gint_64, ==, 10 );
	foreach_copy_struct.setter_called = TRUE;
	return;
    }
    if ( safe_strcmp( foreach_copy_struct.type, QOF_TYPE_DOUBLE ) == 0 )
    {
	double_setter = ( double* ) data;
	g_assert_cmpfloat( *double_setter, ==, 10.5 );
	foreach_copy_struct.setter_called = TRUE;
	return;
    }
    if ( safe_strcmp( foreach_copy_struct.type, QOF_TYPE_BOOLEAN ) == 0 )
    {
	gboolean_setter = ( gboolean* ) data;
	g_assert( *gboolean_setter == TRUE );
	foreach_copy_struct.setter_called = TRUE;
	return;
    }
    if ( safe_strcmp( foreach_copy_struct.type, QOF_TYPE_KVP ) == 0 )
    {
	g_assert_cmpint( kvp_frame_compare( ( KvpFrame* ) foreach_copy_struct.data, ( KvpFrame* ) data ), ==, 0 );
	foreach_copy_struct.setter_called = TRUE;
	kvp_frame_delete( ( KvpFrame* ) data );
	return;
    }
    if ( safe_strcmp( foreach_copy_struct.type, QOF_TYPE_CHAR ) == 0 )
    {
	g_assert_cmpstr( "gchar_type", ==, ( gchar* ) data );
	foreach_copy_struct.setter_called = TRUE;
	g_free( data );
	return;
    }
}

static void
test_qof_instance_foreach_copy( Fixture *fixture, gconstpointer pData )
{
    /* Function takes QofParam and QofInstanceCopyData as an input
     * for each param depending on its type 
     * getter takes value from source inst and setter sets it to dest inst
     */
    QofBook *book = NULL;
    QofInstanceCopyData qecd;
    QofInstance *from = NULL, *to = NULL;
    int i;
    
    static QofParam params[] = {
	{ "string_name", QOF_TYPE_STRING, mock_getter, mock_setter, NULL, NULL },
	//{ "date_name", QOF_TYPE_DATE, mock_getter, mock_setter, NULL, NULL }
	//{ "numeric_name", QOF_TYPE_NUMERIC, mock_getter, mock_setter, NULL, NULL },
	{ "guid_name", QOF_TYPE_GUID, mock_getter, mock_setter, NULL, NULL },
	{ "int32_name", QOF_TYPE_INT32, mock_getter, mock_setter, NULL, NULL },
	{ "int64_name", QOF_TYPE_INT64, mock_getter, mock_setter, NULL, NULL },
	//{ "double_name", QOF_TYPE_DOUBLE, mock_getter, mock_setter, NULL, NULL },
	{ "boolean_name", QOF_TYPE_BOOLEAN, mock_getter, mock_setter, NULL, NULL },
	{ "kvp_name", QOF_TYPE_KVP, mock_getter, mock_setter, NULL, NULL },
	{ "char_name", QOF_TYPE_CHAR, mock_getter, mock_setter, NULL, NULL },
	{ "collection_name", QOF_TYPE_COLLECT, mock_getter, NULL, NULL, NULL },
	{ "nonreg_name", "non_registered_type", mock_getter, NULL, NULL, NULL }
    };
    
    qof_class_init();
    qof_class_register ("my_test_object", NULL, params);
    
    from = g_object_new( QOF_TYPE_INSTANCE, NULL );
    to = g_object_new( QOF_TYPE_INSTANCE, NULL );
    g_assert( from );
    from->e_type = "my_test_object";
    g_assert( to );
    init_static_qofsession_pointers();
    g_assert( p_qof_instance_foreach_copy );
    qecd.from = from;
    qecd.to = to;
    qecd.new_session = fixture->session;
    book = qof_session_get_book( fixture->session );
    g_assert( book );
    g_assert_cmpint( g_list_length( ( GList* )qof_book_get_data( book, ENTITYREFERENCE ) ), ==, 0 );
    
    g_test_message( "Test param copying for each registered type" );
    foreach_copy_struct.from = from;
    foreach_copy_struct.to = to;
    foreach_copy_struct.book = book;
    for ( i = 0; i < 8; i++ )
    {
	foreach_copy_struct.getter_called = FALSE;
	foreach_copy_struct.setter_called = FALSE;
	foreach_copy_struct.param = &params[i];
	p_qof_instance_foreach_copy( &params[i], &qecd );
	g_assert( qecd.param == &params[i] );
	g_assert( foreach_copy_struct.getter_called == TRUE );
	if ( safe_strcmp( params[i].param_type, QOF_TYPE_COLLECT ) == 0 )
	{
	    /* one ref per one collection entry */
	    g_assert_cmpint( g_list_length( ( GList* )qof_book_get_data( book, ENTITYREFERENCE ) ), ==, 1 );
	    qof_collection_remove_entity( foreach_copy_struct.col_inst );
	    g_object_unref( foreach_copy_struct.col_inst );
	}   
	else
	{
	    g_assert( foreach_copy_struct.setter_called == TRUE );
	}
    }
    
    g_test_message( "Test param copying for unregistered type" );
    g_assert_cmpint( g_list_length( ( GList* )qof_book_get_data( book, ENTITYREFERENCE ) ), ==, 1 );
    foreach_copy_struct.param = &params[8];
    p_qof_instance_foreach_copy( &params[8], &qecd );
    g_assert( qecd.param == &params[8] );
    g_assert_cmpint( g_list_length( ( GList* )qof_book_get_data( book, ENTITYREFERENCE ) ), ==, 2 );
    
    qof_class_shutdown();
    g_object_unref( from );
    g_object_unref( to );
}

void
test_suite_qofsession ( void )
{
    GNC_TEST_ADD( suitename, "qof session safe save", Fixture, NULL, setup, test_session_safe_save, teardown );
    GNC_TEST_ADD( suitename, "qof instance foreach copy", Fixture, NULL, setup, test_qof_instance_foreach_copy, teardown );
}
