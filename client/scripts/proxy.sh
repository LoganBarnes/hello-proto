#!/usr/bin/env bash

echo "Building proxy image"
docker build -t hello-proto/envoy -f ./envoy.Dockerfile .

if [ "$(docker ps -q -f name=hello-proxy)" ]; then
  echo "Stopping existing container"
  docker stop hello-proxy
fi

if [ "$(docker ps -aq -f status=exited -f name=hello-proxy)" ]; then
    echo "Removing stopped container"
    docker rm hello-proxy
fi

echo "Running proxy container"
if [ "$1" == "--local" ]; then
    docker run -d -p 8080:8080 --network=host --name hello-proxy hello-proto/envoy
else
    docker run -d -p 8080:8080 --name hello-proxy hello-proto/envoy
fi