cd analyzer
yacc -d main.y
lex rules.lex
cd ..
cmake .
make
rm analyzer/lex.yy.c analyzer/y.tab.*
rm Makefile