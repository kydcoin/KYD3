#!/usr/bin/env bash.

export LC_ALL=C.UTF-8

./autogen.sh
./configure LDFLAGS='-L/usr/local/opt/openssl/lib' CPPFLAGS='-I/usr/local/opt/openssl/include' PKG_CONFIG_PATH='/usr/local/opt/openssl/lib/pkgconfig' --with-gui=qt5 --with-boost-system=boost_system-mt --with-boost-filesystem=boost_filesystem-mt --with-boost-thread=boost_thread-mt --with-boost-chrono=boost_chrono-mt --disable-tests
make -j4
make deploy
mv KYD-Qt.dmg $RELEASE_FILE