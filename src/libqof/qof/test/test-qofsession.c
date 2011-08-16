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

extern GSList* get_provider_list( void );
extern gboolean get_qof_providers_initialized( void );
extern void unregister_all_providers( void );

extern void ( *p_qof_instance_foreach_copy )( gpointer data, gpointer user_data );
extern void ( *p_qof_instance_list_foreach )( gpointer data, gpointer user_data );
extern void ( *p_qof_session_load_backend )( QofSession * session, const char * access_method );
extern void ( *p_qof_session_clear_error )( QofSession * session );

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
}

static void
teardown( Fixture *fixture, gconstpointer pData )
{
    qof_session_destroy( fixture->session );
}

static void
test_qof_session_new_destroy( void )
{
    QofSession *session = NULL;
    QofBook *book = NULL;
    
    g_test_message( "Test session initialization" );
    session = qof_session_new();
    g_assert( session );
    g_assert_cmpstr( session->entity.e_type, ==, QOF_ID_SESSION );
    g_assert( session->books );
    g_assert_cmpint( g_list_length( session->books ), ==, 1 );
    book = ( QofBook* ) session->books->data;
    g_assert( book );
    g_assert( QOF_IS_BOOK( book ) );
    g_assert( !session->book_id );
    g_assert( !session->backend );
    g_assert_cmpint( session->lock, ==, 1 );
    g_assert_cmpint( qof_session_get_error( session ), ==, ERR_BACKEND_NO_ERR );
    
    g_test_message( "Test session destroy" );
    qof_session_destroy( session );
    /* all data structures of session get deallocated so we can't really test this place
     * instead qof_session_destroy_backend and qof_session_end are tested
     */
}

static void
test_session_safe_save( Fixture *fixture, gconstpointer pData )
{
    fixture->session->backend = g_new0( QofBackend, 1 );
    fixture->session->backend->safe_sync = safe_sync;
    qof_session_safe_save( fixture->session, percentage_fn );
    g_assert_cmpint( ERR_BACKEND_DATA_CORRUPT, == ,
                     qof_session_get_error( fixture->session ));
    g_assert( NULL == qof_session_get_url( fixture->session ));
}

/********************************************************************
 *    Mock object 
 * ******************************************************************/
#define QOF_TYPE_UNREG "unregistered"
#define MY_TEST_TYPE_NAME "my_test_type"
#define MY_TEST_TYPE_DESC "Test type object"
#define PARAM_STRING_NAME "string_name"
#define PARAM_DATE_NAME "date_name"
#define PARAM_NUMERIC_NAME "numeric_name"
#define PARAM_GUID_NAME "guid_name"
#define PARAM_INT32_NAME "int32_name"
#define PARAM_INT64_NAME "int64_name"
#define PARAM_DOUBLE_NAME "double_name"
#define PARAM_BOOLEAN_NAME "boolean_name"
#define PARAM_KVP_NAME "kvp_name"
#define PARAM_CHAR_NAME "char_name"
#define PARAM_COLLECTION_NAME "collection_name"
#define PARAM_UNREG_NAME "unregistered_name"

/* simple object structure */
typedef struct MyTestType_s
{
    QofInstance 	inst;
    gchar       	*string_value;
    Timespec    	date_value;
    gnc_numeric 	numeric_value;
    GncGUID 		*guid_value;
    gint32      	gint32_value;
    gint64 		gint64_value;
    double      	double_value;
    gboolean		bool_value;
    KvpFrame		*frame;
    gchar		char_value;
    QofCollection	*col;
    QofInstance		*unreg_type;
} MyTestType;

typedef struct MyTestTypeClass_s
{
    QofInstanceClass parent_class;
} MyTestTypeClass;

MyTestType* my_test_type_create( QofBook *book );
gboolean my_test_type_register ( void );

/* setters */
void my_test_type_set_string( MyTestType*, gchar* );
void my_test_type_set_date( MyTestType*, Timespec );
void my_test_type_set_numeric( MyTestType*, gnc_numeric );
void my_test_type_set_guid( MyTestType*, GncGUID* );
void my_test_type_set_gint32( MyTestType*, gint32 );
void my_test_type_set_gint64( MyTestType*, gint64 );
void my_test_type_set_double( MyTestType*, double );
void my_test_type_set_gboolean( MyTestType*, gboolean );
void my_test_type_set_frame( MyTestType*, KvpFrame* );
void my_test_type_set_char( MyTestType*, gchar );
void my_test_type_set_col( MyTestType*, QofCollection* );
void my_test_type_set_unreg_type( MyTestType*, QofInstance* );

/* getter functions */
gchar*	    	my_test_type_get_string( MyTestType* );
Timespec    	my_test_type_get_date( MyTestType* );
gnc_numeric 	my_test_type_get_numeric( MyTestType* );
GncGUID*    	my_test_type_get_guid( MyTestType* );
gint32	    	my_test_type_get_gint32( MyTestType* );
gint64	    	my_test_type_get_gint64( MyTestType* );
double	    	my_test_type_get_double( MyTestType* );
gboolean    	my_test_type_get_gboolean( MyTestType* );
KvpFrame*   	my_test_type_get_frame( MyTestType* );
gchar       	my_test_type_get_char( MyTestType* );
QofCollection* 	my_test_type_get_col( MyTestType* );
QofInstance* 	my_test_type_get_unreg_type( MyTestType* );

/* --- type macros --- */
#define GNC_TYPE_MYTESTYPE            (gnc_my_test_type_get_type ())
#define GNC_MYTESTYPE(o)              \
     (G_TYPE_CHECK_INSTANCE_CAST ((o), GNC_TYPE_MYTESTYPE, MyTestType))
#define GNC_MYTESTYPE_CLASS(k)        \
     (G_TYPE_CHECK_CLASS_CAST((k), GNC_TYPE_MYTESTYPE, MyTestTypeClass))
