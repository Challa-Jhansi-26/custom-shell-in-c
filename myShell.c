#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>

#define MAX_LINE 1024
#define MAX_ARGS 100

typedef struct {
    pid_t pid;
    char command[MAX_LINE];
    int is_running;
} Job;

#define MAX_JOBS 100
Job jobs[MAX_JOBS];
int num_jobs = 0;

void sigint_handler(int sig) {
    printf("\nmysh> ");
    fflush(stdout);
}

void add_job(pid_t pid, char *cmd) {
    if (num_jobs < MAX_JOBS) {
        jobs[num_jobs].pid = pid;
        strncpy(jobs[num_jobs].command, cmd, MAX_LINE - 1);
        jobs[num_jobs].command[MAX_LINE - 1] = '\0';
        jobs[num_jobs].is_running = 1;
        num_jobs++;
    } else {
        printf("Job list full!\n");
    }
}

void list_jobs() {
    printf("[Jobs]\n");
    for (int i = 0; i < num_jobs; i++) {
        printf("[%d] PID %d - %s [%s]\n",
               i + 1,
               jobs[i].pid,
               jobs[i].command,
               jobs[i].is_running ? "Running" : "Stopped");
    }
}

void bring_fg(int job_number) {
    if (job_number < 1 || job_number > num_jobs) {
        printf("Invalid job number\n");
        return;
    }
    pid_t pid = jobs[job_number - 1].pid;
    int status;
    tcsetpgrp(STDIN_FILENO, pid);
    kill(-pid, SIGCONT);
    waitpid(pid, &status, WUNTRACED);
    tcsetpgrp(STDIN_FILENO, getpgrp());
    if (WIFSTOPPED(status)) {
        jobs[job_number - 1].is_running = 0;
    } else {
        jobs[job_number - 1] = jobs[num_jobs - 1];
        num_jobs--;
    }
}

void send_bg(int job_number) {
    if (job_number < 1 || job_number > num_jobs) {
        printf("Invalid job number\n");
        return;
    }
    pid_t pid = jobs[job_number - 1].pid;
    kill(-pid, SIGCONT);
    jobs[job_number - 1].is_running = 1;
}

void parse(char *line, char **args, int *is_background) {
    *is_background = 0;
    line[strcspn(line, "\n")] = 0;

    int i = 0;
    char *token = strtok(line, " ");
    while (token != NULL && i < MAX_ARGS - 1) {
        if (strcmp(token, "&") == 0) {
            *is_background = 1;
        } else {
            args[i++] = token;
        }
        token = strtok(NULL, " ");
    }
    args[i] = NULL;
}

int is_builtin(char **args) {
    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    }
    if (strcmp(args[0], "cd") == 0) {
        if (args[1]) {
            if (chdir(args[1]) != 0) perror("cd failed");
        } else {
            chdir(getenv("HOME"));
        }
        return 1;
    }
    if (strcmp(args[0], "jobs") == 0) {
        list_jobs();
        return 1;
    }
    if (strcmp(args[0], "fg") == 0 && args[1]) {
        bring_fg(atoi(args[1]));
        return 1;
    }
    if (strcmp(args[0], "bg") == 0 && args[1]) {
        send_bg(atoi(args[1]));
        return 1;
    }
    return 0;
}

void handle_redirection(char **args) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0) {
            int fd = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) { perror("open"); exit(1); }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args[i] = NULL;
        } else if (strcmp(args[i], "<") == 0) {
            int fd = open(args[i+1], O_RDONLY);
            if (fd < 0) { perror("open"); exit(1); }
            dup2(fd, STDIN_FILENO);
            close(fd);
            args[i] = NULL;
        }
    }
}

int has_pipe(char **args) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) return 1;
    }
    return 0;
}

void execute_multiple_pipes(char **args) {
    int num_cmds = 0;
    char *commands[MAX_ARGS][MAX_ARGS];
    int i = 0, j = 0;

    // Split into separate commands on "|"
    while (args[i] != NULL) {
        if (strcmp(args[i], "|") == 0) {
            commands[num_cmds][j] = NULL;
            num_cmds++;
            j = 0;
        } else {
            commands[num_cmds][j++] = args[i];
        }
        i++;
    }
    commands[num_cmds][j] = NULL;
    num_cmds++;

    int pipefd[2 * (num_cmds - 1)];
    for (i = 0; i < num_cmds - 1; i++) {
        if (pipe(pipefd + i*2) < 0) {
            perror("pipe");
            exit(1);
        }
    }

    int pid;
    int cmd_no = 0;
    int k;

    while (cmd_no < num_cmds) {
        pid = fork();
        if (pid == 0) {
            // stdin from previous pipe
            if (cmd_no != 0) {
                dup2(pipefd[(cmd_no - 1)*2], 0);
            }
            // stdout to next pipe
            if (cmd_no != num_cmds - 1) {
                dup2(pipefd[cmd_no*2 + 1], 1);
            }
            for (k = 0; k < 2*(num_cmds - 1); k++) {
                close(pipefd[k]);
            }
            handle_redirection(commands[cmd_no]);
            execvp(commands[cmd_no][0], commands[cmd_no]);
            perror("execvp");
            exit(1);
        } else if (pid < 0) {
            perror("fork");
            exit(1);
        }
        cmd_no++;
    }
    for (i = 0; i < 2*(num_cmds - 1); i++) {
        close(pipefd[i]);
    }
    for (i = 0; i < num_cmds; i++) {
        wait(NULL);
    }
}

int main() {
    char line[MAX_LINE];
    char *args[MAX_ARGS];
    int is_background;

    signal(SIGINT, sigint_handler);

    while (1) {
        printf("mysh> ");
        fflush(stdout);

        if (!fgets(line, MAX_LINE, stdin)) {
            break;
        }

        parse(line, args, &is_background);
        if (args[0] == NULL) continue;

        if (is_builtin(args)) continue;

        pid_t pid = fork();

        if (pid == 0) {
            setpgid(0, 0); // new process group
            if (has_pipe(args)) {
                execute_multiple_pipes(args);
            } else {
                handle_redirection(args);
                execvp(args[0], args);
                perror("execvp");
            }
            exit(1);
        } else if (pid > 0) {
            setpgid(pid, pid);
            if (!is_background) {
                tcsetpgrp(STDIN_FILENO, pid);
                waitpid(pid, NULL, WUNTRACED);
                tcsetpgrp(STDIN_FILENO, getpgrp());
            } else {
                add_job(pid, line);
            }
        } else {
            perror("fork");
        }
    }
    return 0;
}
