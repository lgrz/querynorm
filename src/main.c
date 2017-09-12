#include <float.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

#define META_COLS 3

struct slice {
    size_t start;
    size_t end;
};

struct metadata {
    double label;
    int query_id;
    char *docno;
};

struct data {
    double *scores;
    size_t rows;
    size_t cols;
};

struct csv_grid {
    struct metadata *meta;
    struct slice *queries;
    struct data features;
    size_t size;
};

static void
die(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "die: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);

    exit(1);
}

int
main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <file>\n", argv[0]);
        return 1;
    }

    char *infile = argv[1];
    FILE *fp = NULL;
    char buf[BUFSIZ] = { 0 };

    if (!(fp = fopen(infile, "r"))) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    // count lines, fields, queries
    size_t lines = 0;
    size_t fields = 1;
    size_t queries = 0;
    bool did_fields = false;
    int last_id = -1;
    while (fgets(buf, BUFSIZ, fp)) {
        char *tok = NULL;
        int query_id;

        if (!did_fields) {
            char *p = buf;
            while ((p = strchr(p, ','))) {
                ++fields;
                ++p;
            }
            did_fields = true;
        }

        tok = strtok(buf, ",");
        tok = strtok(NULL, ",");
        if (!tok) {
            die("query id");
        }
        query_id = (int)strtol(tok, NULL, 10);
        if (last_id != query_id) {
            last_id = query_id;
            ++queries;
        }

        ++lines;
    }
    DLOG("lines: %ld, fields: %ld, queries: %ld\n", lines, fields, queries);
    if (0 == lines) {
        return 0;
    }

    const size_t num_fields = fields - META_COLS;
    struct csv_grid indata;
    memset(&indata, 0x0, sizeof(indata));
    indata.meta = bmalloc(lines * sizeof(struct metadata));
    indata.queries = bmalloc(queries * sizeof(struct slice));
    indata.features.cols = num_fields;
    indata.features.rows = lines;
    indata.features.scores = bmalloc((lines * num_fields) * sizeof(double));
    indata.size = lines;

    rewind(fp);
    int i = 0;
    size_t q = 0;
    last_id = -1;
    while (fgets(buf, BUFSIZ, fp)) {
        char *tok = NULL;

        tok = strtok(buf, ",");
        if (!tok) {
            die("label");
        }
        indata.meta[i].label = strtod(tok, NULL);
        tok = strtok(NULL, ",");
        if (!tok) {
            die("query id");
        }
        indata.meta[i].query_id = (int)strtol(tok, NULL, 10);
        tok = strtok(NULL, ",");
        if (!tok) {
            die("docno");
        }
        indata.meta[i].docno = strndup(tok, strlen(tok));

        if (last_id != indata.meta[i].query_id) {
            last_id = indata.meta[i].query_id;
            indata.queries[q].start = i;
            if (q > 0) {
                indata.queries[q - 1].end = i;
            }
            ++q;
        }

        int j = 0;
        tok = strtok(NULL, ",");
        while (tok) {
            indata.features.scores[(i * num_fields) + j] = strtod(tok, NULL);
            tok = strtok(NULL, ",");
            ++j;
        }
        ++i;
    }
    if (q > 0) {
        indata.queries[q - 1].end = i;
    }

    // find max, min for the current query
    size_t k;
    double *max_buf = bmalloc(num_fields * sizeof(double));
    double *min_buf = bmalloc(num_fields * sizeof(double));
    for (q = 0; q < queries; ++q) {
        size_t start = indata.queries[q].start;
        size_t end = indata.queries[q].end;
        for (k = start; k < end; ++k) {
            size_t f;
            for (f = 0; f < indata.features.cols; ++f) {
                double cur_score;
                size_t idx = k * num_fields + f;
                if (k == start) {
                    max_buf[f] = -DBL_MAX;
                    min_buf[f] = DBL_MAX;
                }

                cur_score = indata.features.scores[idx];
                if (cur_score > max_buf[f]) {
                    max_buf[f] = cur_score;
                }
                if (cur_score < min_buf[f]) {
                    min_buf[f] = cur_score;
                }
            }
        }
        for (k = start; k < end; ++k) {
            size_t f;
            for (f = 0; f < indata.features.cols; ++f) {
                size_t idx = k * num_fields + f;
                double x = indata.features.scores[idx];
                if (0.0 == max_buf[f] - min_buf[f]) {
                    indata.features.scores[idx] = 0.0;
                    continue;
                }
                indata.features.scores[idx] = (x - min_buf[f]) / (max_buf[f] - min_buf[f]);
            }
        }
    }

    // output normalised data
    for (k = 0; k < indata.size; ++k) {
        printf("%d,%d,%s", (int)indata.meta[k].label, indata.meta[k].query_id,
                           indata.meta[k].docno);
        size_t f;
        for (f = 0; f < indata.features.cols; ++f) {
            size_t idx = k * num_fields + f;
            printf(",%.6f", indata.features.scores[idx]);
        }
        printf("\n");

        free(indata.meta[k].docno);
    }

    free(indata.meta);
    free(indata.queries);
    free(indata.features.scores);
    fclose(fp);

    return 0;
}
