echo "[liblouis-js] starting docker image with emscripten installed..."
docker run -v $(pwd):/src trzeci/emscripten:sdk-tag-1.35.4-64bit /bin/bash -c "./.travis/script/emscripten-build.sh" &&
echo "[liblouis-js] checking builds..." &&

echo "[liblouis-js] prepare package for publish..." &&
cp ./tables/ ./out/tables/ &&
cp ./js-build/package.json ./out/package.json &&
cd out

if [ -n "$VERSION" ]; then
	npm version {$VERSION}
if

echo "[liblouis-js] testing builds..."
npm install . &&
node -e "require('./liblouis-js/testrunner/main.js')"

if [ $? != 0 ]; then
        echo "[liblouis-js] Not publishing as at least one build failed tests."
        exit 1;
fi

if [ -z "$VERSION" ]; then
    echo "[liblouis-js] no build version specified. Not publishing."
    exit 1;
fi
