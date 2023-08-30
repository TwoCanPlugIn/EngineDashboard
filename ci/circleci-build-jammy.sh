#!/usr/bin/env bash

#
# Build the Debian artifacts
#
set -xe

if [ "${CIRCLECI_LOCAL,,}" = "true" ]; then
    if [[ -d ~/circleci-cache ]]; then
        if [[ -f ~/circleci-cache/apt-proxy ]]; then
            cat ~/circleci-cache/apt-proxy | sudo tee -a /etc/apt/apt.conf.d/00aptproxy
            cat /etc/apt/apt.conf.d/00aptproxy
        fi
    fi
fi

# As we are using git submodules
#git submodule update --init opencpn-libs
# Alternative in config.yml
#:
#        command: |
#          git submodule update --init --recursive
#        name: Init git submodule


sudo apt-get -qq update
sudo apt-get install devscripts equivs

rm -rf build && mkdir build && cd build

# Install extra build libs
ME=$(echo ${0##*/} | sed 's/\.sh//g')
EXTRA_LIBS=./ci/extras/extra_libs.txt
if test -f "$EXTRA_LIBS"; then
    while read line; do
        sudo apt-get install $line
    done < $EXTRA_LIBS
fi
EXTRA_LIBS=./ci/extras/${ME}_extra_libs.txt
if test -f "$EXTRA_LIBS"; then
    while read line; do
        sudo apt-get install $line
    done < $EXTRA_LIBS
fi

pwd
sudo mk-build-deps --install ../ci/control

sudo apt-get --allow-unauthenticated install ./*all.deb  || :
sudo apt-get --allow-unauthenticated install -f
rm -f ./*all.deb

tag=$(git tag --contains HEAD)

sudo apt-get install libwxgtk3.2-dev

if [ -n "$BUILD_GTK3" ] && [ "$BUILD_GTK3" = "TRUE" ]; then
  sudo update-alternatives --set wx-config /usr/lib/*-linux-*/wx/config/gtk3-unicode-3.0
fi

if [ -n "$tag" ]; then
  cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
else
  cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/usr/local ..
fi

make -j2
make package
ls -l
