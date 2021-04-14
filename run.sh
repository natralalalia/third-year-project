#!/bin/sh
xhost +
docker run -m 6g --cpus="4" -v "$PWD/.bash_history:/root/.bash_history" -v "/home/natalia/spiNNaker/.spynnaker.cfg:/root/.spynnaker.cfg" -v "$PWD/files:/root/files" --env="DISPLAY" --volume="/tmp/.X11-unix:/tmp/.X11-unix" --network=host -it natralalalia/spinnaker-dev-env bash
