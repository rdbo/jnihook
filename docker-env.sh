#!/bin/sh

docker build -t jnihook .
docker run -v "$(pwd):/app" -it jnihook
