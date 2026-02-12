#ifndef NETC_SERVER_H
#define NETC_SERVER_H

#include <stdint.h>
#include <stddef.h>
#include "ctsl.h"
#include <hashtable.h>
#include <threadpool.h>
#include "netc_http.h"

#define DEFAULT_SOCKET_BUFFER_SIZE ((size_t)4096)

typedef struct
{
    int         linstening_socket_fd;
    uint16_t    listening_port;
    size_t      backlog_number;
    ctsl        logger;
    hashtable  *endpoint_map;
    threadpool *threadpool;
} netc;

void netc_setup(const uint16_t port, const char *log_filename);

bool netc_add_endpoint(const char *method, const char *path,
                       void *(*endpoint_handler)(http_request*, http_response*));

void netc_run(void);

void netc_destroy(void);

#endif // NETC_SERVER_H
