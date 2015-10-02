typedef struct
{
    char filename[1024];
    char path[1024];
    char md5[1024];
    int dataLength;
    int action;
} descriptorBlock;

char * buildMD5(char * filepath);

// ****************************************************************** //
// *                                                                * //
// ****************************************************************** //
char * buildMD5(char * filepath)
{
    int i;
    char * holder = malloc(sizeof(char)*1024);
    char * md5String = malloc(sizeof(char)*1024);
    unsigned char * md5 = malloc(sizeof(unsigned char)*1024);
    memset(holder, 0x00, sizeof(unsigned char)*1024);
    memset(md5String, 0x00, sizeof(unsigned char)*1024);
    memset(md5, 0x00, sizeof(unsigned char)*1024);
    md5sum(filepath, md5);
    for(i = 0; i < MD5_DIGEST_LENGTH; i++)
    {
        sprintf(holder, "%02x", md5[i]);
        strcat(md5String, holder);
    }
    free(holder);
    free(md5);
    return (md5String);
}

int tryReceive(int s, int datalength)
{
    char * bpeek = malloc(datalength);
    memset(bpeek, 0x00, datalength);
    int tmp_receive = 0;

    while (1)
    {
        if ((tmp_receive = recv(s , bpeek, datalength, MSG_PEEK)) == datalength)
        {
            break;
        }
    }
    return tmp_receive;
}
