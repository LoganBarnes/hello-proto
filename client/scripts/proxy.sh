#!/usr/bin/env bash

# If SERVER_ADDRESS is not already set, get it from the .env files
if [ -z "$SERVER_ADDRESS" ]; then
  [ -f .env ] && export $(grep -v '^#' .env | xargs)
  [ -f .env.local ] && export $(grep -v '^#' .env.local | xargs)
fi

echo "SERVER_ADDRESS: $SERVER_ADDRESS"

# Split the SERVER_ADDRESS into its host and port values
HOST_PORT_ARRAY=(${SERVER_ADDRESS//:/ })
if [ ${#HOST_PORT_ARRAY[@]} -ne 2 ]; then
  echo "Server address should be of format 'address:port'"
  exit 1
fi

HOST=${HOST_PORT_ARRAY[0]}
PORT=${HOST_PORT_ARRAY[1]}

echo "HOST: $HOST"
echo "PORT: $PORT"

echo "Building proxy image"
docker build \
  -t hello-proto/envoy \
  --build-arg host=$HOST \
  --build-arg port=$PORT \
  -f ./envoy.Dockerfile .

if [ "$(docker ps -q -f name=hello-proxy)" ]; then
  echo "Stopping existing container"
  docker stop hello-proxy
fi

if [ "$(docker ps -aq -f status=exited -f name=hello-proxy)" ]; then
    echo "Removing stopped container"
    docker rm hello-proxy
fi

echo "Running proxy container"
if [ "$HOST" == "localhost" ]; then
    docker run -d -p 8080:8080 --network=host --name hello-proxy hello-proto/envoy
else
    docker run -d -p 8080:8080 --name hello-proxy hello-proto/envoy
fi