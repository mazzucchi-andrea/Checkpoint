#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024

typedef struct {
    char size[32];
    char cache_flush[32];
    char mod[32];
    char ops[32];
    char writes[32];
    char reads[32];
    char ckpt_time[32];
    char ckpt_restore_time[32];
} CkptRow;

int main(int argc, char *argv[]) {
    FILE *ckpt_file, *output_file;
    char line[MAX_LINE_LENGTH];
    char *endptr;
    int size, cache_flush, ops;

    if (argc < 4) {
        fprintf(stderr, "Usage: %s <size> <cache_flush> <ops>\n", argv[0]);
        return EXIT_FAILURE;
    }

    size = strtol(argv[1], &endptr, 16);
    if (*endptr != '\0' || size <= 0) {
        fprintf(stderr, "Size must be in hex\n");
        return EXIT_FAILURE;
    }

    cache_flush = strtol(argv[2], &endptr, 10);
    if (*endptr != '\0' || cache_flush < 0 || cache_flush > 1) {
        fprintf(stderr, "cache_flush must be 0 or 1\n");
        return EXIT_FAILURE;
    }

    ops = strtol(argv[3], &endptr, 10);
    if (*endptr != '\0' || ops <= 0) {
        fprintf(stderr, "Ops must be an integer greater or eqaul to 1.\n");
        return EXIT_FAILURE;
    }

    printf("Generate plot data for 0x%x, %d, %d\n", size, cache_flush, ops);

    ckpt_file = fopen("MVM_CKPT/ckpt_test_results.csv", "r");
    output_file = fopen("plot_data.csv", "w");

    if (!ckpt_file || !output_file) {
        perror("Error opening files");
        return EXIT_FAILURE;
    }

    fprintf(output_file, "size,cache_flush,ops,writes,reads,64_ckpt_time,64_ckpt_restore_time,128_ckpt_time,128_ckpt_"
                         "restore_time,256_ckpt_time,256_ckpt_restore_time,512_ckpt_time,512_ckpt_restore_time\n");

    CkptRow ckpt_rows_64[14];
    CkptRow ckpt_rows_128[14];
    CkptRow ckpt_rows_256[14];
    CkptRow ckpt_rows_512[14];

    CkptRow ckpt_row;
    int i = 0, j = 0, k = 0, l = 0;
    fgets(line, sizeof(line), ckpt_file);
    while (fgets(line, sizeof(line), ckpt_file)) {
        sscanf(line, "%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%s", ckpt_row.size, ckpt_row.cache_flush, ckpt_row.mod,
               ckpt_row.ops, ckpt_row.writes, ckpt_row.reads, ckpt_row.ckpt_time, ckpt_row.ckpt_restore_time);
        if (strtol(ckpt_row.size, &endptr, 16) != size || strtol(ckpt_row.cache_flush, &endptr, 10) != cache_flush ||
            strtol(ckpt_row.ops, &endptr, 10) != ops) {
            continue;
        }
        if (strcmp(ckpt_row.mod, "64") == 0) {
            ckpt_rows_64[i] = ckpt_row;
            i++;
        }
        if (strcmp(ckpt_row.mod, "128") == 0) {
            ckpt_rows_128[j] = ckpt_row;
            j++;
        }
        if (strcmp(ckpt_row.mod, "256") == 0) {
            ckpt_rows_256[k] = ckpt_row;
            k++;
        }
        if (strcmp(ckpt_row.mod, "512") == 0) {
            ckpt_rows_512[l] = ckpt_row;
            l++;
        }
    }

    for (i = 0; i < 14; i++) {
        fprintf(output_file, "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n", ckpt_rows_64[i].size,
                ckpt_rows_64[i].cache_flush, ckpt_rows_64[i].ops, ckpt_rows_64[i].writes, ckpt_rows_64[i].reads,
                ckpt_rows_64[i].ckpt_time, ckpt_rows_64[i].ckpt_restore_time, ckpt_rows_128[i].ckpt_time,
                ckpt_rows_128[i].ckpt_restore_time, ckpt_rows_256[i].ckpt_time, ckpt_rows_256[i].ckpt_restore_time,
                ckpt_rows_512[i].ckpt_time, ckpt_rows_512[i].ckpt_restore_time);
    }
    fclose(output_file);

    return EXIT_SUCCESS;
}
