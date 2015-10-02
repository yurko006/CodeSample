#define _GNU_SOURCE
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <netinet/ip.h>
#include "md5sum.h"
#include <sys/stat.h>
#include <ftw.h>
#include <stdbool.h>
#include <sys/types.h>
#include "../A5includes/a5header.h"

// LAST VERSION

FILE * catalog;

descriptorBlock fileTable[1024];

int fileTableIndex = 0;

// ****************************************************************** //
// *                                                                * //
// ****************************************************************** //
int DirDrill (char * path, struct stat * info, int type)
{
    char * s;
    switch(type)
    {
    case FTW_F:

        //printf("This is the filename %s\n: " , path);
        //printf("This is the extension %s\n: " ,strrchr(path , '.') );
        if((s = strrchr(path , '.')) != NULL)
        {

            if(strcmp(s , ".png") == 0 || strcmp(s , ".PNG") == 0 ||
                    strcmp(s , ".gif") == 0 || strcmp(s , ".GIF") == 0 ||
                    strcmp(s , ".tiff") == 0 || strcmp(s , ".TIFF") == 0)
            {
                printf("We are processing: %s\n", path);
                strcpy(fileTable[fileTableIndex].path, path);
                strcpy(fileTable[fileTableIndex].filename, basename(path));
                strcpy(fileTable[fileTableIndex].md5, buildMD5(path));
                fileTable[fileTableIndex].dataLength = info->st_size;
                fprintf(catalog, "%s,%i,%s\n",
                        fileTable[fileTableIndex].filename,
                        (int)info->st_size,
                        fileTable[fileTableIndex].md5);
                fileTableIndex++;
            }
        }
        break;
    case FTW_D:
        break;
    default:
        break;
    }
    return 0;
}

// ****************************************************************** //
// *                                                                * //
// ****************************************************************** //
descriptorBlock * buildMessageBlock(char * filename)
{
    printf("Enter buildMessageBlock\n");
    struct stat st;
    descriptorBlock  * blk = malloc(sizeof(descriptorBlock));
    memset(blk, 0x00, sizeof(descriptorBlock));
    stat(filename,&st);
    strcpy(blk->path, filename);
    strcpy(blk->filename, basename(filename));
    strcpy(blk->md5, buildMD5(filename));
    blk->dataLength = st.st_size;
    printf("Leave sendCatalogToClient\n");
    return blk;
}

// ****************************************************************** //
// *                                                                * //
// ****************************************************************** //
int  sendCatalogToClient(int client_sock)
{
    printf("Enter sendCatalogToClient\n");

    int read_size = 0;
    int write_size = 0;

    char buffer[1024];
    memset(buffer,0x00,1024);

    descriptorBlock  * blk = buildMessageBlock("./catalog.csv");
    write(client_sock , (void *)blk , sizeof(descriptorBlock));
    int cat = open(blk->path, O_RDONLY);
    read_size = read(cat,  (void *)&buffer, 1);
    while(write_size < blk->dataLength)
    {
        write_size += write(client_sock , (void *)&buffer , read_size);
        memset(buffer,0x00,1024);
        read_size = read(cat,  (void *)&buffer, 1);
    }
    close(cat);
    free(blk);

    printf("Leave sendCatalogToClient\n");
    return(read_size);
}

int  sendFileToClient(int client_sock, char * filename)
{
    printf("Enter sendFileToClient\n");

    int read_size = 0;
    int write_size = 0;

    char buffer[1024];
    memset(buffer,0x00,1024);

    int i = 0;

    while(strcmp(filename, fileTable[i++].filename) != 0);
    printf("----->%s  %s\n",filename,fileTable[i-1].path);

    descriptorBlock  * blk = buildMessageBlock(fileTable[i-1].path);
    write(client_sock , (void *)blk , sizeof(descriptorBlock));

    int file = open(blk->path, O_RDONLY);
    read_size = read(file,  (void *)&buffer, 1);
    while(write_size < blk->dataLength)
    {
        write_size += write(client_sock , (void *)&buffer , read_size);
        memset(buffer,0x00,1024);
        read_size = read(file,  (void *)&buffer, 1);
    }
    close(file);
    free(blk);

    printf("Leave sendFileToClient\n");
    return(read_size);
}