#define GNC_IS_MYTESTYPE(o)           \
     (G_TYPE_CHECK_INSTANCE_TYPE ((o), GNC_TYPE_MYTESTYPE))
#define GNC_IS_MYTESTYPE_CLASS(k)     \
     (G_TYPE_CHECK_CLASS_TYPE ((k), GNC_TYPE_MYTESTYPE))
#define GNC_MYTESTYPE_GET_CLASS(o)    \
     (G_TYPE_INSTANCE_GET_CLASS ((o), GNC_TYPE_MYTESTYPE, MyTestTypeClass))

GType gnc_my_test_type_get_type( void );

/* GObject Initialization */
QOF_GOBJECT_IMPL( gnc_my_test_type, MyTestType, QOF_TYPE_INSTANCE);

static void
gnc_my_test_type_init( MyTestType* obj )
{
}

static void
gnc_my_test_type_dispose_real( GObject *objp )
{
}

static void
gnc_my_test_type_finalize_real( GObject* objp )
{
    MyTestType* mtt = GNC_MYTESTYPE( objp );

    if ( mtt->frame != NULL )
	kvp_frame_delete( mtt->frame );
    if ( mtt->string_value != NULL )
	g_free( mtt->string_value );
    if ( mtt->guid_value != NULL )
	g_free( mtt->guid_value );
    if ( mtt->col != NULL )
	qof_collection_destroy( mtt->col );
    if ( mtt->unreg_type != NULL )
	g_object_unref( mtt->unreg_type );
    mtt->frame = NULL;
    mtt->string_value = NULL;
    mtt->guid_value = NULL;
    mtt->col = NULL;
    mtt->unreg_type = NULL;
}

MyTestType* 
my_test_type_create( QofBook *book )
{
    MyTestType *mtt;

    g_assert( book );
    mtt = g_object_new( GNC_TYPE_MYTESTYPE, NULL );
    qof_instance_init_data( &mtt->inst, MY_TEST_TYPE_NAME, book );
    mtt->string_value = NULL;
    mtt->guid_value = NULL;
    mtt->bool_value = FALSE;
    mtt->frame = NULL;
    mtt->col = NULL;
    mtt->unreg_type = NULL;
    qof_event_gen( &mtt->inst, QOF_EVENT_CREATE, NULL );
    return mtt;
}

/* get/set string */

void
my_test_type_set_string( MyTestType* mtt, gchar* value )
{
    g_assert( mtt );
    g_assert( value );
    mtt->string_value = g_strdup( value );
}

gchar*
my_test_type_get_string( MyTestType* mtt )
{
    g_assert( mtt );
    return mtt->string_value;
}

/* get/set date */

void
my_test_type_set_date( MyTestType* mtt, Timespec value )
{
    g_assert( mtt );
    mtt->date_value = value;
}

Timespec
my_test_type_get_date( MyTestType* mtt )
{
    g_assert( mtt );
    return mtt->date_value;
}

/* get/set numeric */

void
my_test_type_set_numeric( MyTestType* mtt, gnc_numeric value )
{
    g_assert( mtt );
    mtt->numeric_value = value;
}

gnc_numeric
my_test_type_get_numeric( MyTestType* mtt )
{
    g_assert( mtt );
    return mtt->numeric_value;
}

/* get/set guid */

void
my_test_type_set_guid( MyTestType* mtt, GncGUID* value )
{
    char cm_sa[GUID_ENCODING_LENGTH + 1];
    gchar *cm_string;

    g_assert( mtt );
    g_assert( value );
    mtt->guid_value = g_new( GncGUID , 1 );
    guid_to_string_buff( value, cm_sa);
    cm_string = g_strdup( cm_sa );
    g_assert( string_to_guid( cm_string, mtt->guid_value ) );
    g_free(cm_string);
}

GncGUID*
my_test_type_get_guid( MyTestType* mtt )
{
    g_assert( mtt );
    return mtt->guid_value;
}

/* get/set int32 */

void
my_test_type_set_gint32( MyTestType* mtt, gint32 value )
{
    g_assert( mtt );
    mtt->gint32_value = value;
}

gint32
my_test_type_get_gint32( MyTestType* mtt )
{
    g_assert( mtt );
    return mtt->gint32_value;
}

/* get/set int64 */

void
my_test_type_set_gint64( MyTestType* mtt, gint64 value )
{
    g_assert( mtt );
    mtt->gint64_value = value;
}

gint64
my_test_type_get_gint64( MyTestType* mtt )
{
    g_assert( mtt );
    return mtt->gint64_value;
}

/* get/set double */

void
my_test_type_set_double( MyTestType* mtt, double value )
{
    g_assert( mtt );
    mtt->double_value = value;
}

double
my_test_type_get_double( MyTestType* mtt )
{
    g_assert( mtt );
    return mtt->double_value;
}

/* get/set boolean */

void
my_test_type_set_gboolean( MyTestType* mtt, gboolean value )
{
    g_assert( mtt );
    mtt->bool_value = value;
}

gboolean
my_test_type_get_gboolean( MyTestType* mtt )
{
    g_assert( mtt );
    return mtt->bool_value;
}

/* get/set frame */

void
my_test_type_set_frame( MyTestType* mtt, KvpFrame* value )
{
    g_assert( mtt );
    g_assert( value );
    mtt->frame = kvp_frame_copy( value );
}

KvpFrame*
my_test_type_get_frame( MyTestType* mtt )
{
    g_assert( mtt );
    return mtt->frame;
}

/* get/set char */

void
my_test_type_set_char( MyTestType* mtt, gchar value )
{
    g_assert( mtt );
    mtt->char_value = value;
}

