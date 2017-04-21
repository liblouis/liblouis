FROM debian:jessie

MAINTAINER Liblouis Maintainers "liblouis-liblouisxml@freelists.org"

# developer environment
RUN apt-get update
RUN apt-get install -y make autoconf automake libtool pkg-config

# for python bindings
RUN apt-get install -y python

# for documentation
RUN apt-get install -y texinfo

# for tests
RUN apt-get install -y libyaml-dev

# compile and install liblouis
ADD . /tmp/liblouis
WORKDIR /tmp/liblouis
RUN ./autogen.sh
RUN ./configure --enable-ucs4
RUN make
RUN make install
RUN ldconfig

# install python bindings
WORKDIR /tmp/liblouis/python
RUN python setup.py install

# clean up
WORKDIR /root
RUN rm -rf /tmp/liblouis