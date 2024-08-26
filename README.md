### Project Description:
See [here](https://teaching.csse.uwa.edu.au/units/CITS2002/projects/project1.php)


## The *ml* syntax
According to [CSSE](https://teaching.csse.uwa.edu.au/units/CITS2002/projects/project1-syntax.php):
>   `|`       means a choice
>   `[...]`   means zero or one
>   `(...)*`  means zero or more
> 
> Program lines commence at the left-hand margin (no indentation).
> Statements forming the body of a function are indented with a single tab.

```
program:
           ( program-item )*

program-item:
           statement
        |  function identifier ( identifier )*
           ←–tab–→ statement1
           ←–tab–→ statement2
           ....

statement:
           identifier "<-" expression
        |  print  expression
        |  return expression
        |  functioncall

expression:
          term   [ ("+" | "-")  expression ]

term:
          factor [ ("*" | "/")  term ]

factor:
          realconstant
        | identifier
        | functioncall
        | "(" expression ")"

functioncall:
          identifier "(" [ expression ( "," expression )* ] ")"
```