gchar
my_test_type_get_char( MyTestType* mtt )
{
    g_assert( mtt );
    return mtt->char_value;
}

/* get/set collection */

void
my_test_type_set_col( MyTestType* mtt, QofCollection* value )
{
    QofIdType type;
    
    g_assert( mtt );
    g_assert( value );
    type = qof_collection_get_type( value );
    mtt->col = qof_collection_new( type );
}

QofCollection*
my_test_type_get_col( MyTestType* mtt )
{
    g_assert( mtt );
    return mtt->col;
}

/* get/set unreg type */

void
my_test_type_set_unreg_type( MyTestType* mtt, QofInstance* value )
{
    g_assert( mtt );
    g_assert( value );
    mtt->col = g_object_new( QOF_TYPE_INSTANCE, NULL );
}

QofInstance*
my_test_type_get_unreg_type( MyTestType* mtt )
{
    g_assert( mtt );
    return mtt->unreg_type;
}

static QofObject my_test_type_object_def =
{
interface_version:
    QOF_OBJECT_VERSION,
e_type:
    MY_TEST_TYPE_NAME,
type_label:
    MY_TEST_TYPE_DESC,
create:
    (gpointer)my_test_type_create,
book_begin:
    NULL,
book_end:
    NULL,
is_dirty:
    NULL,
mark_clean:
    NULL,
foreach:
    qof_collection_foreach,
printable:
    NULL,
version_cmp:
    (int (*)(gpointer, gpointer)) qof_instance_version_cmp,
};

gboolean 
my_test_type_register( void )
{
    static QofParam params[] = {
	{ PARAM_STRING_NAME, QOF_TYPE_STRING, (QofAccessFunc)my_test_type_get_string, (QofSetterFunc)my_test_type_set_string, NULL, NULL },
	{ PARAM_DATE_NAME, QOF_TYPE_DATE, (QofAccessFunc)my_test_type_get_date, (QofSetterFunc)my_test_type_set_date, NULL, NULL },
	{ PARAM_NUMERIC_NAME, QOF_TYPE_NUMERIC, (QofAccessFunc)my_test_type_get_numeric, (QofSetterFunc)my_test_type_set_numeric, NULL, NULL },
	{ PARAM_GUID_NAME, QOF_TYPE_GUID, (QofAccessFunc)my_test_type_get_guid, (QofSetterFunc)my_test_type_set_guid, NULL, NULL },
	{ PARAM_INT32_NAME, QOF_TYPE_INT32, (QofAccessFunc)my_test_type_get_gint32, (QofSetterFunc)my_test_type_set_gint32, NULL, NULL },
	{ PARAM_INT64_NAME, QOF_TYPE_INT64, (QofAccessFunc)my_test_type_get_gint64, (QofSetterFunc)my_test_type_set_gint64, NULL, NULL },
	{ PARAM_DOUBLE_NAME, QOF_TYPE_DOUBLE, (QofAccessFunc)my_test_type_get_double, (QofSetterFunc)my_test_type_set_double, NULL, NULL },
	{ PARAM_BOOLEAN_NAME, QOF_TYPE_BOOLEAN, (QofAccessFunc)my_test_type_get_gboolean, (QofSetterFunc)my_test_type_set_gboolean, NULL, NULL },
	{ PARAM_KVP_NAME, QOF_TYPE_KVP, (QofAccessFunc)my_test_type_get_frame, (QofSetterFunc)my_test_type_set_frame, NULL, NULL },
	{ PARAM_CHAR_NAME, QOF_TYPE_CHAR, (QofAccessFunc)my_test_type_get_char, (QofSetterFunc)my_test_type_set_char, NULL, NULL },
	{ PARAM_COLLECTION_NAME, QOF_TYPE_COLLECT, (QofAccessFunc)my_test_type_get_col, (QofSetterFunc)my_test_type_set_col, NULL, NULL },
	{ PARAM_UNREG_NAME, QOF_TYPE_UNREG, (QofAccessFunc)my_test_type_get_unreg_type, (QofSetterFunc)my_test_type_set_unreg_type, NULL, NULL }
    };

    qof_class_register( MY_TEST_TYPE_NAME, NULL, params );
    return qof_object_register( &my_test_type_object_def );
}

static void
fill_data( MyTestType *mtt )
{
    Timespec ts;
    gnc_numeric gnc_n;
    GncGUID *gnc_guid;
    
    g_assert( mtt );
    mtt->string_value = g_strdup( "abcdefghijklmn" );
    ts.tv_sec = 5;
    ts.tv_nsec = 6;
    mtt->date_value = ts;
    gnc_n.num = 3;
    gnc_n.denom = 7;
    mtt->numeric_value = gnc_n;
    gnc_guid = g_new( GncGUID, 1 );
    guid_new( gnc_guid );
    mtt->guid_value = gnc_guid;
    mtt->gint32_value = 555;
    mtt->gint64_value = 777;
    mtt->double_value = 10.5;
    mtt->bool_value = TRUE;
    mtt->frame = kvp_frame_new();
    mtt->char_value = 'y';
    mtt->col = qof_collection_new( "new_type" );
    mtt->unreg_type = g_object_new( QOF_TYPE_INSTANCE, NULL );
}

/********************************************************************
 * Mock object end
 * *****************************************************************/

/*******************************************
 *	TESTS
 *
 *******************************************/

