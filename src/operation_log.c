#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "operation_log.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#endif

static void report_error(const char *message)
{
    fprintf(stderr, "amcore operation logging: %s\n", message);
}

#ifdef _WIN32
static char *local_data_root(void)
{
    const wchar_t *root = _wgetenv(L"LOCALAPPDATA");
    char *utf8;
    int length;

    if (root == NULL || root[0] == L'\0' ||
        !((root[0] != L'\0' && root[1] == L':' &&
           (root[2] == L'\\' || root[2] == L'/')) ||
          (root[0] == L'\\' && root[1] == L'\\'))) {
        report_error("LOCALAPPDATA must specify an absolute directory");
        return NULL;
    }
    length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, root, -1,
                                NULL, 0, NULL, NULL);
    if (length == 0) {
        report_error("cannot encode LOCALAPPDATA as UTF-8");
        return NULL;
    }
    utf8 = malloc((size_t)length);
    if (utf8 == NULL) {
        report_error("cannot allocate database path");
        return NULL;
    }
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, root, -1,
                             utf8, length, NULL, NULL)) {
        report_error("cannot encode LOCALAPPDATA as UTF-8");
        free(utf8);
        return NULL;
    }
    return utf8;
}

static int make_directory(const char *path)
{
    int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path,
                                    -1, NULL, 0);
    wchar_t *wide;
    DWORD attributes;
    int success;

    if (length == 0) {
        return 0;
    }
    wide = malloc((size_t)length * sizeof(*wide));
    if (wide == NULL) {
        return 0;
    }
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1,
                            wide, length)) {
        free(wide);
        return 0;
    }
    success = CreateDirectoryW(wide, NULL) != 0;
    if (!success && GetLastError() == ERROR_ALREADY_EXISTS) {
        attributes = GetFileAttributesW(wide);
        success = attributes != INVALID_FILE_ATTRIBUTES &&
                  (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }
    free(wide);
    return success;
}
#else
static char *local_data_root(void)
{
    const char *root;
    const char *suffix;
    char *path;

#ifdef __APPLE__
    root = getenv("HOME");
    suffix = "/Library/Application Support";
#else
    root = getenv("XDG_DATA_HOME");
    suffix = "";
    if (root == NULL || root[0] != '/') {
        root = getenv("HOME");
        suffix = "/.local/share";
    }
#endif
    if (root == NULL || root[0] != '/') {
        report_error("user data location must specify an absolute directory");
        return NULL;
    }
    path = malloc(strlen(root) + strlen(suffix) + 1);
    if (path == NULL) {
        report_error("cannot allocate database path");
        return NULL;
    }
    snprintf(path, strlen(root) + strlen(suffix) + 1, "%s%s", root, suffix);
    return path;
}

static int make_directory(const char *path)
{
    struct stat info;
    if (mkdir(path, 0700) == 0) {
        return 1;
    }
    return errno == EEXIST && stat(path, &info) == 0 && S_ISDIR(info.st_mode);
}
#endif

static int ensure_directory(char *path)
{
    char *separator;
    int success;

    if (make_directory(path)) {
        return 1;
    }
    separator = strrchr(path, '/');
#ifdef _WIN32
    {
        char *backslash = strrchr(path, '\\');
        if (backslash != NULL && (separator == NULL || backslash > separator)) {
            separator = backslash;
        }
    }
#endif
    if (separator == NULL || separator == path) {
        return 0;
    }
    {
        char saved = *separator;
        *separator = '\0';
        success = ensure_directory(path);
        *separator = saved;
    }
    return success && make_directory(path);
}

static char *database_path(void)
{
    char *root = local_data_root();
    char *path;
    size_t directory_length;
    size_t path_capacity;
#ifdef _WIN32
    const char *directory = "\\" AMCORE_LOG_PROJECT_NAME;
    const char *filename = "\\" AMCORE_LOG_PROJECT_NAME ".db";
#else
    const char *directory = "/" AMCORE_LOG_PROJECT_NAME;
    const char *filename = "/" AMCORE_LOG_PROJECT_NAME ".db";
#endif

    if (root == NULL) {
        return NULL;
    }
    directory_length = strlen(root) + strlen(directory);
    path_capacity = directory_length + strlen(filename) + 1;
    path = malloc(path_capacity);
    if (path == NULL) {
        report_error("cannot allocate database path");
        free(root);
        return NULL;
    }
    snprintf(path, path_capacity, "%s%s", root, directory);
    free(root);
    if (!ensure_directory(path)) {
        report_error("cannot create user data directory");
        free(path);
        return NULL;
    }
    snprintf(path + directory_length, path_capacity - directory_length,
             "%s", filename);
    return path;
}

amcore_operation_start amcore_operation_begin(void)
{
    amcore_operation_start start = {0, 0, 0};
#ifdef _WIN32
    FILETIME wall;
    ULARGE_INTEGER value;
    LARGE_INTEGER ticks;

    GetSystemTimeAsFileTime(&wall);
    value.LowPart = wall.dwLowDateTime;
    value.HighPart = wall.dwHighDateTime;
    start.executed_at_us = (int64_t)(value.QuadPart / 10) - 11644473600000000LL;
    if (!QueryPerformanceCounter(&ticks)) {
        report_error("cannot read monotonic clock");
        return start;
    }
    start.ticks = ticks.QuadPart;
#else
    struct timespec wall;
    struct timespec monotonic;

    if (clock_gettime(CLOCK_REALTIME, &wall) != 0 ||
        clock_gettime(CLOCK_MONOTONIC, &monotonic) != 0) {
        report_error("cannot read operation clocks");
        return start;
    }
    start.executed_at_us = (int64_t)wall.tv_sec * 1000000 + wall.tv_nsec / 1000;
    start.ticks = (int64_t)monotonic.tv_sec * 1000000000 + monotonic.tv_nsec;
#endif
    start.valid = 1;
    return start;
}

