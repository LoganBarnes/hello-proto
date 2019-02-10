# hello-proto

## Dependencies

### Client:

- [Yarn](https://yarnpkg.com/lang/en/docs/install)
- [Docker](https://docs.docker.com/install/)

### Server:

- [CMake](https://cmake.org/download/)
- [gRPC](https://github.com/grpc/grpc/blob/master/BUILDING.md)
- A C++ build system (platform dependent)
  - Tested with recent versions of
    - GCC (Linux)
    - Clang (OS X Xcode)
    - ~~MSVC (Windows VisualStudio)~~ (maybe later)

The first two server dependencies are available via homebrew if using **OS X**:

```
brew install cmake
brew tap grpc/grpc
brew install grpc
```

## Server

### Project setup

(From inside the `server` directory)

```bash
cmake -E make_directory build        # create build dir
cmake -E chdir build cmake ..        # configure project
cmake -E chdir build cmake --build . # build project
```

#### Run server (CTRL + C to quit)

```bash
# The address argument is optional and defaults to 0.0.0.0:9090
./build/bin/hello_server 0.0.0.0:50055
```

## Client

### Project setup

(From inside the `client` directory)

Note for **OS X** users. Because of Docker-y things you will probably have to set your [network](#network-connections) environment first using your network IP address (as opposed to localhost).

```bash
yarn           # install dependencies
yarn generate  # generate protobuf files
yarn proxy     # setup grpc-web proxy server
```

#### Compiles and hot-reloads for development

```bash
yarn serve
```

#### Compiles and minifies for production

```bash
yarn build
```

#### Formats files

```bash
yarn format
```

#### Lints and fixes files

```bash
yarn lint
```

### Network connections

The default parameters are all set to handle clients and servers running on the same machine (localhost). If you want to connect to external servers you can create a file `.env.local` with the variable `SERVER_ADDRESS` set to the address you want to connect to:

```bash
#file: .env.local
SERVER_ADDRESS="195.162.0.27:50055" # <- replace with server address

```

(**OS X** users will probably have to set the above with their network IP address when working locally)

If you want other computers on your network to be able to connect to your dev server to view the web client you can also set the `VUE_APP_PROXY_ADDRESS` variable to your local network IP address:

```bash
#file: .env.local
SERVER_ADDRESS="195.162.0.27:50055" # <- replace with server address and port
VUE_APP_PROXY_ADDRESS="195.162.0.22:8080" # <- Local network IP w/ 8080 port

```

On Unix systems your local IP address can be found using this SO [answer](https://stackoverflow.com/a/13322549/4832835):

```bash
>$ ifconfig | sed -En 's/127.0.0.1//;s/.*inet (addr:)?(([0-9]*\.){3}[0-9]*).*/\2/p'
195.162.0.22
```

(This should also be the address you see after running `yarn serve`)

## All together now!

(From the root directory)

```bash
# move to server directory
pushd server

# build and run server
cmake -E make_directory build        && \
cmake -E chdir build cmake ..        && \
cmake -E chdir build cmake --build . && \
./build/bin/hello_server &

# move to client directory
popd
cd client

# build and run client
yarn          && \
yarn generate && \
yarn proxy    && \
yarn serve

```
