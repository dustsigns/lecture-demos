#Utility to create a clean compressed archive from this directory
# Andreas Unterweger, 2016-2018
#This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

name=$(basename "$PWD")
make clean > /dev/null
cd ..
tar --exclude="$name/.git" --exclude="$name/.gitignore" -czf "$name.tar.gz" "$name"
