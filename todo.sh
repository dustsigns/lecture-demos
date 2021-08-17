#Utility to list all TODOs in sources, headers, scripts and Makefiles
# Andreas Unterweger, 2017-2021
#This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

echo "To-dos in source and header files:"
find . -iname '*.[ch]pp' -print | xargs grep -o --with-filename --line-number --color=always -e '/[/*]TODO: .*'  | sed 's/\/[/*]TODO: \(.*\)/\1/' | sed 's/\*\/$//' | tee >(wc -l) &
wait $!
echo "To-dos in script and Make files:"
find . -iname 'Makefile' -print -o -iname '*.mak' -print -o -iname '*.sh' -print | grep -v ./todo.sh | xargs grep -o --with-filename --line-number --color=always -e '#TODO: .*'  | sed 's/#TODO: \(.*\)/\1/' | tee >(wc -l) &
wait $!
echo "Global to-dos:"
cat global_todos.txt | tee >(wc -l) &
wait $!
