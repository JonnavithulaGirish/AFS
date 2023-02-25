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
#include <memory>
#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <time.h>
#include <chrono>
#include <errno.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <mutex>
#include <fstream>

#ifdef BAZEL_BUILD
#include "examples/protos/helloworld.grpc.pb.h"
#else
#include "afs.grpc.pb.h"
#endif

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::ServerReader;
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

using namespace std;

mutex mtx;
mutex clsmtx;

string consistency_winner_file = "winner";

string serverBaseDir("/users/Girish/serverfs/");

// Logic and data behind the server's behavior.
class AfsServiceImpl final : public AFS::Service {
  Status GetAttr(ServerContext* context, const GetAttrRequest* request,
                  GetAttrResponse* reply) override {

    string path = serverBaseDir + request->path();
    std::cout<< "GetAttr Got Called with path:: "<< path <<std::endl;
    struct stat buf;

    //Get File Attributes
    int ret= lstat(path.c_str(), &buf);
    if ( ret == -1)
    {
      //return error status on failure
      reply->set_errnum(errno);
	    return Status::OK;
    }

    //Send File Attributes Data
    reply->set_statbuf((char *)&buf,sizeof(struct stat));
    reply->set_errnum(1);
    return Status::OK;
  }

  Status Open(ServerContext* context, const OpenRequest* request, ServerWriter<OpenResponse>* writer) override {
    string path = serverBaseDir + request->path();
    std::cout<< "Open Got Called with path:: "<< path <<std::endl;
    mtx.lock();
    cout<< "acquired lock "<<endl;
    int fd = open(path.c_str(), O_CREAT|O_RDWR, 0777);

    cout<<"Open Flags : "<<request->flags()<<endl;
    cout<< "open suss fd "<< fd << endl;
    struct stat statbuf;
    int ret = lstat(path.c_str(), &statbuf);
    if (ret == -1)
    {
      cout << "Open:: lstat failed, errno - " << errno << endl;
      OpenResponse reply;
      reply.set_errnum(errno);
      writer->Write(reply);
      mtx.unlock();
      return Status::OK;
    }
    cout<< "open:: lstat res::  "<<ret<<endl;

    unsigned int filesz = statbuf.st_size;
    int chunksz = 4000;
    int offset = 0;
    ret = 1;
    while (ret)
    {
      char chunk[chunksz];
      ret = pread(fd, chunk, chunksz, offset);
      cout<< "open:: ret val::  "<< ret <<endl;
      cout<< "open:: data sent:: " <<chunk << endl; 
      if (ret == -1)
      {
        cout << "Open:: pread failed, errno - " << errno << endl;  
        OpenResponse reply;
        reply.set_errnum(errno);
        writer->Write(reply);
        mtx.unlock();
        return Status::OK;
      }
      offset += ret;
      OpenResponse reply;
      reply.set_errnum(1);
      reply.set_filedata((char *)chunk, ret);
      writer->Write(reply);

    }
    fsync(fd);
    close(fd);

    mtx.unlock();
    cout<< "released lock "<<endl;

    return Status::OK;
  }

  Status Close(ServerContext* context, ServerReader<CloseRequest>* reader, CloseResponse* reply) override
  {
      CloseRequest request;
      int flag = 1;
      int offset = 0;
      int fd;
      string suffix;

      char clientid = '*';

     std::cout<< "Close Got Called with path:: " <<std::endl;
      ofstream logEvents("~/logs/serverevents", fstream::app);
      auto microseconds_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
      //time_t seconds = time (NULL);
      string originalPath,tempPath;
      while (reader->Read(&request)) {
        if(flag == 1){
          flag = 0;
          originalPath = (serverBaseDir+request.path()).c_str();
          tempPath = originalPath+to_string(microseconds_since_epoch);
          fd = open(tempPath.c_str(), O_CREAT|O_RDWR, 0777);
          if(fd == -1){
            cout<<"Close :: open failed"<<endl;
            reply->set_errnum(errno);
            return Status::OK;   
          }

          if (request.path() == consistency_winner_file)
          {
            microseconds_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            if (request.filedata().size() > 0)
              clientid = request.filedata()[0];
            mtx.lock();
            logEvents << microseconds_since_epoch << " 1 " << clientid << endl; 
            mtx.unlock();
          }
        }
        //cout << "close got this filedata:: "<< request.filedata().c_str() << endl;
        int ret = pwrite(fd, request.filedata().c_str(), request.filedata().size(), offset);
        if(ret == -1){
          cout << "close:: pwrite failed, errno - " << errno;
          reply->set_errnum(errno);
          unlink(tempPath.c_str());
          return Status::OK;   
        }
        offset+=ret;
      }
      // string tempPath = originalPath+to_string(microseconds_since_epoch);
      mtx.lock();

      fsync(fd);

      cout << "renaming the file::  " << originalPath.c_str() << endl; 
      if(rename(tempPath.c_str(), originalPath.c_str()) == -1){
        //return error status on failure
        cout << "renaming failed " << originalPath << tempPath << endl; 
        reply->set_errnum(errno);
        return Status::OK;
      }

  
      close(fd);

      if (request.path() == consistency_winner_file)
      {
        microseconds_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        logEvents << microseconds_since_epoch << " 2 " << clientid << endl;
      }
      cout << "close successful " << endl; 
      //Send File Data
      reply->set_errnum(1);  
      mtx.unlock();

      logEvents.close();
      
      return Status::OK;    
  }


  
  // Status Open(ServerContext* context, const OpenRequest* request,
  //                 OpenResponse* reply) override {
    
