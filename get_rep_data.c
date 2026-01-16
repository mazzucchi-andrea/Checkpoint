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
    char reps[32];
    char ckpt_time[32];
    char restore_time[32];
} SimpleCkptRow;

typedef struct {
    char size[32];
    char cache_flush[32];
    char mod[32];
    char ops[32];
    char writes[32];
    char reads[32];
    char reps[32];
    char ckpt_time[32];
    char restore_time[32];
} CkptRow;

typedef struct {
    char size[32];
    char cache_flush[32];
    char mod[32];
    char ops[32];
    char writes[32];
    char reads[32];
    char reps[32];
    char ckpt_time[32];
    char restore_time[32];
} MVMRow;

int main(int argc, char *argv[]) {
    FILE *ckpt_file, *mvm_file, *output_file, *simple_ckpt_file;
    char line[MAX_LINE_LENGTH];
    char *endptr;
    int size, cache_flush, mod, ops, reps;

    if (argc < 6) {
        fprintf(stderr, "Usage: %s <size> <cache_flush> <mod> <ops> <reps>\n",
                argv[0]);
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
    if (*endptr != '\0' || (mod != 8 && mod != 16 && mod != 32 && mod != 64)) {
        fprintf(stderr, "mod must be 8, 16, 32, 64 instead of %d\n", mod);
        return EXIT_FAILURE;
    }

    ops = strtol(argv[4], &endptr, 10);
    if (*endptr != '\0' || ops <= 0) {
        fprintf(stderr, "Ops must be an integer greater or equal to 1.\n");
        return EXIT_FAILURE;
    }

    reps = strtol(argv[5], &endptr, 10);
    if (*endptr != '\0' || reps <= 0) {
        fprintf(stderr, "reps must be an integer greater or equal to 1.\n");
        return EXIT_FAILURE;
    }

    printf("Generate plot data for 0x%x, %d, %d, %d, %d\n", size, cache_flush,
           mod, ops, reps);

    ckpt_file = fopen("MVM_CKPT/ckpt_repeat_test_results.csv", "r");
    mvm_file = fopen("MVM/mvm_repeat_test_results.csv", "r");
    simple_ckpt_file =
        fopen("SIMPLE_CKPT/simple_ckpt_repeat_test_results.csv", "r");
    output_file = fopen("plot_data.csv", "w");

    if (!simple_ckpt_file || !ckpt_file || !output_file) {
        perror("Error opening files");
        return EXIT_FAILURE;
    }

    fprintf(output_file, "size,cache_flush,mod,ops,writes,reads,reps,ckpt_time,"
                         "ckpt_restore_time,mvm_time,mvm_restore_time,simple_"
                         "ckpt_time,simple_restore_time\n");

    SimpleCkptRow simple_ckpt_row;
    CkptRow ckpt_row;
    MVMRow mvm_row;

    fgets(line, sizeof(line), ckpt_file);
    while (fgets(line, sizeof(line), ckpt_file)) {
        sscanf(line, "%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%s",
               ckpt_row.size, ckpt_row.cache_flush, ckpt_row.mod, ckpt_row.ops,
               ckpt_row.writes, ckpt_row.reads, ckpt_row.reps,
               ckpt_row.ckpt_time, ckpt_row.restore_time);
        if (strtol(ckpt_row.size, &endptr, 16) != size ||
            strtol(ckpt_row.cache_flush, &endptr, 10) != cache_flush ||
            strtol(ckpt_row.mod, &endptr, 10) != mod ||
            strtol(ckpt_row.ops, &endptr, 10) != ops ||
            strtol(ckpt_row.reps, &endptr, 10) != reps) {
            continue;
        }
        fprintf(output_file, "%s,%s,%s,%s,%s,%s,%s,%s,%s", ckpt_row.size,
                ckpt_row.cache_flush, ckpt_row.mod, ckpt_row.ops,
                ckpt_row.writes, ckpt_row.reads, ckpt_row.reps,
                ckpt_row.ckpt_time, ckpt_row.restore_time);

        fseek(mvm_file, 0, SEEK_SET);
        fgets(line, sizeof(line), mvm_file);
        while (fgets(line, sizeof(line), mvm_file)) {
            sscanf(line, "%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%s",
                   mvm_row.size, mvm_row.cache_flush, mvm_row.mod, mvm_row.ops,
                   mvm_row.writes, mvm_row.reads, mvm_row.reps,
                   mvm_row.ckpt_time, mvm_row.restore_time);
            if (!strcmp(ckpt_row.size, mvm_row.size) &&
                !strcmp(ckpt_row.cache_flush, mvm_row.cache_flush) &&
                !strcmp(ckpt_row.mod, mvm_row.mod) &&
                !strcmp(ckpt_row.ops, mvm_row.ops) &&
                !strcmp(ckpt_row.writes, mvm_row.writes) &&
                !strcmp(ckpt_row.reads, mvm_row.reads) &&
                !strcmp(ckpt_row.reps, mvm_row.reps)) {
                fprintf(output_file, ",%s,%s", mvm_row.ckpt_time,
                        mvm_row.restore_time);
                break;
            }
        }

        fseek(simple_ckpt_file, 0, SEEK_SET);
        fgets(line, sizeof(line), simple_ckpt_file);
        while (fgets(line, sizeof(line), simple_ckpt_file)) {
            sscanf(line, "%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%s",
                   simple_ckpt_row.size, simple_ckpt_row.cache_flush,
                   simple_ckpt_row.ops, simple_ckpt_row.writes,
                   simple_ckpt_row.reads, simple_ckpt_row.reps,
                   simple_ckpt_row.ckpt_time,
                   simple_ckpt_row.restore_time);
            if (!strcmp(ckpt_row.size, simple_ckpt_row.size) &&
                !strcmp(ckpt_row.cache_flush, simple_ckpt_row.cache_flush) &&
                !strcmp(ckpt_row.ops, simple_ckpt_row.ops) &&
                !strcmp(ckpt_row.writes, simple_ckpt_row.writes) &&
                !strcmp(ckpt_row.reads, simple_ckpt_row.reads) &&
                !strcmp(ckpt_row.reps, simple_ckpt_row.reps)) {
                fprintf(output_file, ",%s,%s\n",
                        simple_ckpt_row.ckpt_time,
                        simple_ckpt_row.restore_time);
                break;
            }
        }
    }

    fclose(simple_ckpt_file);
    fclose(ckpt_file);
    fclose(output_file);

    return EXIT_SUCCESS;
}
