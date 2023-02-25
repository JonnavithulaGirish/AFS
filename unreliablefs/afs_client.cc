/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <errno.h>
#include <memory>
#include <string>
#include <filesystem>
#include <sys/stat.h>
#include <openssl/sha.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <fuse.h>
#include <grpcpp/grpcpp.h>
#include <ios>
#include <fstream>


#ifdef BAZEL_BUILD
#include "examples/protos/helloworld.grpc.pb.h"
#else
#include "afs.grpc.pb.h"
#endif

using FS::AFS;
using FS::GetAttrRequest;
using FS::GetAttrResponse;
using FS::OpenRequest;
using FS::OpenResponse;
using FS::CloseRequest;
using FS::CloseResponse;
using FS::MkdirRequest;
using FS::MkdirResponse;
using FS::OpendirRequest;
using FS::OpendirResponse;
using FS::RmdirRequest;
using FS::RmdirResponse;
using FS::ReleasedirRequest;
using FS::ReleasedirResponse;
using FS::RenameRequest;
using FS::RenameResponse;
using FS::TruncateRequest;
using FS::TruncateResponse;
using FS::MknodRequest;
using FS::MknodResponse;
using FS::ReaddirRequest;
using FS::ReaddirResponse;
using FS::CreatRequest;
using FS::CreatResponse;
using FS::UnlinkRequest;
using FS::UnlinkResponse;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;



using namespace std;

char *mountPoint;
char *cacheDir;
char serverNodePort[] = "localhost:50051";

string sha256(const string str)
{
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, str.c_str(), str.size());
  SHA256_Final(hash, &sha256);
  stringstream ss;
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
  {
    ss << hex << setw(2) << setfill('0') << (int)hash[i];
  }
  return ss.str();
}

class AfsClientSingleton
{

private:
  std::unique_ptr<AFS::Stub> stub_;
  static AfsClientSingleton *instancePtr;
  string m_mountPoint;
  string m_cacheDir;
  std::unordered_map <int,std::string> fileMap;
  //std::unordered_map <uint64_t, std::string> dirMap;

  AfsClientSingleton()
  {
  }

  

  AfsClientSingleton(std::shared_ptr<Channel> channel) : stub_(AFS::NewStub(channel))
  {
     m_mountPoint= mountPoint;
     m_cacheDir="/users/Girish/afscache/";
    //m_mountPoint = filesystem::absolute(filesystem::path(mountPoint)).string();
    //m_cacheDir = filesystem::absolute(filesystem::path(cacheDir)).string()+"/";
  }

public:
  AfsClientSingleton(const AfsClientSingleton &obj) = delete;

  static AfsClientSingleton *getInstance(std::string serverIP)
  {
    if (instancePtr == nullptr)
    {
      instancePtr = new AfsClientSingleton(grpc::CreateChannel(serverIP, grpc::InsecureChannelCredentials()));
      return instancePtr;
    }
    else
      return instancePtr;
  }

    string removeMountPointPrefix(string inputPath)
  {
   
    string s_inputPath = filesystem::absolute(filesystem::path(inputPath)).string();
    if(s_inputPath.find(m_cacheDir) != string::npos){
      return s_inputPath.substr(m_cacheDir.size());
    }
    return inputPath;
  }

  int GetAttr(std::string path, struct stat *buf)
  {
    GetAttrRequest request;
    GetAttrResponse reply;
    ClientContext context;

    //Setting request parameters
    path = removeMountPointPrefix(path);
    cout << "GetAttr() called with path " << path << endl;
    request.set_path(path);

    //Trigger RPC Call for GetAttr
    Status status = stub_->GetAttr(&context, request, &reply);

    //On Response received
    if (status.ok() && reply.errnum() == 1)
    {
      memcpy((char *)buf, reply.statbuf().c_str(), sizeof(struct stat));

      // return local file stat if cache copy is latest
      struct stat localbuf;
      string localCacheFilePath = m_cacheDir+sha256(path);
      int ret = lstat(localCacheFilePath.c_str(), &localbuf);
      if (ret != -1)
      {
        if ((buf->st_mtim.tv_sec < localbuf.st_mtim.tv_sec) || (buf->st_mtim.tv_sec == localbuf.st_mtim.tv_sec && buf->st_mtim.tv_nsec <= localbuf.st_mtim.tv_nsec))
        {
          memcpy((char *)buf, &localbuf, sizeof(struct stat));
        }
      }

      return 1;
    }
    else
    {
      if(reply.errnum() != 1)
        errno = reply.errnum();
      cout << "Error on GetAttr() with errno :: "<< errno << endl;
      cout << status.error_code() << ": " << status.error_message() << std::endl;
      return -1;
    }
  }

