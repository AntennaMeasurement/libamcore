#ifndef AMCORE_OPERATION_LOG_PATH_H
#define AMCORE_OPERATION_LOG_PATH_H

/* The caller owns the returned UTF-8 path. Errors are reported to stderr. */
char *amcore_operation_log_path(int create_directory);

#endif
