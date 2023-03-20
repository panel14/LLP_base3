cd analayzer
yacc -d main.y
lex rules.lex
cd ..
cmake .
make
rm analayzer/lex.yy.c analayzer/y.tab.*
rm Makefile