#define _GNU_SOURCE
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <netinet/ip.h>
#include <unistd.h>    //write
#include "md5sum.h"
#include <sys/stat.h>
#include <ftw.h>
#include <stdbool.h>
#include <sys/types.h>
#include "../A5includes/a5header.h"

// LAST VERSION

char catalog[1024];
char filename[1024];
int receive_size = 0;

char * images_dir = "./images";

char * IP;

int port;

FILE * htmlfile;

char * fileNames[1024];

int transferCatalog()
{
    int sock;
    struct sockaddr_in server;
    //Create socket
    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1)
    {
        printf("Could not create socket");
    }

    server.sin_addr.s_addr = inet_addr(IP);
    server.sin_family = AF_INET;
    server.sin_port = htons( port );

    struct linger so_linger;
    so_linger.l_onoff = 1;
    so_linger.l_linger = 30;
    setsockopt(sock,SOL_SOCKET,SO_LINGER,&so_linger,sizeof so_linger);

    int optval;
    socklen_t optlen = sizeof(optval);
    getsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen);
    optval = 1;
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen);

    int sendbuff = 65535;
    optlen = sizeof(sendbuff);
    getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendbuff, &optlen);
    getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &sendbuff, &optlen);

    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }

// ****************************************************************** //
// *  CATALOG RECEIVE logic START                                   * //
// ****************************************************************** //
    descriptorBlock blk;
    memset(&blk,0x00,sizeof(descriptorBlock));
    blk.action = 0;
    //printf("before \n");
    int w = write(sock , (void *)&blk , sizeof(descriptorBlock));
    //printf("after write %i\n",w);
// ****************************************************************** //
// *   Receive message descriptor block                             * //
// ****************************************************************** //
    memset(&blk,0x00,sizeof(descriptorBlock));

    tryReceive(sock,sizeof(descriptorBlock));
    w = recv(sock , &blk, sizeof(descriptorBlock),0);

    char * buffer = malloc(sizeof(unsigned char)*blk.dataLength);
    memset(buffer, 0x00, sizeof(unsigned char)*blk.dataLength);
    strcpy(catalog,blk.filename);

// ****************************************************************** //
// *   Receive data payload linked to the message descriptor block  * //
// ****************************************************************** //
    tryReceive(sock,blk.dataLength);
    receive_size = 0;
    do
    {
        // ************************************************************************** //
        // *  Note: move the buffer base address to match the received data offset  * //
        // ************************************************************************** //
        receive_size += recv(sock , &buffer[receive_size] , 1 , 0);
    }
    while (receive_size < blk.dataLength);

    if( receive_size == blk.dataLength)
    {
// ****************************************************************** //
// *    Catalog data has been received save it to file system       * //
// ****************************************************************** //
        int cat = open(blk.filename ,O_CREAT | O_TRUNC | O_WRONLY,S_IRUSR);
        write(cat,buffer,blk.dataLength);
        close(cat);
        free(buffer);
    }
// ****************************************************************** //
// *  CATALOG RECEIVE logic END                                     * //
// ****************************************************************** //
    close(sock);
    return 0;
}

char * lineptr = NULL;

char * getCatalogItem(int e)
{
    size_t length = 0;
    int itemIndex = 0;
    lineptr = NULL;

    FILE * c = fopen(catalog,"r");

    while(1)
    {
        int r = getline(&lineptr, &length, c);

        if (r <= 0)
            break;

        if (itemIndex == 0)
        {
            itemIndex++;
            continue;
        }

        if(itemIndex == e)
        {
            fclose(c);
            return lineptr;
        }

        itemIndex++;
    }

    fclose(c);
    return NULL;
}


