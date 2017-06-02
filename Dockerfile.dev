FROM debian:jessie

MAINTAINER Liblouis Maintainers "liblouis-liblouisxml@freelists.org"

# developer environment
RUN apt-get update
RUN apt-get install -y make autoconf automake libtool pkg-config

# for cross-compiling
RUN apt-get install -y mingw32 mingw-w64 libc6-dev-i386

# for running cross-compiled tests
RUN dpkg --add-architecture i386 && apt-get update && apt-get install -y wine32 wine64

# for check_yaml
WORKDIR /root/src/
RUN apt-get install -y curl patch
RUN curl -L https://github.com/yaml/libyaml/tarball/0.1.4 | tar zx
RUN mv yaml-libyaml-4dce9c1 libyaml
WORKDIR /root/src/libyaml
ADD libyaml_mingw.patch /root/src/libyaml/mingw.patch
RUN patch -p1 <mingw.patch
RUN ./bootstrap
RUN ./configure --host i586-mingw32msvc --prefix=/root/build/win32/libyaml
RUN make
RUN make install

ADD . /root/src/liblouis
WORKDIR /root/src/liblouis

# create Makefile
RUN ./autogen.sh
RUN ./configure
