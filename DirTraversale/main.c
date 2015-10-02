#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
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
#include <limits.h>
#include <ftw.h>

const int MAX_DIR_PATH = 2048;

//Create all mutexes
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t updatef = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t updated = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t threadIncrementer = PTHREAD_MUTEX_INITIALIZER;

pthread_spinlock_t lock;


int dirCount  = 0;
int fileCount = 0;
int jpgCount = 0;
int gifCount = 0;
int bmpCount = 0;
int pngCount = 0;
int msCount = 100;

volatile sig_atomic_t timerFlag = 0;

struct sigaction sa;
struct itimerval timer;

struct f_infos
{
    pthread_t thread_id;
    struct stat * info;
    int inode;
    int size;
    char * path;
    char * filename;
    char * filetype;
    char * currentDir;
    char * date;

};

struct thread_info
{
    pthread_t thread_id;
    struct stat * info;
    char * Dir;
    struct dirent * currFile;
    int currFileCount;
};

const int MAX_THREAD = 2048;
struct thread_info * threads;
int INDEX_THREAD = 0;

FILE * htmlfile;
FILE * logFile;
char * inputdirectory;
char * outputdirectory;
int mode = 0;

/*
 Function: timer handler function triggered by ITIMER_REAL / SIGALRM
*/
static void timer_handler (int signum )
{
    if(timerFlag == 1)
    {
        return;
    }

    printf("|");

    int localDirCount;
    int localFileCount;

    pthread_mutex_lock(&updated);
    localDirCount = dirCount;
    pthread_mutex_unlock(&updated);

    pthread_mutex_lock(&updatef);
    localFileCount = fileCount;
    pthread_mutex_unlock(&updatef);

    fprintf(logFile, "Time 	%i ms	#dir	%i	#files	%i\n", msCount, localDirCount, localFileCount);

    fflush(logFile);
    msCount += 100;
}

/*
 Function: initalize a signal timer
*/
void timer_start()
{
    /* Install timer_handler as the signal handler for SIGVTALRM.  */
    memset (&sa, 0x00, sizeof (sa));

    sigemptyset(&sa.sa_mask);
    sigfillset(&sa.sa_mask);
    sigaddset (&sa.sa_mask, SIGALRM);
    sa.sa_handler = &timer_handler;
    sa.sa_flags = 0;
    sigaction (SIGALRM, &sa, NULL);

    /* Configure the timer to expire after 100 msec...  */
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 100;
    /* ... and every 100 msec after that.  */
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 100;
    /* Start a virtual timer.  It counts down whenever this process is
       executing.  */
    printf("^");
    int rc = setitimer(ITIMER_REAL, &timer, NULL);
    if (rc == -1)
        printf("TIMER ERROR:%s\n",strerror(errno));



}

void incrementFileCount(int extension)
{
    pthread_mutex_lock(&updatef);

    fileCount++;

    switch(extension)
    {
    case 1:
        gifCount++;
        break;
    case 2:
        bmpCount++;
        break;
    case 3:
        jpgCount++;
        break;
    case 4:
        pngCount++;
        break;
    }

    pthread_mutex_unlock(&updatef);
}

void incrementDirCount(void)
{
    pthread_mutex_lock(&updated);
    dirCount++;
    pthread_mutex_unlock(&updated);
}

void writeHtml(struct f_infos * info)
{
    pthread_mutex_lock(&mut);
    fprintf(htmlfile, "<a href=\".%s/%s\"> <img src=\".%s/%s\" width=100 height=100></img></a> <p align= \"left\"> %d, %s, %s, %d bytes, %s, %i, %s</p>\n",
            info->currentDir,
            info->filename,
            info->currentDir,
            info->filename,
            info->inode,
            info->filename,
            info->filetype,
            (int)info->size,
            info->date,
            (int)info->thread_id,
            info->path);
    fflush(htmlfile);
    pthread_mutex_unlock(&mut);
    return;
}

