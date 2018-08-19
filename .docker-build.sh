#! /bin/bash
set -e
yum install -y gmp-devel gcc gcc-c++ boost boost-devel boost-system boost-thread || ( apt-get update && apt-get -y install g++ libgmp-dev libboost-dev libboost-system-dev libboost-thread-dev autoconf automake libtool make)
mkdir -p /build
cd /build
/src/configure --srcdir=/src --enable-plrs || ( echo '#### Contents of config.log: ####'; cat config.log; exit 1)
make -j2 && make check
