#include "userapp.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define PROC_FULL_PATH "/proc/kmlab/status"

static FILE* procfile;
// Register process with kernel module by printing it to procfile
void register_process(unsigned int pid)
{
    fprintf(procfile, "%d\n", pid);
    fflush(procfile);
}

int main(int argc, char* argv[])
{
    procfile = fopen(PROC_FULL_PATH, "w");

    if (procfile == NULL){
        printf("Error in opening procfile\n");
        return 1;
    }

    int __expire = 10;
    time_t start_time = time(NULL);

    if (argc == 2) {
        __expire = atoi(argv[1]);
    }

    register_process(getpid());

    // Terminate user application if the time has expired
    // Dummy user application calculation 
    while (1) {
        if ((int)(time(NULL) - start_time) > __expire) {
            break;
        }
    }
    
    fclose(procfile);


	return 0;
}
