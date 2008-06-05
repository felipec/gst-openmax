#include <check.h>
#include <OMX_Core.h>

#include <dlfcn.h>

static const char *lib_name;
static void *dl_handle;
static OMX_ERRORTYPE (*init) (void);
static OMX_ERRORTYPE (*deinit) (void);
static OMX_ERRORTYPE (*get_handle) (OMX_HANDLETYPE *handle,
                                    OMX_STRING name,
                                    OMX_PTR data,
                                    OMX_CALLBACKTYPE *callbacks);
static OMX_ERRORTYPE (*free_handle) (OMX_HANDLETYPE handle);

START_TEST (test_basic)
{
    OMX_ERRORTYPE omx_error;
    omx_error = init ();
    fail_if (omx_error != OMX_ErrorNone);
    omx_error = deinit ();
    fail_if (omx_error != OMX_ErrorNone);
}
END_TEST

START_TEST (test_handle)
{
    OMX_ERRORTYPE omx_error;
    OMX_HANDLETYPE omx_handle;

    omx_error = init ();
    fail_if (omx_error != OMX_ErrorNone);

    omx_error = get_handle (&omx_handle, "OMX.check.dummy", NULL, NULL);
    fail_if (omx_error != OMX_ErrorNone);

    omx_error = free_handle (omx_handle);
    fail_if (omx_error != OMX_ErrorNone);

    omx_error = deinit ();
    fail_if (omx_error != OMX_ErrorNone);
}
END_TEST

Suite *
util_suite (void)
{
    Suite *s = suite_create ("libomxil");

    lib_name = "./standalone/libomxil-foo.so";
    {
        dl_handle = dlopen (lib_name, RTLD_LAZY);
        if (!dl_handle)
        {
            /** @todo report error. */
        }

        init = dlsym (dl_handle, "OMX_Init");
        deinit = dlsym (dl_handle, "OMX_Deinit");
        get_handle = dlsym (dl_handle, "OMX_GetHandle");
        free_handle = dlsym (dl_handle, "OMX_FreeHandle");
    }

    /* Core test case */
    TCase *tc_core = tcase_create ("Core");
    tcase_add_test (tc_core, test_basic);
    tcase_add_test (tc_core, test_handle);
    suite_add_tcase (s, tc_core);

    return s;
}

int
main (void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = util_suite ();
    sr = srunner_create (s);
    srunner_run_all (sr, CK_NORMAL);
    number_failed = srunner_ntests_failed (sr);
    srunner_free (sr);

    return (number_failed == 0) ? 0 : 1;
}
