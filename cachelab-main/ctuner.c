
/*
** Ezra Anthony opa14
** Romit Ghosh rg1129
** Javier Martinez jm1941
** Vineeth Reddy vcr24
*/
//Header Files.//Header Files.
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>

//Function displays how to use the program if arguments are missing or invalid.
static void usage(const char *program) {
    fprintf(stderr,
        "Usage: %s -p <h|m|e> -r <target_rate> -b <csim_binary> -t <trace_file>\n"
        "-p   metric to optimize: h = hit rate, m = miss rate, e = eviction rate\n"
        "-r   target rate between 0.00 and 100.00\n"
        "-b   path to csim binary\n"
        "-t   name of Valgrind/trace file to feed to csim\n",
        program);
}

//Main Function
int main(int argc, char **argv) {
    //Create variables to store command line arguments. The metric, the target rate, the csim-binary, and the tracefile.
    char metric = 0; //The performance metric to be used (h, m, or e). h is for hit rate. m is for miss rate. e is for eviction rate.
    double target = -1.0; //The performance rate to achieve.
    //Hit rate: make sure it's ABOVE the target rate. Miss rate and eviction rate: make sure it's BELOW the target rate.
    char *csim_bin = NULL; //Path to the csim binary from part A. This will usually just be ./csim-ref.
    char *trace_file = NULL; //Name of the valgrind trace to use for learning.

    //Parse the arguments from the command line.
    //-p: performance metric. -r: target rate. -b: path to cache simulator. -t: trace file.
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc)
            metric = argv[++i][0];
        else if (strcmp(argv[i], "-r") == 0 && i + 1 < argc)
            target = atof(argv[++i]);
        else if (strcmp(argv[i], "-b") == 0 && i + 1 < argc)
            csim_bin = argv[++i];
        else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc)
            trace_file = argv[++i];
        else {
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    //Ensure that the metric is valid, the target is non-negative and the paths and files are provided.
    //Print out a statement stating if there are missing or invalid required arguments.
    int validMetric = (metric == 'h' || metric == 'm' || metric == 'e');
    if (!validMetric || target < 0.0 || !csim_bin || !trace_file) {
        fprintf(stderr, "Error: missing or invalid argument(s).\n");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    //tracking what will need to be printed at the end
    //Store best csim command and output and best result (hit/miss/evict rate).
    char *best_ctuner_output = malloc(256);
    char *best_csim_output = malloc(128);

    //this makes the hit rate compare easier
    float best_result = 0.00;

    //This makes the miss and eviction rate comparison easier
    if (metric == 'm' || metric == 'e'){
        best_result = 100.00;
    }

    int found = 0; //Track if a valid configuration was found

    //Loops through all combinations of the cache parameters.
    //Number of sets up to 2^5. Lines per set/Associativity up to 4. Block size up to 2^5.
    for (int s = 1; (1 << s) <= 32; s++) {
        for (int E = 1; E <= 4; E++) {
            for (int b = 1; (1 << b) <= 32; b++) {

                //Formatting the command string to execute csim.c.
                char cmd[256];
                snprintf(cmd, sizeof(cmd), "%s -s %d -E %d -b %d -t %s", csim_bin, s, E, b, trace_file);

                //Run csim.c.
                FILE *fp = popen(cmd, "r");
                if (!fp) {
                    perror("popen failed");
                    continue;
                }

                //Parses the output line for performance stats.
                int hits = 0, misses = 0, evicts = 0;
                char line[128];
                while (fgets(line, sizeof(line), fp)) {
                    sscanf(line, "hits:%d misses:%d evictions:%d", &hits, &misses, &evicts);
                }
                pclose(fp);

                int total_memory_accesses = hits + misses;
                if (total_memory_accesses == 0) continue; //Skip invalid configurations.

                float result = 0.00;

                //calculate hit rate, miss rate, or eviction rate depending on the metric.
                if (metric == 'h') {
                    result = ((1.00 * hits) / total_memory_accesses) * 100.00;

                    //Check if the metric for the current configuration meets the required threshold.
                    //For the hit rate metric, we are looking for the highest possible result >= the target rate
                    if (result >= target && result > best_result) {
                        best_result = result;
                        sprintf(best_ctuner_output, "%s -s %d -E %d -b %d -t %s", csim_bin, s, E, b, trace_file);
                        sprintf(best_csim_output, "hits:%d misses:%d evictions:%d", hits, misses, evicts);
                        found = 1;

                        //Early exit if perfect hit rate (can't do better)
                        if (best_result >= 100.00) goto end_search;
                    }

                } else { 
                    if (metric == 'm') {
                        result = ((1.00 * misses) / total_memory_accesses) * 100.00;
                    } else if (metric == 'e') {
                        result = ((1.00 * evicts) / total_memory_accesses) * 100.00;
                    }

                    //Check if the metric for the current configuration meets the required threshold.
                    //If either m or e, we want the result to be as low as possible <= the target rate.
                    if (result <= target && result < best_result) {
                        best_result = result;
                        sprintf(best_ctuner_output, "%s -s %d -E %d -b %d -t %s", csim_bin, s, E, b, trace_file);
                        sprintf(best_csim_output, "hits:%d misses:%d evictions:%d", hits, misses, evicts);
                        found = 1;

                        //Early exit if perfect (0% miss or evict rate)
                        if (best_result <= 0.00) goto end_search;
                    }
                }
            }
        }
    }

end_search:

    //Output
    if (!found) {
        printf("No valid configuration found\n");
    } else {
        //Print out optimal configuration and results.
        printf("%s\n", best_ctuner_output);
        printf("%s\n", best_csim_output);
    }

    //Free allocated memory
    free(best_ctuner_output);
    free(best_csim_output);

    return EXIT_SUCCESS;
}
