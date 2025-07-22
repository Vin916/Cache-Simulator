#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>

int verbose = 0;    //This variable determines if we are in the verbose setting 
int s, E, b;
char *trace_file;

// Cache line struct
typedef struct {
    int valid;
    unsigned long tag;
    int lru_counter;
} cache_line;

// 2D array of cache_line structs
cache_line **cache;             //Pointer of a pointer of the structure cache line, effectiveley making a 2d array

// Stats
int hit_count = 0;
int miss_count = 0;
int eviction_count = 0;

// Get set index and tag from address
unsigned long get_set_index(unsigned long address) {
    return (address >> b) & ((1 << s) - 1);             //extract set index from given memory address in cache simulation
}

unsigned long get_tag(unsigned long address) {
    return address >> (s + b);      // using right shift operator to shift to the address of the tag
}

// Simulate cache access
void access_cache(char operation, unsigned long address) {
    unsigned long set_index = get_set_index(address);
    unsigned long tag = get_tag(address);
    cache_line *set = cache[set_index];

    // LRU replacement: find LRU line and if there's a hit
    int hit = 0;
    int empty_index = -1;
    int lru_index = 0;
    int max_lru = -1;

    for (int i = 0; i < E; i++) {
        if (set[i].valid && set[i].tag == tag) {    //if accessing a variable that already is being stored in the cache
            hit = 1;                                //that's a hit!
            set[i].lru_counter = 0;                 //it was most recently used now (0 lines ago)
            if (verbose) printf(" hit");
        } else {
            set[i].lru_counter++;           //aging cache line by 1 unless it was just used
            if (!set[i].valid && empty_index == -1)
                empty_index = i;
            if (set[i].lru_counter > max_lru) {
                max_lru = set[i].lru_counter;
                lru_index = i;
            }
        }
    }

    if (hit) {
        hit_count++;
        return;
    }

    miss_count++;
    if (verbose) printf(" miss");

    int index_to_replace;
    if (empty_index != -1) {
        index_to_replace = empty_index;
    } else {
        index_to_replace = lru_index;
        eviction_count++;
        if (verbose) printf(" eviction");
    }

    set[index_to_replace].valid = 1;
    set[index_to_replace].tag = tag;
    set[index_to_replace].lru_counter = 0;
}

void simulate_trace(FILE *fp) {
    char op;
    unsigned long addr;
    int size;


    // Parse memory operations from the trace file: 
    // 'L' and 'S' = 1 cache access, 'M' = 2 accesses, 'I' is ignored
    while (fscanf(fp, " %c %lx,%d", &op, &addr, &size) == 3) {
        if (op == 'I') continue;

        if (verbose) printf("%c %lx,%d", op, addr, size);
        access_cache(op, addr);
        if (op == 'M') access_cache(op, addr);  // M causes two accesses
        if (verbose) printf("\n");
    }
}

// Initialize the cache (allocate memory for bothe diensions of 2D array and set  values to 0)
void init_cache() {
    int S = 1 << s;                             //Compute number of sets: S = 2^s (1 << s shifts 1 left by s bits)
    cache = malloc(S * sizeof(cache_line *));
    for (int i = 0; i < S; i++) {
        cache[i] = malloc(E * sizeof(cache_line));
        for (int j = 0; j < E; j++) {
            cache[i][j].valid = 0;
            cache[i][j].tag = 0;
            cache[i][j].lru_counter = 0;
        }
    }
}

//this frees the memory allocated for the cache in both dimensions of the 2D array (preventing memory leaks)
void free_cache() {
    int S = 1 << s;
    for (int i = 0; i < S; i++) {
        free(cache[i]);
    }
    free(cache);
}

int main(int argc, char **argv) {
    char c;
    while ((c = getopt(argc, argv, "s:E:b:t:vh")) != -1) {
        switch (c) {
            case 's': s = atoi(optarg); break;          //What do any of these lines do? (EA)
            case 'E': E = atoi(optarg); break;
            case 'b': b = atoi(optarg); break;
            case 't': trace_file = optarg; break;
            case 'v': verbose = 1; break;
            case 'h':
            default:
                printf("Usage: ./csim [-v] -s <s> -E <E> -b <b> -t <tracefile>\n");
                exit(0);
        }
    }

    //Checking to make sure the file exists/is accessed properly
    if (!trace_file) {
        fprintf(stderr, "Missing trace file\n");
        exit(1);
    }

    init_cache();   //This line creates the cache essentially, preparing it to receive data.

    FILE *fp = fopen(trace_file, "r");
    if (!fp) {
        perror("fopen");
        exit(1);
    }

    simulate_trace(fp);
    fclose(fp);
    free_cache();

    printf("hits:%d misses:%d evictions:%d\n", hit_count, miss_count, eviction_count);
    return EXIT_SUCCESS;
}
