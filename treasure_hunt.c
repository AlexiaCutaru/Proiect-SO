#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define TREASURE_FILE "treasures.dat"

typedef struct {
    int id;
    char username[50];
    float latitude;
    float longitude;
    char clue[100];
    int value;
} Treasure;

void create_symlink(const char *hunt_id) 
{
    char target[100];
    char linkname[100];

    snprintf(target, sizeof(target), "%s/logged_hunt", hunt_id);

    snprintf(linkname, sizeof(linkname), "logged_hunt-%s", hunt_id);

    if (access(linkname, F_OK) == 0) {
        printf("Symlink %s already exists. Skipping.\n", linkname);
        return;
    }

    if (symlink(target, linkname) == -1) {
        perror("Failed to create symbolic link");
    } else {
        printf("Symlink created: %s → %s\n", linkname, target);
    }
}

void add_treasure() 
{
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
    scanf("%s", t.clue);
    printf("Enter Value: ");
    scanf("%d", &t.value);
    
    int fd = open(TREASURE_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        perror("Error opening file");
        return;
    }
    write(fd, &t, sizeof(Treasure));
    close(fd);
    printf("Treasure added successfully!\n");
}

void list_treasures() 
{
    Treasure t;
    int fd = open(TREASURE_FILE, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        return;
    }
    
    printf("Treasure List:\n");
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("ID: %d, User: %s, Location: (%f, %f), Clue: %s, Value: %d\n",
               t.id, t.username, t.latitude, t.longitude, t.clue, t.value);
    }
    close(fd);
}

void remove_treasure(int id) 
{
    Treasure t;
    int fd = open(TREASURE_FILE, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        return;
    }
    
    int temp_fd = open("temp.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (temp_fd < 0) {
        perror("Error creating temp file");
        close(fd);
        return;
    }
    
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.id != id) {
            write(temp_fd, &t, sizeof(Treasure));
        }
    }
    close(fd);
    close(temp_fd);
    
    remove(TREASURE_FILE);
    rename("temp.dat", TREASURE_FILE);
    printf("Treasure %d removed successfully!\n", id);
}

int main(int argc, char *argv[]) 
{
    if (argc < 2) {
        printf("Usage: %s <command> [args]\n", argv[0]);
        return 1;
    }
    
    if (strcmp(argv[1], "add") == 0) 
    {
        add_treasure();
    } 
    else if (strcmp(argv[1], "list") == 0) 
    {
        list_treasures();
    } 
    else if (strcmp(argv[1], "remove") == 0 && argc == 3) 
    {
        int id = atoi(argv[2]);
        remove_treasure(id);
    } 
    else 
    {
        printf("Invalid command\n");
    }
    return 0;
}
