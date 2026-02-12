/* after sudo make install you can import the library with <> */
#include <netc_server.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

void *index_handler(http_request *req, http_response *res)
{
    http_response_add_body(res, "ciao questo è il body");
    return NULL;
}

void *user_handler(http_request *req, http_response *res)
{
    http_response_add_body(res, "{users: [{'name': 'Davide'}, {'name': 'sissi'}]}");
    return NULL;
}

int main(void)
{
    netc_setup(8080, NULL, 10);

    netc_add_endpoint(GET, "/", index_handler);
    netc_add_endpoint(GET, "/users", user_handler);

    netc_run();

    return 0;
}