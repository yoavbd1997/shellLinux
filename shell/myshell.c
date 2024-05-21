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
#include "LineParser.h"
// all the includes were taken from stackoverflow and the reading materials.
// we got help from stackoverflow, github and etc...
// using the pipe and the array that connect them learnt from the lecture and the reading materials.
typedef struct hist // learnt from previous labs
// except from the new functions and key words that we learnt from this lab and etc, the logic code, the new structs and etc were our idea.
{
    cmdLine *c;
    int size;
    char *input;
    bool toFree;
    struct hist *next;
} hist;

typedef struct process
{
    cmdLine *cmd;         /* the parsed command line*/
    pid_t pid;            /* the process id that is running the command*/
    int status;           /* status of the process: RUNNING/SUSPENDED/TERMINATED */
    struct process *next; /* next process in chain */
    int index;
} process;
void freeProcess(process *p)
{
    free(p);
}
char *getStatusString(int s)
{
    if (s == 1)
    {
        return "Running";
    }
    else if (s == 0)
    {
        return "Suspend";
    }
    return "Terminate";
}
void updateProcessList(process **list) //  the keywords "WUNTRACED", "WIFSIGNALED", "WIFSTOPPED" and "WIFEXITED" taken from stackoverflow
{
    int s;
    process *curr = *list;
    while (curr != NULL)
    {
        pid_t pid = waitpid(curr->pid, &s, WUNTRACED);
        if (pid == -1)
        {
            curr->status = -1;
        }
        else if (pid == 0)
        {
            curr->status = 1;
        }
        else if (WIFSIGNALED(s))
        {
            curr->status = -1;
        }
        else if (WIFSTOPPED(s))
        {
            curr->status = 0;
        }
        else if (WIFEXITED(s))
        {
            curr->status = -1;
        }

        else if (WIFCONTINUED(s))
        {
            curr->status = 1;
        }
        curr = curr->next;
    }
}

void clean(process **list)
{
    process *curr = (*list);
    while (curr != NULL && curr->status == -1)
    {
        *list = curr->next;
        freeProcess(curr);
        curr = *list;
    }
    if (curr == NULL)
    {
        return;
    }

    process *sec = curr->next;
    while (sec != NULL)
    {
        if (sec->status == -1)
        {
            curr->next = sec->next;
            freeProcess(sec);
            sec = curr->next;
        }
        else
        {
            curr = curr->next;
            sec = sec->next;
        }
    }
}
void printProcessList(process **process_list)
{
    process **list = process_list;
    process *curr = *process_list;
    updateProcessList(process_list);

    while (curr != NULL)
    {
        printf("PID          Command      STATUS\n");
        printf("%d          %s      %s\n", curr->pid, curr->cmd->arguments[curr->index], getStatusString(curr->status));
        curr = curr->next;
    }
    clean(list);
}
void addProcess(process **myList, cmdLine *cmd, pid_t pid, int i)
{
    process *p = (process *)malloc(sizeof(process));
    if (cmd != NULL)
    {
        p->cmd = cmd;
    }
    p->pid = pid;
    p->status = 1;
    p->index = i;
    p->next = NULL;
    if (*myList != NULL)
    {
        process *curr = *myList;
        while (curr->next != NULL)
        {
            curr = curr->next;
        }
        curr->next = p;
    }
    else
    {
        *myList = p;
    }
}
void addProcess2(process **myList, cmdLine *cmd, pid_t pid, pid_t pid2, int i, int i2)
{
    process *p = (process *)malloc(sizeof(process));
    process *p2 = (process *)malloc(sizeof(process));
    if (cmd != NULL)
    {
        p->cmd = cmd;
    }
    p->pid = pid;
    p->status = 1;
    p->index = i;
    p->next = p2;
    if (cmd != NULL)
    {
        p2->cmd = cmd->next;
    }
    p2->pid = pid2;
    p2->status = 1;
    p2->index = i2;
    p2->next = NULL;
    if (*myList != NULL)
    {
        process *curr = *myList;
        while (curr->next != NULL)
        {
            curr = curr->next;
        }
        curr->next = p;
    }
    else
    {
        *myList = p;
    }
}
process *getProcess(process **mylist, pid_t pid)
{
    process *curr = *mylist;
    int it = (int)pid;
    while (curr != NULL && (int)curr->pid != it)
    {
        curr = curr->next;
    }
    return curr;
}
void freeProcessList(process *process_list)
{
    while (process_list != NULL)
    {
        process *keep = process_list->next;
        free(process_list);
        process_list = keep;
    }
}

