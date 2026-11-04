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

( cd build-deb && cpack -G DEB )

echo
echo "Fertig:"
ls -1 build-deb/*.deb
