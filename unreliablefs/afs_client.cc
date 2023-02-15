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

#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "examples/protos/helloworld.grpc.pb.h"
#else
#include "afs.grpc.pb.h"
#endif

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using FS::AFS;
using FS::GetAttrRequest;
using FS::GetAttrResponse;

using namespace std;

class AfsClientSingleton{

private:  
  std::unique_ptr<AFS::Stub> stub_;
  static AfsClientSingleton* instancePtr;

  AfsClientSingleton()
  {
  }

  AfsClientSingleton(std::shared_ptr<Channel> channel)
      : stub_(AFS::NewStub(channel)) {}
   
public:
  AfsClientSingleton(const AfsClientSingleton& obj) = delete;
 
  static AfsClientSingleton* getInstance(std::string serverIP)
  {
    if (instancePtr == nullptr)
    {
      instancePtr = new AfsClientSingleton(grpc::CreateChannel(serverIP, grpc::InsecureChannelCredentials()));  
      return instancePtr;
    }
    else
      return instancePtr;
  }

   int GetAttr(std::string path, struct stat* buf) {
    GetAttrRequest request;
    request.set_path(path);
    
    GetAttrResponse reply;
    
    ClientContext context;
    Status status = stub_->GetAttr(&context, request, &reply);
    
    if (status.ok()) 
    {
      memcpy(buf, reply.statbuf, sizeof(struct stat));
      return 1;
    } 
    else {
      cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return 0;
    }
  }
  
};

AfsClientSingleton* AfsClientSingleton ::instancePtr = NULL;

extern "C" void afsGetAttr(char* path, struct stat *buf)
{
  AfsClientSingleton *afsClient = AfsClientSingleton::getInstance(std::string("localhost:50051"));
  afsClient->GetAttr(string(path), buf);
}
