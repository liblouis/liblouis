if [ "$TRAVIS_PULL_REQUEST" != "false" -o "$TRAVIS_BRANCH" != "master" ]; then
	echo "[liblouis-js] Not publishing. Is pull request or non-master branch."
	exit 0
fi

if [ -z "$BUILD_VERSION" ]; then
	echo "[liblouis-js] no build version specified. Not publishing."
	exit 0
fi

echo "[liblouis-js] publishing builds..."

git config user.name "Travis CI" &&
git config user.email "liblouis@users.noreply.github.com" &&

# --- decrypt and enable ssh key that allows us to push to the
#     liblouis/js-build repository.

openssl aes-256-cbc -K $encrypted_cf3facfb36cf_key -iv $encrypted_cf3facfb36cf_iv -in ./.travis/secrets/deploy_key.enc -out deploy_key -d &&
chmod 600 deploy_key &&
eval `ssh-agent -s` &&
ssh-add deploy_key &&

# --- push commit and tag to repository. (This will automatically
#     publish the package in the bower registry.)

cd ../js-build &&
git add --all &&
git commit -m "Automatic build of version ${BUILD_VERSION}" &&
git tag -a ${BUILD_VERSION} -m "automatic build for version ${BUILD_VERSION}" &&
git push git@github.com:liblouis/js-build.git master &&
git push git@github.com:liblouis/js-build.git $BUILD_VERSION

# --- push in npm registry and move the `next` tag pointing
#     to the latest development version if necessary
# TODO: get credentials
# npm publish

# if [ -z $TRAVIS_TAG ]
# 	npm dist-tag v0.0.0-${BUILD_VERSION} next
# 	npm dist-tag v0.0.0-${BUILD_VERSION} ${TRAVIS_COMMIT}
# fi
