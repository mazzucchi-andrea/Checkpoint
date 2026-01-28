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
    char ckpt_mean[32];
    char ckpt_ci[32];
    char restore_mean[32];
    char restore_ci[32];
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

    ckpt_file = fopen("MVM_GRID_CKPT_BS/ckpt_test_results.csv", "r");
    output_file = fopen("plot_data.csv", "w");

    if (!ckpt_file || !output_file) {
        perror("Error opening files!");
        return EXIT_FAILURE;
    }

    fprintf(output_file,
            "size,cache_flush,ops,writes,reads,"
            "8_ckpt_mean,8_ckpt_ci,8_restore_mean,8_restore_ci,"
            "16_ckpt_mean,16_ckpt_ci,16_restore_mean,16_restore_ci,"
            "32_ckpt_mean,32_ckpt_ci,32_restore_mean,32_restore_ci,"
            "64_ckpt_mean,64_ckpt_ci,64_restore_mean,64_restore_ci\n");

    CkptRow ckpt_rows_8[14];
    CkptRow ckpt_rows_16[14];
    CkptRow ckpt_rows_32[14];
    CkptRow ckpt_rows_64[14];

    CkptRow ckpt_row;
    int i = 0, j = 0, k = 0, l = 0;
    fgets(line, sizeof(line), ckpt_file);
    while (fgets(line, sizeof(line), ckpt_file)) {
        sscanf(line, "%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%s",
               ckpt_row.size, ckpt_row.cache_flush, ckpt_row.mod, ckpt_row.ops,
               ckpt_row.writes, ckpt_row.reads, ckpt_row.ckpt_mean,
               ckpt_row.ckpt_ci, ckpt_row.restore_mean, ckpt_row.restore_ci);
        if (strtol(ckpt_row.size, &endptr, 16) != size ||
            strtol(ckpt_row.cache_flush, &endptr, 10) != cache_flush ||
            strtol(ckpt_row.ops, &endptr, 10) != ops) {
            continue;
        }
        if (strcmp(ckpt_row.mod, "8") == 0) {
            ckpt_rows_8[i] = ckpt_row;
            i++;
        }
        if (strcmp(ckpt_row.mod, "16") == 0) {
            ckpt_rows_16[j] = ckpt_row;
            j++;
        }
        if (strcmp(ckpt_row.mod, "32") == 0) {
            ckpt_rows_32[k] = ckpt_row;
            k++;
        }
        if (strcmp(ckpt_row.mod, "64") == 0) {
            ckpt_rows_64[l] = ckpt_row;
            l++;
        }
    }

    for (i = 0; i < 14; i++) {
        fprintf(
            output_file,
            "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
            ckpt_rows_8[i].size, ckpt_rows_8[i].cache_flush, ckpt_rows_8[i].ops,
            ckpt_rows_8[i].writes, ckpt_rows_8[i].reads,
            ckpt_rows_8[i].ckpt_mean, ckpt_rows_8[i].ckpt_ci,
            ckpt_rows_8[i].restore_mean, ckpt_rows_8[i].restore_ci,
            ckpt_rows_16[i].ckpt_mean, ckpt_rows_16[i].ckpt_ci,
            ckpt_rows_16[i].restore_mean, ckpt_rows_16[i].restore_ci,
            ckpt_rows_32[i].ckpt_mean, ckpt_rows_32[i].ckpt_ci,
            ckpt_rows_32[i].restore_mean, ckpt_rows_32[i].restore_ci,
            ckpt_rows_64[i].ckpt_mean, ckpt_rows_64[i].ckpt_ci,
            ckpt_rows_64[i].restore_mean, ckpt_rows_64[i].restore_ci);
    }
    fclose(output_file);

    return EXIT_SUCCESS;
}
