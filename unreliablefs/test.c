#include<stdlib.h>
#include<stdio.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <unistd.h>

int main(){
    struct stat buf;
    int x= open("/home/girish/fusemnt/abcd", O_RDONLY);
    printf("%d\n", x);
    close(x);
}