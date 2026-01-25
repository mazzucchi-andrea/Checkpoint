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
    char reps[32];
    char ckpt_mean[32];
    char ckpt_ci[32];
    char restore_mean[32];
    char restore_ci[32];
} GridCkptRow;

typedef struct {
    char size[32];
    char cache_flush[32];
    char mod[32];
    char ops[32];
    char writes[32];
    char reads[32];
    char reps[32];
    char ckpt_mean[32];
    char ckpt_ci[32];
    char restore_mean[32];
    char restore_ci[32];
} CPatchRow;

typedef struct {
    char size[32];
    char cache_flush[32];
    char ops[32];
    char writes[32];
    char reads[32];
    char reps[32];
    char ckpt_mean[32];
    char ckpt_ci[32];
    char restore_mean[32];
    char restore_ci[32];
} SimpleCkptRow;

int main(int argc, char* argv[]) {
    FILE *ckpt_file, *mvm_file, *output_file, *simple_ckpt_file;
    char line[MAX_LINE_LENGTH];
    char* endptr;
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

    fprintf(output_file,
            "size,cache_flush,mod,ops,writes,reads,reps,grid_ckpt_bs_mean,grid_"
            "ckpt_bs_ci,grid_ckpt_bs_restore_mean,grid_ckpt_bs_restore_ci,c_"
            "grid_ckpt_bs_mean,c_grid_ckpt_bs_ci,c_grid_ckpt_bs_restore_mean,c_"
            "grid_ckpt_bs_restore_ci,simple_ckpt_mean,simple_ckpt_ci,simple_"
            "restore_mean,simple_restore_ci\n");

    GridCkptRow grid_row;
    CPatchRow c_patch_row;
    SimpleCkptRow simple_ckpt_row;

    fgets(line, sizeof(line), ckpt_file);
    while (fgets(line, sizeof(line), ckpt_file)) {
        sscanf(line,
               "%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%s",
               grid_row.size, grid_row.cache_flush, grid_row.mod, grid_row.ops,
               grid_row.writes, grid_row.reads, grid_row.reps,
               grid_row.ckpt_mean, grid_row.ckpt_ci, grid_row.restore_mean,
               grid_row.restore_ci);
        if (strtol(grid_row.size, &endptr, 16) != size ||
            strtol(grid_row.cache_flush, &endptr, 10) != cache_flush ||
            strtol(grid_row.mod, &endptr, 10) != mod ||
            strtol(grid_row.ops, &endptr, 10) != ops ||
            strtol(grid_row.reps, &endptr, 10) != reps) {
            continue;
        }
        fprintf(output_file, "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s", grid_row.size,
                grid_row.cache_flush, grid_row.mod, grid_row.ops,
                grid_row.writes, grid_row.reads, grid_row.reps,
                grid_row.ckpt_mean, grid_row.ckpt_ci, grid_row.restore_mean,
                grid_row.restore_ci);

        fseek(mvm_file, 0, SEEK_SET);
        fgets(line, sizeof(line), mvm_file);
        while (fgets(line, sizeof(line), mvm_file)) {
            sscanf(line,
                   "%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,]"
                   ",%s",
                   c_patch_row.size, c_patch_row.cache_flush, c_patch_row.mod,
                   c_patch_row.ops, c_patch_row.writes, c_patch_row.reads,
                   c_patch_row.reps, c_patch_row.ckpt_mean, c_patch_row.ckpt_ci,
                   c_patch_row.restore_mean, c_patch_row.restore_ci);
            if (!strcmp(grid_row.size, c_patch_row.size) &&
                !strcmp(grid_row.cache_flush, c_patch_row.cache_flush) &&
                !strcmp(grid_row.mod, c_patch_row.mod) &&
                !strcmp(grid_row.ops, c_patch_row.ops) &&
                !strcmp(grid_row.writes, c_patch_row.writes) &&
                !strcmp(grid_row.reads, c_patch_row.reads) &&
                !strcmp(grid_row.reps, c_patch_row.reps)) {
                fprintf(output_file, ",%s,%s,%s,%s", c_patch_row.ckpt_mean,
                        c_patch_row.ckpt_ci, c_patch_row.restore_mean,
                        c_patch_row.restore_ci);
                break;
            }
        }

        fseek(simple_ckpt_file, 0, SEEK_SET);
        fgets(line, sizeof(line), simple_ckpt_file);
        while (fgets(line, sizeof(line), simple_ckpt_file)) {
            sscanf(line,
                   "%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%s",
                   simple_ckpt_row.size, simple_ckpt_row.cache_flush,
                   simple_ckpt_row.ops, simple_ckpt_row.writes,
                   simple_ckpt_row.reads, simple_ckpt_row.reps,
                   simple_ckpt_row.ckpt_mean, simple_ckpt_row.ckpt_ci,
                   simple_ckpt_row.restore_mean, simple_ckpt_row.restore_ci);
            if (!strcmp(grid_row.size, simple_ckpt_row.size) &&
                !strcmp(grid_row.cache_flush, simple_ckpt_row.cache_flush) &&
                !strcmp(grid_row.ops, simple_ckpt_row.ops) &&
                !strcmp(grid_row.writes, simple_ckpt_row.writes) &&
                !strcmp(grid_row.reads, simple_ckpt_row.reads) &&
                !strcmp(grid_row.reps, simple_ckpt_row.reps)) {
                fprintf(output_file, ",%s,%s,%s,%s\n",
                        simple_ckpt_row.ckpt_mean, simple_ckpt_row.ckpt_ci,
                        simple_ckpt_row.restore_mean,
                        simple_ckpt_row.restore_ci);
                break;
            }
        }
    }

    fclose(simple_ckpt_file);
    fclose(ckpt_file);
    fclose(output_file);

    return EXIT_SUCCESS;
}
