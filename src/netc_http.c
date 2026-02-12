#include "netc_http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

hashtable *http_hashtable;

void init_http_hashtable()
{
    http_hashtable = hashtable_create(hash_int, compare_uint8_t);
    if (http_hashtable == NULL) exit(EXIT_FAILURE);

    uint16_t key = HTTP_STATUS_OK;
    hashtable_put(http_hashtable, &key, sizeof(key), "OK", strlen("OK") + 1);

    key = HTTP_STATUS_NOT_FOUND;
    hashtable_put(http_hashtable, &key, sizeof(key), "Not found", strlen("Not found") + 1);

    key = HTTP_STATUS_INTERNAL_SERVER_ERROR;
    hashtable_put(http_hashtable, &key, sizeof(key), "Internal server error", strlen("Internal server error") + 1);
}

void destroy_http_hashtable()
{
    hashtable_destroy(http_hashtable);
}

const char *http_methods[] = {
    GET, POST, PUT, DELETE, HEAD, OPTIONS, PATCH, CONNECT, TRACE
};

const uint8_t http_methods_count = sizeof(http_methods) / sizeof(http_methods[0]);

http_request *http_request_parse(const char *raw_request)
{
    if (raw_request == NULL) return NULL;

    http_request *request = malloc(sizeof(http_request));
    if (request == NULL) return NULL;

    uint16_t offset = 0;
    if (sscanf(raw_request, "%7s %255s %15s\r\n%hn",
        request->method, request->path, request->version, &offset) != 3)
    {
        free(request);
        return NULL;
    }

    request->headers = hashtable_create(hash_string, compare_string);
    if (request->headers == NULL)
    {
        free(request);
        return NULL;
    }

    /* parse headers */
    const char *headers_start = raw_request + offset;
    const char *headers_end = strstr(headers_start, "\r\n\r\n");
    if (headers_end == NULL)
    {
        hashtable_destroy(request->headers);
        free(request);
        return NULL;
    }

    const char *line_start = headers_start;
    while (line_start < headers_end)
    {
        const char *line_end = strstr(line_start, "\r\n");
        if (line_end == NULL || line_end > headers_end)
        {
            hashtable_destroy(request->headers);
            free(request);
            return NULL;
        }

        char key[256] = { 0 }, value[256] = { 0 };
        if (sscanf(line_start, "%255[^:]: %255[^\r\n]", key, value) != 2)
        {
            hashtable_destroy(request->headers);
            free(request);
            return NULL;
        }

        if (hashtable_put(request->headers, key, strlen(key) + 1, value, strlen(value) + 1) == false)
        {
            hashtable_destroy(request->headers);
            free(request);
            return NULL;
        }
        line_start = line_end + 2;
    }

    /* parse body */
    const char *body_start = headers_end + 4;
    if (*body_start == '\0')
    {
        request->body = NULL;
    }
    else
    {
        request->body = strdup(body_start);
        if (request->body == NULL)
        {
            hashtable_destroy(request->headers);
            free(request);
            return NULL;
        }
    }

    return request;
}

void *http_request_get_header(const http_request *request, const char *header_name)
{
    if (request == NULL || header_name == NULL)
        return NULL;

    return hashtable_get(request->headers, header_name);
}

void http_request_free(http_request *request)
{
    if (request == NULL) return;

    hashtable_destroy(request->headers);
    free(request->body);
    free(request);
}

bool http_response_default(http_response *response)
{
    if (response == NULL) return false;

    response->status_code = HTTP_STATUS_OK;
    response->status_text = hashtable_get(http_hashtable, &response->status_code);

    response->headers = hashtable_create(hash_string, compare_string);
    if (response->headers == NULL) return false;

    hashtable_put(response->headers, "Server", strlen("Server") + 1, "NetC", strlen("NetC") + 1);

    response->body = NULL;
    return true;
}

bool http_response_set_status(http_response *response, const uint16_t status_code)
{
    if (response == NULL || status_code < 200 || status_code > 600)
        return false;

    response->status_code = status_code;
    free(response->status_text);
    response->status_text = hashtable_get(http_hashtable, &response->status_code);

    return response->status_text != NULL ? true : false;
}

bool http_response_add_header(http_response *response,
                              const char *key, const char *value)
{
    if (response == NULL || key == NULL || value == NULL)
        return false;

    return hashtable_put(response->headers, key, strlen(key) + 1, value, strlen(value) + 1);
}

bool http_response_add_body(http_response *response, const char *body)
{
    if (response == NULL || body == NULL)
        return false;

    size_t content_length = strlen(body);
    response->body = malloc(content_length + 1);
    if (response->body == NULL) return false;

    strcpy(response->body, body);

    char content_length_str[20];
    snprintf(content_length_str, sizeof(content_length_str), "%zu", content_length);
    return hashtable_put(response->headers, "Content-Length", strlen("Content-Length") + 1, content_length_str, strlen(content_length_str) + 1);
}

char *http_response_to_string(const http_response *response)
{
    if (response == NULL) return NULL;

    char *response_string = malloc(1024);
    if (response_string == NULL) return NULL;

    int offset = snprintf(response_string, 1024, "HTTP/1.1 %d %s\r\n",
                          response->status_code, response->status_text);
    if (offset < 0) return NULL;

    /* add headers */
    char **keys = (char**)hashtable_keyset(response->headers);
    for (size_t i = 0; keys[i] != NULL; i++)
    {
        char *value = hashtable_get(response->headers, keys[i]);
        int written = snprintf(response_string + offset, 1024 - offset, "%s: %s\r\n", keys[i], value);
        if (written < 0) return NULL;
        offset += written;
        free(value);
    }
    free(keys);

    /* add body */
    if (response->body != NULL)
    {
        int written = snprintf(response_string + offset, 1024 - offset, "\r\n%s", response->body);
        if (written < 0) return NULL;
        offset += written;
    }

    return response_string;
}

void http_response_free(http_response *response)
{
    if (response == NULL) return;

    free(response->status_text);
    hashtable_destroy(response->headers);
    free(response->body);
}
