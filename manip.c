#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define CHAR_SIZE 255
#define CHARACTERS_NUM 25

// https://stackoverflow.com/questions/3536153/c-dynamically-growing-array

typedef struct {
    int *array;
    size_t used;
    size_t size;
} Array;

void initArray(Array *a, size_t initialSize) {
    a->array = (uint32_t *)malloc(initialSize * sizeof(uint32_t));
    a->used = 0;
    a->size = initialSize;
}

void insertArray(Array *a, uint32_t element) {
    // a->used is the number of used entries, because a->array[a->used++]
    //updates a->used only *after* the array has been accessed.
    // Therefore a->used can go up to a->size 
    if (a->used == a->size) {
        a->size *= 2;
        a->array = (uint32_t *)realloc(a->array, a->size * sizeof(uint32_t));
    }
    a->array[a->used++] = element;
}

void freeArray(Array *a) {
    free(a->array);
    a->array = NULL;
    a->used = a->size = 0;
}

typedef struct {
    uint8_t match;
    uint32_t start;
    uint32_t end;
} Match;

typedef struct {
    Match *array;
    size_t used;
    size_t size;
} MatchArray;

void initMatchArray(MatchArray *a, size_t initialSize) {
    a->array = (Match *)malloc(initialSize * sizeof(Match));
    a->used = 0;
    a->size = initialSize;
}

void insertMatchArray(MatchArray *a, Match element) {
    // a->used is the number of used entries, because a->array[a->used++]
    //updates a->used only *after* the array has been accessed.
    // Therefore a->used can go up to a->size 
    if (a->used == a->size) {
        a->size *= 2;
        a->array = (Match *)realloc(a->array, a->size * sizeof(Match));
    }
    a->array[a->used++] = element;
}

void freeMatchArray(MatchArray *a) {
    free(a->array);
    a->array = NULL;
    a->used = a->size = 0;
}


// ----------------------------------------------------------------------
static const char *CHARACTERS[CHARACTERS_NUM] = {
    "drmario",
    "mario",
    "luigi",
    "bowser",
    "peach",
    "yoshi",
    "dk",
    "cfalcon",
    "ganondorf",
    "falco",
    "fox",
    "ness",
    "ics",
    "kirby",
    "samus",
    "zelda",
    "link",
    "ylink",
    "pichu",
    "pikachu",
    "jigglypuff",
    "mewtwo",
    "gnw",
    "marth",
    "roy"
};

int reverse_character_lookup(char *s) {
    int i;
    for (i=0; i<CHARACTERS_NUM; i++) {
        int cmp = strcmp(CHARACTERS[i], s);
        if (!cmp) return i;
    }
    return -1;
}

void character_lookup(char *s, size_t size, uint32_t character){
    char *char_string;
    if (character >= CHARACTERS_NUM)
        strncpy(s, "unknown", size-1);
    else strncpy(s, CHARACTERS[character], size-1);
    s[size-1] = '\0';
}

void rng_adv(uint32_t *seed) {
    // integer overflow automatically accounted for
    *seed = ((*seed * 214013) + 2531011);
}

uint32_t rng_int(uint32_t *seed, uint32_t max_val) {
    uint32_t top_bits = *seed >> 16;
    return (max_val * top_bits) >> 16;
}

void check_sequence(uint32_t seed, uint32_t prev_seed, Match *m, Array *characters){
    int i;
    m->start = prev_seed;
    m->end = 0;
    m->match = 0;
    for(i=0; i<characters->used-1; i++){
        rng_adv(&seed);
        rng_adv(&seed);
        if (rng_int(&seed, 25) != characters->array[i+1]) {
            return;
        }
    }
    rng_adv(&seed);
    m->match = 1;
    m->end = seed;
}

