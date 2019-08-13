#!/usr/bin/env bash
#
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8
brew uninstall --force --ignore-dependencies openssl autoconf fontconfig gdbm boost@1.57 glib harfbuzz jpeg libffi libtiff pixman python@2 sqlite automake freetype gdk-pixbuf gmp icu4c libcroco libpng libtool pango pkg-config qt watch berkeley-db@4 cairo fribidi gettext graphite2 iperf libevent librsvg miniupnpc pcre protobuf readline || exit 1
travis_retry brew update
brew cleanup -s
travis_retry brew install autoconf fontconfig gdbm openssl@1.0 boost@1.57 glib harfbuzz jpeg libffi libtiff pixman python@2 sqlite automake freetype gdk-pixbuf gmp icu4c libcroco libpng libtool pango pkg-config qt watch berkeley-db@4 cairo fribidi gettext graphite2 iperf libevent librsvg miniupnpc pcre protobuf readline
brew link --overwrite --force boost@1.57