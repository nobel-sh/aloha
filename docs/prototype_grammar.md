## EBNF(Likely to change) 

program             ::= (function | comment)*

function            ::= "fun" identifier "(" parameters ")" "do" statements "end"

parameters          ::= (parameter ("," parameter)*)?
parameter           ::= identifier ":" type

statements          ::= statement (";" statement)*
statement           ::= assignment | function_call | if_statement | return_statement | expression

assignment          ::= "var" identifier "=" expression
function_call       ::= identifier "(" arguments ")"
if_statement        ::= "if" condition "then" statements ("else" statements)? "end"
return_statement    ::= "return" expression

condition           ::= expression ("<" | ">" | "==" | "!=") expression

expression          ::= term (("+" | "-") term)*
term                ::= factor (("*" | "/") factor)*
factor              ::= number | identifier | "(" expression ")"

type                ::= "int" | "float" | "string"

comment             ::= single_line_comment | multi_line_comment
single_line_comment ::= "//" string "\n"
multi_line_comment  ::= "/*" string "*/"

identifier          ::= letter (letter | digit)*
number              ::= digit+
string              ::= '"' (letter | digit | special_character)* '"'

letter              ::= "a" | "b" | ... | "z" | "A" | "B" | ... | "Z"
digit               ::= "0" | "1" | ... | "9"
special_character   ::= any valid special character

arguments           ::= expression ("," expression)*nop ::= `-´  |  not  |  `#´
