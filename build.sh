CFLAGS="-Wall -Wextra -pedantic"

gcc $CFLAGS -g -o compiler compiler.c lexer.c 
./compiler $1 $2 $3