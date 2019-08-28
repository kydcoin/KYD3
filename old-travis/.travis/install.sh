#!/usr/bin/env bash.

export LC_ALL=C.UTF-8
# docker pull "$DOCKER_NAME_TAG"
env | grep -E '^(CCACHE_|WINEDEBUG|LC_ALL|BOOST_TEST_RANDOM|CONFIG_SHELL)' | tee /tmp/env
BITCOIN_CONFIG_ALL="--disable-dependency-tracking --prefix=$TRAVIS_BUILD_DIR/depends/$HOST"
# DOCKER_ID=$(docker run $DOCKER_ADMIN -idt --mount type=bind,src=$TRAVIS_BUILD_DIR,dst=$TRAVIS_BUILD_DIR --mount type=bind,src=$CCACHE_DIR,dst=$CCACHE_DIR -w $TRAVIS_BUILD_DIR --env-file /tmp/env $DOCKER_NAME_TAG)

# DOCKER_EXEC () {
#   docker exec $DOCKER_ID bash -c "cd $PWD && $*"
# }

travis_fold start "Installing"
# travis_retry DOCKER_EXEC apt-get update
# travis_retry DOCKER_EXEC apt-get install --no-install-recommends --no-upgrade -qq $PACKAGES $DOCKER_PACKAGES
# DOCKER_EXEC add-apt-repository ppa:bitcoin/bitcoin -y
# travis_retry DOCKER_EXEC apt-get install -y libdb4.8-dev libdb4.8++-dev libevent-dev
travis_fold end "Installing"
ls
travis_fold start "autogen"
./autogen.sh
travis_fold end "autogen"

travis_fold start "configure"
./configure --cache-file=config.cache $BITCOIN_CONFIG_ALL $BITCOIN_CONFIG || ( cat config.log && false)
travis_fold end "configure"

travis_fold start "make"
mkdir release
make -j4
make install
if [[ $HOST = *-apple-* ]]; then
  make deploy
fi
echo $TRAVIS_BUILD_DIR
cd $TRAVIS_BUILD_DIR/depends/$HOST/bin && tar -cvzf $TRAVIS_BUILD_DIR/$NAME.tar.gz kyd*
cd $TRAVIS_BUILD_DIR
ls
travis_fold end "make"
