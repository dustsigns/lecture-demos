#Utility to create a clean compressed archive from this directory with essential files only
# Andreas Unterweger, 2018
#This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

name=$(basename "$PWD")
make clean > /dev/null
cd ..
#TODO: Add hint for missing screenshots folder
(find "$name" -type f | grep -v '^'$name'\($\|/\(.git\|.gitignore\|screenshots\|testdata\|[^/]\+.sh\)\)' && find "$name/testdata/" -iname '*.txt') | tar czf "${name}_mini.tar.gz" --files-from -
