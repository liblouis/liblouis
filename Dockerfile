FROM debian:bullseye@sha256:2c407480ad7c98bdc551dbb38b92acb674dc130c8298f2e0fa2ad34da9078637

LABEL maintainer="Liblouis Maintainers <liblouis-liblouisxml@freelists.org>"

# Fetch build dependencies
RUN apt-get update && apt-get -y dist-upgrade && apt-get install -y \
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
   && apt-get clean && rm -rf /var/lib/apt/lists/*

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
