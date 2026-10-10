#!/usr/bin/env bash
#
# Build for Debian in a docker container
#
# Copyright (c) 2021 Alec Leamas
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

set -xe

# Create build script
#
if [ -n "$TRAVIS_BUILD_DIR" ]; then
    ci_source="$TRAVIS_BUILD_DIR"
elif [ -d ~/project ]; then
    ci_source=~/project
else
    ci_source="$(pwd)"
fi

cd $ci_source
git submodule update --init opencpn-libs

cat > $ci_source/build.sh << "EOF"
set -x

apt -y update
apt -y install acl devscripts equivs wget git lsb-release

mk-build-deps /ci-source/build-deps/control
apt install -q -y ./opencpn-build-deps*deb
apt-get -q --allow-unauthenticated install -f

apt-get install -y cmake

cd /ci-source
getfacl -R /ci-source > /ci-source.permissions

chown root:root /ci-source
git config --global --add safe.directory /ci-source

rm -rf build-debian; mkdir build-debian; cd build-debian
cmake "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Release}" \
   -DOCPN_TARGET_TUPLE="@TARGET_TUPLE@" \
    ..

git config --global --add safe.directory /ci-source

make -j $(nproc) VERBOSE=1 tarball
ldd  app/*/lib/opencpn/*.so

cd /
setfacl --restore=/ci-source.permissions
EOF

sed -i "s/@TARGET_TUPLE@/$TARGET_TUPLE/" $ci_source/build.sh

# Run script in docker image
#
docker run \
    -e "CLOUDSMITH_STABLE_REPO" \
    -e "CLOUDSMITH_BETA_REPO" \
    -e "CLOUDSMITH_UNSTABLE_REPO" \
    -e "CIRCLE_BUILD_NUM" \
    -e "TRAVIS_BUILD_NUMBER" \
    -v "$ci_source:/ci-source:rw" \
    debian:$OCPN_TARGET /bin/bash -xe /ci-source/build.sh
rm -f $ci_source/build.sh


# Install cloudsmith-cli (for upload) and cryptography (for git-push).
#
if pyenv versions &>/dev/null;  then
    pyenv versions | tr -d '*' | awk '{print $1}' | tail -1 \
        > $HOME/.python-version
fi
python3 -m pip install -q --user "urllib3<2.0.0"   # See #520
python3 -m pip install -q --user cloudsmith-cli cryptography

# python install scripts in ~/.local/bin, teach upload.sh to use in it's PATH:
echo 'export PATH=$PATH:$HOME/.local/bin' >> ~/.uploadrc
