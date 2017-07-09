#define _GNU_SOURCE

#include <stdio.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdlib.h>
#include <sqlite3.h>

enum action {
    CREATE
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

//TODO: Analyze, validate and dispatch input.
enum action analyze_input(char *input) {
    return CREATE;
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
        if (action == CREATE) {
            struct Entry entry = create_entry(input);
            write_to_db(db, entry);
            free_entry(entry);
        } else {
            puts("Unknown action.");
        }
        free(input);
    }

    sqlite3_close(db);
    return 0;
}
