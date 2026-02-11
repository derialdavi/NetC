#include "ctsl.h"

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <strings.h>

const char* get_level_color(const char *level);

bool ctsl_init(ctsl *logger, const char *filename)
{
    if (logger == NULL) return false;

    pthread_mutex_init(&logger->shared_resource_mutex, NULL);

    if (filename == NULL)
    {
        logger->fd = STDOUT_FILENO;
        logger->is_terminal = true;
        return false;
    }

    int file_fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (file_fd < 0)
    {
        perror("Error opening log file");
        return false;
    }

    logger->fd = file_fd;
    logger->is_terminal = isatty(logger->fd);
    return true;
}

void ctsl_print(const ctsl *logger, const char *level, const char *fmt, ...)
{
    const char *color = get_level_color(level);
    if (logger == NULL || level == NULL || fmt == NULL || color == NULL)
        return;

    /* get current time */
    time_t now = time(NULL);
    struct tm t;
    localtime_r(&now, &t);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &t);

    va_list args, args_copy;
    va_start(args, fmt);
    va_copy(args_copy, args);

    int msg_len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (msg_len <= 0)
    {
        va_end(args_copy);
        return;
    }

    char *msg_buf = malloc(msg_len + 1);
    if (msg_buf == NULL)
    {
        va_end(args_copy);
        return;
    }

    vsnprintf(msg_buf, msg_len + 1, fmt, args_copy);
    va_end(args_copy);

    char *log_line;
    int total_len = logger->is_terminal
        ? asprintf(&log_line, "%s - %s%s%s - %s\n",
                   timestamp, color, level, COLOR_RESET, msg_buf)
        : asprintf(&log_line, "%s - %s - %s\n",
                   timestamp, level, msg_buf);
    free(msg_buf);

    if (total_len < 0)
        return;

    pthread_mutex_lock((pthread_mutex_t*)&logger->shared_resource_mutex);
    if (write(logger->fd, log_line, total_len) < 0)
        perror("Error writing log");
    pthread_mutex_unlock((pthread_mutex_t*)&logger->shared_resource_mutex);
    free(log_line);
}

void ctsl_destroy(ctsl *logger)
{
    if (logger->fd != STDOUT_FILENO && logger->fd != STDERR_FILENO)
        close(logger->fd);

    pthread_mutex_destroy(&logger->shared_resource_mutex);
}

const char* get_level_color(const char *level)
{
    if (strcasecmp(level, "ERROR") == 0)
        return COLOR_ERROR;
    else if (strcasecmp(level, "WARNING") == 0)
        return COLOR_WARNING;
    else if (strcasecmp(level, "INFO") == 0)
        return COLOR_INFO;
    else
        return NULL;
}
