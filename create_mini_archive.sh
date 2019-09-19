#Utility to create a clean compressed archive from this directory with essential files only
# Andreas Unterweger, 2018-2019
#This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

name=$(basename "$PWD")
make clean > /dev/null
cd ..
(find "$name" -type f | grep -v '^'$name'\($\|/\(.git\|.gitignore\|screenshots\|testdata\|[^/]\+.sh\)\)' && find "$name/screenshots/" -iname '*.txt' && find "$name/testdata/" -iname '*.txt') | tar czf "${name}_mini.tar.gz" --files-from -