static void
test_qof_instance_foreach_copy( Fixture *fixture, gconstpointer pData )
{
    /* Function takes QofParam and QofInstanceCopyData as an input
     * for each param depending on its type 
     * getter takes value from source inst and setter sets it to dest inst
     */
    QofBook *book = NULL;
    QofParam *param;
    QofInstanceCopyData *qecd = NULL;
    MyTestType *from = NULL, *to = NULL;

    /* setup */
    qof_object_initialize();
    qof_class_init();
    book = qof_session_get_book( fixture->session );
    g_assert( book );
    g_assert( my_test_type_register() );
    from = ( MyTestType* ) qof_object_new_instance( MY_TEST_TYPE_NAME, book );
    to = ( MyTestType* ) qof_object_new_instance( MY_TEST_TYPE_NAME, book );
    qecd = g_new0( QofInstanceCopyData, 1 );
    g_assert( qecd );
    init_static_qofsession_pointers();
    g_assert( p_qof_instance_foreach_copy );
    
    /* init */
    qecd->from = QOF_INSTANCE( from );
    qecd->to = QOF_INSTANCE( to );
    qecd->new_session = fixture->session;
    g_assert_cmpint( g_list_length( ( GList* )qof_book_get_data( book, ENTITYREFERENCE ) ), ==, 0 );
    fill_data( from );
    
    g_test_message( "Test param copying for each registered type" );
    g_assert_cmpstr( from->string_value, !=, to->string_value );
    param = ( QofParam* ) qof_class_get_parameter( MY_TEST_TYPE_NAME, PARAM_STRING_NAME );
    p_qof_instance_foreach_copy( ( gpointer ) param, qecd );
    g_assert( qecd->param == param );
    g_assert_cmpstr( from->string_value, ==, to->string_value );
    
    g_assert( !timespec_equal( &from->date_value, &to->date_value ) );
    param = ( QofParam* ) qof_class_get_parameter( MY_TEST_TYPE_NAME, PARAM_DATE_NAME );
    p_qof_instance_foreach_copy( ( gpointer ) param, qecd );
    g_assert( qecd->param == param );
    g_assert( timespec_equal( &from->date_value, &to->date_value ) );
    
    g_assert( !gnc_numeric_eq( from->numeric_value, to->numeric_value ) );
    param = ( QofParam* ) qof_class_get_parameter( MY_TEST_TYPE_NAME, PARAM_NUMERIC_NAME );
    p_qof_instance_foreach_copy( ( gpointer ) param, qecd );
    g_assert( qecd->param == param );
    g_assert( gnc_numeric_eq( from->numeric_value, to->numeric_value ) );
    
    g_assert( !guid_equal( from->guid_value, to->guid_value ) );
    param = ( QofParam* ) qof_class_get_parameter( MY_TEST_TYPE_NAME, PARAM_GUID_NAME );
    p_qof_instance_foreach_copy( ( gpointer ) param, qecd );
    g_assert( qecd->param == param );
    g_assert( guid_equal( from->guid_value, to->guid_value ) );
        
    g_assert_cmpint( from->gint32_value, !=, to->gint32_value );
    param = ( QofParam* ) qof_class_get_parameter( MY_TEST_TYPE_NAME, PARAM_INT32_NAME );
    p_qof_instance_foreach_copy( ( gpointer ) param, qecd );
    g_assert( qecd->param == param );
    g_assert_cmpint( from->gint32_value, ==, to->gint32_value );
    
    g_assert_cmpint( from->gint64_value, !=, to->gint64_value );
    param = ( QofParam* ) qof_class_get_parameter( MY_TEST_TYPE_NAME, PARAM_INT64_NAME );
    p_qof_instance_foreach_copy( ( gpointer ) param, qecd );
    g_assert( qecd->param == param );
    g_assert_cmpint( from->gint64_value, ==, to->gint64_value );
    
    g_assert_cmpfloat( from->double_value, !=, to->double_value );
    param = ( QofParam* ) qof_class_get_parameter( MY_TEST_TYPE_NAME, PARAM_DOUBLE_NAME );
    p_qof_instance_foreach_copy( ( gpointer ) param, qecd );
    g_assert( qecd->param == param );
    g_assert_cmpfloat( from->double_value, ==, to->double_value );
    
    g_assert( from->bool_value != to->bool_value );
    param = ( QofParam* ) qof_class_get_parameter( MY_TEST_TYPE_NAME, PARAM_BOOLEAN_NAME );
    p_qof_instance_foreach_copy( ( gpointer ) param, qecd );
    g_assert( qecd->param == param );
    g_assert( from->bool_value == to->bool_value );
    
    g_assert_cmpint( kvp_frame_compare( from->frame, to->frame ), !=, 0 );
    param = ( QofParam* ) qof_class_get_parameter( MY_TEST_TYPE_NAME, PARAM_KVP_NAME );
    p_qof_instance_foreach_copy( ( gpointer ) param, qecd );
    g_assert( qecd->param == param );
    g_assert_cmpint( kvp_frame_compare( from->frame, to->frame ), ==, 0 );
    
    g_assert_cmpuint( from->char_value, !=, to->char_value );
    param = ( QofParam* ) qof_class_get_parameter( MY_TEST_TYPE_NAME, PARAM_CHAR_NAME );
    p_qof_instance_foreach_copy( ( gpointer ) param, qecd );
    g_assert( qecd->param == param );
    g_assert_cmpuint( from->char_value, ==, to->char_value );
    
    g_assert( from->col != NULL );
    g_assert( to->col == NULL );
    param = ( QofParam* ) qof_class_get_parameter( MY_TEST_TYPE_NAME, PARAM_COLLECTION_NAME );
    p_qof_instance_foreach_copy( ( gpointer ) param, qecd );
    g_assert( qecd->param == param );
    g_assert( to->col == NULL );
    /* empty col doesn't produce any ref data */
    g_assert_cmpint( g_list_length( ( GList* )qof_book_get_data( book, ENTITYREFERENCE ) ), ==, 0 );
    
    g_test_message( "Test param copying for unregistered type" );
    g_assert( from->unreg_type != NULL );
    g_assert( to->unreg_type == NULL );
    param = ( QofParam* ) qof_class_get_parameter( MY_TEST_TYPE_NAME, PARAM_UNREG_NAME );
    p_qof_instance_foreach_copy( ( gpointer ) param, qecd );
    g_assert( qecd->param == param );
    g_assert( to->unreg_type == NULL );
    /* ref data increased by 1 */
    g_assert_cmpint( g_list_length( ( GList* )qof_book_get_data( book, ENTITYREFERENCE ) ), ==, 1 );

    qof_class_shutdown();
    qof_object_shutdown();
    g_free( qecd );
    g_object_unref( from );
    g_object_unref( to );
}

