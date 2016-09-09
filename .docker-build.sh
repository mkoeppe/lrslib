#! /bin/bash
set -e
yum install -y gmp-devel gcc gcc-c++ boost boost-devel boost-system boost-thread #automake autoconf libtool
mkdir -p /build
cd /build
/src/configure --srcdir=/src --enable-plrs  || ( echo '#### Contents of config.log: ####'; cat config.log; exit 1)
make -j2 && make check
