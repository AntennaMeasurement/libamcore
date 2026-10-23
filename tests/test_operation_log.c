#include <amcore/amcore.h>

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>

#define WAVELENGTH_TABLE "amcore_free_space_wavelength_" AMCORE_VERSION
#define FARFIELD_TABLE "amcore_far_field_distance_" AMCORE_VERSION

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static int scalar(sqlite3 *database, const char *sql, sqlite3_int64 *value)
{
    sqlite3_stmt *statement = NULL;
    int success = sqlite3_prepare_v2(database, sql, -1, &statement, NULL) == SQLITE_OK;
    if (success) {
        success = sqlite3_step(statement) == SQLITE_ROW;
    }
    if (success) {
        *value = sqlite3_column_int64(statement, 0);
    } else {
        fprintf(stderr, "query failed: %s\n", sqlite3_errmsg(database));
    }
    sqlite3_finalize(statement);
    return success;
}

static int expect_count(sqlite3 *database, const char *sql, sqlite3_int64 expected)
{
    sqlite3_int64 actual = -1;
    if (!scalar(database, sql, &actual) || actual != expected) {
        fprintf(stderr, "expected %lld, got %lld: %s\n",
                (long long)expected, (long long)actual, sql);
        return 0;
    }
    return 1;
}

static int exercise_calls(void)
{
    double output = 123.0;
    const double invalid[] = {NAN, INFINITY, -INFINITY, 0.0};
    size_t i;

    if (amcore_free_space_wavelength(1e9, &output) != AMCORE_STATUS_SUCCESS ||
        fabs(output - 0.299792458) > 1e-12) {
        return 0;
    }
    output = 123.0;
    for (i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        if (amcore_free_space_wavelength(invalid[i], &output) !=
                AMCORE_STATUS_INVALID_ARGUMENT || output != 123.0) {
            return 0;
        }
    }
    if (amcore_free_space_wavelength(1e9, NULL) != AMCORE_STATUS_INVALID_ARGUMENT ||
        amcore_free_space_wavelength(DBL_MIN, &output) != AMCORE_STATUS_OUT_OF_RANGE ||
        output != 123.0) {
        return 0;
    }
    if (amcore_far_field_distance(1e9, 1.0, &output) != AMCORE_STATUS_SUCCESS ||
        fabs(output - 2e9 / 299792458.0) > 1e-12) {
        return 0;
    }
    output = 123.0;
    return amcore_far_field_distance(1e9, -1.0, &output) ==
               AMCORE_STATUS_INVALID_ARGUMENT &&
           amcore_far_field_distance(NAN, 1.0, &output) ==
               AMCORE_STATUS_INVALID_ARGUMENT &&
           amcore_far_field_distance(1e9, DBL_MAX, &output) ==
               AMCORE_STATUS_OUT_OF_RANGE &&
           amcore_far_field_distance(1e9, 1.0, NULL) ==
               AMCORE_STATUS_INVALID_ARGUMENT &&
           output == 123.0;
}