static void
test_qof_instance_list_foreach( Fixture *fixture, gconstpointer pData )
{
    /* Function takes QofInstance and QofInstanceCopyData as an input
     * 
     */
    QofInstanceCopyData *qecd = NULL;
    MyTestType *source = NULL;
    QofBook *source_book = NULL;
    
    /* setup */
    qof_object_initialize();
    qof_class_init();
    source_book = qof_book_new();
    g_assert( source_book );
    g_assert( my_test_type_register() );
    source = ( MyTestType* ) qof_object_new_instance( MY_TEST_TYPE_NAME, source_book );
    qecd = g_new0( QofInstanceCopyData, 1 );
    g_assert( qecd );
    init_static_qofsession_pointers();
    g_assert( p_qof_instance_list_foreach );
    
    /* init */
    qecd->new_session = fixture->session;
    fill_data( source );
    
    /* tests */
    g_test_message( "Test for compliance error" );
    qecd->error = FALSE;
    my_test_type_object_def.foreach = NULL;
    p_qof_instance_list_foreach( source, qecd );
    g_assert( qecd->error );
    g_assert( qecd->from == ( QofInstance* )source );
    
    g_test_message( "Test copy of object is done, list of params created" );
    qecd->error = FALSE;
    qecd->from = NULL;
    qecd->to = NULL;
    qecd->param_list = NULL;
    my_test_type_object_def.foreach = qof_collection_foreach;
    p_qof_instance_list_foreach( source, qecd ); /* run */
    g_assert( qecd->from == ( QofInstance* )source );
    g_assert( qecd->to );
    g_assert( QOF_IS_INSTANCE( qecd->to ) );
    g_assert( qecd->from != qecd->to );
    g_assert_cmpint( qof_instance_guid_compare( QOF_INSTANCE( source ), qecd->to ), ==, 0 );
    g_assert( qecd->param_list );
    g_assert_cmpint( g_slist_length( qecd->param_list ), ==, 12 );
    g_assert( !qecd->error );
    
    /* clean up */
    g_object_unref( source );
    g_object_unref( qecd->to );
    qof_book_destroy( source_book );
    g_free( qecd );
    qof_object_shutdown();
    qof_class_shutdown();
}

static struct
{
    QofBackend *be;
    gboolean data_compatible;
    gboolean check_data_type_called;
    gboolean backend_new_called;
} load_backend_struct;

static gboolean 
mock_check_data_type( const char* book_id )
{
    g_assert( book_id );
    g_assert_cmpstr( book_id, ==, "my book" );
    load_backend_struct.check_data_type_called = TRUE;
    return load_backend_struct.data_compatible;
}

static QofBackend* 
mock_backend_new( void )
{
    QofBackend *be = NULL;
    
    be = g_new0( QofBackend, 1 );
    g_assert( be );
    load_backend_struct.be = be;
    load_backend_struct.backend_new_called = TRUE;
    return be;
}

static void
test_qof_session_load_backend( Fixture *fixture, gconstpointer pData )
{
    QofBackendProvider *prov = NULL;
    QofBook *book = NULL;
    
    /* init */
    prov = g_new0( QofBackendProvider, 1 );
    init_static_qofsession_pointers();
    
    g_test_message( "Test when no provider is registered" );
    g_assert( !get_qof_providers_initialized() );
    g_assert( get_provider_list() == NULL );
    p_qof_session_load_backend( fixture->session, "file" );
    g_assert( get_qof_providers_initialized() );
    g_assert_cmpint( qof_session_get_error( fixture->session ), ==, ERR_BACKEND_NO_HANDLER );
    g_assert_cmpstr( qof_session_get_error_message( fixture->session ), ==, "failed to load 'file' using access_method" );
    p_qof_session_clear_error( fixture->session );
    
    g_test_message( "Test with provider registered but access method not supported" );
    prov->access_method = "unsupported";
    qof_backend_register_provider( prov );
    g_assert( get_provider_list() );
    g_assert_cmpint( g_slist_length( get_provider_list() ), ==, 1 );
    p_qof_session_load_backend( fixture->session, "file" );
    g_assert_cmpint( qof_session_get_error( fixture->session ), ==, ERR_BACKEND_NO_HANDLER );
    g_assert_cmpstr( qof_session_get_error_message( fixture->session ), ==, "failed to load 'file' using access_method" );
    p_qof_session_clear_error( fixture->session );
    
    g_test_message( "Test with access method supported but type incompatible" );
    prov->access_method = "file";
    prov->check_data_type = mock_check_data_type;
    load_backend_struct.data_compatible = FALSE;
    load_backend_struct.check_data_type_called = FALSE;
    fixture->session->book_id = g_strdup( "my book" );
    p_qof_session_load_backend( fixture->session, "file" );
    g_assert( load_backend_struct.check_data_type_called );
    g_assert_cmpint( qof_session_get_error( fixture->session ), ==, ERR_BACKEND_NO_HANDLER );
    g_assert_cmpstr( qof_session_get_error_message( fixture->session ), ==, "failed to load 'file' using access_method" );
    p_qof_session_clear_error( fixture->session );
    
    g_test_message( "Test with type compatible but backend_new not set" );
    prov->backend_new = NULL;
    load_backend_struct.data_compatible = TRUE;
    load_backend_struct.check_data_type_called = FALSE;
    p_qof_session_load_backend( fixture->session, "file" );
    g_assert( load_backend_struct.check_data_type_called );
    g_assert_cmpint( qof_session_get_error( fixture->session ), ==, ERR_BACKEND_NO_HANDLER );
    g_assert_cmpstr( qof_session_get_error_message( fixture->session ), ==, "failed to load 'file' using access_method" );
    p_qof_session_clear_error( fixture->session );
    
    g_test_message( "Test with type compatible backend_new set" );
    prov->backend_new = mock_backend_new;
    load_backend_struct.be = NULL;
    load_backend_struct.data_compatible = TRUE;
    load_backend_struct.check_data_type_called = FALSE;
    load_backend_struct.backend_new_called = FALSE;
    g_assert( fixture->session->backend == NULL );
    book = qof_session_get_book( fixture->session );
    g_assert( book );
    g_assert( qof_book_get_backend( book ) == NULL );
    p_qof_session_load_backend( fixture->session, "file" );
    g_assert( load_backend_struct.check_data_type_called );
    g_assert( load_backend_struct.backend_new_called );
    g_assert( load_backend_struct.be );
    g_assert( load_backend_struct.be == fixture->session->backend );
    g_assert( prov == fixture->session->backend->provider );
    g_assert( qof_book_get_backend( book ) == load_backend_struct.be );
    g_assert_cmpint( qof_session_get_error( fixture->session ), ==, ERR_BACKEND_NO_ERR );

    unregister_all_providers();
    g_assert_cmpint( g_slist_length( get_provider_list() ), ==, 0 );
}

