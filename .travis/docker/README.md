These files can be used to debug issues in a travis build locally.

1. install Docker.
2. get the docker image for the trusty environment used by Travis CI. Specify
   the latest tag available, not the tag shown here:
   ```
   docker pull travisci/ci-garnet:packer-1487190256
   ```
3. Start the docker image.
   ```
   docker run --name travis-debug -dit travisci/ci-garnet:packer-1487190256 /sbin/init
   docker exec -it travis-debug bash -l
   su - travis
   ```
4. Customize the trusty instance by execute the correct setup script for
   your built. For example:
   ```
   chmod +x ./.travis/docker/setupenv-emscripten.sh &&
   ./.travis/docker/setupenv-emscripten.sh
   ```
5. Run the scripts your `.travis.yaml` file executes. For example:
   ```
   ./.travis/before_install/emscripten.sh
   ./.travis/script/emscripten.sh
   ```
