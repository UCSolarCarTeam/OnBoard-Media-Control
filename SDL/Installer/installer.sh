#!/bin/bash

echo "This script will download and install all dependencies for the Raspberry Pi running Raspbian (Debian)"

USER=`whoami`
if [ $USER = "root" ]
	then
	echo "Running as ROOT"
else
	echo "***********YOU ARE NOT ROOT************" 
	exit 0
fi

#OpenCV	(grabbing camera frames)
#ao.h 	(playing music)
#mpg123 (loading music)
#g++ 	(compiling)
apt-get install libopencv-dev
apt-get install libao-dev
apt-get install g++
apt-get install libmpg123-dev

#SDL stuff

#removes all old folders of SDL2
ls | grep SDL2 | sudo xargs rm -r

#SDL2.0
wget https://www.libsdl.org/release/SDL2-2.0.3.tar.gz
tar -xzvf SDL2-2.0.3.tar.gz
cd SDL2-2.0.3
./configure --host=armv71-raspberry-linux-gnueabihf
make -j4 
make install -j4 
cd ..

#SDL2_ttf
wget https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-2.0.12.tar.gz 
tar -xzvf SDL2_ttf-2.0.12.tar.gz 
cd SDL2_ttf-2.0.12 
./configure 
make -j4 
make install -j4 
cd ..

#SDL2_image
wget https://www.libsdl.org/projects/SDL_image/release/SDL2_image-2.0.0.tar.gz
tar -xzvf SDL2_image-2.0.0.tar.gz
cd SDL2_image-2.0.0 
./configure
make -j4 
make install -j4 
cd ..