static struct
{
    QofBackend *be;
    QofBook *oldbook;
    gboolean error;
    gboolean load_called;
} load_session_struct;

static void 
mock_load( QofBackend *be, QofBook *book, QofBackendLoadType type )
{
    g_assert( be );
    g_assert( book );
    g_assert( be == load_session_struct.be );
    g_assert( book != load_session_struct.oldbook );
    g_assert( qof_book_get_backend( book ) == be );
    if ( load_session_struct.error )
	qof_backend_set_error( be, ERR_BACKEND_DATA_CORRUPT ); /* just any valid error */
    load_session_struct.load_called = TRUE;
}

static void
test_qof_session_load( Fixture *fixture, gconstpointer pData )
{
    /* Method initializes a new book and loads data into it
     * if load fails old books are restored
     */
    QofBackend *be = NULL;
    QofBook *newbook = NULL;
    
    /* init */
    fixture->session->book_id = g_strdup( "my book" );
    be = g_new0( QofBackend, 1 );
    g_assert( be );
    fixture->session->backend = be;
    be->load = mock_load;
    
    g_test_message( "Test when no error is produced" );
    g_assert( be->percentage == NULL );
    load_session_struct.be = be;
    load_session_struct.oldbook = qof_session_get_book( fixture->session );
    g_assert_cmpint( g_list_length( fixture->session->books ), ==, 1 );
    load_session_struct.error = FALSE;
    load_session_struct.load_called = FALSE;
    qof_session_load( fixture->session, percentage_fn );
    newbook = qof_session_get_book( fixture->session );
    g_assert( newbook );
    g_assert( load_session_struct.oldbook != newbook );
    g_assert_cmpint( g_list_length( fixture->session->books ), ==, 1 );
    load_session_struct.load_called = TRUE;
    
    g_test_message( "Test when no is produced" );
    load_session_struct.oldbook = qof_session_get_book( fixture->session );
    g_assert_cmpint( g_list_length( fixture->session->books ), ==, 1 );
    load_session_struct.error = TRUE;
    load_session_struct.load_called = FALSE;
    qof_session_load( fixture->session, percentage_fn );
    newbook = qof_session_get_book( fixture->session );
    g_assert( newbook );
    g_assert( load_session_struct.oldbook == newbook );
    g_assert_cmpint( g_list_length( fixture->session->books ), ==, 1 );
    load_session_struct.load_called = TRUE;
}

static struct
{
    QofBackend *be;
    QofSession *session;
    const char *book_id;
    gboolean backend_new_called;
    gboolean session_begin_called;
    gboolean produce_error;
} session_begin_struct;

static void
mock_session_begin( QofBackend *be, QofSession *session, const char *book_id,
                    gboolean ignore_lock, gboolean create, gboolean force )
{
    g_assert( be );
    g_assert( be == session_begin_struct.be );
    g_assert( session );
    g_assert( session == session_begin_struct.session );
    g_assert( book_id );
    g_assert_cmpstr( book_id, ==, session_begin_struct.book_id );
    g_assert( ignore_lock );
    g_assert( !create );
    g_assert( force );
    if ( session_begin_struct.produce_error )
    {
	qof_backend_set_error( be, ERR_BACKEND_DATA_CORRUPT );
	qof_backend_set_message( be, "push any error" );
    }
    session_begin_struct.session_begin_called = TRUE;
}

static QofBackend* 
mock_backend_new_for_begin( void )
{
    QofBackend *be = NULL;
    
    be = g_new0( QofBackend, 1 );
    g_assert( be );
    be->session_begin = mock_session_begin;
    session_begin_struct.be = be;
    session_begin_struct.backend_new_called = TRUE;
    return be;
}

