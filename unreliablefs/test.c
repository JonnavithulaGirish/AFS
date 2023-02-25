#include<stdlib.h>
#include<stdio.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>


int main(){
    // Rename Test Case//
    // printf("Hi startover\n");
    // int ret =  rename("/home/rishideepreddy/AFSMount/def","/home/rishideepreddy/AFSMount/abc");
    // if(ret == 0){
    //     printf("Rename Successful\n");
    //     struct stat buff;
    //     lstat("/home/rishideepreddy/AFSMount/def",&buff);
    //     printf("%ld\n", buff.st_ino);
    // }else{
    //     printf("Rename failed\n");
    // }
    
    // creat test case//
    // int ret = creat("/home/araghavan/cs739/mntpt/helloworld", 0777);
    // printf("%d - creat\n", ret);
    // return 0;

    // read and write Test Case//
    // struct stat buf;
    // char str[100];
    // struct stat buff;
    // lstat("/users/Girish/fusemnt/abcd12",&buff);
    // printf("%ld\n", buf.st_ino);
    // int x= open("/users/Girish/fusemnt/abcd12", O_CREAT | O_RDWR);
    // printf("open filehandler value:: %d\n", x);
    //  if(read(x,str,100) != -1) {
    //     printf("read the following:  %s\n", str);
    // }
    // int h= close(x);
    // if(h == -1){
    //     printf("Close failed");
    // }
    // printf("%d\n",h);
    // char str1[5]="abcde"; 
    // int fd= open("/users/Girish/fusemnt/abcd12", O_RDWR);
    // printf("y:: %d\n", fd);
    // int z = write(fd,str1,5);
    // printf("z:: %d\n",z);
    // if( z == 5) {
    //     printf("write Successful\n");
    // }
    // int m = close(fd);
    // printf("m:: %d\n",m);
    // if(m == 0)
    //     printf("close successful\n");
    // DIR *dir = opendir("/users/Girish/fusemnt");
    // if (!dir) {
    //     return -errno;
    // }

    // struct dirent *de;
    // while ((de = readdir((DIR *)dir)) != NULL) {
    //     struct stat st;
    //     memset(&st, 0, sizeof(st));
    //     st.st_ino = de->d_ino;
    //     st.st_mode = de->d_type << 12;
    //     printf("%s %ld %d\n", de->d_name, st.st_ino, st.st_mode);
    // }

    // int ret = closedir(dir);
    // int ret = creat("/users/Girish/fusemnt/qwerty6", 0777);
    // // int ret = unlink("/users/Girish/fusemnt/qwerty5");
    // printf("ret - %d\n", ret);
    // close(ret);

    // struct stat buf;
    // ret = lstat("/users/Girish/fusemnt/qwerty6", &buf);
    // printf("%d - %d \n", ret, errno);

    // int x= open("abcd", O_CREAT | O_RDWR);
    // printf("%d\n",x);
    // if(read(x,str,100) != -1) {
    //     printf("read the following:  %s\n", str);
    // }

    int fd = creat("/users/Girish/fusemnt/kavyat", 0777);
    printf("creat - %d\n", fd);
    printf("close - %d\n", close(fd));
    int ret = unlink("/users/Girish/fusemnt/kavyat");
    printf("unlink - %d\n", ret);
    return 0;

}