void execute(cmdLine *c, process **myList) // this function learnt from part one of this lab mostly
{

    int pipech[2];
    bool check = false;
    if (c->next != NULL)
    {
        check = true;
    }
    if (check)
    {
        if (pipe(pipech) == -1)
        {
            fprintf(stderr, "pipe\n");
            exit(1);
        }
        pid_t child1 = fork();
        pid_t child2 = fork();
        addProcess2(myList, c, child1, child2, 0, 0);
        if (child1 == -1)
        {
            fprintf(stderr, "fork1\n");
            exit(1);
        }
        else if (child1 == 0)
        {
            close(STDOUT_FILENO); // STD_FILENO learnt from chat gpt.
            dup(pipech[1]);
            close(pipech[0]);
            close(pipech[1]);
            if (c->inputRedirect != NULL)
            {
                FILE *f = fopen(c->inputRedirect, "r");
                if (f == NULL)
                {
                    perror("cant open");
                    _exit(1);
                }
                if (dup2(fileno(f), 0) == -1) // dup2 and fileno are functions that we heard from chatgpt
                {
                    perror("cant dup");
                    _exit(1);
                }
                printf("\n");
                fclose(f);
            }
            if (c->outputRedirect != NULL)
            {
                FILE *f = fopen(c->outputRedirect, "w");
                if (f == NULL)
                {
                    perror("cant open");
                    _exit(1);
                }
                if (dup2(fileno(f), 1) == -1)
                {
                    perror("cant dup");
                    _exit(1);
                }
            }
            if (execvp(c->arguments[0], c->arguments) == -1)
            {
                fprintf(stderr, "execvp1\n");
                exit(1);
            }
        }
        else
        {

            if (child2 == -1)
            {
                fprintf(stderr, "fork\n");
                exit(1);
            }
            else if (child2 == 0)
            {
                close(STDIN_FILENO);
                dup(pipech[0]);
                close(pipech[1]);
                close(pipech[0]);

                if (c->next->inputRedirect != NULL)
                {
                    FILE *f = fopen(c->next->inputRedirect, "r");
                    if (f == NULL)
                    {
                        perror("cant open");
                        _exit(1);
                    }
                    if (dup2(fileno(f), 0) == -1) // dup2 and fileno are functions that we heard from chatgpt
                    {
                        perror("cant dup");
                        _exit(1);
                    }
                    printf("\n");
                    fclose(f);
                }
                if (c->next->outputRedirect != NULL)
                {
                    FILE *f = fopen(c->next->outputRedirect, "w");
                    if (f == NULL)
                    {
                        perror("cant open");
                        _exit(1);
                    }
                    if (dup2(fileno(f), 1) == -1)
                    {
                        perror("cant dup");
                        _exit(1);
                    }
                }
                if (execvp(c->next->arguments[0], c->next->arguments) == -1)
                {
                    fprintf(stderr, "execvp\n");
                    exit(1);
                }
                close(STDOUT_FILENO);
            }
            else
            {
                close(pipech[0]);
                close(pipech[1]);

                if (c->blocking == 1)
                {
                    int s;
                    if (waitpid(child1, &s, 0) == -1) // waitpid is from moodle
                    {
                        perror("waitpid");
                        _exit(1);
                    }
                }
                if (c->next->blocking == 1)
                {
                    int s2;
                    if (waitpid(child2, &s2, 0) == -1)
                    {
                        printf("waitpid");
                        exit(1);
                    }
                }
            }
        }
    }
    else
    {
        pid_t pid = fork();
        addProcess(myList, c, pid, 0);
        if (pid == -1)
        {
            fprintf(stderr, "fork\n");
            exit(1);
        }
        else if (pid == 0)
        {
            fprintf(stderr, "pid: %d\n", getpid());
            fprintf(stderr, "executing command: %s\n", c->arguments[0]);
            if (c->inputRedirect != NULL)
            {
                FILE *f = fopen(c->inputRedirect, "r");
                if (f == NULL)
                {
                    perror("cant open");
                    _exit(1);
                }
                if (dup2(fileno(f), 0) == -1) // dup2 and fileno are functions that we heard from chatgpt
                {
                    perror("cant dup");
                    _exit(1);
                }
                printf("\n");
                fclose(f);
            }
            if (c->outputRedirect != NULL)
            {
                FILE *f = fopen(c->outputRedirect, "w");
                if (f == NULL)
                {
                    perror("cant open");
                    _exit(1);
                }
                if (dup2(fileno(f), 1) == -1)
                {
                    perror("cant dup");
                    _exit(1);
                }
            }
            if (execvp(c->arguments[0], c->arguments) == -1) // execvp is from moodle
            {
                perror("cant exe");
                _exit(1);
            }
        }
        else if (pid == -1)
        {
            perror("fork");
            _exit(1);
        }
        else
        {
            if (c->blocking == 1)
            {
                int s;
                if (waitpid(pid, &s, 0) == -1) // waitpid is from moodle
                {
                    perror("waitpid");
                    _exit(1);
                }
            }
        }
    }
}
int numberOfHis(hist **myHist)
{
    hist *cur = (*myHist);
    int sum = 0;
    while (cur != NULL)
    {
        sum = sum + 1;
        cur = cur->next;
    }
    return sum;
}
void printHist(hist **myHist, cmdLine *c)
{
    int b = numberOfHis(myHist);
    if (c->argCount > 1)
    {
      
        b = atoi(c->arguments[1]);
    }
    hist *curr = (*myHist);
    while (curr != NULL && numberOfHis(myHist)-b>0)
    {
        curr = curr->next;
        b=b+1;
    }

    while (curr != NULL )
    {
        for (int i = 0; i < 2048; i++)
        {
            if (curr->input[i] == '\0')
            {

                break;
            }
            else
            {
                printf("%c", curr->input[i]);
            }
        }
        printf("\n");
        curr = curr->next;
    }
}