static void
test_qof_session_begin( Fixture *fixture, gconstpointer pData )
{
    gboolean ignore_lock, create, force;
    QofBackend *be = NULL;
    QofBackendProvider *prov = NULL;
    
    /* setup */
    ignore_lock = TRUE;
    create = FALSE;
    force = TRUE;
    
    be = g_new0( QofBackend, 1 );
    g_assert( be );
    g_assert_cmpint( g_slist_length( get_provider_list() ), ==, 0 );
    prov = g_new0( QofBackendProvider, 1 );
    prov->backend_new = mock_backend_new_for_begin;
    
    /* run tests */
    g_test_message( "Test when book_id is set backend is not changed" );
    fixture->session->backend = be;
    fixture->session->book_id = g_strdup("my book");
    qof_session_begin( fixture->session, "my book", ignore_lock, create, force );
    g_assert( fixture->session->backend == be );
    
    g_test_message( "Test when session book_id is not set and book_id passed is null backend is not changed" );
    g_free( fixture->session->book_id );
    fixture->session->book_id = NULL;
    qof_session_begin( fixture->session, NULL, ignore_lock, create, force );
    g_assert( fixture->session->backend == be );
    
    g_test_message( "Test default access_method parsing" );
    /* routine will destroy old backend 
     * parse access_method as 'file' and try to find backend
     * as there is no backend registered error will be raised
     */
    qof_session_begin( fixture->session, "default_should_be_file", ignore_lock, create, force );
    g_assert( fixture->session->backend == NULL );
    g_assert( fixture->session->book_id == NULL );
    g_assert_cmpint( qof_session_get_error( fixture->session ), ==, ERR_BACKEND_NO_HANDLER );
    g_assert_cmpstr( qof_session_get_error_message( fixture->session ), ==, "failed to load 'file' using access_method" );
    
    g_test_message( "Test access_method parsing" );
    qof_session_begin( fixture->session, "postgres://localhost:8080", ignore_lock, create, force );
    g_assert( fixture->session->backend == NULL );
    g_assert( fixture->session->book_id == NULL );
    g_assert_cmpint( qof_session_get_error( fixture->session ), ==, ERR_BACKEND_NO_HANDLER );
    g_assert_cmpstr( qof_session_get_error_message( fixture->session ), ==, "failed to load 'postgres' using access_method" );
    
    g_test_message( "Test with valid backend returned and session begin set; error is produced" );
    session_begin_struct.session = fixture->session;
    session_begin_struct.book_id = "postgres://localhost:8080";
    session_begin_struct.backend_new_called = FALSE;
    session_begin_struct.session_begin_called = FALSE;
    session_begin_struct.produce_error = TRUE;
    prov->access_method = "postgres";
    qof_backend_register_provider( prov );
    qof_session_begin( fixture->session, "postgres://localhost:8080", ignore_lock, create, force );
    g_assert( fixture->session->backend );
    g_assert( session_begin_struct.be == fixture->session->backend );
    g_assert( session_begin_struct.backend_new_called == TRUE );
    g_assert( session_begin_struct.session_begin_called == TRUE );
    g_assert( fixture->session->book_id == NULL );
    g_assert_cmpint( qof_session_get_error( fixture->session ), ==, ERR_BACKEND_DATA_CORRUPT );
    g_assert_cmpstr( qof_session_get_error_message( fixture->session ), ==, "push any error" );
    
    g_test_message( "Test normal session_begin execution" );
    session_begin_struct.backend_new_called = FALSE;
    session_begin_struct.session_begin_called = FALSE;
    session_begin_struct.produce_error = FALSE;
    qof_session_begin( fixture->session, "postgres://localhost:8080", ignore_lock, create, force );
    g_assert( fixture->session->backend );
    g_assert( session_begin_struct.be == fixture->session->backend );
    g_assert( session_begin_struct.backend_new_called == TRUE );
    g_assert( session_begin_struct.session_begin_called == TRUE );
    g_assert( fixture->session->book_id );
    g_assert_cmpstr( fixture->session->book_id, ==, "postgres://localhost:8080" );
    g_assert_cmpint( qof_session_get_error( fixture->session ), ==, ERR_BACKEND_NO_ERR );
    
    unregister_all_providers();
}

static struct
{
    QofBackend *be;
    QofBook *book;
    QofSession *session;
    const char *book_id;
    gboolean sync_called;
    gboolean backend_new_called;
    gboolean session_begin_called;
} session_save_struct;

static void
mock_sync( QofBackend *be, QofBook *book )
{
    g_assert( be );
    g_assert( book );
    g_assert( be == session_save_struct.be );
    g_assert( book == session_save_struct.book );
    session_save_struct.sync_called = TRUE;
}

static void
mock_session_begin_for_save( QofBackend *be, QofSession *session, const char *book_id,
			     gboolean ignore_lock, gboolean create, gboolean force )
{
    g_assert( be );
    g_assert( be == session_save_struct.be );
    g_assert( session );
    g_assert( session == session_save_struct.session );
    g_assert( book_id );
    g_assert_cmpstr( book_id, ==, session_save_struct.book_id );
    g_assert( ignore_lock );
    g_assert( create );
    g_assert( force );
    session_save_struct.session_begin_called = TRUE;
}

static QofBackend* 
mock_backend_new_for_save( void )
{
    QofBackend *be = NULL;
    
    be = g_new0( QofBackend, 1 );
    g_assert( be );
    be->session_begin = mock_session_begin_for_save;
    be->sync = mock_sync;
    session_save_struct.be = be;
    session_save_struct.backend_new_called = TRUE;
    return be;
}