  //   string path = serverBaseDir + request->path();
  //   std::cout<< "Open Got Called with path:: "<< path << std::endl;

  //   //Open File 
  //   int fd = open(path.c_str(), request->flags());
  //   if (fd == -1) {
  //     //return error status on failure
  //     cout<< "a" << endl;
  //     reply->set_errnum(errno);
	//     return Status::OK;
  //   }

  //   //Get File Attributes
  //   struct stat statBuf;
  //   int ret = lstat(path.c_str(), &statBuf);
  //   if (ret == -1)
  //   {
  //     cout<< "b" << endl;
  //     //return error status on failure
  //     reply->set_errnum(errno);
	//     return Status::OK;
  //   }

  //   //Read File
  //   char *buf = new char[statBuf.st_size];
  //   ret = pread(fd, buf, statBuf.st_size, 0);
  //   if (ret == -1) {
  //     cout<< "c" << endl;
  //     //return error status on failure
  //     reply->set_errnum(errno);
	//     return Status::OK;
  //   }
  //   fsync(fd);
  //   close(fd);

  //   //Send File Data
  //   reply->set_filedata(buf, statBuf.st_size);
  //   std::cout<<  reply->filedata()<< " is being returned" << std::endl;
  //   reply->set_errnum(1);
    
  //   delete[] buf;
  //   return Status::OK;
  // }

  Status Creat(ServerContext* context, const CreatRequest* request, CreatResponse* reply) override {
    string path = serverBaseDir + request->path();
    std::cout<< "Creat Got Called with path, mode, flag:: " << path << ", " << request->mode() << ", " << request->flags() << std::endl;
    int fd = open(path.c_str(), request->flags(), 0777);
    if (fd == -1) {
      //return error status on failure
      cout<< "a" << endl;
      reply->set_errnum(errno);
	    return Status::OK;
    }
    fsync(fd);
    close(fd);
    reply->set_errnum(1);
    return Status::OK;
  }

  // Status Close(ServerContext* context, const CloseRequest* request,
                  // CloseResponse* reply) override {
    
    // string path = serverBaseDir + request->path();
    // std::cout<< "Close Got Called with path:: "<< path << std::endl;

    // auto microseconds_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    // //time_t seconds = time (NULL);
    // string tempPath(path + std::to_string(microseconds_since_epoch));
    // //cout << "Creating temp file " << tempPath << endl; 
    // //Open File 
    // int fd = open(tempPath.c_str(), O_CREAT | O_RDWR, 0777);
    // //cout << "Creating temp file status :: " << fd << endl; 
    // if (fd == -1) {
    //   //return error status on failure
    //   reply->set_errnum(errno);
	  //   return Status::OK;
    // }

    // //cout << "Writing the following data::  " << request->filedata() << endl; 
    // //Write File
    // int ret = pwrite(fd, request->filedata().c_str(), request->filedata().size(), 0);
    //cout << "write status::  " << ret << endl; 
  //   if (ret == -1) {
  //     //return error status on failure
  //     reply->set_errnum(errno);
	//     return Status::OK;
  //   }
  //   fsync(fd);

  //   //cout << "renaming the file::  " << path.c_str() << endl; 
  //   if(rename(tempPath.c_str(), path.c_str()) == -1){
  //     //return error status on failure
  //     cout << "renaming failed " << endl; 
  //     reply->set_errnum(errno);
	//     return Status::OK;
  //   }

    
  //   close(fd);
  //   cout << "close successful " << endl; 
  //   //Send File Data
  //   reply->set_errnum(1);
    
  //   return Status::OK;
  // }

   Status Mkdir(ServerContext* context, const MkdirRequest* request, MkdirResponse* response) override
  {
    string path = serverBaseDir + request->path();
    std::cout<< "Mkdir Got Called with path:: "<< path << std::endl;
    int ret = mkdir(path.c_str(), request->mode());

    if (ret == 0 || (ret == -1 && errno == EEXIST))
    {
      response->set_errnum(1);
      return Status::OK;
    }

    response->set_errnum(errno);
    return Status::OK;
  }

