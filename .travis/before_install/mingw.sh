sudo apt-get install -y $(test $ARCH == 32 && echo libc6-dev-i386 || echo mingw-w64) jq texinfo
mkdir -p $HOME/src &&
cd $_ &&
curl -L https://github.com/yaml/libyaml/tarball/0.1.4 | tar zx &&
mv yaml-libyaml-4dce9c1 libyaml &&
cd libyaml &&
patch -p1 <$TRAVIS_BUILD_DIR/libyaml_mingw.patch &&
./bootstrap &&
./configure --host $(test $ARCH == 32 && echo i686-w64-mingw32 || echo x86_64-w64-mingw32) --prefix=$HOME/build/win$ARCH/libyaml &&
make &&
make install
