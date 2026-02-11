#ifdef TEST

#include "unity.h"

#include "netc_http.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void setUp(void)
{
}

void tearDown(void)
{
}

void test_netc_http_InvalidRequestStringShouldFailParse(void)
{
    http_request *request = http_request_parse(NULL);
    TEST_ASSERT_NULL(request);

    request = http_request_parse("");
    TEST_ASSERT_NULL(request);

    request = http_request_parse("INVALID REQUEST");
    TEST_ASSERT_NULL(request);

    request = http_request_parse("GET /index.html");
    TEST_ASSERT_NULL(request);

    request = http_request_parse("GET /index.html HTTP/1.1");
    TEST_ASSERT_NULL(request);

    request = http_request_parse("GET /index.html HTTP/1.1\r\n");
    TEST_ASSERT_NULL(request);

    request = http_request_parse("GET /index.html HTTP/1.1\r\nHost: example.com\r\nThis is body");

    request = http_request_parse("GET /index.html HTTP/1.1\r\nHost: example.com\r\nThis is body\r\n");
    TEST_ASSERT_NULL(request);
}

void test_netc_http_ValidRequestStringShouldParseCorrectly(void)
{
    const char *raw_request = "GET /index.html HTTP/1.1\r\nHost: example.com\r\nUser-Agent: TestAgent\r\n\r\nThis is body";
    http_request *request = http_request_parse(raw_request);
    TEST_ASSERT_NOT_NULL(request);
    TEST_ASSERT_EQUAL_STRING("GET", request->method);
    TEST_ASSERT_EQUAL_STRING("/index.html", request->path);
    TEST_ASSERT_EQUAL_STRING("HTTP/1.1", request->version);

    /* check headers */
    TEST_ASSERT_NOT_NULL(request->headers);

    char *header_value = hashtable_get(request->headers, "Host");
    TEST_ASSERT_NOT_NULL(header_value);
    TEST_ASSERT_EQUAL_STRING("example.com", header_value);
    free(header_value);
    header_value = hashtable_get(request->headers, "User-Agent");
    TEST_ASSERT_NOT_NULL(header_value);
    TEST_ASSERT_EQUAL_STRING("TestAgent", header_value);
    free(header_value);

    /* check body */
    TEST_ASSERT_NOT_NULL(request->body);
    TEST_ASSERT_EQUAL_STRING("This is body", request->body);

    /* cleanup */
    http_request_free(request);
}

void test_netc_http_request_get_header_ShouldReturnNullWithInvalidArguments(void)
{
    http_request request = { 0 };
    TEST_ASSERT_FALSE(http_request_get_header(NULL, NULL));
    TEST_ASSERT_FALSE(http_request_get_header(NULL, "Header-Name"));
    TEST_ASSERT_FALSE(http_request_get_header(&request, NULL));
}

void test_netc_http_request_get_header_ShouldReturnNullWithNonExistingHeader(void)
{
    const char *raw_request = "GET /index.html HTTP/1.1\r\nHost: example.com\r\nUser-Agent: TestAgent\r\n\r\nThis is body";
    http_request *request = http_request_parse(raw_request);

    TEST_ASSERT_NULL(http_request_get_header(request, "Authorization"));

    http_request_free(request);
}

void test_netc_http_request_get_header_ShouldReturnCorrectValueCopy(void)
{
    const char *raw_request = "GET /index.html HTTP/1.1\r\nHost: example.com\r\nUser-Agent: TestAgent\r\n\r\nThis is body";
    http_request *request = http_request_parse(raw_request);

    char *header_value = http_request_get_header(request, "Host");
    TEST_ASSERT_NOT_NULL(header_value);
    TEST_ASSERT_EQUAL_STRING("example.com", header_value);
    free(header_value);

    http_request_free(request);
}

void test_netc_http_ResponseDefaultShouldFailIfPointerIsInvalid(void)
{
    TEST_ASSERT_FALSE(http_response_default(NULL));
}

void test_netc_http_ShouldBuildValidDefaultResponse(void)
{
    http_response response = { 0 };
    TEST_ASSERT_TRUE(http_response_default(&response));
    char *header_value = NULL;

    TEST_ASSERT_EQUAL_INT(HTTP_STATUS_OK, response.status_code);
    TEST_ASSERT_EQUAL_STRING("OK", response.status_text);

    TEST_ASSERT_NOT_NULL(response.headers);
    header_value = hashtable_get(response.headers, "Server");
    TEST_ASSERT_NOT_NULL(header_value);
    TEST_ASSERT_EQUAL_STRING("NetC", header_value);
    free(header_value);
    header_value = hashtable_get(response.headers, "Connection");
    TEST_ASSERT_NOT_NULL(header_value);
    TEST_ASSERT_EQUAL_STRING("Close", header_value);
    free(header_value);

    TEST_ASSERT_NULL(response.body);

    http_response_free(&response);
}

