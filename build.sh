CFLAGS="-Wall -Wextra -pedantic"

mkdir -p build
gcc $CFLAGS -g -o build/main main.c
# ./build/compiler $1 $2 $3