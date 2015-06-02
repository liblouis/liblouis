FROM debian:wheezy

RUN apt-get update
RUN apt-get install -y make autoconf automake libtool pkg-config python python-pip curl
RUN pip install nose

# texinfo
WORKDIR /tmp
RUN curl -L http://ftpmirror.gnu.org/texinfo/texinfo-5.2.tar.gz | tar zx
WORKDIR /tmp/texinfo-5.2
RUN ./configure
RUN make install

# liblouis
ADD . /tmp/liblouis
WORKDIR /tmp/liblouis
RUN ./autogen.sh
RUN ./configure --enable-ucs4
# RUN make clean
RUN make
RUN make check
RUN make install
RUN ldconfig