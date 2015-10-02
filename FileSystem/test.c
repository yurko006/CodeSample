/*
CSci 4061 Spring 2015 Assignment 3
Name1=Nikita Yurkov
Name2=Colin Heim
StudentID1=4411081
StudentID2=4629776
Commentary=This program uses a superblock, directory structure, inode list, and disk blocks that reads in a file or files, and writes them to the mini_filesystem so they can be placed in the proper spot. This happens by mapping the files to their proper inode number and eventually storing the file data in the disk blocks, each block being 512 bytes. 

*/




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
#include "mini_filesystem.h"
#include <limits.h>

int main(int argc, char * argv[])
{
    DIR * id;
    struct dirent *direntp;
    int file;
    char filename[1024];
    char shortFilename[1024];
    FILE * htmlfile;
    struct stat st = {0};
    char output_dir[1000];


    char * input_dir = argv[1];
    //char * output_dir = argv[2];
    strcpy(output_dir , argv[2]);
    char * logfilename = argv[3];

    
        if(mkdir(argv[2], 0700) == 0)  //Makes output directory if needed.
	{
		printf("Output directory created.");
	}


    char htmlname[1000];
    memset(htmlname, 0x00, 1000*sizeof(char));
    strcat(htmlname, "./");
    strcat(htmlname, output_dir);
    strcat(htmlname, "/html_pics.html");

    htmlfile = fopen(htmlname, "w+");
    fprintf(htmlfile, "<html><head><title>Images</title></head><body>");

    int error = Initialize_Filesystem(logfilename);
    if(error == -1)
    {
        perror ("Failed to Initialize_Filesystem");
        return -1;
    }

    char inputname[100];
    memset(inputname, 0x00, 100*sizeof(char));
    strcat(inputname, "./");
    strcat(inputname, input_dir);


    if((id = opendir(inputname)) == NULL)
    {
        perror ("Failed to open input directory");
        return -1;
    }

   
  
    strcat(inputname, "/\0");

    while ((direntp = readdir(id)) != NULL)
    {
        if(direntp->d_type == DT_REG)
        {
            memset(filename, 0x00, 1024*sizeof(char));
            strcat(filename, inputname);
            strcat(filename, direntp->d_name);

            memset(shortFilename, 0x00, 1024*sizeof(char));
            strcat(shortFilename, direntp->d_name);

            char buffer[512];
            memset(buffer, 0x00, 512*sizeof(char));

            ssize_t ret_in;
            int offset = 0;

            file = open(filename, O_RDONLY);

            Create_File(shortFilename);
            Open_File(shortFilename);
            printf("Start processing File:%s  For INODE=%i\n",filename,Superblock.next_free_inode-1);
            while((ret_in = read(file, &buffer, (ssize_t)512)) > 0)
            {
                offset = offset + ret_in;
                printf("%s   bytes read=%i\n",filename,offset);
                Write_File(Superblock.next_free_inode,offset, buffer);

                memset(buffer, 0x00, 512*sizeof(char));
            }
            printf("End processing File:%s\n",filename);
            Close_file(Superblock.next_free_inode);
            close(file);
        }
    }


    close(id);
    char inputname2[100];
    memset(inputname2, 0x00, 100*sizeof(char));
    strcat(inputname2, "./");
    strcat(inputname2, input_dir);

    if((id = opendir(inputname2)) == NULL)
    {
        perror ("Failed to open input directory");
        return -1;
    }


    char output[100];
    memset(output, 0x00, 100*sizeof(char));
    strcat(output, "./");
    strcat(output, output_dir);
    strcat(output, "/\0");


    

    while ((direntp = readdir(id)) != NULL)
    {
        if(direntp->d_type == DT_REG)
        {
            memset(filename, 0x00, 1024*sizeof(char));
            strcat(filename, output);
            strcat(filename, direntp->d_name);
            fprintf(htmlfile,"<a href='%s'><img src='%s' height=200 width = 200/></a>\n", direntp->d_name, direntp->d_name);

            memset(shortFilename, 0x00, 1024*sizeof(char));
            strcat(shortFilename, direntp->d_name);

            int offset = 0;
            int bytesread = 0;

            char buffer[1024];
            memset(buffer, 0x00, 1024*sizeof(char));

            int f1 = open(filename, O_CREAT | O_WRONLY,S_IRUSR);
            int inumber = Open_File(shortFilename);

            bytesread = Read_File(inumber,offset,512, buffer);
            write(f1, buffer, bytesread);
            while(bytesread > 0)
            {
                offset = offset + bytesread;
                bytesread = Read_File(inumber,offset,512, buffer);
                write(f1, buffer, bytesread);
                memset(buffer, 0x00, 1024*sizeof(char));
            }

            close(f1);
            Close_file(inumber);

        }
    }
    fprintf(htmlfile, "</body></html>");
    fclose(htmlfile);
    printf("This is the count: %i\n", Count);
    return 0;
}
