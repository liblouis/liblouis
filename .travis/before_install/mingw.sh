sudo apt-get install -y mingw32 mingw-w64 libc6-dev-i386
mkdir -p $HOME/src &&
cd $_ &&
curl -L https://github.com/yaml/libyaml/tarball/0.1.4 | tar zx &&
mv yaml-libyaml-4dce9c1 libyaml &&
cd libyaml &&
patch -p1 <$TRAVIS_BUILD_DIR/libyaml_mingw.patch &&
./bootstrap &&
./configure --host i586-mingw32msvc --prefix=$HOME/build/win32/libyaml &&
make &&
make install
