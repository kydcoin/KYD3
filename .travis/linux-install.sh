#!/usr/bin/env bash.

export LC_ALL=C.UTF-8

travis_fold start "Installing"

sudo add-apt-repository ppa:bitcoin/bitcoin -y
travis_retry sudo apt-get update
travis_retry sudo apt-get install --no-install-recommends --no-upgrade -qq $PACKAGES $DOCKER_PACKAGES

travis_fold end "Installing"
