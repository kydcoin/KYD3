#!/usr/bin/env bash.

export LC_ALL=C.UTF-8
env | grep -E '^(CCACHE_|WINEDEBUG|LC_ALL|BOOST_TEST_RANDOM|CONFIG_SHELL)' | tee /tmp/env
BITCOIN_CONFIG_ALL="--disable-dependency-tracking --prefix=$TRAVIS_BUILD_DIR/depends/$HOST"

travis_fold start "autogen"
./autogen.sh
travis_fold end "autogen"

travis_fold start "configure"
./configure --cache-file=config.cache $BITCOIN_CONFIG_ALL $BITCOIN_CONFIG || ( cat config.log && false)
travis_fold end "configure"

travis_fold start "make"
mkdir release
git status
make -j4 install
echo $TRAVIS_BUILD_DIR
if [[ $HOST = *-mingw32 ]]; then
    cd $TRAVIS_BUILD_DIR/depends/$HOST/bin && zip $TRAVIS_BUILD_DIR/$RELEASE_FILE kyd*
else
    cd $TRAVIS_BUILD_DIR/depends/$HOST/bin && tar -cvzf $TRAVIS_BUILD_DIR/$RELEASE_FILE kyd*
fi
cd $TRAVIS_BUILD_DIR
ls
travis_fold end "make"
