#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
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
/* Execute the command using this shell program.  */
#define SHELL "/bin/sh"

typedef struct
{
    pid_t pid;
    char iFilename[100];
    char oFilename[100];
    char fileExt[50];
    int processed;
} prcVector;

typedef struct
{
    int j;
    pid_t pid;
} workarea;

prcVector pv[1000];

char * id = "./input_dir";
char * od = "./outputdir";

int  loaded_images = 0;

int main (void)
{
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
        while ((ep = readdir (dp)))
        {
            if (ep->d_type == DT_REG)
            {
                struct stat sf;
                stat (ep->d_name, &sf);
                memset(pv[loaded_images].iFilename,0x00,sizeof(char)*100);
                memset(pv[loaded_images].oFilename,0x00,sizeof(char)*100);
                memset(pv[loaded_images].fileExt,0x00,sizeof(char)*50);
                pv[loaded_images].processed = 0;

                const char delim[]=".";
                char * ext;
                char * ptr=strtok(ep->d_name,delim);
                while(ptr!=NULL)
                {
                    ext = ptr;
                    ptr = strtok (NULL, delim);
                }
                char * root = strtok(ep->d_name,delim);


                strcat(pv[loaded_images].iFilename,id);
                strcat(pv[loaded_images].iFilename,"/");
                strcat(pv[loaded_images].iFilename,root);
                strcat(pv[loaded_images].iFilename,".");
                strcat(pv[loaded_images].iFilename,ext);

                sprintf(pv[loaded_images].fileExt,"%s",ext);

                strcat(pv[loaded_images].oFilename,od);
                strcat(pv[loaded_images].oFilename,"/");
                strcat(pv[loaded_images].oFilename,root);
                strcat(pv[loaded_images].oFilename,".");
                strcat(pv[loaded_images].oFilename,"jpg\0");


                printf("%s  %s\n",pv[loaded_images].iFilename,pv[loaded_images].oFilename);

                loaded_images++;
            }
        }
        (void) closedir (dp);
    }
    else
        puts ("Couldn't open the directory.");

    int i;
    int n = 15;

    for (i = 0; i < n; ++i)
    {
        pid_t p = fork();

        if (p != 0)
        {
            printf("A--------Parent of %i\n",p);
            int status;
            waitpid(p, &status, 0);
        }

        if ( p == 0)
        {
            workarea * w = malloc(sizeof(workarea));
            w->j = 0;
            w->pid = getpid();

            while(w->j < loaded_images)
            {
                if((w->pid % 2 == 0) && strncmp(pv[w->j].fileExt,"png",3) == 0)
                {
                    printf("%i - %i PNG ---------->%s\n",w->pid ,w->pid % 2,pv[w->j].oFilename);

                    struct stat sb;
                    if(stat(pv[w->j].oFilename, &sb) == 0 && S_ISREG(sb.st_mode))
                    {
                        printf("Exist\n");
                    }
                    else
                    {
                        printf("Does not Exist\n");
                        //open (pv[w->j].filename, O_WRONLY | O_CREAT | O_TRUNC, "w+");
                        char command[1000];
                        sprintf(command,"convert %s %s\n",pv[w->j].iFilename,pv[w->j].oFilename);
                        int w = execl (SHELL, SHELL, "-c", command, NULL);
                    }

                    w->j++;
                    continue;
                }

                if(w->pid % 3 == 0 && strncmp(pv[w->j].fileExt,"bmp",3) == 0)
                {
                    printf("%i - %i BMP ---------->%s\n",w->pid,w->pid % 3,pv[w->j].oFilename);

                    struct stat sb;
                    if(stat(pv[w->j].oFilename, &sb) == 0 && S_ISREG(sb.st_mode))
                    {
                        printf("Exist\n");
                    }
                    else
                    {
                        printf("Does not Exist\n");
                        //open (pv[w->j].oFilename, O_WRONLY | O_CREAT | O_TRUNC, "w+");
                        char command[1000];
                        sprintf(command,"convert %s %s\n",pv[w->j].iFilename,pv[w->j].oFilename);
                        int w = execl (SHELL, SHELL, "-c", command, NULL);
                    }

                    w->j++;
                    continue;
                }

                if((w->pid % 2 != 0 || w->pid % 3 != 0) && strncmp(pv[w->j].fileExt,"gif",3) == 0)
                {
                    printf("%i - GIF ---------->%s\n",w->pid ,pv[w->j].oFilename);

                    struct stat sb;
                    if(stat(pv[w->j].oFilename, &sb) == 0 && S_ISREG(sb.st_mode))
                    {
                        printf("Exist\n");
                    }
                    else
                    {
                        printf("Does not Exist\n");
                        //open (pv[w->j].oFilename, O_WRONLY | O_CREAT | O_TRUNC, "w+");
                        char command[1000];
                        sprintf(command,"convert %s %s\n",pv[w->j].iFilename,pv[w->j].oFilename);
                        int w = execl (SHELL, SHELL, "-c", command, NULL);
                    }

                    w->j++;
                    continue;
                }


                struct stat sb;
                if(stat(pv[w->j].oFilename, &sb) == 0 && S_ISREG(sb.st_mode))
                {
                    printf("Exist\n");
                }
                else
                {
                    printf("Does not Exist\n");
                    //open (pv[w->j].oFilename, O_WRONLY | O_CREAT | O_TRUNC, "w+");
                    char command[1000];
                    sprintf(command,"convert %s %s\n",pv[w->j].iFilename,pv[w->j].oFilename);
                    int w = execl (SHELL, SHELL, "-c", command, NULL);
                }

                w->j++;
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
