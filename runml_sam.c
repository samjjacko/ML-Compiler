#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>

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

void closeFiles(FILE * c_fp, FILE * ml_fp) {
    // closes active files, removes temp files created
    fclose(c_fp);
    fclose(ml_fp);
    remove(TEMPNAME);
}

// splits `line` into substrings seperated by spaces
// and places pointers to each of the substrings in `line_substr`
void splitBySpace(char line[], char *line_substr[]) {
    const char delimiter[2] = " "; // splitting by spaces
    char * token = strtok(line, delimiter); // gets the first substring
    line_substr[0] = token;
    int i = 1;
    // adds substrings to `line_substr`
    // adds NULL after the final substring (so we can iterate over it later)
    for(; token != NULL; i++) { 
        token = strtok(NULL, delimiter);  
        line_substr[i] = token;
    }
}

void validateArgv(int argc, char * argv[]) {
    for(int i = 2; i < argc; i++) {
        // per_count stores the number of periods encountered
        int per_count = 0;
        int iterations = 0;
        char *j = argv[i];
        if (*j == '-') j++; 
        for(; *j != '\0'; j++, iterations++) {
            if (*j == '.') {
                // if we've already encountered '.', this can't be valid
                if (per_count) usage();
                per_count++;
            } else if (!isdigit(*j)) {
                usage();
            }
        }
        if (iterations == 0) usage(); // we have a single '-' as an argument, or a weird char. 
        printf("%s \n", argv[i]);
    }
}

void addPreDirectives(char * argv[], FILE * c_fp, int argc) {
    // adds this line so we may call `printf`
    char include[] = "#include <stdio.h> \n";
    if (fputs(include, c_fp) == EOF) {
        fprintf(stderr, "@ Unexpected error writing to file %s \n", TEMPNAME);
    }
    for(int i = 2; i < argc; i++) {
        char directive[BUFSIZ];
        // defines all commandline arguments at top of temporary C file.
        sprintf(directive, "#define arg%d %s \n", i, argv[i]);
        if (fputs(directive, c_fp) == EOF) {
            fprintf(stderr, "@ Unexpected error writing to file %s \n", TEMPNAME);
        }
    }
}

char * formatFunc(char * line_substr[], FILE * ml_fp) {
    // Returns a pointer to a formatted string to be written to c
    // can't have type mismatch because expressions are valid.
    // if any of the arguments in the expression are floats, the whole expression is a float
    // otherwise, it's an int
    // so each function that returns something should have a return type `float`. 
    // types: void or float
    char * vars_in_scope[IDCOUNT]; // pass this to expression handler for each line received in 
    // this function. 
    char argbuffer[BUFSIZ*2]; // because we're adding commas, a type, and a space to each argument
    argbuffer[0] = '\0'; // incase the function has no parameters
    char * argpos = argbuffer;
    for(int i = 2, j = 0; line_substr[i] != NULL; i++, j++) {
        // writes a parameter to argbuffer at position argpos
        sprintf(argpos, "float %s, ", line_substr[i]); 
        argpos += 8 + strlen(line_substr[i]); // 8 for "float , "
        j += 8;
        if (line_substr[i + 1] == NULL) {
            argbuffer[j - 1] = '\0'; // chops off the last comma onwards
        }
    }
    char function_header[BUFSIZ];
    sprintf(function_header, "void %s(%s) {", line_substr[1], argbuffer);
    char * ptr = function_header;
    return ptr;
}

char * formatStat(char * line_substr[], char * identifiers, bool funcscope) {
    // If in function scope, funcscope is true 
    // otherwise false
    int i = 0;
    if (funcscope) i = 1;
    if (!strcmp(line_substr[i], "print")) {
        
    }
    // when handling identifiers, look through the element until you find brackets
    // if you do find brackets, we potentially have a function call
    // check the identifiers list for the function; if it doesn't exist throw a syntax error
    // check the variables in the function - if they're in the current scope then all good
    // otherwise substitute them with `0.0` as per marking description 

    // if we don't find brackets, it must be an assignment - anything else is invalid
    // 
}

void handleProgramItem(char *line_substr[], FILE *c_fp, FILE *ml_fp, char *identifiers[]){
    char * output; 
    if(!strcmp(line_substr[0], "function")) {
        output = formatFunc(line_substr, ml_fp); // formats and writes the function to the C file
        if (fputs(output, c_fp) == EOF) {
            fprintf(stderr, "@ Unexpected error writing to file %s \n", TEMPNAME);
        }
        printf("I am actually god \n");
    } else {
        formatStat(line_substr, identifiers, false);
    }
}

FILE * constructC(char * argv[], int argc) {
    // temporary c file to be written to and executed
    FILE *c_fp = fopen(TEMPNAME, "w"); // to read and write: "w+"
    // the .ml file provided by the path commandline argument
    FILE *ml_fp = fopen(argv[1], "r");
    if (ml_fp == NULL) {
        fprintf(stderr, "! Invalid file path - this file doesn't exist\n");
        closeFiles(c_fp, ml_fp);
    }

    // adds required preprocessor directives, e.g. #include <stdio.h>, 
    // #define arg0 [argv[2]] etc
    addPreDirectives(argv, c_fp, argc); 

    char * declared_identifiers[IDCOUNT]; // stores identifiers that have been declared
    char line[BUFSIZ]; // to store the line read from the .ml file

    while(fgets(line, sizeof line, ml_fp) != NULL) {
        //case handling
        if (line[0] == ' ' || line[0] == '\t') {
            fprintf(stderr, "! Syntax error: lines outside function scope cannot start with ' ' or '\\t' \n");
            closeFiles(c_fp, ml_fp);
        } else if (line[0] == '\n' || line[0] == '#') {
            continue;
        }

        // Stores an array of pointers to space-seperated 
        // substrings of `line`
        char *line_substr[BUFSIZ / 2]; 
        splitBySpace(line, line_substr); // initialises `line_substr`
        
        // handles `program-item`s such as statements and function declarations
        // formats them and writes them to our temporary c file TEMPNAME
        handleProgramItem(line_substr, c_fp, ml_fp, declared_identifiers);
    }
    return c_fp;

}

int main(int argc, char * argv[]) {
    // What did Chris say he wanted the main function to be like?

    // Argument validation
    if (argc < 2 || !strcmp("--help", argv[1])){
        usage();
    } 

    // ensures only valid input commandline arguments have been received 
    // if non-numerical values are encountered, `usage()` is called.
    validateArgv(argc, argv);

    // constructs a temporary C program `c_temp` from a valid ml file
    FILE * c_temp = constructC(argv, argc);

    // // compiles `c_path`, executes the file constructed, and then deletes 
    // // `c_path` and the output file.
    // compileAndExec(c_path);
    return 0;
}
