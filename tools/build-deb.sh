#!/bin/sh
# SPDX-FileCopyrightText: 2026 Christoph Krogmann
# SPDX-License-Identifier: GPL-3.0-only
#
# Builds the Debian package.
#
#     tools/build-deb.sh
#
# The result lands in build-deb/ and is named after the version and the
# architecture it was built on, for example inkpot_1.0.0_amd64.deb.
#
# The package is tied to the Qt version of the system it was built on, so
# build it on the oldest distribution it should run on. Built on Linux Mint 22
# or Ubuntu 24.04 it also installs on everything newer.
set -e

cd "$(dirname "$0")/.."

# The prefix has to be /usr: a package installs there, not in /usr/local, and
# the path is baked into the binary for finding the translations.
cmake -S . -B build-deb -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr

cmake --build build-deb

rm -f build-deb/*.deb

( cd build-deb && cpack -G DEB )

# dpkg-shlibdeps can only name the packages of the system it runs on. Ubuntu
# and Mint carry the 64-bit time_t transition in the package names
# (libqt6gui6t64), Debian 13 kept the plain ones (libqt6gui6), so a package
# built here will not install there. Each such dependency therefore gets the
# other spelling added as an alternative; apt is satisfied once one of the two
# is present.
deb=$(ls -1 build-deb/*.deb)
work=build-deb/relax-depends
rm -rf "$work"
dpkg-deb -R "$deb" "$work"
sed -i -E '/^Depends:/ s/([a-z0-9][a-z0-9+.-]*)t64 (\([^)]*\))/\1t64 \2 | \1 \2/g' \
    "$work/DEBIAN/control"
dpkg-deb --build --root-owner-group "$work" "$deb" > /dev/null
rm -rf "$work"

echo
echo "Fertig:"
ls -1 build-deb/*.deb
echo
dpkg-deb -f "$deb" Depends | fold -s -w 76 | sed 's/^/  /'