  Status Opendir(ServerContext* context, const OpendirRequest* request, OpendirResponse* response) override
  {
    string path = serverBaseDir + request->path();
    std::cout<< "Opendir Got Called with path:: "<< path << std::endl;

    DIR *dir = opendir(path.c_str());
    if (!dir) {
        response->set_errnum(errno);
        return Status::OK;
    }

    int64_t val = (int64_t) dir;
    std::cout<< "Opendir returning filehandle value:: "<< val << std::endl;
    //std::cout<< "Opendir returning value original "<< dir << std::endl;

    response->set_errnum(1);
    response->set_fh(val);
    return Status::OK;
  }

  Status Readdir(ServerContext* context, const ReaddirRequest* request, ReaddirResponse* response) override
  {
    std::cout<< "Readdir Got Called with dir ptr:: "<< request->dp() << std::endl;
    
    vector<struct stat> vst;
    vector<string> vdnames;
    struct dirent *de;

    while ((de = readdir((DIR *)request->dp())) != nullptr) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        vdnames.push_back(string(de->d_name));
        vst.push_back(st);
    }
    response->set_numentries(vst.size());
    
    struct stat ast[vst.size()];
    char adnames[vst.size()][256];
    for (int i = 0; i < vst.size(); i++)
    {
      memcpy((char *)&ast[i], (char *)&vst[i], sizeof(struct stat));
      memcpy((char *)&adnames[i], vdnames[i].c_str(), vdnames[i].size());
      adnames[i][vdnames[i].size()] = 0;
    }

    response->set_dirent((char *)ast, vst.size()*sizeof(struct stat));
    response->set_dnames((char *)adnames, vdnames.size()*256);
    return Status::OK;
  }

  Status Rmdir(ServerContext* context, const RmdirRequest* request, RmdirResponse* response) override
  {
    string path = serverBaseDir + request->path();
    std::cout<< "RemoveDir Got Called with path:: "<< path << std::endl;
    int ret = rmdir(path.c_str()); 
    if (ret == -1) {
        response->set_errnum(errno);
        return Status::OK;
    }

    response->set_errnum(1);
    return Status::OK;
  }

  Status Releasedir(ServerContext* context, const ReleasedirRequest* request, ReleasedirResponse* response) override
  {
    std::cout<< "Releasedir Got Called with filehandle:: "<< request->fh() << std::endl;

    DIR *dir = (DIR *) request->fh();
    int ret = closedir(dir);
    if (ret == -1) {
        response->set_errnum(errno);
        return Status::OK;
    }

    response->set_errnum(1);
    return Status::OK;
  }

  Status Truncate(ServerContext* context, const TruncateRequest* request, TruncateResponse* response) override
  {
    string path = serverBaseDir + request->path();
    std::cout<< "Truncate Got Called with path and len :: "<< path << ", " << request->len() << std::endl;

    int ret = truncate(path.c_str(), request->len());
    if (ret == -1)
    {
      response->set_errnum(errno);
      return Status::OK;
    }

    response->set_errnum(1);
    return Status::OK;
  }
  
  Status Mknod(ServerContext* context, const MknodRequest* request, MknodResponse* response) override
  {
    string path = serverBaseDir + request->path();
    std::cout<< "Mknod Got Called with path, mode, dev :: "<< path << ", " << request->mode() << ", " << request->dev() << std::endl;

    int ret = mknod(path.c_str(), request->mode(), request->dev());
    if (ret == -1)
    {
      response->set_errnum(errno);
      return Status::OK;
    }

    response->set_errnum(1);
    return Status::OK;
  }

  Status Rename(ServerContext* context, const RenameRequest* request, RenameResponse* response) override
  {
    string oldPath = serverBaseDir + request->oldpath();
    string newPath = serverBaseDir + request->newpath();
    std::cout<< "Rename Got Called with path:: "<< oldPath <<" " <<newPath<< std::endl;
    // int ret = mkdir(path.c_str(), request->mode());
    int ret = rename(oldPath.c_str(),newPath.c_str());

    if (ret == -1) {
      response->set_status(errno);
      return Status::OK;
    }

    response->set_status(1);
    return Status::OK;
  }

  Status Unlink(ServerContext* context, const UnlinkRequest* request, UnlinkResponse* response) override
  {
    string path = serverBaseDir + request->path();
    std::cout<< "Unlink Got Called with path :: "<< path << std::endl;

    int ret = unlink(path.c_str());
    if (ret == -1)
    {
      response->set_errnum(errno);
      return Status::OK;
    }

    response->set_errnum(1);
    return Status::OK;
  }
};



void RunServer() {
  std::string server_address("0.0.0.0:50051");
  AfsServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  if (argc > 1)
    serverBaseDir = argv[1];
  RunServer();
  return 0;
}