// ****************************************************************** //
// *                                                                * //
// ****************************************************************** //
void  buildCatalogCsv(char * dir)
{
    printf("Enter buildCatalogCsv\n");
    catalog = fopen("./catalog.csv","w+");
    fprintf(catalog, "filename, size, checksum\n");
    ftw(dir, (__ftw_func_t)DirDrill, 0);
    fclose(catalog);
    printf("Leave buildCatalogCsv\n");
    return;
}

// ****************************************************************** //
// *                                                                * //
// ****************************************************************** //
main(int argc, char *argv[])
{
    int portNumber;
    int socket_desc , client_sock , c , receive_size, read_size = 0;
    descriptorBlock * buffer;
    struct sockaddr_in server , client;

    memset(fileTable, 1024, sizeof(descriptorBlock));

    if(argc != 3)
    {
        perror("Invalid amount of arguements");
        return 1;
    }

    portNumber = atoi(argv[1]);

    buildCatalogCsv(argv[2]);

    //Create socket
    socket_desc = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }

    puts("Socket created");

    struct linger so_linger;
    so_linger.l_onoff = 1;
    so_linger.l_linger = 50;
    setsockopt(socket_desc,SOL_SOCKET,SO_LINGER,&so_linger,sizeof so_linger);

    int optval;
    socklen_t optlen = sizeof(optval);
    getsockopt(socket_desc, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen);
    optval = 1;
    setsockopt(socket_desc, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen);

    int priority = 5;
    int iptos = IPTOS_CLASS_CS6;
    setsockopt(socket_desc, SOL_SOCKET, SO_PRIORITY, &priority, sizeof(priority));

    int sendbuff = 65535;
    optlen = sizeof(sendbuff);
    getsockopt(socket_desc, SOL_SOCKET, SO_SNDBUF, &sendbuff, &optlen);
    getsockopt(socket_desc, SOL_SOCKET, SO_RCVBUF, &sendbuff, &optlen);

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons( portNumber );


    while(1)
    {
        if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
        {
            //print the error message
            perror("bind failed. Error");
            sleep(2);
        }
        else break;
    }

    puts("bind done");

    while (1)
    {
        //Listen
        listen(socket_desc , 3);

        //Accept and incoming connection
        puts("Waiting for incoming connections...");
        c = sizeof(struct sockaddr_in);

        //accept connection from an incoming client
        client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
        if (client_sock < 0)
        {
            perror("accept failed");
            return 1;
        }

        puts("Connection accepted");

        receive_size = 0;
        int tmp_receive = 0;
        buffer = malloc(sizeof(descriptorBlock));
        memset(buffer, 0x00, sizeof(descriptorBlock));

        tryReceive(client_sock,sizeof(descriptorBlock));

        tmp_receive = 0;
        do
        {
            // ************************************************************************** //
            // *  Note: move the buffer base address to match the received data offset  * //
            // ************************************************************************** //
            tmp_receive = recv(client_sock , &buffer[receive_size] , sizeof(descriptorBlock), 0);
            if (tmp_receive == -1)
                exit(-1);
            receive_size = receive_size + tmp_receive;
        }
        while (receive_size < sizeof(descriptorBlock));

        puts("Descriptor accepted");

        switch(buffer->action)
        {
        case 0:
            printf("Catalog Request has been received\n");
            read_size = sendCatalogToClient(client_sock);
            break;
        case 1:
            printf("File Request has been received\n");
            read_size = sendFileToClient(client_sock, buffer->filename);
            break;
        default:
            // ****************************************************************** //
            // *  Send file catalog.csv to the connected client                 * //
            // ****************************************************************** //
            printf("unkonwn request received\n");
            break;
        }

        if(read_size == 0)
        {
            puts("Client disconnected");
            fflush(stdout);
        }
        else if(read_size == -1)
        {
            perror("recv failed");
        }
    }

    return 0;
}


