#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>


// Edit a text file named, for example, program.ml
// Pass program.ml as a command-line argument to your runml program
// runml validates the ml program, reporting any errors
// runml generates C11 code in a file named, for example, ml-12345.c (where 12345 could be a process-ID)
// runml uses your system's C11 compiler to compile ml-12345.c
// runml executes the compiled C11 program ml-12345, passing any optional command-line arguments (real numbers)
// runml removes any files that it created

//EXTRA RULES
// * nested functions aren't allowed
// * BUFSIZ for a line should be 1024 (macOS)
// * White space can only appear at the beginning of a line if it's a tab under a function definition
// * Assume all statements (e.g. increment(3) + increment(4)) are valid and don't need to be checked. 
// * We have to make sure the ml program is syntactically complete - we can't throw it at the compiler
// * and let it handle errors
// That being said, we don't need to check expressions. 
// Don't need to have -Werror in compilation
// We don't need to check for parameter mismatches in function calls (Amitiva)

#define BUFSIZE 1024
#define IDCOUNT 50
#define IDSIZE 12
#define TEMPNAME "temp.c"

void usage() {
    printf("@ Usage: ./runml.out path/to/file.ml arg0 arg1 arg2 [...] \n");
    printf("@ arg0, arg1, ... must be integers (e.g. 1, 3000, ...), ");
    printf("or floating-point numbers (e.g. 3.1415). \n");
    printf("@ Args can be signed. \n");
    exit(EXIT_FAILURE);
}

// splits `line` into substrings seperated by spaces
// and places pointers to each of the substrings in `formatted_line`
void splitBySpace(char line[], char *formatted_line[]) {
    const char delimiter[2] = " "; // splitting by spaces
    char * token = strtok(line, delimiter); // gets the first substring
    formatted_line[0] = token;
    int i = 1;
    // adds substrings to `formatted_line`
    // adds NULL after the final substring (so we can iterate over it later)
    for(; token != NULL; i++) { 
        token = strtok(NULL, delimiter);  
        formatted_line[i] = token;
    }
}

void constructC(char path[], float ml_args[]) {
    // if just an expression, check if the variable has been 
    // instantiated - if not, replace with 0.0
    // temporary C file which will execute our C code
    FILE *c_file = fopen(TEMPNAME, "w"); // to read and write: "w+"
    // the .ml file provided by the path commandline argument
    FILE *ml_file = fopen(path, "r");
    if (ml_file == NULL) {
        fprintf(stderr, "! Invalid file path - this file doesn't exist\n");
        fclose(c_file);
        remove(TEMPNAME);
        // usage();
    }
    // char * declared_identifiers[IDCOUNT]; // stores functions that have been declared
    char line[BUFSIZ]; // to store the line read from the .ml file

    while(fgets(line, sizeof line, ml_file) != NULL) {
        // Stores an array of pointers to space-seperated 
        // substrings of `line`
        char *formatted_line[BUFSIZ / 2]; 
        // initialises `formatted_line`
        splitBySpace(line, formatted_line);
        // handles consistent cases such as print, function declaration, and 
        // assignment.
        if(!strcmp(formatted_line[0], "function")) {
            // we have a function call
            // Add "formatted_line" to data, or maybe just write it directly to the C file
            formatFunc(formatted_line);
            printf("I am actually god \n");
        } else if (!strcmp(formatted_line[0], "print")) {
            //format print statement
            
        }
    }
    // return c_file;
    // DON'T WRITE ANYTHING TO THE C FILE UNTIL ALL OF THE UNINITIALISED VARIABLES HAVE BEEN FOUND
}

int main(int argc, char * argv[]) {
    // What did Chris say he wanted the main function to be like?

    // Argument validation
    if (argc < 2 || !strcmp("--help", argv[1])){
        usage();
    } 

    char *endptr;
    float ml_args[argc]; // to store commandline args

    // converts args to floats and terminates if chars are encountered
    for(int i = 2; i < argc; i++) { 
        ml_args[i - 2] = strtof(argv[i], &endptr);
        if (*endptr != '\0') { 
            usage(); // strtof has terminated on a non-numeric char
        }
    }

    // constructs a temporary C program
    constructC(argv[1], ml_args);

    // // compiles `c_path`, executes the file constructed, and then deletes 
    // // `c_path` and the output file.
    // compileAndExec(c_path);
    return 0;
}
