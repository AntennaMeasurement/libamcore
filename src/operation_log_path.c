#include "operation_log_path.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <errno.h>
#include <sys/stat.h>
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

char *amcore_operation_log_path(int create_directory)
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
    if (create_directory && !ensure_directory(path)) {
        report_error("cannot create user data directory");
        free(path);
        return NULL;
    }
    snprintf(path + directory_length, path_capacity - directory_length,
             "%s", filename);
    return path;
}
