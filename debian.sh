#!/usr/bin/env bash
kernel_version="5.10.104-tegra"
tmp_folder=$(mktemp -d)
mkdir -p "$tmp_folder"/lib/firmware/update
cp -r firmware/* "$tmp_folder"/lib/firmware/update
mkdir -p "$tmp_folder"/lib/modules/$kernel_version/updates
# Find all .ko files and copy them to the correct folder that keeps hierarchy
find . -name "*.ko" -exec cp -v --parents {} "$tmp_folder"/lib/modules/$kernel_version/updates \;
# Copy the debian folder to the tmp folder
mkdir -p "$tmp_folder"/DEBIAN
cp debian/* "$tmp_folder"/DEBIAN
cd "$tmp_folder" || exit
# Create the debian package
name=$(grep -oP '(?<=Package: ).*' "$tmp_folder"/DEBIAN/control)
version=$(grep -oP '(?<=Version: ).*' "$tmp_folder"/DEBIAN/control)
dpkg-deb --build "$tmp_folder" "$name"-"$version"+$kernel_version.deb
