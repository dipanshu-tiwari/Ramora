#include "logging_helper.h"

static FILE *log_fp = NULL;

static void log_timestamp(FILE *fp) {
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);

    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);

    fprintf(fp, "[%s] ", buf);
}

int log_open(const char *path) {
    log_fp = fopen(path, "a");
    if (!log_fp)
        return -1;

    setvbuf(log_fp, NULL, _IOLBF, 0);
    return 0;
}

void log_close() {
    if (log_fp) {
        fclose(log_fp);
        log_fp = NULL;
    }
}

void die(const char *mssg) {
    FILE *out = log_fp ? log_fp : stderr;

    log_timestamp(out);
    fprintf(out, "[ERROR] | %s\n", mssg);
    fflush(out);
}

void msg(const char *mssg) {
    FILE *out = log_fp ? log_fp : stderr;

    log_timestamp(out);
    fprintf(out, "[WARNING] | %s\n", mssg);
    fflush(out);
}

void note(const char *mssg) {
    FILE *out = log_fp ? log_fp : stderr;

    log_timestamp(out);
    fprintf(out, "[NOTE] | %s\n", mssg);
    fflush(out);
}
