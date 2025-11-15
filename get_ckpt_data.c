#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024

typedef struct {
    char size[32];
    char cache_flush[32];
    char ops[32];
    char writes[32];
    char reads[32];
    char mvm_time[32];
} MVMRow;

typedef struct {
    char size[32];
    char cache_flush[32];
    char ops[32];
    char writes[32];
    char reads[32];
    char simple_ckpt_time[32];
    char simple_restore_time[32];
} SimpleCkptRow;

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
    FILE *mvm_file, *simple_ckpt_file, *ckpt_file, *output_file;
    char line[MAX_LINE_LENGTH];
    char *endptr;
    int size, cache_flush, mod, ops;

    if (argc < 5) {
        fprintf(stderr, "Usage: %s <size> <cache_flush> <mod> <ops>\n", argv[0]);
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

    mod = strtol(argv[3], &endptr, 10);
    if (*endptr != '\0' || (mod != 64 && mod != 128 && mod != 256 && mod != 512)) {
        fprintf(stderr, "mod must be 64, 128, 256, 512 instead of %d\n", mod);
        return EXIT_FAILURE;
    }

    ops = strtol(argv[4], &endptr, 10);
    if (*endptr != '\0' || ops <= 0) {
        fprintf(stderr, "Ops must be an integer greater or eqaul to 1.\n");
        return EXIT_FAILURE;
    }

    printf("Generate plot data for 0x%x, %d, %d, %d\n", size, cache_flush, mod, ops);

    mvm_file = fopen("MVM/mvm_test_results.csv", "r");
    simple_ckpt_file = fopen("SIMPLE_CKPT/simple_ckpt_test_results.csv", "r");
    ckpt_file = fopen("MVM_CKPT/ckpt_test_results.csv", "r");
    output_file = fopen("plot_data.csv", "w");

    if (!mvm_file || !simple_ckpt_file || !ckpt_file || !output_file) {
        perror("Error opening files");
        return EXIT_FAILURE;
    }

    fprintf(output_file, "size,cache_flush,mod,ops,writes,reads,ckpt_time,ckpt_restore_time,mvm_time,simple_ckpt_time,"
                         "simple_restore_time\n");

    MVMRow mvm_row;
    SimpleCkptRow simple_ckpt_row;
    CkptRow ckpt_row;

    fgets(line, sizeof(line), ckpt_file);
    while (fgets(line, sizeof(line), ckpt_file)) {
        sscanf(line, "%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%s", ckpt_row.size, ckpt_row.cache_flush, ckpt_row.mod,
               ckpt_row.ops, ckpt_row.writes, ckpt_row.reads, ckpt_row.ckpt_time, ckpt_row.ckpt_restore_time);
        if (strtol(ckpt_row.size, &endptr, 16) != size || strtol(ckpt_row.cache_flush, &endptr, 10) != cache_flush ||
            strtol(ckpt_row.mod, &endptr, 10) != mod || strtol(ckpt_row.ops, &endptr, 10) != ops) {
            continue;
        }
        fprintf(output_file, "%s,%s,%s,%s,%s,%s,%s,%s", ckpt_row.size, ckpt_row.cache_flush, ckpt_row.mod, ckpt_row.ops,
                ckpt_row.writes, ckpt_row.reads, ckpt_row.ckpt_time, ckpt_row.ckpt_restore_time);

        fseek(mvm_file, 0, SEEK_SET);
        fgets(line, sizeof(line), mvm_file);
        while (fgets(line, sizeof(line), mvm_file)) {
            sscanf(line, "%[^,],%[^,],%[^,],%[^,],%[^,],%s", mvm_row.size, mvm_row.cache_flush, mvm_row.ops,
                   mvm_row.writes, mvm_row.reads, mvm_row.mvm_time);
            if (strcmp(ckpt_row.size, mvm_row.size) == 0 && strcmp(ckpt_row.cache_flush, mvm_row.cache_flush) == 0 &&
                strcmp(ckpt_row.ops, mvm_row.ops) == 0 && strcmp(ckpt_row.writes, mvm_row.writes) == 0 &&
                strcmp(ckpt_row.reads, mvm_row.reads) == 0) {
                fprintf(output_file, ",%s", mvm_row.mvm_time);
                break;
            }
        }

        fseek(simple_ckpt_file, 0, SEEK_SET);
        fgets(line, sizeof(line), simple_ckpt_file);
        while (fgets(line, sizeof(line), simple_ckpt_file)) {
            sscanf(line, "%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%s", simple_ckpt_row.size, simple_ckpt_row.cache_flush,
                   simple_ckpt_row.ops, simple_ckpt_row.writes, simple_ckpt_row.reads, simple_ckpt_row.simple_ckpt_time,
                   simple_ckpt_row.simple_restore_time);
            if (strcmp(ckpt_row.size, simple_ckpt_row.size) == 0 &&
                strcmp(ckpt_row.cache_flush, simple_ckpt_row.cache_flush) == 0 &&
                strcmp(ckpt_row.ops, simple_ckpt_row.ops) == 0 &&
                strcmp(ckpt_row.writes, simple_ckpt_row.writes) == 0 &&
                strcmp(ckpt_row.reads, simple_ckpt_row.reads) == 0) {
                fprintf(output_file, ",%s,%s\n", simple_ckpt_row.simple_ckpt_time, simple_ckpt_row.simple_restore_time);
                break;
            }
        }
    }

    fclose(mvm_file);
    fclose(simple_ckpt_file);
    fclose(ckpt_file);
    fclose(output_file);

    return EXIT_SUCCESS;
}
