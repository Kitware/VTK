#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BUFFERSIZE 1024

int main(int argc, char *argv[]) {
    FILE *infile = NULL;
    FILE *outfile = NULL;
    char *buffer = NULL;
    char argbuf[8];
    unsigned int bytes = 0;
    unsigned int idx = 0;
    unsigned int lineidx = 0;
    unsigned int stripXlines = 3;
   
    if(argc < 3)
        exit(1);
    if(NULL == (infile = fopen(argv[1], "rb")))
        exit(2);
    if(NULL == (outfile = fopen(argv[2], "wb+")))
        exit(3);
    if(argc > 3)
        if(argv[3][0] == '-')
            if(argv[3][1] == 'l') {
                strcpy(argbuf, &argv[3][2]);
                stripXlines = atoi(argbuf);
            }
    buffer = (char*)malloc(BUFFERSIZE);
    if(buffer) {
        while(!feof(infile)) {
            /* read the file into the buffer. */
            bytes = fread(buffer, 1, BUFFERSIZE, infile);  
            if(lineidx < stripXlines) {
                for(idx = 0; idx < bytes; idx++) {
                    if(buffer[idx] == '\n') {
                        lineidx++;
                        if(buffer[idx+1] == '\r')
                            idx++;
                    }
                    if(lineidx >= stripXlines) {
                        fwrite(&buffer[idx+1], 1, bytes-idx-1, outfile);
                        idx = bytes;
                    }
                }
            }
            else
                fwrite(buffer, 1, bytes, outfile);
        }
        free(buffer);
    }
    fclose(outfile);
    fclose(infile);
    
    return 0;
}