  int Open(std::string path, int flags)
  {
    path = removeMountPointPrefix(path);
    cout << "Open call at " << path << endl;
    string localCacheFilePath = m_cacheDir + sha256(path);

    bool bFetchFromServer = false;

    struct stat serverbuf;
    int ret = GetAttr(path, &serverbuf);
    if (ret == -1)
    {
      // file not found in server
      // 1 - if flag is O_CREAT, we create a file on server and on cache and return cache fd
      // 2 - flag does not have CREAT, return -1, set errno = ENOENT

      if ((flags & O_CREAT) == O_CREAT)
      {
        cout<<"hu "<<endl;
        bFetchFromServer = true;
      }
      else 
      {
        cout << "GetAttr() inside open failed with errno " << errno << endl;
        return -1;
      }
    }
    else 
    {
      // compare times with local. If latest, use cache, else fetch from server.
      struct stat localbuf;
      ret = lstat(path.c_str(), &localbuf);
      if (ret == -1)
      {
        // if file does not exist in local, then create, otherwise set errno return -1
        if (errno == ENOENT)
        {
          bFetchFromServer = true;
        }
        else 
        // errno already set
          return -1;
      }
      else 
      {
        if ((serverbuf.st_mtim.tv_sec < localbuf.st_mtim.tv_sec) || (serverbuf.st_mtim.tv_sec == localbuf.st_mtim.tv_sec && serverbuf.st_mtim.tv_nsec <= localbuf.st_mtim.tv_nsec))
        {
          
          int fd = open(localCacheFilePath.c_str(), flags);
          if (fd == -1)
          {
            cout << "Open locacl cache file failed with errno " << errno << endl;
            return -1;
          }
          cout<< "Open:: (file on server and client but client latest) Using file in local cache with fd ::"<< fd << endl;
          fileMap[fd] = path;
          return fd;
        }
        else
          //stale, get from server
          bFetchFromServer = true;
      }
    }

    if (bFetchFromServer)
    {
       int offset = 0;
      // update/ create local cache
      int fd = open(localCacheFilePath.c_str(),O_CREAT | O_RDWR, 0777);
      if (fd == -1)
      {
        cout << "Open:: opening local cache failed, errno - " << errno << endl;
        return -1;
      }

      OpenRequest request;
      OpenResponse reply;
      ClientContext context;

      request.set_path(path);
      request.set_flags(flags);

      std::unique_ptr<ClientReader<OpenResponse> > reader(stub_->Open(&context, request));
      while (reader->Read(&reply)) {
        if (reply.errnum() != 1)
        {
          errno = reply.errnum();
          cout << "Open failed at server, errno -  " << errno << endl;
          return -1;
        }
        
        int ret = pwrite(fd, reply.filedata().c_str(), reply.filedata().size(), offset);
        cout << "Open:: server sent data size- " << reply.filedata().size() << endl;
        if (ret == -1)
        {
          cout << "Open:: pwrite failed, errno - " << errno;
          return -1;
        }
        offset += ret;
      }
      Status status = reader->Finish();

      if (status.ok() && reply.errnum() == 1)
      {
        fsync(fd);
        cout << "Open sucessful !" << endl;
        fileMap[fd] = path;
        return fd;
      }
      else 
      {
        close(fd);
        errno = reply.errnum();
        cout << "Open (with O_CREAT) failed with error - " << errno << endl;
        cout << "Error code , Error msg from grpc if any" << endl;
        cout << status.error_code() << ": " << status.error_message() << std::endl;
        return -1;
      }
    }

    cout << "Open:: should not reach here " << endl;
    return -1;
  }

  // int Open(std::string path, int flags)
  // {
  //   cout<< "Open call with @Path:: " << path << endl;
  //   path = removeMountPointPrefix(path);
  //   string absoluteCachePath = m_cacheDir + sha256(path);
  //   int fd = open(absoluteCachePath.c_str(), flags);
  //   if (fd != -1)
  //   {
  //     cout<< "Open found file in local @Path:: " << absoluteCachePath << endl;
  //     // file in local cache, check if stale
  //     struct stat statBuf, lstatBuf;