char * transferFile(char * fn)
{
    int sock;
    struct sockaddr_in server;
    char path[1024];

    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1)
    {
        printf("Could not create socket");
    }

    server.sin_addr.s_addr = inet_addr(IP);
    server.sin_family = AF_INET;
    server.sin_port = htons( port );

    struct linger so_linger;
    so_linger.l_onoff = 1;
    so_linger.l_linger = 30;
    setsockopt(sock,SOL_SOCKET,SO_LINGER,&so_linger,sizeof so_linger);

    int optval;
    socklen_t optlen = sizeof(optval);
    getsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen);
    optval = 1;
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen);

    int priority = 6;
    int iptos = IPTOS_CLASS_CS6;
    setsockopt(sock, SOL_SOCKET, SO_PRIORITY, &priority, sizeof(priority));

    int sendbuff = 65535;
    optlen = sizeof(sendbuff);
    getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendbuff, &optlen);
    getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &sendbuff, &optlen);

    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return NULL;
    }

// ****************************************************************** //
// *  CATALOG RECEIVE logic START                                   * //
// ****************************************************************** //
    descriptorBlock blk;
    memset(&blk,0x00,sizeof(descriptorBlock));
    blk.action = 1;
    strcpy(blk.filename,fn);
    write(sock , (void *)&blk , sizeof(descriptorBlock));

// ****************************************************************** //
// *   Receive message descriptor block                             * //
// ****************************************************************** //
    memset(&blk,0x00,sizeof(descriptorBlock));
    tryReceive(sock,sizeof(descriptorBlock));
    recv(sock , &blk, sizeof(descriptorBlock) , 0);
    char * buffer = malloc(sizeof(unsigned char)*blk.dataLength);
    memset(buffer, 0x00, sizeof(unsigned char)*blk.dataLength);
    memset(filename, 0x00, 1024);
    strcpy(filename,blk.filename);

// ****************************************************************** //
// *   Receive data payload linked to the message descriptor block  * //
// ****************************************************************** //
    tryReceive(sock,blk.dataLength);
    receive_size = 0;
    do
    {
        // ************************************************************************** //
        // *  Note: move the buffer base address to match the received data offset  * //
        // ************************************************************************** //
        receive_size += recv(sock , &buffer[receive_size] , 1, 0);
    }
    while (receive_size < blk.dataLength);

    if ( receive_size == blk.dataLength)
    {
// ****************************************************************** //
// *    Catalog data has been received save it to file system       * //
// ****************************************************************** //
        memset(&path,0x00,1024);
        strcat(path,images_dir);
        strcat(path,"/");
        strcat(path,blk.filename);
        int f = open(path ,O_CREAT | O_TRUNC | O_WRONLY,S_IRUSR);
        write(f,buffer,blk.dataLength);
        close(f);
        free(buffer);
    }
// ****************************************************************** //
// *  CATALOG RECEIVE logic END                                     * //
// ****************************************************************** //
    return path;
}