void File(char * entry,struct thread_info * info)
{
    char filePath[MAX_DIR_PATH];
    char * fExt;
    struct stat st;

    printf("F");

    char * p = strpbrk(entry,".");

    if (p != NULL)
    {
        fExt =  ++p;

        sprintf(filePath,"%s/%s",info->Dir,entry);
        stat(filePath,&st);

        struct f_infos * fi = malloc(sizeof(struct f_infos));
        memset (fi, 0x00, sizeof(struct f_infos));
        fi->filename = malloc(sizeof(char) * MAX_DIR_PATH);
        memset (fi->filename, 0x00, sizeof(sizeof(char) * MAX_DIR_PATH));
        fi->path = malloc(sizeof(char) * MAX_DIR_PATH);
        memset (fi->path, 0x00, sizeof(sizeof(char) * MAX_DIR_PATH));
        fi->filetype = malloc(sizeof(char) * MAX_DIR_PATH);
        memset (fi->filetype, 0x00, sizeof(sizeof(char) * MAX_DIR_PATH));
        fi->currentDir = malloc(sizeof(char) * MAX_DIR_PATH);
        memset (fi->currentDir, 0x00, sizeof(sizeof(char) * MAX_DIR_PATH));
        fi->date = malloc(sizeof(char) * MAX_DIR_PATH);
        memset (fi->date, 0x00, sizeof(sizeof(char) * MAX_DIR_PATH));

        fi->thread_id = pthread_self();
        strftime(fi->date, 20, "%d-%m-%y %H:%M:%S", localtime((const time_t *)&st.st_mtim));
        fi->inode = st.st_ino;
        fi->size = st.st_size;
        fi->filename = entry;
        fi->currentDir = info->Dir;
        fi->path = (char *)&filePath;


        strcpy(fi->filetype, fExt);

        if (strcmp(fExt,"gif") ==  0)
        {
            incrementFileCount(1);
            writeHtml(fi);
        }

        if (strcmp(fExt,"bmp") ==  0)
        {
            incrementFileCount(2);
            writeHtml(fi);
        }

        if (strcmp(fExt,"jpg") ==  0)
        {
            incrementFileCount(3);
            writeHtml(fi);
        }

        if (strcmp(fExt,"png") ==  0)
        {
            incrementFileCount(4);
            writeHtml(fi);
        }
    }
    return;
}


static void * doWorkBMP(void *arg)
{
    struct thread_info * threadParameters = arg;

    int k = 0;

    if(threadParameters->currFileCount == 0)
    {
        pthread_exit(EXIT_SUCCESS);
    }

    for(k = 0; k < threadParameters->currFileCount; k++)
    {
        File(basename(threadParameters->currFile[k].d_name),threadParameters);
    }
    pthread_exit(EXIT_SUCCESS);
}


static void * doWorkJPG(void *arg)
{
    struct thread_info * threadParameters = arg;

    int k = 0;

    if(threadParameters->currFileCount == 0)
    {
        pthread_exit(EXIT_SUCCESS);
    }

    for(k = 0; k < threadParameters->currFileCount; k++)
    {
        File(basename(threadParameters->currFile[k].d_name),threadParameters);
    }
    pthread_exit(EXIT_SUCCESS);
}

static void * doWorkPNG(void *arg)
{
    struct thread_info * threadParameters = arg;

    int k = 0;

    if(threadParameters->currFileCount == 0)
    {
        pthread_exit(EXIT_SUCCESS);
    }

    for(k = 0; k < threadParameters->currFileCount; k++)
    {
        File(basename(threadParameters->currFile[k].d_name),threadParameters);
    }

    pthread_exit(EXIT_SUCCESS);
}

static void * doWorkGIF(void *arg)
{
    struct thread_info * threadParameters = arg;

    int k = 0;

    if(threadParameters->currFileCount == 0)
    {
        pthread_exit(EXIT_SUCCESS);
    }

    for(k = 0; k < threadParameters->currFileCount; k++)
    {
        File(basename(threadParameters->currFile[k].d_name),threadParameters);
    }
    pthread_exit(EXIT_SUCCESS);
}


