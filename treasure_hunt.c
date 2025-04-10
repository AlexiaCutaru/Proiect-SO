#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>

#define MAX_PATH 512

typedef struct {
    int id;
    char username[50];
    float latitude;
    float longitude;
    char clue[100];
    int value;
} Treasure;

void build_path(char *buffer, const char *hunt_id, const char *file) {
    snprintf(buffer, MAX_PATH, "%s/%s", hunt_id, file);
}

void create_symlink(const char *hunt_id) {
    char target[MAX_PATH], linkname[MAX_PATH];
    snprintf(target, sizeof(target), "%s/logged_hunt", hunt_id);
    snprintf(linkname, sizeof(linkname), "logged_hunt-%s", hunt_id);

    unlink(linkname); // Remove old link
    if (symlink(target, linkname) == -1) {
        perror("Failed to create symbolic link");
    } else {
        printf("Symlink created: %s → %s\n", linkname, target);
    }
}

void add_treasure(const char *hunt_id) {
    Treasure t;
    printf("Enter Treasure ID: ");
    scanf("%d", &t.id);
    printf("Enter Username: ");
    scanf("%s", t.username);
    printf("Enter Latitude: ");
    scanf("%f", &t.latitude);
    printf("Enter Longitude: ");
    scanf("%f", &t.longitude);
    printf("Enter Clue: ");
    scanf(" %[^\n]", t.clue); // Read with spaces
    printf("Enter Value: ");
    scanf("%d", &t.value);

    mkdir(hunt_id, 0755);  // Ensure directory exists

    char treasure_path[MAX_PATH], log_path[MAX_PATH];
    build_path(treasure_path, hunt_id, "treasures.dat");
    build_path(log_path, hunt_id, "logged_hunt");

    int fd = open(treasure_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        perror("Error opening treasure file");
        return;
    }
    write(fd, &t, sizeof(Treasure));
    close(fd);

    // Log operation
    int log_fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    dprintf(log_fd, "Added treasure %d\n", t.id);
    close(log_fd);

    create_symlink(hunt_id);

    printf("Treasure added successfully!\n");
}

void list_treasures(const char *hunt_id) {
    Treasure t;
    char path[MAX_PATH];
    build_path(path, hunt_id, "treasures.dat");

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("Error opening treasure file");
        return;
    }

    struct stat st;
    if (stat(path, &st) == 0) {
        printf("Hunt: %s\n", hunt_id);
        printf("File Size: %ld bytes\n", st.st_size);
        printf("Last Modified: %s\n", strtok(ctime(&st.st_mtime), "\n"));
    } else {
        perror("Could not retrieve file info");
    }

    printf("Treasures:\n");
    int count = 0;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("------------------------------------------------------------\n");
        printf("ID: %d\n", t.id);
        printf("User: %s\n", t.username);
        printf("Location: (%.6f, %.6f)\n", t.latitude, t.longitude);
        printf("Clue: %s\n", t.clue);
        printf("Value: %d\n", t.value);
        count++;
    }

    if (count == 0) {
        printf("No treasures found in this hunt.\n");
    } else {
        printf("Total Treasures: %d\n", count);
    }

    close(fd);
}


void view_treasure(const char *hunt_id, int id) {
    Treasure t;
    char path[MAX_PATH];
    build_path(path, hunt_id, "treasures.dat");

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        return;
    }

    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.id == id) {
            printf("Treasure Found:\n");
            printf("ID: %d\nUser: %s\nLatitude: %f\nLongitude: %f\nClue: %s\nValue: %d\n",
                   t.id, t.username, t.latitude, t.longitude, t.clue, t.value);
            close(fd);
            return;
        }
    }

    printf("Treasure with ID %d not found.\n", id);
    close(fd);
}

void remove_treasure(const char *hunt_id, int id) {
    Treasure t;
    char path[MAX_PATH], temp_path[MAX_PATH];
    build_path(path, hunt_id, "treasures.dat");
    build_path(temp_path, hunt_id, "temp.dat");

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("Error opening treasure file");
        return;
    }

    int temp_fd = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (temp_fd < 0) {
        perror("Error creating temp file");
        close(fd);
        return;
    }

    int found = 0;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.id != id) {
            write(temp_fd, &t, sizeof(Treasure));
        } else {
            found = 1;
        }
    }

    close(fd);
    close(temp_fd);

    if (!found) {
        printf("Treasure with ID %d not found.\n", id);
        remove(temp_path);
        return;
    }

    remove(path);
    rename(temp_path, path);
    printf("Treasure %d removed successfully!\n", id);
}

void remove_hunt(const char *hunt_id) {
    DIR *dir;
    struct dirent *entry;
    char path[MAX_PATH];

    dir = opendir(hunt_id);
    if (!dir) {
        perror("Could not open hunt directory");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        snprintf(path, sizeof(path), "%s/%s", hunt_id, entry->d_name);
        if (remove(path) != 0) {
            perror("Error removing file");
        }
    }
    closedir(dir);

    if (rmdir(hunt_id) == 0) {
        printf("Hunt '%s' deleted successfully.\n", hunt_id);
    } else {
        perror("Failed to remove hunt directory");
    }

    char link_name[MAX_PATH];
    snprintf(link_name, sizeof(link_name), "logged_hunt-%s", hunt_id);
    unlink(link_name);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <command> <hunt_id> [id]\n", argv[0]);
        return 1;
    }

    const char *command = argv[1];
    const char *hunt_id = argv[2];

    if (strcmp(command, "add") == 0) {
        add_treasure(hunt_id);
    } else if (strcmp(command, "list") == 0) {
        list_treasures(hunt_id);
    } else if (strcmp(command, "view") == 0 && argc == 4) {
        int id = atoi(argv[3]);
        view_treasure(hunt_id, id);
    } else if (strcmp(command, "remove") == 0 && argc == 4) {
        int id = atoi(argv[3]);
        remove_treasure(hunt_id, id);
    } else if (strcmp(command, "remove_hunt") == 0) {
        remove_hunt(hunt_id);
    } else {
        printf("Invalid command or arguments.\n");
    }

    return 0;
}

