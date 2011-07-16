/* Modified by bstanley 20010320
 * Added do_test macro, do_test_call and do_test_call_args,
 * print_test_results, set_success_print.
 *
 * Modified by bstanley 20010323
 * removed testing functionality which depends on the rest of gnucash -
 * sepearated into gnc-test-stuff.h
 *
 */

/* Outline of a test program using the new testing functions:
#include "test-stuff.h"
int main( int argc, char* argv[] )
{
	int a, b;
	g_log_set_always_fatal( G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING );
	a = b = 1;
	do_test( a == b, "integer equality" );
	do_test( a != b, "integer inequality? (should fail)" );

	do_test_args( a == b, "fancy info", __FILE__, __LINE__, "a = %d, b = %b", a, b );

	print_test_results();
	return get_rv();
}
*/
/* If you want to see test passes, use
set_success_print(TRUE);
before you execute the tests.
Otherwise, only failures are printed out.
*/


#ifndef TEST_STUFF_H
#define TEST_STUFF_H

#include <glib.h>
#include <stdlib.h>

/**
 * Use this to indicate the result of a test.
 * The result is TRUE for success, FALSE for failure.
 * title describes the test
 * Tests are automatically identified by their source file and line.
 */
#define do_test( result, title ) do_test_call( result, title, __FILE__, __LINE__ )
#define success( title ) success_call( title, __FILE__, __LINE__ );
#define failure( title ) failure_call( title, __FILE__, __LINE__ );

/** This one doesn't work because macros can't take a variable number of arguments.
 * well, apparently gcc can, but it's non-standard.
 * Apparently C99 can, too, but it's not exactly standard either.
#define do_test_args( result, title, format ) do_test_call( result, title, __FILE__, __LINE__, format, ... );
*/

/** 
 * Use this macro to format informative test path output when using g_test_add.
 * Suite stands for tests' pack, while path for individual test name. 
*/

#define GNC_TEST_ADD( suite, path, fixture, data, setup, test, teardown )\
{\
    gchar *testpath = g_strdup_printf( "%s/%s", suite, path );\
    g_test_add( testpath, fixture, data, setup, test, teardown );\
    g_free( testpath );\
}

/** 
 * Use this macro to format informative test path output when using g_test_add_func.
 * Suite stands for tests' pack, while path for individual test name. 
*/

#define GNC_TEST_ADD_FUNC( suite, path, test )\
{\
    gchar *testpath = g_strdup_printf( "%s/%s", suite, path );\
    g_test_add_func( testpath, test );\
    g_free( testpath );\
}

/**
 * Test Support
 *
 * Struct and functions for unit test support:
 * Intercept and report GLib error messages to test functions.
 * Ensure that mock functions are called, and with the right data pointer
 *
 */

typedef struct
{
    GLogLevelFlags log_level;
    gchar *log_domain;
    gchar *msg;
} TestErrorStruct;

 /**
  * Pass this to g_test_log_set_fatal_handler(), setting user_data to
  * a pointer to TestErrorStruct to intercept and handle expected
  * error and warning messages. It will g_assert if an error is
  * received which doesn't match the log_level, log_domain, and
  * message in the struct (if they're set), or return FALSE to prevent
  * the message from aborting. Be sure to g_free() the
  * TestErrorData:msg after you're done testing it.
  */
gboolean test_handle_faults( const char *log_domain, GLogLevelFlags log_level,
			     const gchar *msg, gpointer user_data);
/**
 * When you know you're going to get a useless log message, pass this
 * to g_log_set_default_handler to shut it up.
 */
void test_silent_logger(  const char *log_domain, GLogLevelFlags log_level,
			  const gchar *msg, gpointer user_data );
/**
 * Call this from a mock object to indicate that the mock has in fact
 * been called
 */
void test_set_called( const gboolean val );

/**
 * Destructively tests (meaning that it resets called to FALSE) and
 * returns the value of called.
 */
const test_reset_called( void );

/**
 * Set the test data pointer with the what you expect your mock to be
 * called with.
 */
void test_set_data( gpointer data );

/**
 * Destructively retrieves the test data pointer. Call from your mock
 * to ensure that it received the expected data.
 */
const gpointer test_reset_data( void );

/**
 * A handy function to use to free memory from lists of simple
 * pointers. Call g_list_free_full(list, (GDestroyNotify)*test_free).
 */
void test_free( gpointer data );

/* Privately used to indicate a test result. You may use these if you
 * wish, but it's easier to use the do_test macro above.
 */
gboolean do_test_call(
    gboolean result,
    const char* test_title,
    const char* filename,
    int line );
gboolean do_test_args(
    gboolean result,
    const char* test_title,
    const char* filename,
    int line,
    const char* format, ... );


/**
 * Prints out the number of tests passed and failed.
 */
void print_test_results(void);

/**
 * Use this to set whether successful tests
 * should print a message.
 * Default is false.
 * Successful test messages are useful while initally constructing the
 * test suite, but when it's completed, no news is good news.
 * A successful test run will be indicated by the message
 * from print_test_results().
 */
void set_success_print( gboolean in_should_print );

/* Value to return from main. Set to 1 if there were any fails, 0 otherwise. */
int get_rv(void);

/** Testing primitives.
 * Sometimes you just have to put the results of
 * a test into different forks of the code.
 */
void success_call(
    const char *test_title,
    const char *file,
    int line );

void success_args(
    const char *test_title,
    const char *file,
    int line,
    const char *format,
    ... );

void failure_call(
    const char *test_title,
    const char *file,
    int line);

void failure_args(
    const char *test_title,
    const char *file,
    int line,
    const char *format,
    ... );

gboolean get_random_boolean(void);
gint get_random_int_in_range(int start, int end);
void random_character_include_funky_chars (gboolean use_funky_chars);
gchar get_random_character(void);
gchar* get_random_string(void);
gchar * get_random_string_length_in_range(int minlen, int maxlen);
gchar* get_random_string_without(const char *exclude_chars);
gint64 get_random_gint64(void);
double get_random_double(void);
const char* get_random_string_in_array(const char* str_list[]);

#endif /* TEST_STUFF_H */
