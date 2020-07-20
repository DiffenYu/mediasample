#!/bin/bash -ex
IMAGE_NAME=centos76_mediasample
PORT=9222

docker run -p $PORT:22 \
    -e DISPLAY=$DISPLAY \
    --net=host \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    -v "$HOME/.Xauthority:/root/.Xauthority:rw" \
    --name centos-ocv \
    --privileged \
    -v ${PWD}:${PWD} \
    -v $(dirname $SSH_AUTH_SOCK):$(dirname $SSH_AUTH_SOCK) \
    --env SSH_AUTH_SOCK="${SSH_AUTH_SOCK}" \
    -it --rm $IMAGE_NAME