main(int argc, char *argv[])
{
    bool passive, interactive;
    char * pchar;
    char extension[1024];
    int maxFiles = 0;

    struct stat st = {0};

    if (stat(images_dir, &st) == -1)
    {
        mkdir(images_dir, 0700);
    }
    htmlfile = fopen("./images/download.html", "w+");
    fprintf(htmlfile, "<html><head><title>Images</title></head><body><h1>Downloaded Images</h1><pre>");
    if(argc < 3)
    {
        perror("Not enough arguements");
        return 1;
    }

    if(argc == 4)
    {
        passive = true;
        sprintf(extension, "%s", argv[3]);

    }
    else
    {
        interactive = true;
    }

    IP = argv[1];
    pchar = argv[2];
    port = atoi(argv[2]);

    transferCatalog();

// ****************************************************************** //
// *  CONSOLE GUI Dialog logic START                                * //
// ****************************************************************** //
    printf("===============================\n");

    char * lineptr = NULL;
    size_t length = 0;
    int itemIndex = 0;

    FILE * c = fopen(catalog,"r");

    printf("Dumping contents of %s\n", catalog);

    while(1)
    {
        int r = getline(&lineptr, &length, c);
        if (r <= 0)
            break;

        if (itemIndex == 0)
        {
            itemIndex++;
            continue;
        }

        int filesize;
        char filename[1024], md5hash[1024];

        const char delimiters[] = ",";
        char *running;
        char *token;

        running = strdupa (lineptr);
        token = strsep (&running, delimiters);    /* token => "words" */
        strcpy(filename, token);
        token = strsep (&running, delimiters);    /* token => "separated" */
        filesize = atoi(token);
        token = strsep (&running, delimiters);    /* token => "by" */
        strcpy(md5hash, token);
        printf("[%i] %s\n",itemIndex, filename);
        fileNames[itemIndex] = malloc(1024);
        memset(fileNames[itemIndex], 0x00, 1024);
        strcpy(fileNames[itemIndex], filename);
        itemIndex++;
    }

    maxFiles = itemIndex-1;

    fclose(c);
    free(lineptr);

    printf("===============================\n");

    if(passive)
    {
        printf("Running in passive mode. Downloading %s files.\n", extension);
        length = 0;
        itemIndex = 0;
        lineptr = NULL;

        c = fopen(catalog,"r");

        while(1)
        {
            int filesize;
            char md5hash[1024];
            char filename[1024];

            const char delimiters[] = ",";
            char *running;
            char *token;

            int r = getline(&lineptr, &length, c);

            if (r <= 0)
                break;

            if (itemIndex == 0)
            {
                itemIndex++;
                continue;
            }

            memset(md5hash,0x00,1024);
            running = strdupa (lineptr);
            token = strsep (&running, delimiters);    /* token => "words" */
            strcpy(filename, token);
            token = strsep (&running, delimiters);    /* token => "separated" */
            filesize = atoi(token);
            token = strsep (&running, delimiters);    /* token => "by" */
            strcpy(md5hash, token);

            if(interactive || (passive && strcmp(extension, strrchr(filename , '.'))==0))
            {
                printf("Downloading %s\n", filename);
                char * clientMd5 = buildMD5(transferFile(filename));
                char * tag = malloc(1024);
                sprintf(tag, "<a href=\"%s\">%s</a><br />", filename, filename);
                fprintf(htmlfile, tag);

                switch(strncmp(clientMd5, md5hash,strlen(clientMd5)))
                {
                case 0:
                    fprintf(htmlfile, "<p>Checksum match! </p>");
                    break;
                default:
                    fprintf(htmlfile, "<p>Checksum mismatch! </p>");
                    printf("%s checkSum not valid\n", filename);
                    break;
                }
                free(tag);
            }
        }
        fclose(c);
    }

    if(interactive)
    {
        while(1)
        {
            char string [256];
            printf("Enter ID to download (0 to quit): ");
            gets(string);
            int key = atoi(string);

            if (key==0)
                break;

            if (key>maxFiles)
                continue;

            if (key < 0)
                continue;

            int filesize;
            char md5hash[1024];
            char filename[1024];

            const char delimiters[] = ",";
            char *running;
            char *token;

            lineptr = getCatalogItem(key);

            memset(md5hash,0x00,1024);
            running = strdupa (lineptr);
            token = strsep (&running, delimiters);    /* token => "words" */
            strcpy(filename, token);
            token = strsep (&running, delimiters);    /* token => "separated" */
            filesize = atoi(token);
            token = strsep (&running, delimiters);    /* token => "by" */
            strcpy(md5hash, token);

            printf("Downloading %s\n", filename);

            char * clientMd5 = buildMD5(transferFile(filename));

            char * tag = malloc(1024);
            sprintf(tag, "<a href=\"%s\">%s</a><br />", filename, filename);
            fprintf(htmlfile, tag);

            switch(strncmp(clientMd5, md5hash,strlen(clientMd5)))
            {
            case 0:
                fprintf(htmlfile, "<p>Checksum match! </p>");
                break;
            default:
                fprintf(htmlfile, "<p>Checksum mismatch! </p>");
                printf("%s checkSum not valid\n", filename);
                break;
            }
            free(tag);
        }
    }


    printf("Check sum computation finished\n");
    printf("Rendering html file\n");
    printf("Computation completed\n");
    fprintf(htmlfile,"</pre></body></html>");
    fclose(htmlfile);
    printf("===============================\n");

    return 0;
}