  //     cout<< "Checking with Server GetAttr" << endl;
  //     // Call getAttr to check if local file data is not stale
  //     int ret = GetAttr(path, &statBuf);
  //     if (ret == -1 && (flags & O_CREAT != O_CREAT))
  //     {
  //       cout << "Open :: file not in server and O_CREAT flag not passed\n";
  //       return -1;
  //     }
  //     else if (ret  != -1)
  //     {
  //       memset(&lstatBuf, 0, sizeof(struct stat));
  //       if (lstat(absoluteCachePath.c_str(), &lstatBuf) == -1)
  //       {
  //         //return error status on failure
  //         // never reaches here!
  //         return -errno;
  //       }

  //       // cache not stale
  //       if ((statBuf.st_mtim.tv_sec < lstatBuf.st_mtim.tv_sec) || (statBuf.st_mtim.tv_sec == lstatBuf.st_mtim.tv_sec && statBuf.st_mtim.tv_nsec <= lstatBuf.st_mtim.tv_nsec)){
  //         cout<< "Using file in local cache with fd::"<< fd << endl;
  //         fileMap[fd] = path;
  //         return fd;
  //       }
  //     }
  //   }

  //   OpenRequest request;
  //   OpenResponse reply;
  //   ClientContext context;

  //   request.set_path(path);
  //   request.set_flags(flags);
  //   cout<< "Calling Server with Open @path:: "<< path << endl;
  //   Status status = stub_->Open(&context, request, &reply);
  //   //cout << "calling Open with the following path :: " << request.path()<< endl;
  //   if (status.ok() && reply.errnum() == 1)
  //   {
  //     cout<< "Creating file in localcache @path:: "<< absoluteCachePath << endl;
  //     //Save the data in a new file and return file descriptor
  //     int fd1 = open(absoluteCachePath.c_str(), flags | O_CREAT,0777);
  //     if (fd1 == -1)
  //     {
  //       cout<< "a" <<endl;
  //       return -errno;
  //     }
  //     cout<< "writing to localcache @path:: "<< absoluteCachePath << endl;
  //     int ret = pwrite(fd1, reply.filedata().c_str(), reply.filedata().size(), 0);
  //     if (ret == -1)
  //     {
  //       cout<< "b" <<endl;
  //       ret = -errno;
  //     }
  //     fsync(fd1);
  //     fileMap[fd1] = path;
  //     cout<< "returning localcachefile fd:: "<< fd1 << endl;
  //     //cout << "created file on client with the following name :: " << absoluteCachePath << endl;
  //     return fd1;
  //   }
  //   else
  //   {
  //     // need to update errno:: GJ
  //     if(reply.errnum() != 1)
  //       errno = reply.errnum();
  //     cout << "Some Error on AfsOpen()"<< errno <<endl;
  //     cout << status.error_code() << ": " << status.error_message() << std::endl;
  //     return -1;
  //   }
  // }

  int Close(int fd )
  {
    ClientContext context;
    if (fileMap.find(fd) == fileMap.end())
    {
      cout << "Close:: incorrect fd " << endl;
      errno = ENOENT;
      return -1;
    }
    string serverPath = fileMap[fd];
    CloseRequest req;
    CloseResponse reply;
    req.set_path(serverPath);
    cout << "Close called at path and fd" << serverPath << ", " << fd << endl;
    int ret = 1;
    int chunksz = 4000, offset = 0;

    std::unique_ptr<ClientWriter<CloseRequest> > writer(
        stub_->Close(&context, &reply));

    while (ret)
    {
      char chunk[chunksz];
      ret = pread(fd, chunk, chunksz, offset);
      if (ret == -1)
      {
        cout << "Close:: pread failed errno - " << errno << endl;
        return -1;
      }
      req.set_filedata((char*)chunk, ret);
      if (!writer->Write(req))
      {
        cout << "Close:: broken stream" << endl;
        return -1;
      }
      offset += ret;
    }

    writer->WritesDone();
    Status status = writer->Finish();
    if (status.ok() && reply.errnum() == 1)
      {
        cout << "Close sucessful !" << endl;
        close(fd);
        fileMap.erase(fd);
        return 0;
      }
      else 
      {
        // close(fd);
        errno = reply.errnum();
        cout << "Close failed on server with error - " << errno << endl;
        cout << "Error code , Error msg from grpc if any" << endl;
        cout << status.error_code() << ": " << status.error_message() << std::endl;
        return -1;
      }
  }

