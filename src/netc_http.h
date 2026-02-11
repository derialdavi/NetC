#ifndef NETC_HTTP_H
#define NETC_HTTP_H

#include <stdint.h>
#include <hashtable.h>

#define GET     "GET"
#define POST    "POST"
#define PUT     "PUT"
#define DELETE  "DELETE"
#define HEAD    "HEAD"
#define OPTIONS "OPTIONS"
#define PATCH   "PATCH"
#define CONNECT "CONNECT"
#define TRACE   "TRACE"

#define HTTP_STATUS_OK                    (uint16_t) 200
#define HTTP_STATUS_NOT_FOUND             (uint16_t) 404
#define HTTP_STATUS_INTERNAL_SERVER_ERROR (uint16_t) 500

extern const char *http_methods[];
extern const uint8_t http_methods_count;
extern hashtable *http_hashtable;

typedef struct
{
    char       method[8];
    char       path[256];
    char       version[16];
    hashtable *headers;
    char      *body;
} http_request;

typedef struct
{
    uint16_t   status_code;
    char      *status_text;
    hashtable *headers;
    char      *body;
} http_response;

__attribute__((constructor)) void init_http_hashtable();
__attribute__((destructor))  void destroy_http_hashtable();

/**
 * @brief reads an http request from a raw string and returns a structured
 * http_request object. If not NULL, the returned pointer must be freed by
 * the caller
 *
 * @param raw_request the raw http request string
 * @return http_request* a pointer to a http_request object, or NULL if the
 * input is invalid
 */
http_request *http_request_parse(const char *raw_request);

/**
 * @brief get a copy of the header value. If not NULL, the returned pointer
 * must be freed by the caller
 *
 * @param request pointer to the request where to find the header
 * @param header_name string containing the header name
 * @return void* pointer to the copy of the value, NULL on error
 */
void *http_request_get_header(const http_request *request, const char *header_name);

/**
 * @brief frees memory taken by a request
 *
 * @param request pointer to the request to free
 */
void http_request_free(http_request *request);

/**
 * @brief sets the value for a default NetC response
 *
 * @param response pointer to the response to edit
 * @return true on success
 * @return false on failure
 */
bool http_response_default(http_response *response);

/**
 * @brief edit the status code and sets the relative status
 * text based on the status_code argument
 *
 * @param response pointer to the response to edit
 * @param status_code status code to set
 * @return true on success
 * @return false on failure
 */
bool http_response_set_status(http_response *response, const uint16_t status_code);

/**
 * @brief add a key-value pair for the headers hashtable
 *
 * @param resonse pointer to the response to edit
 * @param key name of the header
 * @param value value of the header
 * @return true on success add
 * @return false on failure
 */
bool http_response_add_header(http_response *resonse,
                              const char *key, const char *value);

/**
 * @brief add a body to the response
 *
 * @param response pointer to the response to edit
 * @param body pointer to the text of the body
 * @return true on success
 * @return false on failure
 */
bool http_response_add_body(http_response *response, const char *body);

/**
 * @brief return a string in the correct http format based on the
 * response data. If not NULL, the returned pointer must be freed
 * by the caller
 *
 * @param response pointer to the response to get the data from
 * @return char* pointer to the allocated string response
 */
char *http_response_to_string(const http_response *response);

/**
 * @brief frees memory taken by a request
 *
 * @param response pointer to the response to free
 */
void http_response_free(http_response *response);

#endif // NETC_HTTP_H
