#define _GNU_SOURCE

#include <stdio.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <pcre.h>

enum action {
    CREATE,
    SHOW,
    NOOP
};

struct Entry {
    time_t start;
    time_t end;
    char *activity;
    char *comment;
};

void free_entry(struct Entry entry) {
    free(entry.comment);
    free(entry.activity);
}

time_t get_timestamp(char *token) {
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));
    strptime(token, "%Y-%m-%d %H:%M", &tm);
    return mktime(&tm);
}

struct Entry create_entry(char *input) {
    struct Entry entry;
    for (int i = 0; i < 4; i++) {
        char *token = strsep(&input, ";");
        switch (i) {
            case 0:
                entry.start = get_timestamp(token);
                break;
            case 1:
                entry.end = get_timestamp(token);
                break;
            case 2:
                entry.activity = malloc(sizeof(token));
                strcpy(entry.activity, token);
                break;
            case 3:
                entry.comment = malloc(sizeof(token));
                strcpy(entry.comment, token);
                break;
            default:
                break;
        }
    }
    return entry;
}

void write_to_db(sqlite3 *db, struct Entry entry) {
    char *db_error = NULL;
    char *sql = sqlite3_mprintf("INSERT INTO zeit "  \
                                "VALUES (%i, %i, %Q, %Q); ",
                                entry.start, entry.end, entry.activity, entry.comment);

    int db_result = sqlite3_exec(db, sql, NULL, NULL, &db_error);

    if (db_result != SQLITE_OK) {
        fprintf(stderr, "Error: %s\n", db_error);
        sqlite3_free(db_error);
    } else {
        fprintf(stdout, "Created \n");
    }
    sqlite3_free(sql);
}

void store_new_entry(sqlite3 *db, char *valid_input) {
    struct Entry entry = create_entry(valid_input);
    if (entry.end > entry.start) {
        write_to_db(db, entry);
    } else {
        puts("End must be after start.");
    }
    free_entry(entry);
}

int print_db_result(void *data, int argc, char **argv, char **col_name) {
    for (int i = 0; i < argc; i++) {
        printf("%s\t\t", argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

void show_last_entries(sqlite3 *db, int n) {
    puts("Start\t\tEnd\t\tActivity\t\tComment");

    char *db_error = NULL;
    char *sql = sqlite3_mprintf("SELECT datetime(start, 'unixepoch'), datetime(end, 'unixepoch'), " \
                                "activity, comment FROM zeit ORDER BY start DESC LIMIT %i", n);

    int db_result = sqlite3_exec(db, sql, print_db_result, NULL, &db_error);
    if (db_result != SQLITE_OK) {
        fprintf(stderr, "Error: %s\n", db_error);
        sqlite3_free(db_error);
    }
}

enum action analyze_input(char *input) {
    char *regex = "\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2};\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2};.+;.*";
    const char *error;
    int erroffset;
    pcre *re = pcre_compile(regex, 0, &error, &erroffset, 0);
    int rc;
    int ovector[5000];
    rc = pcre_exec(re, 0, input, (int)strlen(input), 0, 0, ovector, 5000);

    if (rc == 1) {
        return CREATE;
    } else if (strcmp(input, "show") == 0) {
        return SHOW;
    } else {
        return NOOP;
    }
}

int main(int argc, char **argv) {
    sqlite3 *db;
    int rc = sqlite3_open("/home/jens/Code/C/zeit/zeit.db", &db); //TODO: Make path configurable
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    while (1) {
        char *input = readline("Input: ");
        if (input == NULL) { break; }
        add_history(input);
        enum action action = analyze_input(input);
        switch (action) {
            case CREATE:
                store_new_entry(db, input);
                break;
            case SHOW:
                show_last_entries(db, 10);
                break;
            case NOOP:
                puts("Invalid syntax.");
                break;
        }
        free(input);
    }

    sqlite3_close(db);
    return 0;
}
