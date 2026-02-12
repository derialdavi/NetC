#include "netc_server.h"

#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>

struct context
{
    int            client_sfd;
    http_request  *request;
    void*        (**handler_function)(http_request*, http_response*);
};

void bind_server(const size_t port);
void netc_shutdown_signal_handler(int sig);
char *read_client_socket(int client_sfd);
int accept_client();
void *endpoint_default_middleware(void *context);

netc server;

void netc_setup(const uint16_t port, const char *log_filename)
{
    if (ctsl_init(&server.logger, log_filename) == false)
    {
        fprintf(stderr, "Error initializing logger, won't be able to print any log...\n");
        return;
    }

    /* create and setup socket */
    server.linstening_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server.linstening_socket_fd < 0)
    {
        char *err_msg = strerror(errno);
        ctsl_print(&server.logger, CTSL_ERROR, "Error creating socket: %s", err_msg);
        ctsl_destroy(&server.logger);
        exit(EXIT_FAILURE);
    }

    const int opt = 1;
    if (setsockopt(server.linstening_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        char *err_msg = strerror(errno);
        ctsl_print(&server.logger, CTSL_ERROR, "Error setting up socket: %s", err_msg);
        ctsl_destroy(&server.logger);
        exit(EXIT_FAILURE);
    }

    bind_server(port);

    /* NetC configuration */
    server.listening_port = port;
    server.backlog_number = 5;
    server.endpoint_map = hashtable_create(hash_string, compare_string);
    if (server.endpoint_map == NULL)
    {
        char *err_msg = strerror(errno);
        ctsl_print(&server.logger, CTSL_ERROR, "Error creating endpoint hashtable: %s", err_msg);
        ctsl_destroy(&server.logger);
        exit(EXIT_FAILURE);
    }
    server.threadpool = threadpool_create(10);
    if (server.threadpool == NULL)
    {
        char *err_msg = strerror(errno);
        ctsl_print(&server.logger, CTSL_ERROR, "Error creating server threadpool: %s", err_msg);
        ctsl_destroy(&server.logger);
        hashtable_destroy(server.endpoint_map);
        exit(EXIT_FAILURE);
    }

    ctsl_print(&server.logger, CTSL_INFO, "Server setted up successfully!");
}

bool netc_add_endpoint(const char *method, const char *path,
                       void *(*endpoint_handler)(http_request*, http_response*))
{
    if (method == NULL || path == NULL || endpoint_handler == NULL)
    {
        ctsl_print(&server.logger, CTSL_WARNING, "Invalid endpoint or handler function");
        return false;
    }

    /* concatenate method and path */
    size_t size = strlen(method) + strlen(path) + 1;
    char *key;
    if ((key = malloc(size)) == NULL)
    {
        char *err_msg = strerror(errno);
        ctsl_print(&server.logger, CTSL_WARNING, "Failed to allocate memory for |%s %s| endpoint: %s", method, path, err_msg);
        return false;
    }
    snprintf(key, size, "%s%s", method, path);

    /* insert endpoint and respective handler function to hashtable */
    if (hashtable_put(server.endpoint_map, key, strlen(key) + 1, &endpoint_handler, sizeof(endpoint_handler)) == false)
    {
        ctsl_print(&server.logger, CTSL_WARNING, "Failed to add |%s %s| endpoint", method, path);
        return false;
    }
    free(key);

    return true;
}