void freeHist(hist **h)
{
    hist *curr = *h;
    while (curr != NULL)
    {
        free(curr->input);
        if (curr->c != NULL && curr->toFree) // check if the pointer is NULL before freeing
        {
            freeCmdLines(curr->c);
        }
        hist *keep = curr->next;
        free(curr);
        curr = keep;
    }
    *h = NULL;
}

void checkHist(cmdLine *c, hist **myHist, char input[2048])
{
    hist *p = (hist *)malloc(sizeof(hist));
    if (c != NULL)
    {
        p->c = c;
    }

    char *input2 = (char *)malloc(2048 * sizeof(char));
    int size = 0;
    for (int i = 0; i < 2048; i++)
    {
        if (input[i] == '\0')
        {
            input2[i] = '\0';
            break;
        }
        input2[i] = input[i]; // copy character from input to input2
        size++;
    }
    p->input = input2;
    p->size = size;
    p->toFree = true;
    hist *curr = *myHist;
    hist *prev = NULL;
    while (curr != NULL && curr->next != NULL)
    {
        prev = curr;
        curr = curr->next;
    }

    if (curr == NULL)
    {
        *myHist = p;
    }
    else
    {

        curr->next = p;
    }
    p->next = NULL;
    if (numberOfHis(myHist) >= 20)
    {
        hist *first = (*myHist)->next;
        prev = (*myHist);
        if (first != NULL)
        {
            (*myHist)=first;
            prev->next=NULL;
        }
        free(prev->input);
        freeCmdLines(prev->c);
        free(prev);
    }
}

