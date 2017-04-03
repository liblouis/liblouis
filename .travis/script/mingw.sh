./autogen.sh &&
export CPPFLAGS="-I/$HOME/build/win32/libyaml/include/" &&
export LDFLAGS="-L/$HOME/build/win32/libyaml/lib/" &&
./configure --host i586-mingw32msvc $ENABLE_UCS4 --with-yaml &&
make LDFLAGS="$LDFLAGS -avoid-version -Xcompiler -static-libgcc"
