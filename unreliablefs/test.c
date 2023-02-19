#include<stdlib.h>
#include<stdio.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <unistd.h>

int main(){
    struct stat buf;
    char str[100];
    struct stat buff;
    // lstat("/home/girish/fusemnt/abcd",&buff);
    // printf("%ld\n", buf.st_ino);
    int x= open("/home/girish/fusemnt/abcd", O_RDWR);
    printf("open filehandler value:: %d\n", x);
    
    if(read(x,&str,100) != -1) {
        printf("read the following:  %s\n", str);
    }
    close(x);
    char str1[5]="1234\0";
    int y= open("/home/girish/fusemnt/abcd", O_RDWR);
    printf("%d\n", y);
    if(write(y,str1,5) != -1) {
        printf("write Successful\n");
    }
    close(y);
}