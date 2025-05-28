CFLAGS="-Wall -Wextra -pedantic"

mkdir -p build
gcc $CFLAGS -g -o build/compiler arena.c compiler.c lexer.c parser.c code_gen.c
# ./build/compiler $1 $2 $3