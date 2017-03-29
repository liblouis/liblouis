
# --- will contain build output
mkdir out &&

# --- get tools required for built
echo "[liblouis-js] updating packages in docker container..." &&
apt-get update &&
DEBIAN_FRONTEND=noninteractive apt-get -yq install autotools-dev dh-autoreconf &&
echo "[liblouis-js] starting build process in docker image..." &&

# --- actual build process
./autogen.sh &&

# make liblouis with UTF-16
echo "[liblouis-js] configuring and making UTF-16 builds..." &&
emconfigure ./configure &&
emmake make &&

echo "[liblouis-js] building UTF-16 with no tables..." &&
emcc liblouis/.libs/liblouis.so -s RESERVED_FUNCTION_POINTERS=1\
 -s EXPORTED_FUNCTIONS="['_lou_version', '_lou_translateString', '_lou_translate',\
'_lou_backTranslateString', '_lou_backTranslate', '_lou_hyphenate',\
'_lou_compileString', '_lou_getTypeformForEmphClass', '_lou_dotsToChar',\
'_lou_charToDots', '_lou_registerLogCallback', '_lou_setLogLevel',\
'_lou_logFile', '_lou_logPrint', '_lou_logEnd', '_lou_setDataPath',\
'_lou_getDataPath', '_lou_getTable', '_lou_checkTable',\
'_lou_readCharFromFile', '_lou_free', '_lou_charSize']" -s MODULARIZE=1\
 -s EXPORT_NAME="'liblouisBuild'" -s EXTRA_EXPORTED_RUNTIME_METHODS="['FS',\
'Runtime', 'NODEFS', 'stringToUTF16', 'stringToUTF32', 'Pointer_Stringify']" --pre-js ./liblouis-js/inc/pre.js\
 --post-js ./liblouis-js/inc/post.js -o ./out/build-no-tables-utf16.js &&

cat ./liblouis-js/inc/append.js >> ./out/build-no-tables-utf16.js &&

echo "[liblouis-js] building UTF-16 with tables embeded..." &&
emcc liblouis/.libs/liblouis.so -s RESERVED_FUNCTION_POINTERS=1\
 -s EXPORTED_FUNCTIONS="['_lou_version', '_lou_translateString', '_lou_translate',\
'_lou_backTranslateString', '_lou_backTranslate', '_lou_hyphenate',\
'_lou_compileString', '_lou_getTypeformForEmphClass', '_lou_dotsToChar',\
'_lou_charToDots', '_lou_registerLogCallback', '_lou_setLogLevel',\
'_lou_logFile', '_lou_logPrint', '_lou_logEnd', '_lou_setDataPath',\
'_lou_getDataPath', '_lou_getTable', '_lou_checkTable',\
'_lou_readCharFromFile', '_lou_free', '_lou_charSize']" -s MODULARIZE=1\
 -s EXPORT_NAME="'liblouisBuild'" -s EXTRA_EXPORTED_RUNTIME_METHODS="['FS',\
'Runtime', 'NODEFS', 'stringToUTF16', 'stringToUTF32', 'Pointer_Stringify']" --pre-js ./liblouis-js/inc/pre.js\
 --post-js ./liblouis-js/inc/post.js --embed-files tables@/ -o ./out/build-tables-embeded-utf16.js &&

cat ./liblouis-js/inc/append.js >> ./out/build-tables-embeded-utf16.js &&

# make liblouis with UTF-32
echo "[liblouis-js] configuring and making UTF-32 builds..." &&
emconfigure ./configure --enable-ucs4 &&
emmake make &&

echo "[liblouis-js] building UTF-32 with no tables..." &&
emcc liblouis/.libs/liblouis.so -s RESERVED_FUNCTION_POINTERS=1\
 -s EXPORTED_FUNCTIONS="['_lou_version', '_lou_translateString', '_lou_translate',\
'_lou_backTranslateString', '_lou_backTranslate', '_lou_hyphenate',\
'_lou_compileString', '_lou_getTypeformForEmphClass', '_lou_dotsToChar',\
'_lou_charToDots', '_lou_registerLogCallback', '_lou_setLogLevel',\
'_lou_logFile', '_lou_logPrint', '_lou_logEnd', '_lou_setDataPath',\
'_lou_getDataPath', '_lou_getTable', '_lou_checkTable',\
'_lou_readCharFromFile', '_lou_free', '_lou_charSize']" -s MODULARIZE=1\
 -s EXPORT_NAME="'liblouisBuild'" -s EXTRA_EXPORTED_RUNTIME_METHODS="['FS',\
'Runtime', 'NODEFS', 'stringToUTF16', 'stringToUTF32', 'Pointer_Stringify']" --pre-js ./liblouis-js/inc/pre.js\
 --post-js ./liblouis-js/inc/post.js -o ./out/build-no-tables-utf32.js &&

cat ./liblouis-js/inc/append.js >> ./out/build-no-tables-utf32.js &&

echo "[liblouis-js] building UTF-32 with tables embeded..." &&
emcc liblouis/.libs/liblouis.so -s RESERVED_FUNCTION_POINTERS=1\
 -s EXPORTED_FUNCTIONS="['_lou_version', '_lou_translateString', '_lou_translate',\
'_lou_backTranslateString', '_lou_backTranslate', '_lou_hyphenate',\
'_lou_compileString', '_lou_getTypeformForEmphClass', '_lou_dotsToChar',\
'_lou_charToDots', '_lou_registerLogCallback', '_lou_setLogLevel',\
'_lou_logFile', '_lou_logPrint', '_lou_logEnd', '_lou_setDataPath',\
'_lou_getDataPath', '_lou_getTable', '_lou_checkTable',\
'_lou_readCharFromFile', '_lou_free', '_lou_charSize']" -s MODULARIZE=1\
 -s EXPORT_NAME="'liblouisBuild'" -s EXTRA_EXPORTED_RUNTIME_METHODS="['FS',\
'Runtime', 'NODEFS', 'stringToUTF16', 'stringToUTF32', 'Pointer_Stringify']" --pre-js ./liblouis-js/inc/pre.js\
 --post-js ./liblouis-js/inc/post.js --embed-files tables@/ -o ./out/build-tables-embeded-utf32.js &&

cat ./liblouis-js/inc/append.js >> ./out/build-tables-embeded-utf32.js &&

echo "[liblouis-js] done building in docker image..."
