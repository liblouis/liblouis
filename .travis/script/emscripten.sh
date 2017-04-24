# --- build all four currently published builds of liblouis (UTF-16 with
#     tables, UTF-16 wihout tables, UTF-32 with tables and UTF-32  without
#     tables) in a docker image that has all necessary build tools installed.
rm -f ../js-build/build-*.js

echo "[liblouis-js] starting docker image with emscripten installed..."
docker run --rm -v $(pwd):/src dolp/liblouis-js-build-travis:1.37.3-64bit /bin/bash -c "./.travis/script/emscripten-build.sh"

if [ $? != 0 ]; then
        echo "[liblouis-js] Build failed. Aborting..."
        exit 1
fi

# --- collect all files that are necessary for a publish in package
#     managers.
echo "[liblouis-js] bundling files to package for publish..." &&
rm -rf ../js-build/tables/ &&
cp -R ./tables/ ../js-build/tables/ &&
cp -Rf ./out/* ../js-build/

if [ -n "$BUILD_VERSION" ]; then
	cd ../js-build
	npm version --no-git-tag-version $BUILD_VERSION
	cd -
fi

ls -lah ../js-build

# --- make sure the package we just build is not damaged.
echo "[liblouis-js] testing builds..."

# NOTE: `npm link` exposes the newly build npm package to the
# test runner. The `--production`-flag is necessary as it ensures
# that no publicly published package of the build is downloaded
# and tested instead.
cd liblouis-js &&
npm link ../../js-build --production &&
# NOTE: we only test the build in NodeJS
node testrunner/main.js &&
cd ..

if [ $? != 0 ]; then
        echo "[liblouis-js] Not publishing as at least one build failed tests."
        exit 1
fi
