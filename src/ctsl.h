#ifndef CTSL_H
#define CTSL_H

#include <pthread.h>

#define CTSL_INFO    "INFO"
#define CTSL_WARNING "WARNING"
#define CTSL_ERROR   "ERROR"

#define COLOR_ERROR   "\033[37;41m"
#define COLOR_WARNING "\033[30;43m"
#define COLOR_INFO    "\033[0m"
#define COLOR_RESET   "\033[0m"

typedef struct
{
    int             fd;
    bool            is_terminal;
    pthread_mutex_t shared_resource_mutex;
} ctsl;

bool ctsl_init(ctsl *logger, const char *filename);
void ctsl_print(const ctsl *logger, const char *level, const char *fmt, ...);
void ctsl_destroy(ctsl *);

#endif // CTSL_H
