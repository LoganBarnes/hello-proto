# hello-proto

## Client

### Project setup

(From inside `client` directory)

```
yarn               # install dependencies
yarn generate      # generate protobuf files
yarn proxy --local # setup grpc-web proxy server
```

#### Compiles and hot-reloads for development

```
yarn serve --port 8081
```

#### Compiles and minifies for production

```
yarn build
```

#### Lints and fixes files

```
yarn lint
```

## Server

### Project setup

(From inside `server` directory)

```
cmake -E make_directory build        # create build dir
cmake -E chdir build cmake ..        # configure project
cmake -E chdir build cmake --build . # build project
```

#### Run server (CTRL + C to quit)

```
./build/hello_server
```

## All together now!

(From the root directory)

```
# move to server directory
pushd server

# build and run server
cmake -E make_directory build        && \
cmake -E chdir build cmake ..        && \
cmake -E chdir build cmake --build . && \
./build/hello_server &

# move to client directory
popd
cd client

# build and run client
yarn               && \
yarn generate      && \
yarn proxy --local && \
yarn serve --port 8081

```

## Dependencies

### Client:

- [yarn](https://yarnpkg.com/lang/en/docs/install)
- [docker](https://docs.docker.com/install/)

### Server:

- [cmake](https://cmake.org/download/)
- [grpc](https://github.com/grpc/grpc/blob/master/BUILDING.md)
- A C++ build system (platform dependent)
  - Tested with recent versions of
    - GCC (Linux)
    - Clang (OSX Xcode)
    - ~~MSVC (Windows VisualStudio)~~ (soon)

The first two are available via homebrew if using **OS X**:

```
brew install cmake
brew tap grpc/grpc
brew install grpc
```