  // int Close(int fh){
  //     cout<< "Closing file with @fh:: "<< fh << endl;
  //   CloseRequest request;
  //   CloseResponse reply;
  //   ClientContext context;
  //   std::string serverPath;
    
    
  //   if (fileMap.find(fh) == fileMap.end())
  //       return -1;
  //   else
  //       serverPath = fileMap[fh];
  //   cout<< "Found file in localcache @path:: "<< serverPath << endl;
  
  //   string absoluteCachePath = m_cacheDir + sha256(serverPath);
  //   //Get File Attributes
  //   struct stat statBuf;
  //   if (lstat(absoluteCachePath.c_str(), &statBuf) == -1)
  //   {
  //     //return error status on failure
  //     return -1;
  //   }
  //   //cout << "lstat called on local client and fileinode num is :: "<< statBuf.st_ino <<endl;

  //   //Read File
  //   char *buf = new char[statBuf.st_size];
  //   if (pread(fh, buf, statBuf.st_size, 0) == -1) {
  //     //return error status on failure
  //     return -1;
  //   }
  //   //cout << "file reading is complete on client :: "<< buf <<endl;
  //   //Send File Data
  //   request.set_path(serverPath);
  //   request.set_filedata(buf, statBuf.st_size);
  //   cout << "Calling AfsClose with the following serverPath :: "<< request.path() <<endl;
  //   std::cout<<  request.filedata()<< " is being sent" << std::endl;

  //   Status status = stub_->Close(&context, request, &reply);
      
  //   if (status.ok() && reply.errnum() == 1){
  //     return 0;
  //   }
  //   else{
  //       // need to update errno:: GJ
  //       cout << "Some Error on AfsClose()" <<endl;
  //       cout << status.error_code() << ": " << status.error_message() << std::endl;
  //       return -1;
  //   }
  // }


  int Mkdir(string path, int flags)
  {
    cout << "Mkdir called @path " << path << endl;
    MkdirRequest request;
    MkdirResponse reply;
    ClientContext context;

    path = removeMountPointPrefix(path);
    request.set_path(path);
    request.set_mode(flags);

    //Trigger RPC Call for Mkdir
    Status status = stub_->Mkdir(&context, request, &reply);


    //On Response received
    if (status.ok() && reply.errnum() == 1)
    {
      cout << "Mkdir status ok " << reply.errnum() << endl;
      mkdir((m_cacheDir+path).c_str(), flags);
      return 0;
    }
    else
    {
      if(reply.errnum() !=1)
        errno = reply.errnum();
      cout << "Mkdir status not ok :/" << endl;
      cout << status.error_code() << ": " << status.error_message() << std::endl;
      return -1;
    }
  }


  int Rmdir(string path)
  {
    cout << "Rmdir called @path " << path << endl;
    RmdirRequest request;
    RmdirResponse reply;
    ClientContext context;

    path = removeMountPointPrefix(path);
    request.set_path(path);

    //Trigger RPC Call for Rmdir
    Status status = stub_->Rmdir(&context, request, &reply);


    //On Response received
    if (status.ok() && reply.errnum() == 1)
    {
      cout << "Rmdir status ok " << reply.errnum() << endl;
      return 0;
    }
    else
    {
      if(reply.errnum() !=1)
        errno = reply.errnum();
      cout << "Rmdir status not ok :/" << endl;
      cout << status.error_code() << ": " << status.error_message() << std::endl;
      return -1;
    }
  }


  int64_t Opendir(string path)
  {
    cout << "Opendir called @path " << path << endl;
    OpendirRequest request;
    OpendirResponse reply;
    ClientContext context;

    path = removeMountPointPrefix(path);
    cout << "Opendir updated @path " << path << endl;
    request.set_path(path);

    //Trigger RPC Call for Opendir
    Status status = stub_->Opendir(&context, request, &reply);


    //On Response received
    if (status.ok() && reply.errnum() == 1)
    {
      cout << "Opendir status ok " << reply.errnum() << endl;
      return reply.fh();
    }
    else
    {
      if(reply.errnum() !=1)
        errno = reply.errnum();
      cout << "Opendir status not ok :/" << endl;
      cout << status.error_code() << ": " << status.error_message() << std::endl;
      return -1;
    }
  }