void test_netc_http_response_set_status_ShouldFailWithInvalidArguments(void)
{
    http_response response = { 0 };
    TEST_ASSERT_FALSE(http_response_set_status(NULL, 199));
    TEST_ASSERT_FALSE(http_response_set_status(NULL, 200));
    TEST_ASSERT_FALSE(http_response_set_status(&response, 199));
}

void test_netc_http_response_set_status_ShouldOnlyChangeStatus(void)
{
    http_response response = { 0 };
    http_response_default(&response);

    TEST_ASSERT_TRUE(http_response_set_status(&response, HTTP_STATUS_NOT_FOUND));
    TEST_ASSERT_EQUAL_UINT16(HTTP_STATUS_NOT_FOUND, response.status_code);
    TEST_ASSERT_EQUAL_STRING("Not found", response.status_text);

    char *expected_headers[] = { "Connection", "Server" };
    char **headers = (char**) hashtable_keyset(response.headers);
    TEST_ASSERT_EQUAL_STRING_ARRAY(expected_headers, headers, 2);
    TEST_ASSERT_NULL(headers[2]);
    free(headers);

    http_response_free(&response);
}

void test_netc_http_response_add_header_ShouldFailWithInvalidArguments(void)
{
    http_response response = { 0 };
    http_response_default(&response);

    TEST_ASSERT_FALSE(http_response_add_header(NULL, NULL, NULL));
    TEST_ASSERT_FALSE(http_response_add_header(NULL, NULL, "Valore"));
    TEST_ASSERT_FALSE(http_response_add_header(NULL, "Chiave", NULL));
    TEST_ASSERT_FALSE(http_response_add_header(NULL, "Chiave", "Valore"));
    TEST_ASSERT_FALSE(http_response_add_header(&response, NULL, NULL));
    TEST_ASSERT_FALSE(http_response_add_header(&response, NULL, "Valore"));
    TEST_ASSERT_FALSE(http_response_add_header(&response, "Chiave", NULL));

    http_response_free(&response);
}

void test_netc_http_response_add_header_ShouldOnlyAddHeader(void)
{
    http_response response = { 0 };
    http_response_default(&response);

    TEST_ASSERT_TRUE(http_response_add_header(&response, "Authorization", "Bearer"));

    TEST_ASSERT_EQUAL_UINT16(HTTP_STATUS_OK, response.status_code);
    TEST_ASSERT_EQUAL_STRING("OK", response.status_text);

    char *expected_headers[] = { "Connection", "Authorization", "Server" };
    char **headers = (char**) hashtable_keyset(response.headers);
    TEST_ASSERT_EQUAL_STRING_ARRAY(expected_headers, headers, 3);
    TEST_ASSERT_NULL(headers[3]);
    free(headers);

    TEST_ASSERT_NULL(response.body);

    http_response_free(&response);
}

void test_netc_http_response_add_body_ShouldFailWithInvalidArguments(void)
{
    http_response response = { 0 };
    http_response_default(&response);

    TEST_ASSERT_FALSE(http_response_add_body(NULL, NULL));
    TEST_ASSERT_FALSE(http_response_add_body(NULL, "Body"));
    TEST_ASSERT_FALSE(http_response_add_body(&response, NULL));

    http_response_free(&response);
}

void test_netc_http_response_add_body_ShouldAddBodyAndContentLengthHeader(void)
{
    http_response response = { 0 };
    http_response_default(&response);

    TEST_ASSERT_TRUE(http_response_add_body(&response, "This is a beautiful body!"));

    TEST_ASSERT_EQUAL_UINT16(HTTP_STATUS_OK, response.status_code);
    TEST_ASSERT_EQUAL_STRING("OK", response.status_text);

    TEST_ASSERT_NOT_NULL(response.body);
    TEST_ASSERT_EQUAL_STRING("This is a beautiful body!", response.body);

    char *expected_headers[] = { "Connection", "Server", "Content-Length" };
    char **headers = (char**) hashtable_keyset(response.headers);
    TEST_ASSERT_EQUAL_STRING_ARRAY(expected_headers, headers, 3);
    TEST_ASSERT_NULL(headers[3]);
    free(headers);

    http_response_free(&response);
}

void test_netc_http_response_to_string_ShouldReturnNullWithInvalidArguments(void)
{
    TEST_ASSERT_NULL(http_response_to_string(NULL));
}

void test_netc_http_response_to_string_ShouldReturnValidHttpResponseString(void)
{
    http_response response = { 0 };
    http_response_default(&response);
    http_response_add_header(&response, "X-Custom-Header", "CustomValue");
    http_response_add_body(&response, "This is the body");

    char *response_string = http_response_to_string(&response);
    TEST_ASSERT_NOT_NULL(response_string);

    const char *expected_response =
        "HTTP/1.1 200 OK\r\n"
        "Connection: Close\r\n"
        "X-Custom-Header: CustomValue\r\n"
        "Server: NetC\r\n"
        "Content-Length: 17\r\n"
        "\r\n"
        "This is the body";

    TEST_ASSERT_EQUAL_MEMORY(expected_response, response_string, strlen(expected_response));

    free(response_string);
    http_response_free(&response);
}

#endif // TEST
