CFLAGS="-Wall -Wextra -pedantic"

mkdir -p build
gcc $CFLAGS -g -o build/compiler compiler.c lexer.c 
./build/compiler $1 $2 $3