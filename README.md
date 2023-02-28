# Installing RAG-AFS
## Install GRPC C++
Please follow the steps below:
https://grpc.io/docs/languages/cpp/quickstart/
Run the following commands to set the GRPC local installation to $PATH.
```sh
export  MY_INSTALL_DIR=$HOME/.local
export  PATH="$MY_INSTALL_DIR/bin:$PATH"
```

## Prerequisites for unreliable FS and RAG AFS:

- CentOS: `dnf install -y gcc -y cmake fuse fuse-devel`
- Ubuntu: `apt-get install -y gcc cmake fuse libfuse-dev`
- FreeBSD: `pkg install gcc cmake fusefs-libs pkgconf`
- OpenBSD: `pkg_add cmake`
- macOS: `brew install --cask osxfuse`
- Install openssl - `sudo apt-get install libssl-dev`

## Building the project
```sh
$ cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
$ cmake --build build --parallel
```
## Running the AFS server:

```sh
./afs_server <server directory subtree exposed to the clients>
```
Example:
```sh
./afs_server /home/araghavan/ragAFS
```

## Running the AFS client:
Update line #82 `char serverNodePort[] = "localhost:50051";` with the nodePort of the server. Default fallback - `localhost:50051`
Our AFS client runs on top of UnreliableFS which itself runs on top of FUSE.
Please issue the following commands:
```sh
cd build/unreliablefs
./unreliablefs <mount_point> -f -basedir=<cache_dir>
```
- mount_point refers to the location (in the directory structure) where AFS should be mounted
- cache_dir refers to the location that AFS should use for caching.
For example,
```sh
./unreliablefs /users/Girish/fusemnt/ -f -basedir=/users/Girish/afscache/
```

## Injecting errors:
See unreliable FS documentation in [unreliablefs.1](https://ligurio.github.io/unreliablefs/unreliablefs.1.html) and [unreliablefs.conf.5](https://ligurio.github.io/unreliablefs/unreliablefs.conf.5.html).

## Who's the winner?
For extracting information out of our system we log appropriate data from clients and server.
Server logs are written to a file `serverevents` at $CWD. 
Client logs are written to the file `rpctimes` at client's $CWD. 
Run the following command simultaneously in two different nodes to issue concurrent requests to the server.
```sh
python3 winner.py
```
Place the parse_server_logs.py script in the same directory as the server logs and run the following command to extract a summary from the logs. 
```sh
python3 parse_server_logs.py
```