static int one (const struct dirent *unused)
{
    return 1;
}

static void * doWorkV1(void *arg)
{
    printf("W");



    struct thread_info * threadParameters = arg;
    struct dirent **eps;
    char * fExt;
    int numFiles;

    struct dirent * BMPs = calloc(100,sizeof(struct dirent *));
    int BMPindex = 0;
    struct dirent * JPGs = calloc(100,sizeof(struct dirent *));
    int JPGindex = 0;
    struct dirent * GIFs = calloc(100,sizeof(struct dirent *));
    int GIFindex = 0;
    struct dirent * PNGs = calloc(100,sizeof(struct dirent *));
    int PNGindex = 0;

    switch(mode)
    {

    case 1:
        numFiles = scandir (threadParameters->Dir, &eps, one, alphasort);
        if (numFiles >= 0)
        {
            int cnt;
            for (cnt = 0; cnt < numFiles; ++cnt)
            {
                File(basename(eps[cnt]->d_name),threadParameters);
            }
        }
        else
            perror ("Couldn't open the directory");
        break;

    case 2:
        numFiles = scandir (threadParameters->Dir, &eps, one, alphasort);

        if (numFiles >= 0)
        {
            int cnt;
            for (cnt = 0; cnt < numFiles; ++cnt)
            {
                char * p = strpbrk(basename(eps[cnt]->d_name),".");

                if (p != NULL)
                {
                    fExt =  ++p;
                    if (strcmp(fExt, "gif") == 0 || strcmp(fExt, "jpg") == 0 || strcmp(fExt, "bmp") == 0 || strcmp(fExt, "png") == 0)
                    {
                        if(strcmp(fExt, "gif") == 0)
                        {
                            GIFs[GIFindex] = *eps[cnt];
                            GIFindex++;
                        }
                        else if(strcmp(fExt, "jpg") == 0)
                        {
                            JPGs[JPGindex] = *eps[cnt];
                            JPGindex++;
                        }
                        else if(strcmp(fExt, "bmp") == 0)
                        {
                            BMPs[BMPindex] = *eps[cnt];
                            BMPindex++;
                        }
                        else if(strcmp(fExt, "png") == 0)
                        {
                            PNGs[PNGindex] = *eps[cnt];
                            PNGindex++;
                        }
                    }
                }
            }

            if(GIFindex > 0)
            {
                pthread_mutex_lock(&threadIncrementer);

                pthread_attr_t * attr = (pthread_attr_t  *) malloc(sizeof(pthread_attr_t));
                pthread_attr_init(attr);
                threads[INDEX_THREAD].currFile = GIFs;
                threads[INDEX_THREAD].currFileCount = GIFindex;

                threads[INDEX_THREAD].Dir = malloc(sizeof(char) * MAX_DIR_PATH);
                memset (threads[INDEX_THREAD].Dir, 0x00, sizeof(sizeof(char) * MAX_DIR_PATH));
                sprintf(threads[INDEX_THREAD].Dir,"%s",threadParameters->Dir);

                pthread_create(&threads[INDEX_THREAD].thread_id, attr, &doWorkGIF, &threads[INDEX_THREAD]);

                INDEX_THREAD++;

                pthread_mutex_unlock(&threadIncrementer);
            }

            if(JPGindex > 0)
            {
                pthread_mutex_lock(&threadIncrementer);

                pthread_attr_t * attr = (pthread_attr_t  *) malloc(sizeof(pthread_attr_t));
                pthread_attr_init(attr);
                threads[INDEX_THREAD].currFile = JPGs;
                threads[INDEX_THREAD].currFileCount = JPGindex;

                threads[INDEX_THREAD].Dir = malloc(sizeof(char) * MAX_DIR_PATH);
                memset (threads[INDEX_THREAD].Dir, 0x00, sizeof(sizeof(char) * MAX_DIR_PATH));
                sprintf(threads[INDEX_THREAD].Dir,"%s",threadParameters->Dir);

                pthread_create(&threads[INDEX_THREAD].thread_id, attr, &doWorkJPG, &threads[INDEX_THREAD]);

                INDEX_THREAD++;

                pthread_mutex_unlock(&threadIncrementer);
            }

            if(BMPindex > 0)
            {
                pthread_mutex_lock(&threadIncrementer);

                pthread_attr_t * attr = (pthread_attr_t  *) malloc(sizeof(pthread_attr_t));
                pthread_attr_init(attr);
                threads[INDEX_THREAD].currFile = BMPs;
                threads[INDEX_THREAD].currFileCount = BMPindex;

                threads[INDEX_THREAD].Dir = malloc(sizeof(char) * MAX_DIR_PATH);
                memset (threads[INDEX_THREAD].Dir, 0x00, sizeof(sizeof(char) * MAX_DIR_PATH));
                sprintf(threads[INDEX_THREAD].Dir,"%s",threadParameters->Dir);

                pthread_create(&threads[INDEX_THREAD].thread_id, attr, &doWorkBMP, &threads[INDEX_THREAD]);

                INDEX_THREAD++;

                pthread_mutex_unlock(&threadIncrementer);
            }

            if(PNGindex > 0)
            {
                pthread_mutex_lock(&threadIncrementer);

                pthread_attr_t * attr = (pthread_attr_t  *) malloc(sizeof(pthread_attr_t));
                pthread_attr_init(attr);
                threads[INDEX_THREAD].currFile = PNGs;
                threads[INDEX_THREAD].currFileCount = PNGindex;

                threads[INDEX_THREAD].Dir = malloc(sizeof(char) * MAX_DIR_PATH);
                memset (threads[INDEX_THREAD].Dir, 0x00, sizeof(sizeof(char) * MAX_DIR_PATH));
                sprintf(threads[INDEX_THREAD].Dir,"%s",threadParameters->Dir);

                pthread_create(&threads[INDEX_THREAD].thread_id, attr, &doWorkPNG, &threads[INDEX_THREAD]);

                INDEX_THREAD++;

                pthread_mutex_unlock(&threadIncrementer);
            }

        }
        else
        {
            perror ("Couldn't open the directory");
        }

        break;

    default:
        break;
    }

    pthread_exit(EXIT_SUCCESS);
}