  int64_t Readdir(int64_t dp, int *sz, char ***dnames)
  {
    cout << "Readdir called @dirptr " << dp << endl;
    ReaddirRequest request;
    ReaddirResponse reply;
    ClientContext context;

    request.set_dp(dp);

    Status status = stub_->Readdir(&context, request, &reply);

    //On Response received
    if (status.ok())
    {
      cout << "Readdir status ok " << endl;
      if (reply.numentries() == 0)
        return 0;

      cout << "a" << endl;
      *sz = reply.numentries();

      struct stat* ast = new struct stat[reply.numentries()];
      struct stat* de = (struct stat*)reply.dirent().c_str();
      
      cout << "b" << endl;
      *dnames = new char*[reply.numentries()];
      const char *tmp = reply.dnames().c_str();

      for (int i = 0; i < reply.numentries(); i++)
      {
        cout << "c" << endl;
        memcpy((char *)&ast[i], (char *)(de+i), sizeof(struct stat));
        char *str = new char[256];
        memcpy(str, tmp+i*256, 256);
        cout << str << endl;
        *(*dnames+i) = str;
        cout << *(*dnames+i) << endl;
      }
      cout << 'd' << endl;
      
      return (int64_t)ast;
    }
    else
    {
      cout << "Readdir status not ok :/" << endl;
      cout << status.error_code() << ": " << status.error_message() << std::endl;
      return 0;
    }
  }

  int Releasedir(int64_t fh)
  {
    cout << "Releasedir called @file_handle " << fh << endl;
    ReleasedirRequest request;
    ReleasedirResponse reply;
    ClientContext context;

    
    request.set_fh(fh);

    //Trigger RPC Call for Opendir
    Status status = stub_->Releasedir(&context, request, &reply);

    //On Response received
    if (status.ok() && reply.errnum() == 1)
    {
      cout << "Releasedir status ok " << reply.errnum() << endl;
      return 0;
    }
    else
    {
      if(reply.errnum() !=1)
        errno = reply.errnum();
      cout << "Releasedir status not ok :/" << endl;
      cout << status.error_code() << ": " << status.error_message() << std::endl;
      return -1;
    }
  }

  int Rename(std::string oldPath, std::string newPath){
    cout << "Rename called @oldPath " << oldPath<<" @newPath "<<newPath << endl;
    RenameRequest request;
    RenameResponse reply;
    ClientContext context;

    oldPath = removeMountPointPrefix(oldPath);
    newPath = removeMountPointPrefix(newPath);

    request.set_oldpath(oldPath);
    request.set_newpath(newPath);

    Status status = stub_->Rename(&context, request, &reply);


    //On Response received
    if (status.ok() && reply.status() == 1)
    {
      cout << "Rename status ok " << reply.status() << endl;
      return reply.status();
    }
    else
    {
      if(reply.status() !=1)
        errno = reply.status();
      cout << "Rename status not ok :/" << endl;
      cout << status.error_code() << ": " << status.error_message() << std::endl;
      return -1;
    }

  }

  int Truncate(string path, int len)
  {
    cout << "Truncate called @path " << path << endl;
    TruncateRequest request;
    TruncateResponse reply;
    ClientContext context;

    //Setting request parameters
    path = removeMountPointPrefix(path);
    request.set_path(path);
    request.set_len(len);

    //Trigger RPC Call for GetAttr
    Status status = stub_->Truncate(&context, request, &reply);

    //On Response received
    if (status.ok() && reply.errnum() == 1)
    { 
      return 0;
    }
    else
    {
      if(reply.errnum() != 1)
        errno = reply.errnum();
      cout << "Error on Truncate() with errno :: "<< errno << endl;
      cout << status.error_code() << ": " << status.error_message() << std::endl;
      return -1;
    }
  }

  int Mknod(string path, mode_t mode, dev_t dev)
  {
    cout << "Mknod called @path with mode and dev =" << path <<", " << mode << "," << dev << endl;
    MknodRequest request;
    MknodResponse reply;
    ClientContext context;

    //Setting request parameters
    path = removeMountPointPrefix(path);
    request.set_path(path);
    request.set_mode(mode);
    request.set_dev(dev);

    //Trigger RPC Call for GetAttr
    Status status = stub_->Mknod(&context, request, &reply);

    //On Response received
    if (status.ok() && reply.errnum() == 1)
    { 
      return 0;
    }
    else
    {
      if(reply.errnum() != 1)
        errno = reply.errnum();
      cout << "Error on Mknod() with errno :: "<< errno << endl;
      cout << status.error_code() << ": " << status.error_message() << std::endl;
      return -1;
    }
  }

