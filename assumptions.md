### Assumptions
1. 
    We cannot have something like:
    ```
    hi <- 3 + 5000
     x <- hi + 3
    ```
    where both of these statements are in global scope, as "hi" is not a constant
2. 
    If the decimal part of the float terminates with 0's, e.g. `3.00000`, we print `3` 
