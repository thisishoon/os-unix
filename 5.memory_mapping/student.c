#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "student.h"

#define MSG(x...) fprintf (stderr, x)
#define STRERROR  strerror (errno)

int fd;
static Student *student;

int main (int argc, char **argv){
    char * filename = "./student.dat";
    char * flag = "";
    int output=0;
    char * update_string = "";
    int update_digit = 0;
    char * attr_name = "";
    char buf[1024];
    if(argc==2){
        attr_name = argv[1];
        if(student_attr_is_integer(attr_name)){
                flag = "digit";
            }
            else{
                flag = "string";
            }
    }
    else if(argc==4){
        attr_name = argv[3];
        if(argv[1][1]=='f'){
            filename = argv[2];
            if(student_attr_is_integer(attr_name)){
                flag = "digit";
            }
            else{
                flag = "string";
            }
            
        }
        else if(argv[1][1]=='s'){
            output=1;
            if(student_attr_is_integer(attr_name)){
                update_digit = atoi(argv[2]);
                flag = "digit";
            }
            else{
                update_string = argv[2];
                flag = "string";
            }
        }
    }
    else if(argc==6){
        output=1;
        attr_name = argv[5];
        filename = argv[2];
        if(student_attr_is_integer(attr_name)){
            update_digit = atoi(argv[4]);
            flag = "digit";
        }
        else{
            update_string = argv[4];
            flag = "string";
        }
    }
    else{
        MSG("usage error");
        return 0;
    }
    
    const int size = sizeof(Student);
    fd = open(filename, O_RDWR, 0666);
    if(fd==-1){
        fd = open(filename, O_RDWR|O_CREAT, 0666);
        ftruncate(fd, size);
    }
    student = (Student*)mmap(0, size, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);
    size_t offset = student_get_offset_of_attr(attr_name);

    if(student==MAP_FAILED) MSG("ERROR");

    if(output==1){
        if(strcmp(flag, "string")==0){
            memcpy((char *)student+offset, update_string, strlen(update_string)+1);
            printf("%s\n", (char*)student+offset);
        }
        else if(strcmp(flag, "digit")==0){
            memcpy((char *)student+offset, &update_digit, sizeof(update_digit)+1);
            int num=0;
            memcpy(&num, (char *)student+offset, sizeof(int));
            printf("%d\n", num);
        }
    }
    else{
        if(strcmp(flag, "string")==0){
            printf("%s\n", (char*)student+offset);
        }
        else if(strcmp(flag, "digit")==0){
            int num=0;
            memcpy(&num, (char *)student+offset, sizeof(int));
            printf("%d\n", num);
        }
    }

    msync(student, size, MS_SYNC);
    munmap(student, size);
    close(fd);

    return 0;
}
