#! /bin/bash
set -e
yum install -y gmp-devel gcc boost #automake autoconf libtool
mkdir build
cd build
/src/configure --srcdir=/src && make -j2 && make check
