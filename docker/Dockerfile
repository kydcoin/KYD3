FROM ubuntu:18.04

RUN apt update -y
RUN apt upgrade -y
RUN apt install git make build-essential libtool autotools-dev automake pkg-config bsdmainutils curl git g++-mingw-w64-x86-64 g++-mingw-w64-i686 mingw-w64-i686-dev -y
RUN apt install software-properties-common -y && add-apt-repository ppa:bitcoin/bitcoin -y &&  apt-get update && apt-get install libdb4.8-dev libdb4.8++-dev -y
CMD update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix # Set the default mingw32 g++ compiler option to posix.
CMD update-alternatives --set i686-w64-mingw32-g++ /usr/bin/i686-w64-mingw32-g++ # Set the default mingw32 g++ compiler option to posix.
CMD cd /KYD3
ENTRYPOINT /bin/bash

