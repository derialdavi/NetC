#ifdef TEST

#include "unity.h"

#include "netc_server.h"
#include "ctsl.h"
#include "netc_http.h"

#include <stdio.h>
#include <stdint.h>
#include <hashtable.h>

extern netc server;

void *test_handler(http_request *req, http_response *res)
{
    (void)req; (void)res;
    puts("task eseguito");
    return NULL;
}

void setUp(void)
{
}

void tearDown(void)
{
}

void test_netc_server_setup_ShouldSetupCorrectly(void)
{
    netc_setup(8080, "logs/test.log");

    TEST_ASSERT_EQUAL_UINT16(8080, server.listening_port);
    TEST_ASSERT_EQUAL_size_t(5, server.backlog_number);
    TEST_ASSERT_FALSE(server.logger.is_terminal);
    TEST_ASSERT_NOT_NULL(server.endpoint_map);
    TEST_ASSERT_NOT_NULL(server.threadpool);

    netc_destroy();
}

void test_netc_server_add_endpoint_ShouldFailToAddHandlerWithInvalidArguments(void)
{
    netc_setup(8080, "logs/test.txt");

    TEST_ASSERT_FALSE(netc_add_endpoint(NULL, NULL, NULL));
    TEST_ASSERT_FALSE(netc_add_endpoint(NULL, NULL, test_handler));
    TEST_ASSERT_FALSE(netc_add_endpoint(NULL, "/", NULL));
    TEST_ASSERT_FALSE(netc_add_endpoint(NULL, "/", test_handler));
    TEST_ASSERT_FALSE(netc_add_endpoint(GET, NULL, NULL));
    TEST_ASSERT_FALSE(netc_add_endpoint(GET, NULL, test_handler));
    TEST_ASSERT_FALSE(netc_add_endpoint(GET, "/", NULL));

    netc_destroy();
}

void test_netc_server_add_endpoint_ShouldAddHandlerWithValidArguments(void)
{
    netc_setup(8080, "logs/test.txt");

    TEST_ASSERT_TRUE(netc_add_endpoint(GET, "/", test_handler));
    void *(**got_function)(http_request *, http_response*)
        = hashtable_get(server.endpoint_map, "GET/");
    TEST_ASSERT_NOT_NULL(*got_function);
    TEST_ASSERT_EQUAL_PTR(test_handler, *got_function);
    free(got_function);

    TEST_ASSERT_TRUE(netc_add_endpoint(POST, "/users", test_handler));
    got_function = hashtable_get(server.endpoint_map, "POST/users");
    TEST_ASSERT_NOT_NULL(*got_function);
    TEST_ASSERT_EQUAL_PTR(test_handler, *got_function);
    free(got_function);

    netc_destroy();
}

#endif // TEST
