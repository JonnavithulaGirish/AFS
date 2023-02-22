#include<stdlib.h>
#include<stdio.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <unistd.h>

int main(){
    printf("Hi startover\n");
    int ret =  rename("/home/rishideepreddy/AFSMount/def","/home/rishideepreddy/AFSMount/abc");
    if(ret == 0){
        printf("Rename Successful\n");
        struct stat buff;
        lstat("/home/rishideepreddy/AFSMount/def",&buff);
        printf("%ld\n", buff.st_ino);
    }else{
        printf("Rename failed\n");
    }
    // struct stat buf;
    // char str[100];
    // struct stat buff;
    // lstat("/home/girish/fusemnt/abcd",&buff);
    // printf("%ld\n", buf.st_ino);
    // int x= open("/home/girish/fusemnt/abcd", O_RDWR);
    // printf("open filehandler value:: %d\n", x);
    
    // if(read(x,str,100) != -1) {
    //     printf("read the following:  %s\n", str);
    // }
    // close(x);
    
    // char str1[5]="1234\0";
    // int y= open("/home/girish/fusemnt/abcd", O_RDWR);
    // printf("y:: %d\n", y);
    // int z =write(y,str1,5);
    // printf("z:: %d\n",z);
    // if( z == 1) {
    //     printf("write Successful\n");
    // }
    // int m =close(y);
    // printf("m:: %d\n",m);
    // if(m == 1)
    //     printf("close successful\n");
}