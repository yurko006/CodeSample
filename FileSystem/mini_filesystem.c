#include "mini_filesystem.h"
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>
#include <sys/time.h>
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

//Definitions of Lower Level Functions
int Search_Directory(char* filename);
int Add_to_Directory(char* filename, int Inode_Number);
Inode * Inode_Read(int Inode_Number);
int Block_Read(int block_number, int Number_of_bytes, char* String_to_read_in);
int Block_Write(int block_number, char* String_to_write);
Super_block Superblock_Read();
int Superblock_Write(Super_block * block);

//Global variables
FILE * logFile;
char msgBuffer[256];


void writeLog(char * chStructure, char * chReadWrite)
{
    // <Time Stamp> <File structure Accessed> <Read/Write>

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

    fprintf(logFile, "%s: %s %s\n",cBuffer , chStructure, chReadWrite);
}


//ALL DONE
int Initialize_Filesystem(char * log_filename)
{
    logFile = fopen(log_filename, "w+");

    int i;

    Superblock.next_free_block = 0;
    Superblock.next_free_inode = 0;


    for(i = 0; i < 8192; i++)
    {
        Disk_Blocks[i] = (char *) malloc(512*sizeof(char));
        memset(Disk_Blocks[i], 0x00, 512*sizeof(char));
    }

    for(i = 0; i < 128; i++)
    {
        Inode * node = Inode_Read(i);
        node->Inode_Number = 0;
        node->Start_Block = 0;
        node->End_Block = 0;
        node->File_Size = 0;
        node->Flag = 0;
        node->Group_Id = 0;
        node->User_Id = 0;

        memset(Directory_Structure[i].Filename, 0x00, 16*sizeof(char));
        Directory_Structure[i].Inode_Number = 0;
    }

    Count = 0;

    sprintf(msgBuffer,"File System Initialized");
    writeLog(msgBuffer,"");

    return 0;
}


int Create_File(char * filename)
{
    struct stat st = {0};

    fprintf(logFile, "+---------------------------------------------------------------------------------------------+\n");
    fprintf(logFile, "+------------------------------N E W - F I L E------------------------------------------------+\n");
    fprintf(logFile, "+---------------------------------------------------------------------------------------------+\n");

    if(Search_Directory(filename) == -1)
    {
        Add_to_Directory(filename, Superblock.next_free_inode);

        Inode * node = Inode_Read(Superblock.next_free_inode);

        Super_block  localblock = Superblock_Read();

        node->Group_Id = getgid();
        node->User_Id = getuid();

        char fn[1024];
        memset(fn, 0x00, 1024*sizeof(char));
        strcat(fn, "./input_directory/\0");
        strcat(fn, filename);

        stat(fn, &st);
        int size = st.st_size;

        node->File_Size = size;

        if (localblock.next_free_inode == 0)
        {
            node->Start_Block = 0;
            node->End_Block = 0;
        }
        else
        {
            node->Start_Block = Inode_List[localblock.next_free_inode-1].End_Block++;
            node->End_Block = Inode_List[localblock.next_free_inode].Start_Block;
        }

        node->Flag = 0;
        node->Inode_Number = localblock.next_free_inode;

        localblock.next_free_inode++;

        Superblock_Write(&localblock);

        return 0;
    }
    return -1;
}

int Open_File(char * filename)
{
    fprintf(logFile, "+---------------------------------------------------------------------------------------------+\n");
    fprintf(logFile, "+------------------------------O P E N - F I L E----------------------------------------------+\n");
    fprintf(logFile, "+---------------------------------------------------------------------------------------------+\n");
    int index = Search_Directory(filename);
    if( index != -1)
    {
        Inode * node = Inode_Read(index);
        node->Flag = 1;
        return index;
    }

    sprintf(msgBuffer,"Failure Open: %s Directory index=%i\n",filename,-1);
    writeLog(msgBuffer,"");

    return -1;
}


int Read_File(int inode_number, int offset, int count, char * to_read)
{
    Inode * node = Inode_Read(inode_number);
    if (offset >= node->File_Size)
    {
        return -1;
    }

    int i = node->Start_Block + offset / 512;
    int leftover = node->File_Size % 512;

    int numberOfBlocks = offset / 512;
    int numberOfFileBlocks = node->File_Size / 512;


    if(numberOfBlocks == numberOfFileBlocks)
    {
        return Block_Read(i, leftover , to_read);
    }

    return Block_Read(i, 512 , to_read);
}


int Write_File(int inode_number, int offset, char * to_write)
{
    Inode * node = Inode_Read(inode_number-1);
    if (offset > node->File_Size)
    {
        return -1;
    }


    Block_Write(Inode_List[inode_number-1].End_Block, to_write);

    node->End_Block++;

    return 0;
}

int Close_file(int inode_number)
{
    Inode * node = Inode_Read(inode_number);
    if(node->Flag == 1)
    {
        node->Flag = 0;
        return 0;
    }
    else
    {
        return -1;
    }
}

//Calls to lower level functions
//Done
int Search_Directory(char * filename)
{
    Count++;
    writeLog("Directory","Read");
    int i;
    for(i = 0; i < 128; i++)
    {
        if(strcmp(filename, Directory_Structure[i].Filename) == 0)
        {
            return Directory_Structure[i].Inode_Number;
        }
    }
    return -1;
}

// Done
int Add_to_Directory(char * filename, int Inode_Number)
{
    Count++;
    writeLog("Directory", "Write");
    if(Search_Directory(filename) == -1)
    {
        strcpy(Directory_Structure[Inode_Number].Filename, filename);
        Directory_Structure[Inode_Number].Inode_Number = Inode_Number;
        return 0;
    }
    return -1;
}

Inode * Inode_Read(int Inode_Number)
{
    Count++;
    writeLog("Inode", "Read");
    return &Inode_List[Inode_Number];
}

//Works
int Block_Read(int block_number, int Number_of_bytes, char * String_to_read_in)
{
    Count++;
    writeLog("Disk Blocks", "Read");
    memcpy(String_to_read_in,Disk_Blocks[block_number],sizeof(char)*Number_of_bytes);
    return Number_of_bytes;
}

//Works
int Block_Write(int block_number, char * String_to_write)
{
    Count++;
    writeLog("Disk Blocks", "Write");
    memcpy(Disk_Blocks[block_number], String_to_write,sizeof(char)*512);
    return 0;
}

Super_block Superblock_Read()
{
    Count++;
    writeLog("Super Block", "Read");
    return Superblock;
}

int Superblock_Write(Super_block * block)
{
    Count++;
    writeLog("Super Block", "Write");
    Superblock.next_free_block = block->next_free_block;
    Superblock.next_free_inode = block->next_free_inode;
    return 0;
}

