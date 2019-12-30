#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <sys/wait.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MSG(x...) fprintf (stderr, x)
#define STRERROR  strerror (errno)

int main(int argc, char **argv){
    char buf[1024];
    char * p;

    FILE * version = fopen("/proc/version", "r");
    fgets(buf, 1024, version);
    printf("%s\n", buf);
    
    FILE * num_cpu = popen("cat /proc/cpuinfo | grep \"processor\" -c", "r");
    fgets(buf, 1024, num_cpu);
    printf("number of cpus  : %s\n", buf);

    FILE * speed_core = popen("cat /proc/cpuinfo | grep \"cpu MHz\" ", "r");
    fgets(buf, 1024, speed_core);
    printf("%s\n", buf);

    FILE * num_core = popen("cat /proc/cpuinfo  |grep \"cpu cores\"", "r");
    fgets(buf, 1024, num_core);
    printf("%s\n", buf);

    FILE * MemTotal = popen("cat /proc/meminfo | grep \"MemTotal\"", "r");
    fgets(buf, 1024, MemTotal);
    p = strtok(buf, " ");
    p = strtok(NULL, " ");
    int memtotal = atoi(p);

    FILE * MemFree = popen("cat /proc/meminfo | grep \"MemFree\"", "r");
    fgets(buf, 1024, MemFree);
    p = strtok(buf, " ");
    p = strtok(NULL, " ");
    int memfree = atoi(p);

    printf("MemUsed:           %d\n\n", memtotal-memfree);
    printf("MemFree:           %d\n\n", memfree);

    FILE * uptime = fopen("/proc/uptime", "r");
    fgets(buf, 1024, uptime);
    p = strtok(buf, " ");

    time_t current_time;
    time(&current_time);
    time_t boot_time = current_time - atoi(p);
    struct tm *tt = localtime(&boot_time);
    printf("last booting time: %d:%d:%d:%d\n\n", tt->tm_mday, tt->tm_hour, tt->tm_min, tt->tm_sec);

    FILE * stat = popen("cat /proc/stat | grep \"ctxt\"", "r");
    fgets(buf, 1024, stat);
    p = strtok(buf, " ");
    p = strtok(NULL, " ");
    printf("number of context switching: %s\n", p);
    
    
    int total = 0;
    FILE * interrupts = fopen("/proc/interrupts", "r");
    while(fgets(buf, 1024, interrupts) != NULL){
        if(strchr(buf, ':') != NULL){
            p = strtok(buf, " ");
            p = strtok(NULL, " ");
            total += atoi(p);
            p = strtok(NULL, " ");
            if(p != NULL){
                total += atoi(p);
            }
        }
    }
    printf("number of interrupts:    %d\n\n", total);
    

    FILE * loadavg = fopen("/proc/loadavg", "r");
    fgets(buf, 1024, loadavg);
    p = strtok(buf, " ");
    p = strtok(NULL, " ");
    p = strtok(NULL, " ");
    printf("load average in 15m:  %s\n", p);
    return 0;
}