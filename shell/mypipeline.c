#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
// all the includes were taken from stackoverflow and the reading materials.
// using the pipe and the array that connect them learnt from the lecture and the reading materials.
void mypipeline()
{
    int pipech[2];
    if (pipe(pipech) == -1)
    {
        fprintf(stderr, "pipe\n");
        exit(1);
    }
    pid_t child1 = fork();
    if (child1 == -1)
    {
        fprintf(stderr, "fork1\n");
        exit(1);
    }
    else if (child1 == 0)
    {
        close(STDOUT_FILENO);
        dup(pipech[1]); // learnt it from the reading materials
        close(pipech[0]);
        close(pipech[1]);
        char *c1[] = {"ls", "-l", NULL};
        if (execvp(c1[0], c1) == -1)
        {
            fprintf(stderr, "execvp1\n");
            exit(1);
        }
    }
    else
    {
        close(pipech[1]);
        pid_t child2 = fork();
        if (child2 == -1)
        {
            fprintf(stderr, "fork1\n");
            exit(1);
        }
        else if (child2 == 0)
        {
            close(STDIN_FILENO); // STD_FILENO was learnt from chat gpt.
            dup(pipech[0]);
            close(pipech[1]);
            close(pipech[0]);
            char *c2[] = {"tail", "-n", "2", NULL};
            if (execvp(c2[0], c2) == -1)
            {
                fprintf(stderr, "execvp1\n");
                exit(1);
            }
        }
        else
        {
            close(pipech[0]);
            int s1, s2;
            if (waitpid(child1, &s1, 0) == -1)
            {
                perror("waitpid\n");
                exit(1);
            }
            if (waitpid(child2, &s2, 0) == -1) // moodle
            {
                perror("waitpid\n");
                exit(1);
            }
        }
    }
}
int main(int argc, char const *argv[])
{
    mypipeline();
    return 0;
}
