#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#define MAX_USERS 100
#define MAX_NAME_LEN 64

typedef struct {
    char name[MAX_NAME_LEN];
    int score;
} UserScore;

UserScore scores[MAX_USERS];
int score_count = 0;

void add_score(const char* name, int value) {
    for (int i = 0; i < score_count; ++i) {
        if (strcmp(scores[i].name, name) == 0) {
            scores[i].score += value;
            return;
        }
    }
    if (score_count < MAX_USERS) {
        strncpy(scores[score_count].name, name, MAX_NAME_LEN - 1);
        scores[score_count].score = value;
        score_count++;
    }
}

void process_file(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return;

    char line[256];
    char owner[MAX_NAME_LEN] = "";
    int value = 0;
    int has_owner = 0, has_value = 0;

    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "Owner: %63s", owner) == 1) {
            has_owner = 1;
        } else if (sscanf(line, "Value: %d", &value) == 1) {
            has_value = 1;
        }

        if (has_owner && has_value) {
            add_score(owner, value);
            has_owner = has_value = 0;
        }
    }

    fclose(f);
}

void process_directory(const char* dirname) {
    DIR* dir = opendir(dirname);
    if (!dir) {
        perror("Failed to open directory");
        exit(1);
    }

    struct dirent* entry;
    char filepath[512];

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG && strstr(entry->d_name, ".txt")) {
            snprintf(filepath, sizeof(filepath), "%s/%s", dirname, entry->d_name);
            process_file(filepath);
        }
    }

    closedir(dir);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <HuntDirectory>\n", argv[0]);
        return 1;
    }

    process_directory(argv[1]);

    for (int i = 0; i < score_count; ++i) {
        printf("%s: %d\n", scores[i].name, scores[i].score);
    }

    return 0;
}