void netc_run(void)
{
    if (listen(server.linstening_socket_fd, server.backlog_number) < 0)
    {
        char *err_msg = strerror(errno);
        ctsl_print(&server.logger, CTSL_ERROR, "Error while starting listening for new collection: %s", err_msg);
        netc_destroy();
        exit(EXIT_FAILURE);
    }

    struct sigaction act = { 0 };
    act.sa_handler = netc_shutdown_signal_handler;
    if (sigaction(SIGINT, &act, NULL) == -1)
    {
        char *err_msg = strerror(errno);
        ctsl_print(&server.logger, CTSL_ERROR, "Error while setting up new SIGINT handler: %s", err_msg);
        netc_destroy();
        exit(EXIT_FAILURE);
    }

    ctsl_print(&server.logger, CTSL_INFO, "Listening for new connections at %d\n", server.listening_port);
    while(true)
    {
        int client_sfd = accept_client();
        if (client_sfd < 0)
            continue;

        char *request_string = read_client_socket(client_sfd);

        http_request *request = http_request_parse(request_string);
        if (request == NULL)
        {
            ctsl_print(&server.logger, CTSL_ERROR, "Error while parsing request string: %s", request_string);
            free(request_string);
            continue;
        }
        free(request_string);

        size_t endpoint_len = strlen(request->method) + strlen(request->path) + 1;
        char *endpoint = malloc(endpoint_len);
        if (endpoint == NULL)
        {
            char *err_msg = strerror(errno);
            ctsl_print(&server.logger, CTSL_ERROR, "Error while fetching endpoint: %s", err_msg);
            continue;
        }
        snprintf(endpoint, endpoint_len, "%s%s", request->method, request->path);

        struct context *ctx = malloc(sizeof(struct context));
        if (ctx == NULL)
        {
            char *err_msg = strerror(errno);
            ctsl_print(&server.logger, CTSL_ERROR, "Error allocating memory for context: %s", err_msg);
            free(endpoint);
            http_request_free(request);
            close(ctx->client_sfd);
            continue;
        }
        ctx->client_sfd = client_sfd;
        ctx->request = request;
        ctx->handler_function = hashtable_get(server.endpoint_map, endpoint);

        if (ctx->handler_function == NULL)
        {
            ctsl_print(&server.logger, CTSL_INFO, "%s %s => 404 Not found", request->method, request->path);
            free(endpoint);
            close(ctx->client_sfd);
            http_request_free(request);
            free(ctx);
            continue;
        }

        struct task task = {
            .function = endpoint_default_middleware,
            .argp = ctx
        };

        threadpool_add(server.threadpool, &task);
        free(endpoint);
    }
}

void netc_destroy(void)
{
    close(server.linstening_socket_fd);
    threadpool_destroy(server.threadpool, true);
    hashtable_destroy(server.endpoint_map);
    ctsl_print(&server.logger, CTSL_INFO, "Closing server...");
    ctsl_destroy(&server.logger);
}

void bind_server(const size_t port)
{
    struct sockaddr_in server_info = { 0 };
    server_info.sin_family = AF_INET;
    server_info.sin_addr.s_addr = htonl(INADDR_ANY);
    server_info.sin_port = htons(port);
    if (bind(server.linstening_socket_fd, (const struct sockaddr*)&server_info, sizeof(server_info)) < 0)
    {
        char *err_message = strerror(errno);
        ctsl_print(&server.logger, CTSL_ERROR, "Error binding socket: %s", err_message);
        ctsl_destroy(&server.logger);
        exit(EXIT_FAILURE);
    }
}

int accept_client()
{
    struct sockaddr_in client_info;
    socklen_t client_info_len = sizeof(client_info);
    int client_sfd = accept(server.linstening_socket_fd, (struct sockaddr*)&client_info, &client_info_len);
    if (client_sfd < 0)
    {
        char *err_msg = strerror(errno);
        ctsl_print(&server.logger, CTSL_ERROR, "Error accepting new connection: %s", err_msg);
        return -1;
    }

    return client_sfd;
}

char *read_client_socket(int client_sfd)
{
    char buffer[DEFAULT_SOCKET_BUFFER_SIZE];
    char *read_msg = malloc(1);
    if (read_msg == NULL)
        return NULL;

    int bytes_read, total_length = 0;
    while ((bytes_read = recv(client_sfd, buffer, DEFAULT_SOCKET_BUFFER_SIZE, 0)) > 0)
    {
        total_length += bytes_read;
        char *temp = realloc(read_msg, total_length + 1);
        if (temp == NULL)
        {
            free(read_msg);
            return NULL;
        }
        read_msg = temp;
        memcpy(read_msg + (total_length - bytes_read), buffer, bytes_read);
        read_msg[total_length] = '\0';

        if ((size_t)bytes_read < DEFAULT_SOCKET_BUFFER_SIZE)
            break;
    }

    read_msg[total_length] = '\0';
    return read_msg;
}

void *endpoint_default_middleware(void *context)
{
    struct context *ctx = (struct context*)context;

    http_response res = { 0 };
    http_response_default(&res);

    (*ctx->handler_function)(ctx->request, &res);
    free(ctx->handler_function);

    char *response_string = http_response_to_string(&res);
    if (send(ctx->client_sfd, response_string, strlen(response_string), 0) <= 0)
    {
        char *err_msg = strerror(errno);
        ctsl_print(&server.logger, CTSL_ERROR, "Error sending data to client: %s", err_msg);
        free(response_string);
        return NULL;
    }
    free(response_string);

    ctsl_print(&server.logger, CTSL_INFO, "%s %s => Status %d %s", ctx->request->method, ctx->request->path, res.status_code, res.status_text);

    close(ctx->client_sfd);
    http_request_free(ctx->request);
    http_response_free(&res);
    free(ctx);
    return NULL;
}

void netc_shutdown_signal_handler(int sig)
{
    (void)sig;
    netc_destroy();
    exit(EXIT_SUCCESS);
}
