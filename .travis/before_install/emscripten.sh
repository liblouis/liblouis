export BUILD_VERSION=$TRAVIS_TAG

if [ -z "$TRAVIS_COMMIT" ]; then
    echo "[liblouis-js] not building in travis. Aborting..."
    exit 1;
fi

echo $TRAVIS_TAG | grep "^v[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*$"

if [ $? -eq 1 ]; then
	echo "[liblouis-js] tag is not valid version string."
        export BUILD_VERSION="v0.0.0-${TRAVIS_COMMIT}";
else
	export BUILD_VERSION = $TRAVIS_TAG
fi

echo "[liblouis-js] building version ${BUILD_VERSION}" &&

# --- obtain the latest version of liblouis-js
# contains tests and js snippets appended to builds
git clone --depth 1 https://github.com/liblouis/liblouis-js.git &&
# location we publish/deploy the builds
git clone --depth 1 https://github.com/liblouis/js-build.git &&

echo "[liblouis-js] obtaining docker image of emscripten..." &&
docker pull trzeci/emscripten:sdk-tag-1.35.4-64bit

