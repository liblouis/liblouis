source ./.travis/script/emscripten-build-command.sh &&
mkdir out &&

echo "[liblouis-js] starting build process in docker image..." &&

./autogen.sh &&

echo "[liblouis-js] configuring and making UTF-16 builds..." &&
emconfigure ./configure --disable-shared &&
emmake make &&

buildjs "16" "build-no-tables-utf16.js" &&
buildjs "16" "build-tables-embeded-utf16.js" "--embed-files tables@/" &&

echo "[liblouis-js] configuring and making UTF-32 builds..." &&
emconfigure ./configure --enable-ucs4 --disable-shared &&
emmake make &&

echo "[liblouis-js] building UTF-32 with no tables..." &&
buildjs "32" "build-no-tables-utf32.js" &&
buildjs "32" "build-tables-embeded-utf32.js" "--embed-files tables@/" &&

echo "[liblouis-js] done building in docker image..."
