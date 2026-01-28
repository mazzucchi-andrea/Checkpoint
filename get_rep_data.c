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
    char chunk_size[32];
    char ops[32];
    char writes[32];
    char reads[32];
    char reps[32];
    char ckpt_mean[32];
    char ckpt_ci[32];
    char restore_mean[32];
    char restore_ci[32];
} ChunkCkptRow;

int main(int argc, char *argv[]) {
    FILE *grid_ckpt_file, *chunk_file, *output_file;
    char line[MAX_LINE_LENGTH];
    char *endptr;
    int size, cache_flush, mod, chunk_size, ops, reps;

    if (argc < 7) {
        fprintf(stderr,
                "Usage: %s <size> <cache_flush> <mod> <chunk> <ops> <reps>\n",
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

    chunk_size = strtol(argv[4], &endptr, 10);
    if (*endptr != '\0' || (mod != 8 && mod != 16 && mod != 32 && mod != 64)) {
        fprintf(stderr, "mod must be 8, 16, 32, 64 instead of %d\n", mod);
        return EXIT_FAILURE;
    }

    ops = strtol(argv[5], &endptr, 10);
    if (*endptr != '\0' || ops <= 0) {
        fprintf(stderr, "Ops must be an integer greater or equal to 1.\n");
        return EXIT_FAILURE;
    }

    reps = strtol(argv[6], &endptr, 10);
    if (*endptr != '\0' || reps <= 0) {
        fprintf(stderr, "reps must be an integer greater or equal to 1.\n");
        return EXIT_FAILURE;
    }

    printf("Generate plot data for 0x%x, %d, %d, %d, %d, %d\n", size,
           cache_flush, mod, chunk_size, ops, reps);

    chunk_file = fopen("MVM_CHUNK_SET/chunk_repeat_test_results.csv", "r");
    grid_ckpt_file =
        fopen("MVM_GRID_CKPT_BS/ckpt_repeat_test_results.csv", "r");
    output_file = fopen("plot_data.csv", "w");

    if (!chunk_file || !grid_ckpt_file || !output_file) {
        perror("Error opening files");
        return EXIT_FAILURE;
    }

    fprintf(output_file,
            "size,cache_flush,mod,chunk,ops,writes,reads,reps,grid_ckpt_mean,"
            "grid_ckpt_ci,grid_ckpt_restore_mean,grid_ckpt_restore_ci,chunk_"
            "ckpt_mean,chunk_ckpt_ci,chunk_restore_mean,chunk_restore_ci\n");

    GridCkptRow grid_ckpt_row;
    ChunkCkptRow chunk_ckpt_row;

    fgets(line, sizeof(line), grid_ckpt_file);
    while (fgets(line, sizeof(line), grid_ckpt_file)) {
        sscanf(line,
               "%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%s",
               grid_ckpt_row.size, grid_ckpt_row.cache_flush, grid_ckpt_row.mod,
               grid_ckpt_row.ops, grid_ckpt_row.writes, grid_ckpt_row.reads,
               grid_ckpt_row.reps, grid_ckpt_row.ckpt_mean,
               grid_ckpt_row.ckpt_ci, grid_ckpt_row.restore_mean,
               grid_ckpt_row.restore_ci);
        if (strtol(grid_ckpt_row.size, &endptr, 16) != size ||
            strtol(grid_ckpt_row.cache_flush, &endptr, 10) != cache_flush ||
            strtol(grid_ckpt_row.mod, &endptr, 10) != mod ||
            strtol(grid_ckpt_row.ops, &endptr, 10) != ops ||
            strtol(grid_ckpt_row.reps, &endptr, 10) != reps) {
            continue;
        }
        fprintf(output_file, "%s,%s,%s,%d,%s,%s,%s,%s,%s,%s,%s,%s",
                grid_ckpt_row.size, grid_ckpt_row.cache_flush,
                grid_ckpt_row.mod, chunk_size, grid_ckpt_row.ops,
                grid_ckpt_row.writes, grid_ckpt_row.reads, grid_ckpt_row.reps,
                grid_ckpt_row.ckpt_mean, grid_ckpt_row.ckpt_ci,
                grid_ckpt_row.restore_mean, grid_ckpt_row.restore_ci);

        fseek(chunk_file, 0, SEEK_SET);
        fgets(line, sizeof(line), chunk_file);
        while (fgets(line, sizeof(line), chunk_file)) {
            sscanf(line,
                   "%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,]"
                   ",%s",
                   chunk_ckpt_row.size, chunk_ckpt_row.cache_flush,
                   chunk_ckpt_row.chunk_size, chunk_ckpt_row.ops,
                   chunk_ckpt_row.writes, chunk_ckpt_row.reads,
                   chunk_ckpt_row.reps, chunk_ckpt_row.ckpt_mean,
                   chunk_ckpt_row.ckpt_ci, chunk_ckpt_row.restore_mean,
                   chunk_ckpt_row.restore_ci);
            if (strtol(chunk_ckpt_row.chunk_size, &endptr, 10) == chunk_size &&
                !strcmp(grid_ckpt_row.size, chunk_ckpt_row.size) &&
                !strcmp(grid_ckpt_row.cache_flush,
                        chunk_ckpt_row.cache_flush) &&
                !strcmp(grid_ckpt_row.ops, chunk_ckpt_row.ops) &&
                !strcmp(grid_ckpt_row.writes, chunk_ckpt_row.writes) &&
                !strcmp(grid_ckpt_row.reads, chunk_ckpt_row.reads) &&
                !strcmp(grid_ckpt_row.reps, chunk_ckpt_row.reps)) {
                fprintf(output_file, ",%s,%s,%s,%s\n", chunk_ckpt_row.ckpt_mean,
                        chunk_ckpt_row.ckpt_ci, chunk_ckpt_row.restore_mean,
                        chunk_ckpt_row.restore_ci);
                break;
            }
        }
    }

    fclose(chunk_file);
    fclose(grid_ckpt_file);
    fclose(output_file);

    return EXIT_SUCCESS;
}
