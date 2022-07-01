
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char **argv)
{
        int keylen = 0, i, random;
        char buf[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
        if(argc < 2)
        {
                fprintf(stderr, "usage: keygen 1024\n");
                return 1;
        }
        //input key length
        keylen = atoi(argv[1]);
        if(keylen <= 0)
        {
                fprintf(stderr, "keylen error\n");
                return 1;
        }
        //set the current time to random seeds
        srand((unsigned)time(NULL));
        for(i = 0; i < keylen; i++)
        {
                //get the random num
                random = rand() % 27;
                printf("%c",buf[random]);
        }
        printf("\n");
        return 0;
}

