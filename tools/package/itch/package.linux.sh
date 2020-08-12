#!/bin/bash

PACKAGE_DEST="$TRAVIS_BUILD_DIR/package/virtualxt"
rm -rf $PACKAGE_DEST
mkdir -p $PACKAGE_DEST/bios $PACKAGE_DEST/boot

./tools/package/appimage/build.sh

cp VirtualXT-x86_64.AppImage $PACKAGE_DEST/
#cp -r doc/manual $PACKAGE_DEST
cp boot/freedos.img $PACKAGE_DEST/boot/
cp tools/package/itch/itch.linux.toml $PACKAGE_DEST/.itch.toml

curl -L -o $PACKAGE_DEST/bios/ati_ega_wonder_800_plus.bin "https://github.com/BaRRaKudaRain/PCem-ROMs/raw/master/ATI%20EGA%20Wonder%20800%2B%20N1.00.BIN"
