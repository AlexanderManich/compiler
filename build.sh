CFLAGS="-Wall -Wextra -pedantic"

gcc $CFLAGS -o compiler main.c lexer.c 
./compiler $1 $2 $3