  int Creat(string path, int flags, mode_t mode)
  {
    // completely local to client, no need for rpc
    path = removeMountPointPrefix(path);
    cout << "Creat called @path with flags and mode =" << path <<", " << flags << "," << mode << endl;
    
    CreatRequest request;
    CreatResponse reply;
    ClientContext context;
    request.set_path(path);
    request.set_mode(mode);
    request.set_flags(flags);

    Status status = stub_->Creat(&context, request, &reply);

    if (status.ok() && reply.errnum() == 1)
    { 
      string absoluteCachePath = m_cacheDir + sha256(path);
      int fd = open(absoluteCachePath.c_str(), flags, mode);
      if (fd == -1)
      {
        return -1;
      }
      fileMap[fd] = path;

      return fd;
    }
    else
    {
      if(reply.errnum() != 1)
        errno = reply.errnum();
      cout << "Error on Creat() with errno :: "<< errno << endl;
      cout << status.error_code() << ": " << status.error_message() << std::endl;
      return -1;
    }
  }

  int Unlink (string path)
  {
    path = removeMountPointPrefix(path);
    cout << "Unlink called @path " << path << endl;
    UnlinkRequest request;
    UnlinkResponse reply;
    ClientContext context;
    request.set_path(path);

    Status status = stub_->Unlink(&context, request, &reply);
    if (status.ok() && reply.errnum() == 1)
    { 
      // TODO: erase local cache copy
      return 0;
    }
    else
    {
      if(reply.errnum() != 1)
        errno = reply.errnum();
      cout << "Error on Unlink() with errno :: "<< errno << endl;
      cout << status.error_code() << ": " << status.error_message() << std::endl;
      return -1;
    }
  }
};

AfsClientSingleton *AfsClientSingleton ::instancePtr = NULL;

extern "C" int afsGetAttr(const char *path, struct stat *buf)
{
  AfsClientSingleton *afsClient = AfsClientSingleton::getInstance(std::string(serverNodePort));
  return afsClient->GetAttr(string(path), buf);
}

extern "C" int afsOpen(const char *path, int flags)
{
  AfsClientSingleton *afsClient = AfsClientSingleton::getInstance(std::string(serverNodePort));
  return afsClient->Open(string(path), flags);
}


extern "C" int afsClose(int fh)
{
  AfsClientSingleton *afsClient = AfsClientSingleton::getInstance(std::string(serverNodePort));
  return afsClient->Close(fh);
}

extern "C" int afsMkdir(const char* path, int flags)
{
  AfsClientSingleton *afsClient = AfsClientSingleton::getInstance(std::string(serverNodePort));
  return afsClient->Mkdir(string(path), flags);  
}

extern "C" int afsRmdir(const char* path)
{
  AfsClientSingleton *afsClient = AfsClientSingleton::getInstance(std::string(serverNodePort));
  return afsClient->Rmdir(string(path));  
}

extern "C" int64_t afsOpendir(const char* path)
{
  AfsClientSingleton *afsClient = AfsClientSingleton::getInstance(std::string(serverNodePort));
  return afsClient->Opendir(string(path));  
}

extern "C" int afsReleasedir(int64_t fh)
{
  AfsClientSingleton *afsClient = AfsClientSingleton::getInstance(std::string(serverNodePort));
  return afsClient->Releasedir(fh);  
}

extern "C" int afsTruncate(const char *path, int len)
{
  AfsClientSingleton *afsClient = AfsClientSingleton::getInstance(std::string(serverNodePort));
  return afsClient->Truncate(string(path), len);
}

extern "C" int afsMknod(const char *path, mode_t mode, dev_t dev)
{
  AfsClientSingleton *afsClient = AfsClientSingleton::getInstance(std::string(serverNodePort));
  return afsClient->Mknod(string(path), mode, dev);
}

extern "C" int64_t afsRename(const char* oldPath, const char* newPath)
{
  AfsClientSingleton *afsClient = AfsClientSingleton::getInstance(std::string(serverNodePort));
  return afsClient->Rename(string(oldPath),string(newPath));  
}

extern "C" int afsReaddir(int64_t dp, int *sz, char ***dnames)
{
  AfsClientSingleton *afsClient = AfsClientSingleton::getInstance(std::string(serverNodePort));
  return afsClient->Readdir(dp, sz, dnames);
}

extern "C" int afsCreat(const char *path, int flags, mode_t mode)
{
  AfsClientSingleton *afsClient = AfsClientSingleton::getInstance(std::string(serverNodePort));
  return afsClient->Creat(string(path), flags, mode);
}

extern "C" int afsUnlink(const char *path)
{
  AfsClientSingleton *afsClient = AfsClientSingleton::getInstance(std::string(serverNodePort));
  return afsClient->Unlink(string(path));
}