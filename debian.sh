#!/usr/bin/env bash
kernel_version="5.10.104-tegra"
tmp_folder=$(mktemp -d)
mkdir -p "$tmp_folder"/lib/firmware/updates
cp -r firmware/* "$tmp_folder"/lib/firmware/updates
mkdir -p "$tmp_folder"/lib/modules/$kernel_version/updates
# Find all .ko files and copy them to the correct folder that keeps hierarchy
find . -name "*.ko" -exec cp -v --parents {} "$tmp_folder"/lib/modules/$kernel_version/updates \;
size=$(du -ks "$tmp_folder" | cut -f1)
# Copy the debian folder to the tmp folder
mkdir -p "$tmp_folder"/DEBIAN
cp debian/* "$tmp_folder"/DEBIAN
pushd "$tmp_folder" || exit
# Create the debian package
name=$(grep -oP '(?<=Package: ).*' "$tmp_folder"/DEBIAN/control)
version=$(grep -oP '(?<=Version: ).*' "$tmp_folder"/DEBIAN/control)
sed -i "s/Installed-Size: .*/Installed-Size: $size/g" "$tmp_folder"/DEBIAN/control
dpkg-deb --build "$tmp_folder" "$name"-"$version"+$kernel_version.deb
popd || exit
mv "$tmp_folder"/"$name"-"$version"+$kernel_version.deb ./