static void
test_qof_session_save( Fixture *fixture, gconstpointer pData )
{
    QofBook *book = NULL;
    QofBackend *be = NULL;
    QofBackendProvider *prov = NULL, *reg_prov = NULL;
    
    g_test_message( "Test when book not partial and backend not set" );
    g_assert( fixture->session->backend == NULL );
    book = qof_session_get_book( fixture->session );
    g_assert( book );
    qof_book_set_data( book, PARTIAL_QOFBOOK, GINT_TO_POINTER( FALSE ) );
    qof_session_push_error( fixture->session, ERR_BACKEND_DATA_CORRUPT, "push any error");
    g_assert_cmpint( fixture->session->lock, ==, 1 );
    qof_session_save( fixture->session, NULL );
    g_assert_cmpint( qof_session_get_error( fixture->session ), ==, ERR_BACKEND_NO_HANDLER );
    g_assert_cmpstr( qof_session_get_error_message( fixture->session ), ==, "failed to load backend" );
    g_assert_cmpint( fixture->session->lock, ==, 1 );
    
    g_test_message( "Test when book not partial and backend set; imitate error" );
    be = g_new0( QofBackend, 1 );
    g_assert( be );
    be->sync = mock_sync;
    fixture->session->backend = be;
    g_assert_cmpint( fixture->session->lock, ==, 1 );
    session_save_struct.sync_called = FALSE;
    session_save_struct.be = be;
    session_save_struct.book = book;
    qof_backend_set_error( be, ERR_BACKEND_DATA_CORRUPT );
    qof_backend_set_message( be, "push any error" );
    qof_session_save( fixture->session, percentage_fn );
    g_assert( qof_book_get_backend( book ) == be );
    g_assert( be->percentage == percentage_fn );
    g_assert( session_save_struct.sync_called );
    g_assert_cmpint( fixture->session->lock, ==, 1 );
    g_assert_cmpint( qof_session_get_error( fixture->session ), ==, ERR_BACKEND_DATA_CORRUPT );
    g_assert_cmpstr( qof_session_get_error_message( fixture->session ), ==, "" );
    
    g_test_message( "Test when book not partial and backend set; successful save" );
    g_assert_cmpint( fixture->session->lock, ==, 1 );
    session_save_struct.sync_called = FALSE;
    qof_session_save( fixture->session, percentage_fn );
    g_assert( qof_book_get_backend( book ) == be );
    g_assert( be->percentage == percentage_fn );
    g_assert( session_save_struct.sync_called );
    g_assert_cmpint( fixture->session->lock, ==, 1 );
    g_assert_cmpint( qof_session_get_error( fixture->session ), ==, ERR_BACKEND_NO_ERR );
    
    /* change backend testing 
     * code probably should be moved to separate routine or some existing code can be reused
     * for example: qof_session_load_backend
     */
    g_test_message( "Test when book is partial and current backend supports it; successful save backend not changed" );
    prov = g_new0( QofBackendProvider, 1 );
    prov->partial_book_supported = TRUE;
    fixture->session->backend->provider = prov;
    g_assert_cmpint( fixture->session->lock, ==, 1 );
    qof_book_set_data( book, PARTIAL_QOFBOOK, GINT_TO_POINTER( TRUE ) );
    session_save_struct.sync_called = FALSE;
    qof_session_save( fixture->session, percentage_fn );
    g_assert( fixture->session->backend == be );
    g_assert( fixture->session->backend->provider == prov );
    g_assert( qof_book_get_backend( book ) == be );
    g_assert( be->percentage == percentage_fn );
    g_assert( session_save_struct.sync_called );
    g_assert_cmpint( fixture->session->lock, ==, 1 );
    g_assert_cmpint( qof_session_get_error( fixture->session ), ==, ERR_BACKEND_NO_ERR );
    
    g_test_message( "Test when book is partial and current backend does not support it; backend should be changed" );
    prov->partial_book_supported = FALSE;
    g_assert_cmpint( fixture->session->lock, ==, 1 );
    reg_prov = g_new0( QofBackendProvider, 1 );
    reg_prov->partial_book_supported = TRUE;
    reg_prov->backend_new = mock_backend_new_for_save;
    qof_backend_register_provider( reg_prov );
    g_assert_cmpint( g_slist_length( get_provider_list() ), ==, 1 );
    session_save_struct.book = book;
    session_save_struct.session = fixture->session;
    fixture->session->book_id = g_strdup( "my book" );
    session_save_struct.book_id = "my book";
    session_save_struct.sync_called = FALSE;
    session_save_struct.backend_new_called = FALSE;
    session_save_struct.session_begin_called = FALSE;
    
    qof_session_save( fixture->session, percentage_fn );
    
    g_assert( session_save_struct.backend_new_called );
    g_assert( fixture->session->backend == session_save_struct.be );
    g_assert( fixture->session->backend->provider == reg_prov );
    g_assert( fixture->session->book_id == NULL );
    g_assert( session_save_struct.session_begin_called );
    g_assert( qof_book_get_backend( book ) == session_save_struct.be );
    g_assert( session_save_struct.sync_called );
    g_assert_cmpint( fixture->session->lock, ==, 1 );
    g_assert_cmpint( qof_session_get_error( fixture->session ), ==, ERR_BACKEND_NO_ERR );
    
    unregister_all_providers();
    g_free( prov );
}

void
test_suite_qofsession ( void )
{
    GNC_TEST_ADD_FUNC( suitename, "qof session new and destroy", test_qof_session_new_destroy );
    GNC_TEST_ADD( suitename, "qof session safe save", Fixture, NULL, setup, test_session_safe_save, teardown );
    GNC_TEST_ADD( suitename, "qof instance foreach copy", Fixture, NULL, setup, test_qof_instance_foreach_copy, teardown );
    GNC_TEST_ADD( suitename, "qof instance list foreach", Fixture, NULL, setup, test_qof_instance_list_foreach, teardown );
    GNC_TEST_ADD( suitename, "qof session load backend", Fixture, NULL, setup, test_qof_session_load_backend, teardown );
    GNC_TEST_ADD( suitename, "qof session load", Fixture, NULL, setup, test_qof_session_load, teardown );
    GNC_TEST_ADD( suitename, "qof session begin", Fixture, NULL, setup, test_qof_session_begin, teardown );
    GNC_TEST_ADD( suitename, "qof session save", Fixture, NULL, setup, test_qof_session_save, teardown );
}
