#include<stdlib.h>
#include<stdio.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <assert.h>

int main(){
    int fd = open("/users/Girish/fusemnt/durability", O_RDWR);
    if(fd != -1){
        printf("Successfully opened\n");
        char str1[5]="abcd\n"; 
        int z;
        z = write(fd,str1,5);
        printf("%d\n",z);
        
        z = write(fd,str1,5);
        
        z = write(fd,str1,5);
        
        z = write(fd,str1,5);
        
        z = write(fd,str1,5);
        
        z = write(fd,str1,5);
        
        z = write(fd,str1,5);
        
        z = write(fd,str1,5);
        
        z = write(fd,str1,5);
        
        z = write(fd,str1,5);
        
        z = write(fd,str1,5);
        
        return 1;

    }
    return 0;
}