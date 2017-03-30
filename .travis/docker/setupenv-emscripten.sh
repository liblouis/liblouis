# sets up the environment as travis ci does for emscripten builds.

# required if build directly in main instance and not in docker image
# sudo -E apt-add-repository -y "ppa:ubuntu-toolchain-r/test"
# sudo -E apt-get -yq update &>> ~/apt-get-update.log
# sudo -E apt-get -yq --no-install-suggests --no-install-recommends --force-yes install cmake g++-4.9

export BUILD_JS=yes
gcc --version

git clone https://github.com/liblouis/liblouis.git
cd liblouis
chmod +x ./.travis/before_install/emscripten.sh
chmod +x ./.travis/script/emscripten.sh
