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
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using namespace std;

char *mountPoint;
char *cacheDir;

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

  AfsClientSingleton()
  {
  }

  

  AfsClientSingleton(std::shared_ptr<Channel> channel) : stub_(AFS::NewStub(channel))
  {
     m_mountPoint= mountPoint;
     m_cacheDir="/home/girish/afscache/";
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


  int GetAttr(std::string path, struct stat *buf)
  {
    GetAttrRequest request;
    GetAttrResponse reply;
    ClientContext context;

    //Setting request parameters
    path = removeMountPointPrefix(path);
    request.set_path(path);

    //Trigger RPC Call for GetAttr
    Status status = stub_->GetAttr(&context, request, &reply);

    //On Response received
    if (status.ok() && reply.status() == 1)
    {
      memcpy((char *)buf, reply.statbuf().c_str(), sizeof(struct stat));
      return 1;
    }
    else
    {
      cout << status.error_code() << ": " << status.error_message() << std::endl;
      return -1;
    }
  }

  string removeMountPointPrefix(string inputPath)
  {
   
    string s_inputPath = filesystem::absolute(filesystem::path(inputPath)).string();
    if(s_inputPath.find(m_cacheDir) != string::npos){
      return s_inputPath.substr(m_cacheDir.size());
    }
    return inputPath;
  }

  int Open(std::string path, int flags)
  {
    path = removeMountPointPrefix(path);
    string absoluteCachePath = m_cacheDir + sha256(path);
    int fd = open(absoluteCachePath.c_str(), O_RDONLY);
    if (fd != -1)
    {
      // file in local cache, check if stale
      struct stat statBuf, lstatBuf;

      // Call getAttr to check if local file data is not stale
      GetAttr(path, &statBuf);
      memset(&lstatBuf, 0, sizeof(struct stat));
      if (lstat(absoluteCachePath.c_str(), &lstatBuf) == -1)
      {
        //return error status on failure
        // never reaches here!
        return -errno;
      }

      // cache not stale
      if ((statBuf.st_mtim.tv_sec < lstatBuf.st_mtim.tv_sec) || (statBuf.st_mtim.tv_sec == lstatBuf.st_mtim.tv_sec && statBuf.st_mtim.tv_nsec < lstatBuf.st_mtim.tv_nsec)){
        fileMap[fd] = path;
        return fd;
      }
    }

    OpenRequest request;
    OpenResponse reply;
    ClientContext context;

    request.set_path(path);
    request.set_flags(flags);
    Status status = stub_->Open(&context, request, &reply);
    cout << "calling Open with the following path :: " << request.path()<< endl;
    if (status.ok() && reply.status() == 1)
    {
      //Save the data in a new file and return file descriptor
      int fd1 = open(absoluteCachePath.c_str(), O_WRONLY| O_CREAT,0777);
      if (fd1 == -1)
      {
        return -errno;
      }
      int ret = pwrite(fd1, reply.filedata().c_str(), reply.filedata().size(), 0);
      if (ret == -1)
      {
        ret = -errno;
      }
      fsync(fd1);
      fileMap[fd1] = path;
      cout << "created file on client with the following name :: " << absoluteCachePath << endl;
      return fd1;
    }
    else
    {
      cout << "Some Error on AfsOpen()" <<endl;
      cout << status.error_code() << ": " << status.error_message() << std::endl;
      return -1;
    }
  }

  int Close(int fh){
    CloseRequest request;
    CloseResponse reply;
    ClientContext context;
    std::string serverPath;
    
    
    if (fileMap.find(fh) == fileMap.end())
        return -1;
    else
        serverPath = fileMap[fh];
    
    string absoluteCachePath = m_cacheDir + sha256(serverPath);
    //Get File Attributes
    struct stat statBuf;
    if (lstat(absoluteCachePath.c_str(), &statBuf) == -1)
    {
      //return error status on failure
      return -1;
    }
    //cout << "lstat called on local client and fileinode num is :: "<< statBuf.st_ino <<endl;

    //Read File
    char *buf = new char[statBuf.st_size];
    if (pread(fh, buf, statBuf.st_size, 0) == -1) {
      //return error status on failure
      return -1;
    }
    cout << "file reading is complete on client :: "<< buf <<endl;
    //Send File Data
    request.set_path(serverPath);
    request.set_filedata(buf, statBuf.st_size);
    cout << "Calling AfsClose with the following serverPath :: "<< request.path() <<endl;
    std::cout<<  request.filedata()<< " is being sent" << std::endl;
    Status status = stub_->Close(&context, request, &reply);
      
    if (status.ok() && reply.status() == 1){
      return 1;
    }
    else{
        cout << "Some Error on AfsOpen()" <<endl;
        cout << status.error_code() << ": " << status.error_message() << std::endl;
        return -1;
    }
  }
};

AfsClientSingleton *AfsClientSingleton ::instancePtr = NULL;

extern "C" int afsGetAttr(const char *path, struct stat *buf)
{
  AfsClientSingleton *afsClient = AfsClientSingleton::getInstance(std::string("localhost:50051"));
  return afsClient->GetAttr(string(path), buf);
}

extern "C" int afsOpen(const char *path, int flags)
{
  AfsClientSingleton *afsClient = AfsClientSingleton::getInstance(std::string("localhost:50051"));
  return afsClient->Open(string(path), flags);
}


extern "C" int afsClose(int fh)
{
  AfsClientSingleton *afsClient = AfsClientSingleton::getInstance(std::string("localhost:50051"));
  return afsClient->Close(fh);
}
