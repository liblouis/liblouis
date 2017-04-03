./autogen.sh &&
./configure $ENABLE_UCS4 --with-yaml &&
make &&
make check
