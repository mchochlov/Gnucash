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

void
test_suite_qofsession ( void )
{
    GNC_TEST_ADD( suitename, "qof session safe save", Fixture, NULL, setup, test_session_safe_save, teardown );
    GNC_TEST_ADD( suitename, "qof instance foreach copy", Fixture, NULL, setup, test_qof_instance_foreach_copy, teardown );
}