void  DirItemV1(struct stat * info,char * currDir)
{
    printf("D");
    incrementDirCount();

    pthread_mutex_lock(&threadIncrementer);
    pthread_attr_t * attr = (pthread_attr_t  *) malloc(sizeof(pthread_attr_t));
    pthread_attr_init(attr);
    threads[INDEX_THREAD].Dir = malloc(sizeof(char) * MAX_DIR_PATH);
    memset (threads[INDEX_THREAD].Dir, 0x00, sizeof(sizeof(char) * MAX_DIR_PATH));
    sprintf(threads[INDEX_THREAD].Dir,"%s",currDir);
    threads[INDEX_THREAD].info = info;
    pthread_create(&threads[INDEX_THREAD].thread_id, attr, &doWorkV1,&threads[INDEX_THREAD]);
    INDEX_THREAD++;
    pthread_mutex_unlock(&threadIncrementer);
    return;
}

int DirV1 (char * path, struct stat * info, int type)
{
    switch(type)
    {
    case FTW_F:
        break;
    case FTW_D:
        DirItemV1(info,path);
        break;
    default:
        break;
    }
    return 0;
}

void xSleep(unsigned int mseconds)
{
    clock_t goal = mseconds + clock();
    while (goal > clock());
}


int main(int argc, char * argv[])
{
    printf("Begin\n");

    struct timeval stop, start;
    gettimeofday(&start, NULL);

    //Takes care of initial time
    char cBuffer[100];
    time_t zaman;
    struct tm *ltime;
    static struct timeval _t;
    static struct timezone tz;
    time(&zaman);
    ltime = (struct tm *) localtime(&zaman);
    gettimeofday(&_t, &tz);
    strftime(cBuffer,40,"%d.%m.%y %H:%M:%S",ltime);
    sprintf(cBuffer, "%s.%d", cBuffer,(int)_t.tv_usec);
    //Ends setting of the initial time

    //Checks to make sure command line arguements are in order
    if(argc > 4 || argc <=1)
    {
        perror("Incorrect number of args");
        return -1;
    }
    else if(strcmp(argv[1], "v1") == 0)
    {
        mode = 1;
    }
    else if(strcmp(argv[1], "v2") == 0)
    {
        mode = 2;
    }
    //Ends comand line check

    //
    inputdirectory = argv[2];
    outputdirectory = argv[3];
    char * originalOutputDirectory = malloc(1024);
    originalOutputDirectory = strcpy(originalOutputDirectory, outputdirectory);
    //

    DIR * dirp;

    if ((dirp = opendir(outputdirectory)) == NULL)
    {
        mkdir(outputdirectory, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }

    /*
        code: iinitialize the thread table
    */
    threads = calloc(MAX_THREAD, sizeof(struct thread_info));
    memset (threads, 0x00, sizeof(struct thread_info));
    /*
        code: initialize html file and prime it
    */

    if(mode == 1)
    {
        char * outputLogFile = malloc(1024);
        outputLogFile = strcat(outputdirectory, "/log_v1.txt");
        logFile  = fopen(outputLogFile, "w+");
    }
    else if(mode == 2)
    {
        char * outputLogFile = malloc(1024);
        outputLogFile = strcat(outputdirectory, "/log_v2.txt");
        logFile  = fopen(outputLogFile, "w+");
    }

    char * outputHTML = strcat(originalOutputDirectory,"/html_pics.html");
    htmlfile = fopen(outputHTML, "w+");
    fprintf(htmlfile, "<html><head><title>Images</title></head><body>");
    fprintf(logFile, "Log for variant %i\n\n", mode);
    fprintf(logFile, "------------------------------------------------\n");

    /*
        code: initialize and start the signal timer
    */
    timer_start();

    /*
        code: start the directory traversal engine
    */
    printf("*");
    ftw(inputdirectory, (__ftw_func_t)DirV1, 0);
    printf("*");
    /*
        code: wait for the worker crew to be done with their work
    */
    void * res;
    int i = 0;

    xSleep(15000);

    printf("%i",INDEX_THREAD);
    printf("@");
    for (i=0; i < INDEX_THREAD; i++)
    {
        pthread_join(threads[i].thread_id, &res);
        printf(".");
    }

    /*
        code: finalize the html file and close it
    */
    fprintf(htmlfile, "</body></html>");
    fclose(htmlfile);
    printf("\nEnd\n");

    timerFlag = 1;

    xSleep(10000);

    fprintf(logFile,"\nProgramme initiation: %s\n\n", cBuffer);
    fprintf(logFile,"Number of jpg files = %i\n", jpgCount);
    fprintf(logFile,"Number of bmp files = %i\n", bmpCount);
    fprintf(logFile,"Number of png files = %i\n", pngCount);
    fprintf(logFile,"Number of gif files = %i\n\n", gifCount);
    fprintf(logFile,"Total number of valid images files = %i\n\n", fileCount);
    gettimeofday(&stop, NULL);
    fprintf(logFile,"Total time of execution = %lu ms\n", stop.tv_usec - start.tv_usec);
    fprintf(logFile,"--------------------------------------\n");
    fprintf(logFile,"Number of threads created = %i\n", INDEX_THREAD);

    fclose(logFile);

    printf("\nThis is the amount of files we processed: %i\n", fileCount);
    printf("\nThis is the amount of directories we processed: %i\n", dirCount);
    printf("This is the number of jpg pictures: %i\n", jpgCount);
    printf("This is the number of png pictures: %i\n", pngCount);
    printf("This is the number of gif pictures: %i\n", gifCount);
    printf("This is the number of bmp pictures: %i\n", bmpCount);

    exit(EXIT_SUCCESS);
}
