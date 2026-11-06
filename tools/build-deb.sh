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
# and Mint carry the 64-bit time_t transition in the names of the Qt 6
# libraries (libqt6gui6t64), Debian kept the plain ones (libqt6gui6), and
# Debian 13 does one of each: libqt6core6t64 but libqt6gui6. A package would
# therefore only install on the kind of system it was built on. Every Qt 6
# dependency is listed under both names instead, whichever way round it was
# written; apt is satisfied as soon as one of the two is there.
deb=$(ls -1 build-deb/*.deb)
work=build-deb/both-names
rm -rf "$work"
dpkg-deb -R "$deb" "$work"
awk '
/^Depends:/ {
    value = substr($0, 10)
    out = ""
    while (length(value) > 0) {
        comma = index(value, ", ")
        if (comma > 0) { part = substr(value, 1, comma - 1); value = substr(value, comma + 2) }
        else           { part = value; value = "" }

        name = part; rest = ""
        space = index(part, " ")
        if (space > 0) { name = substr(part, 1, space - 1); rest = substr(part, space) }

        other = ""
        if (name ~ /^libqt6[a-z]+[0-9]+t64$/)
            other = substr(name, 1, length(name) - 3)
        else if (name ~ /^libqt6[a-z]+[0-9]+$/)
            other = name "t64"

        if (other != "")
            part = part " | " other rest

        out = (out == "" ? part : out ", " part)
    }
    print "Depends: " out
    next
}
{ print }
' "$work/DEBIAN/control" > "$work/DEBIAN/control.new"
mv "$work/DEBIAN/control.new" "$work/DEBIAN/control"
dpkg-deb --build --root-owner-group "$work" "$deb" > /dev/null
rm -rf "$work"

echo
echo "Fertig:"
ls -1 build-deb/*.deb
echo
dpkg-deb -f "$deb" Depends | fold -s -w 76 | sed 's/^/  /'