static int run_test(const char *argument)
{
    sqlite3 *database = NULL;
    sqlite3_int64 before_wavelength = 0;
    sqlite3_int64 before_farfield = 0;
    char query[1024];
    time_t started = time(NULL);
    int success;

    if (strcmp(argument, "failure") == 0) {
        return exercise_calls() ? 0 : 1;
    }
    if (sqlite3_open(argument, &database) != SQLITE_OK) {
        fputs("cannot open test database\n", stderr);
        sqlite3_close(database);
        return 1;
    }
    success = sqlite3_exec(database,
        "CREATE TABLE IF NOT EXISTS amcore_free_space_wavelength "
        "(legacy_marker TEXT NOT NULL);"
        "INSERT INTO amcore_free_space_wavelength "
        "SELECT 'preserved' WHERE NOT EXISTS "
        "(SELECT 1 FROM amcore_free_space_wavelength);",
        NULL, NULL, NULL) == SQLITE_OK;
    snprintf(query, sizeof(query),
        "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='%s'",
        WAVELENGTH_TABLE);
    success = success && scalar(database, query, &before_wavelength);
    if (success && before_wavelength != 0) {
        snprintf(query, sizeof(query), "SELECT count(*) FROM \"%s\"",
                 WAVELENGTH_TABLE);
        success = scalar(database, query, &before_wavelength);
    } else if (success) {
        before_wavelength = 0;
    }
    snprintf(query, sizeof(query),
        "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='%s'",
        FARFIELD_TABLE);
    success = success && scalar(database, query, &before_farfield);
    if (success && before_farfield != 0) {
        snprintf(query, sizeof(query), "SELECT count(*) FROM \"%s\"",
                 FARFIELD_TABLE);
        success = scalar(database, query, &before_farfield);
    } else if (success) {
        before_farfield = 0;
    }
    success = success && exercise_calls();
    snprintf(query, sizeof(query), "SELECT count(*) FROM \"%s\"", WAVELENGTH_TABLE);
    success = success && expect_count(database, query, before_wavelength + 10);
    snprintf(query, sizeof(query), "SELECT count(*) FROM \"%s\"", FARFIELD_TABLE);
    success = success && expect_count(database, query, before_farfield + 5);
    success = success &&
        expect_count(database,
            "SELECT count(*) FROM amcore_free_space_wavelength "
            "WHERE legacy_marker='preserved'", 1);
    snprintf(query, sizeof(query),
        "SELECT count(*) FROM \"%s\" WHERE version='%s'",
        WAVELENGTH_TABLE, AMCORE_VERSION);
    success = success && expect_count(database, query, before_wavelength + 10);
    snprintf(query, sizeof(query),
        "SELECT count(*) FROM \"%s\" WHERE version='%s'",
        FARFIELD_TABLE, AMCORE_VERSION);
    success = success && expect_count(database, query, before_farfield + 5);

    snprintf(query, sizeof(query),
        "SELECT count(*) FROM \"%s\" WHERE id > %lld AND "
        "frequency_hz=1e9 AND abs(wavelength_m-0.299792458)<1e-12 AND "
        "return_value=0 AND output_is_null=0 AND version='%s'",
        WAVELENGTH_TABLE,
        (long long)before_wavelength, AMCORE_VERSION);
    success = success && expect_count(database, query, 3);
    snprintf(query, sizeof(query),
        "SELECT count(*) FROM \"%s\" WHERE id > %lld AND "
        "frequency_hz=1e9 AND aperture_length_m=1 AND "
        "abs(distance_m-(2e9/299792458.0))<1e-12 AND return_value=0 "
        "AND version='%s'",
        FARFIELD_TABLE,
        (long long)before_farfield, AMCORE_VERSION);
    success = success && expect_count(database, query, 1);
    snprintf(query, sizeof(query),
        "SELECT count(*) FROM \"%s\" WHERE id > %lld "
        "AND return_value=1", WAVELENGTH_TABLE, (long long)before_wavelength);
    success = success && expect_count(database, query, 6);
    snprintf(query, sizeof(query),
        "SELECT count(*) FROM \"%s\" WHERE id > %lld "
        "AND return_value=2", WAVELENGTH_TABLE, (long long)before_wavelength);
    success = success && expect_count(database, query, 1);
    snprintf(query, sizeof(query),
        "SELECT count(*) FROM \"%s\" WHERE id > %lld "
        "AND return_value=1", FARFIELD_TABLE, (long long)before_farfield);
    success = success && expect_count(database, query, 3);
    snprintf(query, sizeof(query),
        "SELECT count(*) FROM \"%s\" WHERE id > %lld "
        "AND return_value=2", FARFIELD_TABLE, (long long)before_farfield);
    success = success && expect_count(database, query, 1);
    snprintf(query, sizeof(query),
        "SELECT count(*) FROM \"%s\" WHERE frequency_hz='NaN' "
        "AND version='%s'", WAVELENGTH_TABLE, AMCORE_VERSION);
    success = success &&
        expect_count(database, query, (before_wavelength / 10 + 1) * 2);
    snprintf(query, sizeof(query),
        "SELECT count(*) FROM \"%s\" WHERE frequency_hz "
        "IN ('+Infinity','-Infinity')", WAVELENGTH_TABLE);
    success = success &&
        expect_count(database, query, (before_wavelength / 10 + 1) * 2);
    snprintf(query, sizeof(query),
        "SELECT count(*) FROM \"%s\" WHERE return_value<>0 "
        "AND wavelength_m IS NOT NULL", WAVELENGTH_TABLE);
    success = success && expect_count(database, query, 0);
    snprintf(query, sizeof(query),
        "SELECT count(*) FROM \"%s\" WHERE return_value<>0 "
        "AND distance_m IS NOT NULL", FARFIELD_TABLE);
    success = success && expect_count(database, query, 0);
    snprintf(query, sizeof(query),
        "SELECT count(*) FROM \"%s\" WHERE output_is_null=1 "
        "AND return_value=1", WAVELENGTH_TABLE);
    success = success &&
        expect_count(database, query, before_wavelength / 10 + 1);
    snprintf(query, sizeof(query),
        "SELECT count(*) FROM \"%s\" WHERE output_is_null=1 "
        "AND return_value=1", FARFIELD_TABLE);
    success = success &&
        expect_count(database, query, before_farfield / 5 + 1);
    snprintf(query, sizeof(query),
        "SELECT count(*) FROM ("
        "SELECT executed_at_us,duration_ns FROM \"%s\" WHERE id>%lld "
        "UNION ALL SELECT executed_at_us,duration_ns FROM \"%s\" WHERE id>%lld"
        ") WHERE executed_at_us >= %lld AND executed_at_us <= %lld "
        "AND typeof(executed_at_us)='integer' AND typeof(duration_ns)='integer' "
        "AND duration_ns>=0 AND duration_ns<60000000000",
        WAVELENGTH_TABLE, (long long)before_wavelength,
        FARFIELD_TABLE, (long long)before_farfield,
        (long long)started * 1000000, ((long long)time(NULL) + 1) * 1000000);
    success = success && expect_count(database, query, 15);
    snprintf(query, sizeof(query),
        "SELECT count(*) FROM \"%s\" f "
        "JOIN \"%s\" w ON w.id=%lld "
        "WHERE f.id=%lld AND f.executed_at_us<=w.executed_at_us "
        "AND f.duration_ns>=w.duration_ns",
        FARFIELD_TABLE, WAVELENGTH_TABLE,
        (long long)before_wavelength + 8, (long long)before_farfield + 1);
    success = success && expect_count(database, query, 1);
    if (sqlite3_close(database) != SQLITE_OK) {
        success = 0;
    }
    return success ? 0 : 1;
}

#ifdef _WIN32
int wmain(int argc, wchar_t **argv)
{
    int length;
    char *argument;
    int result;
    if (argc != 2) {
        fputs("expected database path or failure\n", stderr);
        return 1;
    }
    length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, argv[1], -1,
                                NULL, 0, NULL, NULL);
    argument = length > 0 ? malloc((size_t)length) : NULL;
    if (argument == NULL ||
        !WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, argv[1], -1,
                             argument, length, NULL, NULL)) {
        fputs("cannot encode test argument\n", stderr);
        free(argument);
        return 1;
    }
    result = run_test(argument);
    free(argument);
    return result;
}
#else
int main(int argc, char **argv)
{
    if (argc != 2) {
        fputs("expected database path or failure\n", stderr);
        return 1;
    }
    return run_test(argv[1]);
}
#endif