uint32_t seed_find(int quick) {
    uint32_t prev_seed = 0x00000001; // init seed
    uint32_t seed = prev_seed;
    rng_adv(&seed);
    char msg[CHAR_SIZE]; // loop messages
    char character[32]; // loop character select
    Array characters; // array of characters entered by user
    initArray(&characters, 16); // array init
    uint32_t i; // loops

    // display characters

    printf("allowed inputs: \n  ");
    for (i=0; i<CHARACTERS_NUM; i++) {
        printf("%s%s", CHARACTERS[i], (i<CHARACTERS_NUM-1) ? ", " : "\n");
        if (!((i+1) % 5) && i<CHARACTERS_NUM-1) printf("\n  ");
    }

    // input loop

    i=1;
    while(1) {
        printf("Character %u -> ", i);
        scanf("%31s", character);
        int character_num = reverse_character_lookup(character);
        if (character_num < 0) {
            while ((getchar()) != '\n');
            char input;
            printf("finished? y/n -> ");
            scanf("%c", &input);
            if (input == 'Y' || input == 'y')
                break;
            continue;
        }
        insertArray(&characters, character_num);
        character_lookup(msg, CHAR_SIZE, character_num);
        i++;
    }

    // print out character list

    printf("No more characters. Doing RNG search...\n");
    printf("Final list:\n");
    for (i=0; i<characters.used; i++) {
        character_lookup(msg, CHAR_SIZE, characters.array[i]);
        printf("%s(%u)%s", 
            msg, 
            characters.array[i], 
            (i < characters.used-1) ? ", " : "\n");
    }
    
    // checking sequences 

    MatchArray matches;
    initMatchArray(&matches, 16);
    uint32_t value;
    for (i=0; i<0xFFFFFFFF; i++) {
        if (rng_int(&seed, 25) == characters.array[0]) {
            Match match;
            check_sequence(seed, prev_seed, &match, &characters);
            if (match.match) {
                insertMatchArray(&matches, match);
                if (quick) {
                    printf("Scanning: 0x%08X -> 0x%08X ... %u matches%s\n", 
                        i, seed, matches.used, (matches.used > 0) ? "!" : ".");
                    freeMatchArray(&matches);
                    freeArray(&characters);
                    return match.end;
                }
            }
        }
        if (!(i % 0x1234517))
            printf("Scanning: 0x%08X -> 0x%08X ... %u matches%s\r", 
                i, seed, matches.used, (matches.used > 0) ? "!" : ".");
        prev_seed = seed;
        rng_adv(&seed);
    }

    // printing results

    printf("Scanning: 0x%08X -> 0x%08X ... %u matches%s\n", 
        i, seed, matches.used, (matches.used > 0) ? "!" : ".");

    printf("List of matches:\n");
    for(i=0; i<matches.used; i++) {
        printf("== Match %d ==\n", i);
        uint32_t current_seed = matches.array[i].end;
        printf(" Start:   0x%08X\n", matches.array[i].start);
        printf(" Current: 0x%08X\n", matches.array[i].end);
        printf(" Next 5: ");
        int j;
        for (j=0; j<6; j++) {
            rng_adv(&current_seed);
            character_lookup(msg, CHAR_SIZE, rng_int(&current_seed, 25));
            printf("%s%s", msg, (j<5) ? ", " : "...\n");
            rng_adv(&current_seed);
        }
    }
    freeMatchArray(&matches);
    freeArray(&characters);
    return 0;
}

// look for rng events given the config file

typedef enum {
    DELAY,
    RAND_INT
} CFG_CMD;

typedef struct {
    CFG_CMD command;
    uint32_t params[16];
} ConfigCommand;

typedef struct {
    char name[256];
    size_t size;
    ConfigCommand commands[512];
} ConfigEntry;

typedef struct {
    size_t size;
    size_t used;
    ConfigEntry *entries;
} ConfigDatabase;

void initConfigDatabase(ConfigDatabase *db, size_t size) {
    db->entries = (ConfigEntry *)malloc(size * sizeof(ConfigEntry));
    db->used = 0;
    db->size = size;
    int i;
    for (i=0; i<size; i++) {
        ConfigEntry *e = &db->entries[i];
        e->size = 0;
    }
}

void insertConfigDatabaseEntry(ConfigDatabase *db, ConfigEntry entry) {
    if (db->used >= db->size)
        return; // don't add if there's no space
    db->entries[db->used++] = entry;
}

void rng_cmd_delay(uint32_t *seed, uint32_t n) {
    int i;
    for(i=0; i<n; i++) {
        rng_adv(seed);
    }
}

int rng_cmd_int(uint32_t *seed, uint32_t max_val, uint32_t lower_bound, uint32_t upper_bound) {
    rng_adv(seed);
    uint32_t n = rng_int(seed, max_val);
    if (n >= lower_bound && n <= upper_bound) return 1;
    return 0;
}

