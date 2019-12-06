# Kipper

An interpreter for KipperScript language(js--) written in C++17 for studying garbage collection.

## Code Status

[![Build Status](https://dev.azure.com/chqc/kipper/_apis/build/status/kipper?branchName=master)](https://dev.azure.com/chqc/kipper/_build/latest?definitionId=1&branchName=master)

## Build from Source

### Requirements

- x86-64
- Git
- CMake (>= 3.12)
- C++17 Compiler
- clang-tidy (optional)

### Build

```
$ git clone https://github.com/hanakeichen/kipper
$ cd kipper
$ mkdir build && cmake -S . -B build
$ cmake -DCMAKE_BUILD_TYPE=Release --build ./build
```

Then you can run with the demo:

```
$ ./build/apps/cli/ks tests/kstest/demo.ks
Hello, Kipper!
```

### Test

```
cd build && ctest
```

## Getting started with KipperScript

Below is the syntax EBNF for the KipperScript spec:

```
program = translate_unit ;

translate_unit = fn_decl_or_statement | translate_unit , fn_decl_or_statement ;

fn_decl_or_statement = fn_decl | statement ;

fn_decl = 'function' , identifier , '(' , parameters , ')' , '{' , statement_list , '}' ;

parameters = identifier_list ;

statement = block
        | if_statement
        | while_statement
        | for_statement
        | return_statement
        | break_statement
        | continue_statement
        | expression_statement ;

if_statement = 'if' , '(' , expression , ')' , statement [ , 'else' , statement ] ;

while_statement = 'while' , '(' , expression , ')' , statement ;

for_statement = 'for' , '(' , [ expression ] , ';' , [ expression ] , ';' , [ expression ] , ')' , statement ;

return_statement = 'return' , [ expression ] , ';' ;

break_statement = 'break' , ';' ;

continue_statement = 'continue' , ';' ;

expression_statement = expression , ';' ;

block = '{' , [ statement_list ] , '}' ;

expression = assignment ;

assignment = conditional_expression | lhs_expression ( [ '+' | '-' | '*' | '/' | '%' ] , '=' ) , assignment ;

conditional_expression = logic_or_expression [ , '?' , assignment , ':' , assignment  ] ;

logic_or_expression = logic_and_expression | logic_or_expression  , '||' , logic_and_expression ;

logic_and_expression = inclusive_or_expression | logic_and_expression , '&&' , inclusive_or_expression ;

inclusive_or_expression = and_expression | inclusive_or_expression , '|' , and_expression ;

and_expression = equality_expression | and_expression , '&' , equality_expression ;

equality_expression = relational_expression 
                    | equality_expression , '==' , relational_expression 
                    | equality_expression , '!=' , relational_expression ;

relational_expression = additive_expression
                    | relational_expression , ( '<' | '>' | '<=' | '>=' ) , additive_expression ;

additive_expression = multi_Expression | additive_expression , ( '+' | '-' ) , multi_Expression ;

multi_Expression = unary_expression | multi_Expression , ( '*' | '/' | '%' ) , unary_expression ;

unary_expression = ( '++' | '--' | '+' | '-' | '!' ) , unary_expression | postfix_expression ;

postfix_expression = lhs_expression [ , ( '++' | '--' ) ] ;

lhs_expression = member_expression | call_expression ;

member_expression = primary_expression
                | member_expression , '[' , expression , ']'
                | member_expression , '.' , identifier ;

call_expression = member_expression , arguments
                | call_expression , ( arguments | '[' , expression , ']' | '.' , identifier ) ;

primary_expression = literal | array_literal | object_literal | expression_name | '(' expression ')' ;

literal = int_literal | double_literal | string_literal | 'true' | 'false' ;

int_literal = '0' | digit_excluding_zero , { digit } ;

double_literal = int_literal , [ '.' , int_literal ] ;

string_literal = '"' , <any Unicode character> , '"' ;

arguments = '(' , [ expression { , ',' , expression } ] , ')' ;

expression_name = identifier ;

array_literal =  '[' , [ element_list ] , ']' ;

element_list = assignment | element_list , assignment ;

object_literal = '{' , [ property_name_value_list ] , '}' ;

property_name_value_list = property_assignment | property_name_value_list , property_assignment ;

property_assignment = property_name , ':' , assignment ;

property_name = identifier | string_literal ;

identifier = ( letter | '$' | '_' ) , { ( letter | digit | '$' | '_' ) } ;

statement_list = statement | statement_list , statement ;

identifier_list = identifier | identifier_list , ',' , identifier ;

letter = "A" | "B" | "C" | "D" | "E" | "F" | "G"
        | "H" | "I" | "J" | "K" | "L" | "M" | "N"
        | "O" | "P" | "Q" | "R" | "S" | "T" | "U"
        | "V" | "W" | "X" | "Y" | "Z" | "a" | "b"
        | "c" | "d" | "e" | "f" | "g" | "h" | "i"
        | "j" | "k" | "l" | "m" | "n" | "o" | "p"
        | "q" | "r" | "s" | "t" | "u" | "v" | "w"
        | "x" | "y" | "z" ;

digit_excluding_zero = "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" ;

digit = "0" | digit_excluding_zero ;
```

- All the text from the ASCII character **#** to the end of the line is ignored.
