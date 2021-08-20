#!/bin/bash

# gotta make sure its in /linux/offline
# it relies on relative paths from there
if [ ! -f "./hobo_vr/DEBIAN/postinst" ]; then
    echo "this script should be run from its directory"
    exit 1
fi

echo "copy driver into package..."
cp -r ../../../hobovr ./hobo_vr/usr/local/share/

dpkg-deb --build hobo_vr

echo "cleanup..."
rm -rf ./hobo_vr/usr/local/share/hobovr
echo "done"