void calculate_rng_distance(ConfigEntry *e, uint32_t base_seed) {
    uint32_t i;
    int j;
    ConfigCommand *c;
    uint32_t seed;
    for(i=0; i<0xFFFFFFFF; i++) {
        seed = base_seed;
        // get entry in database
        int failed = 0;
        for(j=0; j<e->size; j++) {
            // get command in entry
            c = &e->commands[j];
            int ret;
            switch(c->command) {
                case DELAY:
                    rng_cmd_delay(&seed, c->params[0]);
                    break;
                case RAND_INT:
                    ret = rng_cmd_int(&seed, c->params[0], c->params[1], c->params[2]);
                    if (ret == 0) {
                        failed = 1;
                    }
                    break;
                default:
                    break;
            }
            if (failed) break;
        }
        if (!failed) {
            printf("%u iterations away on seed 0x%08X\n", i, base_seed);
            break;
        }
        rng_adv(&base_seed);
    }
}

int rng_event_search(uint32_t seed, int quick) {
    FILE *fp;
    printf("Opening manip.cfg...");
    fp = fopen("manip.cfg","r");
    if (fp == NULL) {
        printf("Error!\n");
        printf("\"manip.cfg\" unable to be opened, errno = %d\n", errno);
        return 1;
    }
    printf("Opened!\n");

    // file reading begin

    char line[1024];
    size_t db_size = 0;

    while(fgets(line, 1024, fp) != NULL) {
        char command[16];
        char params[128];
        sscanf(line, "%s", command);
        if (strcmp("NAME", command) == 0){
            db_size++;
        }
    }

    // create db with the size detected

    printf("Detected %u entries!\n", db_size);
    ConfigDatabase db;
    initConfigDatabase(&db, db_size);
    rewind(fp);

    // add the entries to the database

    printf("Building database...");
    size_t current_entry = -1;
    while(fgets(line, 1024, fp) != NULL) {
        char command[16];
        char params[256];
        sscanf(line, "%s", command);
        if (strcmp("NAME", command) == 0){
            current_entry++;
            sscanf(line, "NAME %[^\n]", params);
            strcpy(db.entries[current_entry].name, params);
        }
        if (strcmp("DELAY", command) == 0){
            uint32_t delay = 0;
            sscanf(line, "DELAY %u", &delay);
            ConfigEntry *e = &db.entries[current_entry];
            ConfigCommand *c = &e->commands[e->size++];
            c->command = DELAY;
            c->params[0] = delay;
        }
        if (strcmp("INT", command) == 0) {
            uint32_t range; 
            uint32_t lower_bound; // inclusive
            uint32_t upper_bound; // inclusive
            sscanf(line, "INT %u %u %u", 
                &range, &lower_bound, &upper_bound);
            ConfigEntry *e = &db.entries[current_entry];
            ConfigCommand *c = &e->commands[e->size++];
            c->command = RAND_INT;
            c->params[0] = range;
            c->params[1] = lower_bound;
            c->params[2] = upper_bound;
        }
    }
    fclose(fp);
    printf("Built!\n");

    // read out databse

    int i;
    for (i=0; i<db.size; i++) {
        ConfigEntry *e = &db.entries[i];
        printf("Entry %d: %s\n", i+1, e->name);
    }

    ConfigEntry *entry;
    uint32_t entry_num;

    printf("Use which entry? (1-%d) -> ", db.size);
    scanf("%u", &entry_num);
    entry = &db.entries[entry_num-1];
    if (!quick) {
        printf("Current seed? 0x");
        scanf("%08x", &seed);
    }

    printf("Using entry \"%s\" on seed 0x%08X...\n", entry->name, seed);

    calculate_rng_distance(entry, seed);

    return 0;
}

// begin

int main() {
    printf("----------------------------------------\n");
    printf("SSBM RNG Seed Character Checking Program\n");
    printf("Written in C by Savestate\n");
    printf("----------------------------------------\n");
    char answer;
    printf("Locate seed? y/n -> ");
    scanf("%c", &answer);
    uint32_t seed;
    int quick = 0;
    if (answer == 'y' || answer == 'Y') {
        while ((getchar()) != '\n');
        printf("Return after 1st result? y/n -> ");
        scanf("%c", &answer);
        if (answer == 'y' || answer == 'Y') quick = 1;
        seed = seed_find(quick);
    }
    rng_event_search(seed, quick);
}