FROM debian:latest@sha256:34cd9e9fd437c0a095ec39cb2e73422c9f30821b0d0848ed74fd0d43bae4d958

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
    python3-venv \
    texinfo \
   && rm -rf /var/lib/apt/lists/*

# compile and install liblouis
ADD . /usr/src/liblouis
WORKDIR /usr/src/liblouis
RUN ./autogen.sh && ./configure --enable-ucs4 && make && make install && ldconfig

# install python bindings into an isolated venv, so we don't need to touch
# Debian's externally-managed system Python environment
RUN python3 -m venv /opt/liblouis-venv
ENV PATH="/opt/liblouis-venv/bin:$PATH"
WORKDIR /usr/src/liblouis/python
RUN pip install .

# clean up
WORKDIR /root
RUN rm -rf /usr/src/liblouis
