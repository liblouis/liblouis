FROM debian:latest@sha256:432f545c6ba13b79e2681f4cc4858788b0ab099fc1cca799cc0fae4687c69070

LABEL maintainer="Liblouis Maintainers <liblouis-liblouisxml@freelists.org>"

# Fetch build dependencies
RUN apt-get update && apt-get install -y \
    autoconf \
    automake \
    curl \
    libtool \
    libyaml-dev \
    make \
    pkg-config \
    python \
    texinfo \
   && rm -rf /var/lib/apt/lists/*

# compile and install liblouis
ADD . /usr/src/liblouis
WORKDIR /usr/src/liblouis
RUN ./autogen.sh && ./configure --enable-ucs4 && make && make install && ldconfig

# install python bindings
WORKDIR /usr/src/liblouis/python
RUN python setup.py install

# clean up
WORKDIR /root
RUN rm -rf /usr/src/liblouis
