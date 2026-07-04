FROM debian:latest@sha256:d07d1b51c39f51188e60be9b64e6bf769fa94e187f092bc32b91305cfa34ba5a

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
    python3-pip \
    python3-setuptools \
    texinfo \
   && rm -rf /var/lib/apt/lists/*

# compile and install liblouis
ADD . /usr/src/liblouis
WORKDIR /usr/src/liblouis
RUN ./autogen.sh && ./configure --enable-ucs4 && make && make install && ldconfig

# install python bindings
WORKDIR /usr/src/liblouis/python
RUN pip install .

# clean up
WORKDIR /root
RUN rm -rf /usr/src/liblouis
