#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>


typedef struct
{
    pid_t pid;
    char filename[100];
    char fileExt[50];
    int processed;
} prcVector;

prcVector pv[1000];

char * id = "./input_dir";
char * od = "./outputdir";

char *images[100000];
char *html[100000];

int  loaded_images = 0;
int  processed_images = 0;

int main (void)
{
    prcVector pv[1000];

    DIR *dp;
    struct dirent *ep;
    struct stat st = {0};
    if (stat(od, &st) == -1)
    {
        mkdir(od, 0700);
    }

    dp = opendir ("./input_dir");

    if (dp != NULL)
    {
        while (ep = readdir (dp))
        {
            if (ep->d_type == DT_REG)
            {
                struct stat sf;
                stat (ep->d_name, &sf);
                memset(pv[loaded_images].filename,0x00,sizeof(char)*100);
                memset(pv[loaded_images].fileExt,0x00,sizeof(char)*50);
                pv[loaded_images].processed = 0;
                sprintf(pv[loaded_images].filename,"%s\n",ep->d_name);
                loaded_images++;
            }
        }
        (void) closedir (dp);
    }
    else
        puts ("Couldn't open the directory.");

    int i;
    int n = 5;

    /* Start children. */
    for (i = 0; i < n; ++i)
    {
        pid_t p = fork();

        if (p != 0)
        {
            pv[i].pid = p;
            printf("parent\n");
            continue;
        }

        if ( p == 0)
        {
            printf("child\n");
            int j = 0;
            int f = 0;

            while(f == 0)
            {
                j = 0;
                while(pv[j].pid != getpid() && j < loaded_images)
                {
                    if (pv[j].pid == 0)
                    {
                        continue;
                    }

                    printf("A--------Child %i found  index %i %i\n",getpid(),j,pv[j].pid);

                    j++;
                }

                if (j < loaded_images)
                {
                    f = 1;
                }

            }

            j = 0;

            while(pv[j].pid != getpid() && j < loaded_images)
            {
                //printf("A--------Child %i at index %i %i - Processing image = %s",getpid(),j,pv[j].pid,pv[j].filename);
                j++;
            }

            --j;

            if (pv[j].pid != getpid())
            {
                pv[j].processed = 1;
                printf("B--------Child %i at index %i - Processing image = %s",getpid(),j,pv[j].filename);
            }

            return EXIT_SUCCESS;
        }

    }


    /* Wait for children to exit. */
    int status;

    pid_t pid;

    while (n > 0)
    {
        pid = wait(&status);
        printf("Child with PID %ld exited with status 0x%x.\n", (long)pid, status);
        --n;  // TODO(pts): Remove pid from the pids array.
    }


    return EXIT_SUCCESS;
}
