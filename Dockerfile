FROM debian:jessie

MAINTAINER Liblouis Maintainers "liblouis-liblouisxml@freelists.org"

RUN apt-get update
RUN apt-get install -y make autoconf automake libtool pkg-config texinfo python python-pip curl
RUN pip install nose
RUN apt-get install -y libyaml-dev

# liblouis
ADD . /tmp/liblouis
WORKDIR /tmp/liblouis
RUN ./autogen.sh
RUN ./configure --enable-ucs4

# the following two lines are needed in order to remove generated
# files (a better solution would be listing all generated file in
# .dockerignore, like is done in .gitignore)
# RUN make distclean
# RUN ./configure --enable-ucs4

RUN make
RUN make install
RUN ldconfig