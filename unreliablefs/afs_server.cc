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

#ifdef BAZEL_BUILD
#include "examples/protos/helloworld.grpc.pb.h"
#else
#include "afs.grpc.pb.h"
#endif

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
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
using namespace std;

string serverBaseDir("/home/girish/serverfs/");

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
      reply->set_status(errno);
	    return Status::OK;
    }

    //Send File Attributes Data
    reply->set_statbuf((char *)&buf,sizeof(struct stat));
    reply->set_status(1);
    return Status::OK;
  }
  
  Status Open(ServerContext* context, const OpenRequest* request,
                  OpenResponse* reply) override {
    
    string path = serverBaseDir + request->path();
    std::cout<< "Open Got Called with path:: "<< path << std::endl;

    //Open File 
    int fd = open(path.c_str(), request->flags());
    if (fd == -1) {
      //return error status on failure
      reply->set_status(-1);
	    return Status::OK;
    }

    //Get File Attributes
    struct stat statBuf;
    int ret = lstat(path.c_str(), &statBuf);
    if (ret == -1)
    {
      //return error status on failure
      reply->set_status(-1);
	    return Status::OK;
    }

    //Read File
    char *buf = new char[statBuf.st_size];
    ret = pread(fd, buf, statBuf.st_size, 0);
    if (ret == -1) {
      //return error status on failure
      reply->set_status(-1);
	    return Status::OK;
    }

    //Send File Data
    reply->set_filedata(buf, statBuf.st_size);
    std::cout<<  reply->filedata()<< " is being returned" << std::endl;
    reply->set_status(1);
    
    delete[] buf;
    close(fd);
    return Status::OK;
  }

  Status Close(ServerContext* context, const CloseRequest* request,
                  CloseResponse* reply) override {
    
    string path = serverBaseDir + request->path();
    std::cout<< "Close Got Called with path:: "<< path << std::endl;

    auto microseconds_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    //time_t seconds = time (NULL);
    string tempPath(path + std::to_string(microseconds_since_epoch));
    //cout << "Creating temp file " << tempPath << endl; 
    //Open File 
    int fd = open(tempPath.c_str(), O_CREAT | O_RDWR, 0777);
    //cout << "Creating temp file status :: " << fd << endl; 
    if (fd == -1) {
      //return error status on failure
      reply->set_status(-1);
	    return Status::OK;
    }

    //cout << "Writing the following data::  " << request->filedata() << endl; 
    //Write File
    int ret = pwrite(fd, request->filedata().c_str(), request->filedata().size(), 0);
    //cout << "write status::  " << ret << endl; 
    if (ret == -1) {
      //return error status on failure
      reply->set_status(-1);
	    return Status::OK;
    }
    fsync(fd);

    //cout << "renaming the file::  " << path.c_str() << endl; 
    if(rename(tempPath.c_str(), path.c_str()) == -1){
      //return error status on failure
      cout << errno << endl;
      cout << "renaming failed " << endl; 
      reply->set_status(-1);
	    return Status::OK;
    }

    
    close(fd);
    cout << "close successful " << endl; 
    //Send File Data
    reply->set_status(1);
    
    return Status::OK;
  }

   Status Mkdir(ServerContext* context, const MkdirRequest* request, MkdirResponse* response) override
  {
    string path = serverBaseDir + request->path();
    std::cout<< "Mkdir Got Called with path:: "<< path << std::endl;
    int ret = mkdir(path.c_str(), request->mode());

    if (ret == 0 || (ret == -1 && errno == EEXIST))
    {
      response->set_status(1);
      return Status::OK;
    }

    response->set_status(errno);
    return Status::OK;
  }

  Status Opendir(ServerContext* context, const OpendirRequest* request, OpendirResponse* response) override
  {
    string path = serverBaseDir + request->path();
    std::cout<< "Opendir Got Called with path:: "<< path << std::endl;

    DIR *dir = opendir(path.c_str());
    if (!dir) {
        response->set_status(errno);
        return Status::OK;
    }

    int64_t val = (int64_t) dir;
    std::cout<< "Opendir returning filehandle value:: "<< val << std::endl;
    //std::cout<< "Opendir returning value original "<< dir << std::endl;

    response->set_status(1);
    response->set_fh(val);
    return Status::OK;
  }


  Status Rmdir(ServerContext* context, const RmdirRequest* request, RmdirResponse* response) override
  {
    string path = serverBaseDir + request->path();
    std::cout<< "RemoveDir Got Called with path:: "<< path << std::endl;
    int ret = rmdir(path.c_str()); 
    if (ret == -1) {
        response->set_status(errno);
        return Status::OK;
    }

    response->set_status(1);
    return Status::OK;
  }

  Status Releasedir(ServerContext* context, const ReleasedirRequest* request, ReleasedirResponse* response) override
  {
    std::cout<< "Releasedir Got Called with filehandle:: "<< request->fh() << std::endl;

    DIR *dir = (DIR *) request->fh();
    int ret = closedir(dir);
    if (ret == -1) {
        response->set_status(errno);
        return Status::OK;
    }

    response->set_status(1);
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
