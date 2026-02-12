#ifdef TEST

#include "unity.h"

#include "ctsl.h"
#include <unistd.h>
#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

void test_ctsl_NullArgumentShouldUseStdout(void)
{
    ctsl logger;
    ctsl_init(&logger, NULL);

    TEST_ASSERT_EQUAL_INT(STDOUT_FILENO, logger.fd);
    TEST_ASSERT_TRUE(logger.is_terminal);
    TEST_ASSERT_EQUAL_INT(0, pthread_mutex_trylock(&logger.shared_resource_mutex));
    pthread_mutex_unlock(&logger.shared_resource_mutex);

    ctsl_destroy(&logger);
}

void test_ctsl_NonNullArgumentShouldOpenFileAndWrite(void)
{
    ctsl logger;
    ctsl_init(&logger, "logs/test.log");

    ctsl_print(&logger, CTSL_INFO, "This is an info message");
    ctsl_print(&logger, CTSL_WARNING, "This is a warning message");
    ctsl_print(&logger, CTSL_ERROR, "This is an error message");

    ctsl_destroy(&logger);

    /* check file text */
    FILE *file = fopen("logs/test.log", "r");
    TEST_ASSERT_NOT_NULL(file);
    char buffer[256];
    TEST_ASSERT_NOT_NULL(fgets(buffer, sizeof(buffer), file));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "This is an info message"));

    TEST_ASSERT_NOT_NULL(fgets(buffer, sizeof(buffer), file));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "This is a warning message"));

    TEST_ASSERT_NOT_NULL(fgets(buffer, sizeof(buffer), file));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "This is an error message"));

    fclose(file);
}

#endif // TEST
