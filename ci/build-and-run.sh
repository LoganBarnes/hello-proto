#!/usr/bin/env bash

set -e # fail script if any individual commands fail

# set cmake path and test version
export PATH=/deps/cmake/bin:$PATH
cmake --version

# install deps
apt update
apt install -y apt-transport-https

# yarn repo
curl -sS https://dl.yarnpkg.com/debian/pubkey.gpg | apt-key add -
echo "deb https://dl.yarnpkg.com/debian/ stable main" | tee /etc/apt/sources.list.d/yarn.list

# node update
curl -sL https://deb.nodesource.com/setup_10.x | sudo -E bash -

apt update
apt install -y lcov nodejs yarn

# build server
pushd server
function build_and_run {
    cmake -E make_directory $1
    cmake -E chdir $1 cmake -DCMAKE_BUILD_TYPE=$2 -DMCS_USE_DEV_FLAGS=ON ..
    cmake -E chdir $1 cmake --build . --parallel
}

build_and_run cmake-build-debug Debug
build_and_run cmake-build-release Release
popd

# build client
pushd client
yarn
yarn generate
yarn build
popd
