#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdarg.h>

#define CMD_FILE "monitor_command.txt"

pid_t monitor_pid = 0;
int monitor_running = 0;

//signal hendler function for child
//this function waits for the monitor process to be finished and displays the status
void sigchld_handler(int signo) 
{
    int status;
    waitpid(monitor_pid, &status, 0);
    monitor_running = 0;
    printf("Monitor terminated with status %d.\n", WEXITSTATUS(status));
}

volatile sig_atomic_t got_sigusr1 = 0;
volatile sig_atomic_t got_sigusr2 = 0;

//flags when the monitor process receives a signal from the parent in the main function
void handle_sigusr1(int sig) 
{
    got_sigusr1 = 1;
}

void handle_sigusr2(int sig) 
{
    got_sigusr2 = 1;
}

void run_monitor() 
{
    struct sigaction sa1, sa2;
    //how a process should respond to a specific signal 
    sa1.sa_handler = handle_sigusr1;
    sigemptyset(&sa1.sa_mask); //no other signals will be blocked during exec of hadle_sigurs1
    sa1.sa_flags = 0; //default behavior
    sigaction(SIGUSR1, &sa1, NULL);

    sa2.sa_handler = handle_sigusr2;
    sigemptyset(&sa2.sa_mask);
    sa2.sa_flags = 0;
    sigaction(SIGUSR2, &sa2, NULL);

    printf("[Monitor] Running and waiting for commands...\n");

    while (1) 
    {
        pause();  // the system pauses until it receives a signal

	//basically stop_monitor process is simulated down below with a simulated delay
        if (got_sigusr2) 
        {
            printf("[Monitor] Received stop signal. Shutting down after delay...\n");
            usleep(500000); // simulate delay
            exit(0);
        }

        if (got_sigusr1) 
        {
            got_sigusr1 = 0;

            FILE *cmd = fopen(CMD_FILE, "r");
            if (!cmd) 
            {
                perror("[Monitor] Failed to open command file");
                continue;
            }

            char op[64], hunt[64];
            int id;

            fscanf(cmd, "%s", op);
            if (strcmp(op, "list_hunts") == 0) 
            {
                printf("[Monitor] Listing hunts:\n");
                // This lists all directories starting with "Hunt"
                system("ls -d Hunt*/ 2>/dev/null | while read dir; do echo -n \"$dir: \"; ./treasure_hunt list \"$dir\" | grep -c 'ID:'; done");
            } 
            else if (strcmp(op, "list_treasures") == 0) 
            {
                fscanf(cmd, "%s", hunt);
                printf("[Monitor] Listing treasures in %s:\n", hunt);
                char cmd_str[128];
                snprintf(cmd_str, sizeof(cmd_str), "./treasure_hunt list %s", hunt);
                //this basically executes the task given that is found in treasure_hunt
                system(cmd_str);
            } 
            else if (strcmp(op, "view_treasure") == 0) 
            {
                fscanf(cmd, "%s %d", hunt, &id);
                printf("[Monitor] Viewing treasure %d in %s:\n", id, hunt);
                char cmd_str[128];
                snprintf(cmd_str, sizeof(cmd_str), "./treasure_hunt view %s %d", hunt, id);
                system(cmd_str);
                //system function executes a shell command (int system(const char *command))
            } 
            else 
            {
                printf("[Monitor] Unknown command: %s\n", op);
            }

            fclose(cmd);
        }
    }
}


//the command we use is written in the CMD_FILE defined previosly and passes SIGUSR1 signal to the monitor
void write_command(const char *format, ...) 
{
    FILE *f = fopen(CMD_FILE, "w");
    if (!f) {
        perror("Failed to write to command file");
        return;
    }
    va_list args;
    va_start(args, format);
    vfprintf(f, format, args);
    va_end(args);
    fclose(f);
    kill(monitor_pid, SIGUSR1);
}

int main() 
{
    //it treats the ending of monitor process
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    char input[256];
    printf("Treasure Hub started!\n");

    while (1) 
    {
        printf("> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "start_monitor") == 0) 
        {
            if (monitor_running) 
            {
                printf("Monitor is already running.\n");
                continue;
            }

            monitor_pid = fork();
            if (monitor_pid < 0) 
            {
                perror("Failed to fork monitor");
                continue;
            }

            if (monitor_pid == 0) 
            {
                run_monitor();
                exit(0); 
            }

            monitor_running = 1;
            printf("Monitor started (PID %d).\n", monitor_pid);
        }

        else if (strcmp(input, "list_hunts") == 0) 
        {
            if (!monitor_running) 
            {
                printf("Monitor is not running.\n");
                continue;
            }
            write_command("list_hunts\n");
        }

        else if (strncmp(input, "list_treasures", 14) == 0) 
        {
            if (!monitor_running) 
            {
                printf("Monitor is not running.\n");
                continue;
            }
            char hunt[64];
            if (sscanf(input, "list_treasures %s", hunt) == 1) 
            {
                write_command("list_treasures %s\n", hunt);
            } 
            else 
            {
                printf("Usage: list_treasures <hunt_id>\n");
            }
        }

        else if (strncmp(input, "view_treasure", 13) == 0) 
        {
            if (!monitor_running) 
            {
                printf("Monitor is not running.\n");
                continue;
            }
            char hunt[64];
            int id;
            if (sscanf(input, "view_treasure %s %d", hunt, &id) == 2) 
            {
                write_command("view_treasure %s %d\n", hunt, id);
            } 
            else 
            {
                printf("Usage: view_treasure <hunt_id> <id>\n");
            }
        }
        
        else if(strncmp(input, "calculate_scores",16) == 0)
        {
            if(!monitor_running)
            {
                printf("Monitor is not running.\n");
                continue;
            }
            char hunt[128];
            
            if(sscanf(input,"calculate_scores %s",hunt) == 1)
            {
            	int pfd[2];
            	if(pipe(pfd) == -1)
            	{
            		perror("error creating the pipe.\n");
            		continue;
            	}
            	
            	pid_t pid = fork();
            	if(pid == -1)
            	{
            	 	perror("error at fork.\n");
            	 	exit(-1);
            	}
            	
            	if(pid == 0)
            	{
            		close(pfd[0]);
            		dup2(pfd[1], STDOUT_FILENO);
            		close(pfd[1]);
            		
            		execl("./calculate_scores","calculate_scores",hunt,NULL);
            		perror("error using execl.\n");
            		exit(-1);
            	}
            	
            	else
            	{
            		close(pfd[1]);
            		char buffer[256];
            		ssize_t n;
            		printf("Scores for %s:\n", hunt);
            		while((n = read(pfd[0], buffer, sizeof(buffer)-1)) > 0)
            		{
            			buffer[n] = '\0';
            			printf("%s", buffer);
            		}
            		close(pfd[0]);
            		waitpid(pid,NULL,0);
            	}
            }
            
            else 
            {
            	printf("Usage: calculate_scores <hunt_directory>\n");
            }
            
        }

        else if (strcmp(input, "stop_monitor") == 0) 
        {
            if (!monitor_running) 
            {
                printf("Monitor is not running.\n");
                continue;
            }
            kill(monitor_pid, SIGUSR2);
            printf("Sent stop signal to monitor.\n");
        }
        
        else if (strcmp(input, "exit") == 0) 
        {
            if (monitor_running) 
            {
                printf("Cannot exit: monitor still running.\n");
                continue;
            }
            printf("Exiting Treasure Hub.\n");
            break;
        }

        else 
        {
            printf("Unknown command.\n");
        }
    }

    return 0;
}

