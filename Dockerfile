FROM debian:latest@sha256:f2150eba68619015058b26d50e47f9fba81213d1cb81633be7928c830f72d180

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
    python3 \
    python3-distutils \
    texinfo \
   && rm -rf /var/lib/apt/lists/*

# compile and install liblouis
ADD . /usr/src/liblouis
WORKDIR /usr/src/liblouis
RUN ./autogen.sh && ./configure --enable-ucs4 && make && make install && ldconfig

# install python bindings
WORKDIR /usr/src/liblouis/python
RUN python3 setup.py install

# clean up
WORKDIR /root
RUN rm -rf /usr/src/liblouis
