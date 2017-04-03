# --- build all four currently published builds of liblouis (UTF-16 with
#     tables, UTF-16 wihout tables, UTF-32 with tables and UTF-32  without
#     tables) in a docker image that has all necessary build tools installed.
echo "[liblouis-js] starting docker image with emscripten installed..."
docker run -v $(pwd):/src dolp/liblouis-js-build-travis:1.37.3-64bit /bin/bash -c "./.travis/script/emscripten-build.sh"

if [ $? != 0 ]; then
        echo "[liblouis-js] Build failed. Aborting..."
        exit 1
fi

# --- collect all files that are necessary for a publish in package
#     managers.
echo "[liblouis-js] bundling files to package for publish..." &&
rm -rf ../js-build/tables/ &&
cp -R ./tables/ ../js-build/tables/ &&
cp -Rf ./out/ ../js-build/

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

if [ "$TRAVIS_PULL_REQUEST" != "false" -o "$TRAVIS_BRANCH" != "master" ]; then
	echo "[liblouis-js] Not publishing. Is pull request or non-master branch."
	exit 0
fi

if [ -z "$BUILD_VERSION" ]; then
	echo "[liblouis-js] no build version specified. Not publishing."
	exit 0
fi

echo "[liblouis-js] publishing builds..."

#git config user.name "Travis CI" &&
#git config user.email "liblouis@users.noreply.github.com" &&

# --- decrypt and enable ssh key that allows us to push to the
#     liblouis/js-build repository.

# openssl aes-256-cbc -K $encrypted_7549c74596ca_key -iv $encrypted_7549c74596ca_iv -in ./.travis/secrets/deploy_key.enc -out deploy_key -d &&
# chmod 600 deploy_key &&
# eval `ssh-agent -s` &&
# ssh-add deploy_key &&

# --- push commit and tag to repository. (This will automatically
#     publish the package in the bower registry.)

# cd ../js-build &&
# git add --all &&
# git commit -m "Automatic build of version ${BUILD_VERSION}" &&
# git tag -a ${BUILD_VERSION} -m "automatic build for version ${BUILD_VERSION}" &&
# git push git@github.com:liblouis/js-build.git master
# git push git@github.com:liblouis/js-build.git $BUILD_VERSION

# --- push in npm registry and move the `next` tag pointing
#     to the latest development version if necessary
# TODO: get credentials
# npm publish

#if [ -z $TRAVIS_TAG ]
	#npm dist-tag v0.0.0-${BUILD_VERSION} next
	#npm dist-tag v0.0.0-${BUILD_VERSION} ${TRAVIS_COMMIT}
#fi