void numberOfCommand(hist **myHist, int index, process **myList)
{ // this function very similar to the main function. most of the code was taken from lab 2 but with more if's for commands.
    hist *curr = *myHist;
    if (index <= numberOfHis(myHist) && index >= 1)
    {
        for (int i = 0; i < index - 1; i++)
        {
            curr = curr->next;
        }
        cmdLine *c = curr->c;
        if (strcmp(c->arguments[0], "history") == 0)
        {
           
            printHist(myHist, c);
        }
        else if (strcmp(c->arguments[0], "procs") == 0)
        {
            printProcessList(myList);
        }
        else if (strcmp(c->arguments[0], "wake") == 0)
        {
            pid_t pid = atoi(c->arguments[1]);
            if (kill(pid, SIGCONT) == -1)
            {
                perror("kill");
            }
        }
        else if (strcmp(c->arguments[0], "suspend") == 0)
        {
            pid_t pid = atoi(c->arguments[1]);
            if (kill(pid, SIGSTOP) == -1)
            {
                perror("kill");
            }
            updateProcessList(myList);
        }
        else if (strcmp(c->arguments[0], "kill") == 0)
        {
            if (c->arguments[1] == NULL)
            {
                printf("Error: Missing argument for kill command\n");
            }
            else
            {
                pid_t pid = atoi(c->arguments[1]);
                if (kill(pid, SIGINT) == -1)
                {
                    printf("kill\n");
                }
                updateProcessList(myList);
            }
        }
        else if (strcmp(c->arguments[0], "cd") != 0)
        {
            execute(c, myList);
        }
        else if (strcmp(c->arguments[0], "cd") == 0)
        {
            if (chdir(c->arguments[1]) == -1)
            {
                fprintf(stderr, "failed to cd: %s\n", c->arguments[1]);
            }
            else
            {
                fprintf(stderr, "in file: %s\n", c->arguments[1]);
            }
        }
    }
    else
    {
        fprintf(stderr, "%d is not illegal\n", index);
    }
}
int main(int argc, char *argv[]) // most of the code was taken from lab 2 but with more if's for commands.
{
    process **myList = malloc(sizeof(process *));
    *myList = NULL;
    hist **myHist = malloc(sizeof(hist *));
    *myHist = NULL;
    bool check = false;
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-d") == 0)
        {
            check = true;
        }
    }
    if (check)
    {
        fprintf(stderr, "arguments: ");
        for (int i = 0; i < argc; i++)
        {
            fprintf(stderr, "%s ", argv[i]);
        }
        fprintf(stderr, "\n");
    }
    while (1)
    {
        char input[2048];
        fgets(input, 2048, stdin);
        input[strcspn(input, "\n")] = '\0';
        if (strlen(input) > 0)
        {
            cmdLine *c = parseCmdLines(input);
            bool done = false;
            if (input[0] == '!')
            {
                if (strlen(c->arguments[0]) == 2)
                {
                    done = true;
                    if (input[1] != '!')
                    {
                        numberOfCommand(myHist, (int)input[1], myList);
                    }
                    else
                    {
                        numberOfCommand(myHist, numberOfHis(myHist), myList);
                    }
                }
            }
            else
            {
                checkHist(c, myHist, input);
            }
            if ((c->argCount > 0) && (strcmp(c->arguments[0], "quit")) == 0)
            {
                break;
            }

            else if ((c->argCount > 0) && (strcmp(c->arguments[0], "history")) == 0)
            {
               
                printHist(myHist, c);
            }
            else if ((c->argCount > 0) && (strcmp(c->arguments[0], "procs")) == 0)
            {
                printProcessList(myList);
            }
            else if ((c->argCount > 1) && (strcmp(c->arguments[0], "wake")) == 0)
            {
                pid_t pid = atoi(c->arguments[1]);
                if (kill(pid, SIGCONT) == -1) // taken from moodle
                {
                    perror("kill");
                }
            }
            else if ((c->argCount > 1) && (strcmp(c->arguments[0], "suspend")) == 0)
            {
                pid_t pid = atoi(c->arguments[1]);
                if (kill(pid, SIGSTOP) == -1)
                {
                    perror("kill");
                }
            }
            else if ((c->argCount > 1) && (strcmp(c->arguments[0], "kill")) == 0)
            {
                if (c->arguments[1] == NULL)
                {
                    printf("Error: Missing argument for kill command\n");
                }
                else
                {
                    pid_t pid = atoi(c->arguments[1]);
                    if (kill(pid, SIGINT) == -1) // taken from lab 2
                    {
                        printf("kill\n");
                    }
                }
            }
            else if ((c->argCount > 0) && (strcmp(c->arguments[0], "cd") != 0))
            {
                if (!done)
                {
                    execute(c, myList);
                }
            }
            else if ((c->argCount > 1) && (strcmp(c->arguments[0], "cd") == 0))
            {
                if (chdir(c->arguments[1]) == -1)
                {
                    fprintf(stderr, "failed to cd: %s\n", c->arguments[1]);
                }
                else
                {
                    fprintf(stderr, "in file: %s\n", c->arguments[1]);
                }
            }
        }
    }
    if (*myList != NULL)
    {

        freeProcessList(*myList);
    }
    if (*myHist != NULL)
    {
        freeHist(myHist);
    }
    free(myList);
    free(myHist);
    return 0;
}