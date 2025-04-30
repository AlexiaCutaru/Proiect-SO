#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define CMD_FILE "/tmp/monitor_command.txt"
#define PIPE_READ 0
#define PIPE_WRITE 1

pid_t monitor_pid = -1;
int pipefd[2];
int monitor_running = 0;

void sigchld_handler(int sig) {
    int status;
    waitpid(monitor_pid, &status, 0);
    monitor_running = 0;
    printf("Monitor has terminated (exit code %d).\n", WEXITSTATUS(status));
}

void monitor_sigusr1_handler(int sig) {
    
}

void start_monitor() {
    if (monitor_running) {
        printf("Monitor is already running.\n");
        return;
    }

    if (pipe(pipefd) < 0) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    monitor_pid = fork();

    if (monitor_pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (monitor_pid == 0) {
        
        close(pipefd[PIPE_READ]);
        dup2(pipefd[PIPE_WRITE], STDOUT_FILENO);
        dup2(pipefd[PIPE_WRITE], STDERR_FILENO);
        close(pipefd[PIPE_WRITE]);

        struct sigaction sa;
        sa.sa_handler = monitor_sigusr1_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGUSR1, &sa, NULL);

        char command[256];

        while (1) {
            pause();  

            FILE *f = fopen(CMD_FILE, "r");
            if (!f) continue;

            if (!fgets(command, sizeof(command), f)) {
                fclose(f);
                continue;
            }
            fclose(f);
            command[strcspn(command, "\n")] = 0;  

            if (strncmp(command, "stop_monitor", 12) == 0) {
                usleep(500000); 
                exit(0);
            }

            if (strncmp(command, "list_hunts", 10) == 0) {
                system("ls hunts 2>/dev/null");
            } else if (strncmp(command, "list_treasures ", 15) == 0) {
                char hunt_id[100];
                sscanf(command, "list_treasures %s", hunt_id);
                char cmd[200];
                snprintf(cmd, sizeof(cmd), "./treasure_hunt list %s", hunt_id);
                system(cmd);
            } else if (strncmp(command, "view_treasure ", 14) == 0) {
                char hunt_id[100];
                int tid;
                sscanf(command, "view_treasure %s %d", hunt_id, &tid);
                char cmd[200];
                snprintf(cmd, sizeof(cmd), "./treasure_hunt view %s %d", hunt_id, tid);
                system(cmd);
            }

            fflush(stdout);
            fsync(STDOUT_FILENO);  
        }
    } else {
        
        struct sigaction sa;
        sa.sa_handler = sigchld_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        sigaction(SIGCHLD, &sa, NULL);

        close(pipefd[PIPE_WRITE]);  
        monitor_running = 1;
        printf("Monitor started (PID %d).\n", monitor_pid);
    }
}

void send_command(const char *cmd) {
    if (!monitor_running) {
        printf("Monitor is not running. Please start it first.\n");
        return;
    }

    FILE *f = fopen(CMD_FILE, "w");
    if (!f) {
        perror("Failed to write command");
        return;
    }
    fprintf(f, "%s\n", cmd);
    fclose(f);

    kill(monitor_pid, SIGUSR1);

    usleep(100000);

    char buffer[256];
    int n;
    int wait_loops = 0;
    while ((n = read(pipefd[PIPE_READ], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[n] = '\0';
        printf("%s", buffer);
        if (n < 255) break;
        usleep(100000);
        if (++wait_loops > 10) break;
    }
}

int main() {
    char input[256];

    printf("Treasure Hub Interface (Phase 2)\n");
    while (1) {
        printf("\n> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0; 

        if (strcmp(input, "start_monitor") == 0) {
            start_monitor();
        } else if (strcmp(input, "stop_monitor") == 0) {
            send_command("stop_monitor");
        } else if (strcmp(input, "list_hunts") == 0) {
            send_command("list_hunts");
        } else if (strncmp(input, "list_treasures ", 15) == 0) {
            send_command(input);
        } else if (strncmp(input, "view_treasure ", 14) == 0) {
            send_command(input);
        } else if (strcmp(input, "exit") == 0) {
            if (monitor_running) {
                printf("Monitor is still running. Use stop_monitor first.\n");
            } else {
                break;
            }
        } else {
            printf("Unknown command.\n");
        }
    }

    return 0;
}