static int elapsed_ns(amcore_operation_start start, int64_t *duration)
{
#ifdef _WIN32
    LARGE_INTEGER ticks;
    LARGE_INTEGER frequency;
    int64_t delta;
    if (!QueryPerformanceCounter(&ticks) ||
        !QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0) {
        report_error("cannot read monotonic clock");
        return 0;
    }
    delta = ticks.QuadPart - start.ticks;
    *duration = (int64_t)((long double)delta * 1000000000 /
                         frequency.QuadPart);
#else
    struct timespec ticks;
    if (clock_gettime(CLOCK_MONOTONIC, &ticks) != 0) {
        report_error("cannot read monotonic clock");
        return 0;
    }
    *duration = (int64_t)ticks.tv_sec * 1000000000 + ticks.tv_nsec - start.ticks;
#endif
    if (*duration < 0) {
        report_error("monotonic clock moved backwards");
        return 0;
    }
    return 1;
}

static int bind_number(sqlite3_stmt *statement, int index, double value)
{
    if (isfinite(value)) {
        return sqlite3_bind_double(statement, index, value);
    }
    return sqlite3_bind_text(statement, index,
                             isnan(value) ? "NaN" :
                             (value > 0.0 ? "+Infinity" : "-Infinity"),
                             -1, SQLITE_STATIC);
}

amcore_status amcore_operation_end(amcore_operation_start start,
                                  const char *function_name,
                                  double frequency_hz,
                                  double aperture_length_m,
                                  const double *output,
                                  amcore_status status)
{
    sqlite3 *database = NULL;
    sqlite3_stmt *statement = NULL;
    char *path;
    char sql[1024];
    int64_t duration;
    int rc;
    int farfield = strcmp(function_name, "amcore_far_field_distance") == 0;
    const char *result_name = farfield ? "distance_m" : "wavelength_m";

    if (!start.valid || !elapsed_ns(start, &duration)) {
        return status;
    }
    path = database_path();
    if (path == NULL) {
        return status;
    }
    rc = sqlite3_open_v2(path, &database,
                         SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
                         SQLITE_OPEN_FULLMUTEX, NULL);
    free(path);
    if (rc != SQLITE_OK) {
        goto cleanup;
    }
    rc = sqlite3_busy_timeout(database, 5000);
    if (rc != SQLITE_OK) {
        goto cleanup;
    }
    /* Identifiers come exclusively from the instrumented functions, not inputs. */
    snprintf(sql, sizeof(sql),
             "CREATE TABLE IF NOT EXISTS \"%s\" ("
             "id INTEGER PRIMARY KEY, frequency_hz REAL, %s"
             "output_is_null INTEGER NOT NULL, return_value INTEGER NOT NULL, "
             "%s REAL, executed_at_us INTEGER NOT NULL, "
             "duration_ns INTEGER NOT NULL CHECK(duration_ns >= 0))",
             function_name, farfield ? "aperture_length_m REAL, " : "",
             result_name);
    rc = sqlite3_exec(database, sql, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        goto cleanup;
    }
    snprintf(sql, sizeof(sql),
             "INSERT INTO \"%s\" (frequency_hz, %soutput_is_null, "
             "return_value, %s, executed_at_us, duration_ns) "
             "VALUES (?1, %s?3, ?4, ?5, ?6, ?7)",
             function_name, farfield ? "aperture_length_m, " : "",
             result_name, farfield ? "?2, " : "");
    rc = sqlite3_prepare_v2(database, sql, -1, &statement, NULL);
    if (rc != SQLITE_OK) {
        goto cleanup;
    }
    rc = bind_number(statement, 1, frequency_hz);
    if (rc == SQLITE_OK && farfield) {
        rc = bind_number(statement, 2, aperture_length_m);
    }
    if (rc == SQLITE_OK) {
        rc = sqlite3_bind_int(statement, 3, output == NULL);
    }
    if (rc == SQLITE_OK) {
        rc = sqlite3_bind_int(statement, 4, status);
    }
    if (rc == SQLITE_OK) {
        rc = status == AMCORE_STATUS_SUCCESS && output != NULL
                 ? sqlite3_bind_double(statement, 5, *output)
                 : sqlite3_bind_null(statement, 5);
    }
    if (rc == SQLITE_OK) {
        rc = sqlite3_bind_int64(statement, 6, start.executed_at_us);
    }
    if (rc == SQLITE_OK) {
        rc = sqlite3_bind_int64(statement, 7, duration);
    }
    if (rc == SQLITE_OK) {
        rc = sqlite3_step(statement);
        if (rc == SQLITE_DONE) {
            rc = SQLITE_OK;
        }
    }

cleanup:
    if (rc != SQLITE_OK) {
        report_error(database != NULL ? sqlite3_errmsg(database) : sqlite3_errstr(rc));
    }
    rc = sqlite3_finalize(statement);
    if (rc != SQLITE_OK) {
        report_error(sqlite3_errstr(rc));
    }
    rc = sqlite3_close(database);
    if (rc != SQLITE_OK) {
        report_error(sqlite3_errstr(rc));
    }
    return status;
}
