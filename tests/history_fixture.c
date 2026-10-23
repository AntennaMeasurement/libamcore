#include <stdio.h>
#include <string.h>
#include <sqlite3.h>

int main(int argc, char **argv)
{
    sqlite3 *database = NULL;
    int rc;

    if (argc != 3)
        return 1;
    rc = sqlite3_open(argv[1], &database);
    if (rc == SQLITE_OK && strcmp(argv[2], "populated") == 0)
        rc = sqlite3_exec(database,
            "CREATE TABLE \"amcore_free_space_wavelength_" AMCORE_VERSION "\" ("
            "id INTEGER PRIMARY KEY, version TEXT, executed_at_us INTEGER,"
            "return_value INTEGER, duration_ns INTEGER, wavelength_m REAL);"
            "WITH RECURSIVE n(x) AS (VALUES(1) UNION ALL SELECT x+1 FROM n WHERE x<12)"
            "INSERT INTO \"amcore_free_space_wavelength_" AMCORE_VERSION "\" "
            "SELECT x, '" AMCORE_VERSION "', 1700000000000000+x*2, 0, x, 1.0 FROM n;"
            "CREATE TABLE \"amcore_far_field_distance_0.1.0\" ("
            "id INTEGER PRIMARY KEY, executed_at_us INTEGER,"
            "return_value INTEGER, duration_ns INTEGER, old_output REAL,"
            "frequency_hz REAL, output_is_null INTEGER, note TEXT);"
            "WITH RECURSIVE n(x) AS (VALUES(1) UNION ALL SELECT x+1 FROM n WHERE x<12)"
            "INSERT INTO \"amcore_far_field_distance_0.1.0\" "
            "SELECT x, 1700000000000000+x*2-1, 1, x, NULL, 'NaN', 0, "
            "'first'||char(10)||'second'||char(9)||'|\\' FROM n;"
            "CREATE TABLE amcore_free_space_wavelength ("
            "id INTEGER, executed_at_us INTEGER, return_value INTEGER, duration_ns INTEGER);"
            "INSERT INTO amcore_free_space_wavelength VALUES(1,1700000000000027,0,1);"
            "CREATE TABLE \"amcore_retired_function_0.0.1\" ("
            "id INTEGER, version TEXT, executed_at_us INTEGER,"
            "return_value INTEGER, duration_ns INTEGER);"
            "INSERT INTO \"amcore_retired_function_0.0.1\" "
            "VALUES(1,'0.0.1',1700000000000026,2,1);"
            "CREATE TABLE unrelated_table (ignored TEXT);",
            NULL, NULL, NULL);
    else if (rc == SQLITE_OK && strcmp(argv[2], "incompatible") == 0)
        rc = sqlite3_exec(database, "CREATE TABLE amcore_broken (id INTEGER)",
                          NULL, NULL, NULL);
    else if (rc == SQLITE_OK && strcmp(argv[2], "ties") == 0)
        rc = sqlite3_exec(database,
            "CREATE TABLE \"amcore_a_0.1.0\" ("
            "id INTEGER, executed_at_us INTEGER, return_value INTEGER, duration_ns INTEGER);"
            "INSERT INTO \"amcore_a_0.1.0\" VALUES(1,1,0,1),(2,1,0,1);"
            "CREATE TABLE \"amcore_b_0.1.0\" ("
            "id INTEGER, executed_at_us INTEGER, return_value INTEGER, duration_ns INTEGER);"
            "INSERT INTO \"amcore_b_0.1.0\" VALUES(1,1,0,1);",
            NULL, NULL, NULL);
    else if (rc == SQLITE_OK && strcmp(argv[2], "many-tables") == 0) {
        int i;
        for (i = 0; i < 510 && rc == SQLITE_OK; ++i) {
            char *sql = sqlite3_mprintf(
                "CREATE TABLE \"amcore_old_function_0.0.%d\" ("
                "id INTEGER, executed_at_us INTEGER, return_value INTEGER, duration_ns INTEGER);"
                "INSERT INTO \"amcore_old_function_0.0.%d\" VALUES(1,%d,0,1)",
                i, i, i);
            rc = sql == NULL ? SQLITE_NOMEM :
                 sqlite3_exec(database, sql, NULL, NULL, NULL);
            sqlite3_free(sql);
        }
    } else if (rc == SQLITE_OK && strcmp(argv[2], "empty") != 0)
        rc = SQLITE_MISUSE;
    if (rc != SQLITE_OK)
        fprintf(stderr, "history fixture: %s\n", sqlite3_errmsg(database));
    {
        int closed = sqlite3_close(database);
        return rc == SQLITE_OK && closed == SQLITE_OK ? 0 : 1;
    }
}
