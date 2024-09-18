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

// So we can keep track of the return/printed type of each function/statement
#define UNKNOWN_TYPE 0
#define INT_TYPE 1
#define FLOAT_TYPE 2

void usage(bool err) {
    int output = stdout;
    char char_one = '@';
    if(err){
        output = stderr;
        char_one = '!';
    }
    fprintf(output, "%c Usage: ./runml.out path/to/file.ml arg0 arg1 arg2 [...] \n", char_one);
    fprintf(output, "%c arg0, arg1, ... must be integers (e.g. 1, 3000, ...), ", char_one);
    fprintf(output, "or floating-point numbers (e.g. 3.1415). \n");
    fprintf(output, "%c Args can be signed. \n", char_one);
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

struct id {
    char id[IDSIZE + 1];
    int type_def;
};

// Checks if commandline args are valid integers or floats
// Standard utility program "usage" text is printed if invalid
void validateArgv(int argc, char * argv[], struct id *declared_ids[], int *ids_stored) {
    for(int i = 2; i < argc; i++) {
        // per_count stores the number of periods encountered
        int per_count = 0;
        int iterations = 0;
        char *j = argv[i];
        if (*j == '-') j++; 
        for(; *j != '\0'; j++, iterations++) {
            if (*j == '.') {
                // if we've already encountered '.', this can't be valid
                if (per_count) usage(true);
                per_count++;
            } else if (!isdigit(*j)) {
                usage(true);
            }
        }
        if (iterations == 0) usage(true); // we have a single '-' as an argument, or a weird char. 

        // placing this argument in `declared_ids`
        char argname[IDSIZE + 1]; // ASSUMPTION - our arg{number} ID mustn't exceed IDSIZE
        sprintf(argname, "arg%d", i - 2);

        if(per_count) { // non-zero per_count => this arg is a float
            struct id cur_arg = {argname, FLOAT_TYPE};
            declared_ids[*ids_stored] = &cur_arg; // address is a pointer to the struct
        } else {
            struct id cur_arg = {argname, INT_TYPE};
            declared_ids[*ids_stored] = &cur_arg; 
        }
        *ids_stored += 1;
        // printf("%s \n", argv[i]);
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

// groups and formats a function's arguments to appropriate C syntax
// if args have not been declared in the current scope, 0.0 is substituted
char * groupCheckArgs(char * line_substr[], int start_index, char * identifiers) {
    char argstr[BUFSIZ];
    char * argpos = argstr; // so we can write to argstr with sprintf
    bool print_stat = false;
    if(*line_substr[start_index] != '(') { // print statement - add brackets
        print_stat = true;
        char * argpos = argstr + 1; // don't complain at me please
        argstr[0] = '(';
    }
    for(int i = start_index, j = 0; line_substr[i] != NULL; i++, j++) {
        sprintf(argpos, "%s, ", line_substr[i]); 
        argpos += 2 + strlen(line_substr[i]); // 2 for ", "
        j += 2;
        if (line_substr[i + 1] == NULL) {
            if (print_stat) {
                argstr[j - 1] = ')'; 
                argstr[j] = '\0';
            } else {
                // the last substring already ends with a bracket
                argstr[j - 1] = '\0';
            }
        }
    }
    return &argstr; 
    // if that doesn't work
    // char * ptr = argstr;
    // return ptr;
}

char * formatStat(char * line_substr[], char * identifiers[], bool funcscope, int type_def) {
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
    // add variables in the assignment to the identifiers
}

char * constructFuncHeader(char * line_substr[]) {
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
    // everything but the return type
    sprintf(function_header, "%s(%s) {", line_substr[1], argbuffer); 
    return &argbuffer; //passes the address of this array
}

// need to pass declared id's through to check the types of arg0, arg1 etc 
char * formatFunc(char * line_substr[], FILE * ml_fp, struct id *declared_ids[], int *ids_stored) {
    // Returns a pointer to a formatted string to be written to c
    // Assumption: can't have type mismatch because expressions are valid.
    // if any of the arguments in the expression are floats, the whole expression is a float
    // otherwise, it's an int

    
    // stores any vars encountered in the function, including their types. 
    struct id *vars_in_scope[IDCOUNT]; 
    // also need to keep track of the number of ID's in scope
    // copies all commandline args to args in function scope
    for(int i = 0; i < *ids_stored; i++) {
        if (strtok(declared_ids[i]->id, "arg") != NULL) {
            // the arg is one of the commandline args
            vars_in_scope[i] = declared_ids[i]; 
        } else break;
    }
    
    char *argbuffer = constructFuncHeader(line_substr);

    // return type is either float, void, or (rarely) int
    // sprintf(function_header, "void %s(%s) {", line_substr[1], argbuffer);

    // constructFuncStatements();
    char line[BUFSIZ];
    char returntype[6];
    char func_output[BUFSIZ * 6];

    while(fgets(line, sizeof line, ml_fp) != NULL) {
        char * temp_substr[BUFSIZ/2];
        splitBySpace(line, temp_substr);
        if (temp_substr[0] == '\t') { // still in funcScope
            // getExpType()
            if (!strcmp(temp_substr[1], "return")) { 
                // Looks through args and determines the return type of the function
                char *exp_type = getExpType(temp_substr, vars_in_scope);
                sprintf(returntype, "%s", exp_type);
            }
        }
    }
}

void handleProgramItem(char *line_substr[], FILE *c_fp, FILE *ml_fp, struct id *declared_ids[], int ids_stored){
    char * output; 
    if(!strcmp(line_substr[0], "function")) {
        // reads the function header, formats the function (and statements in function scope)
        // into a long char array, which is returned to `output`
        output = formatFunc(line_substr, ml_fp, declared_ids, ); 
    } else {
        // formats and writes any other statement to c file
        output = formatStat(line_substr, declared_ids, false, UNKNOWN_TYPE);
    }
    if (fputs(output, c_fp) == EOF) {
            fprintf(stderr, "@ Unexpected error writing to file %s \n", TEMPNAME);
    }
    printf("I am actually god \n");
}

FILE * constructC(char * argv[], int argc, struct id *declared_ids[], int ids_stored) {
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
    char line[BUFSIZ]; // to store the line read from the .ml file

    // main loop
    while(fgets(line, sizeof line, ml_fp) != NULL) {
        // case handling
        if (line[0] == ' ' || line[0] == '\t') {
            fprintf(stderr, "! Syntax error: lines outside function scope cannot start with ' ' or '\\t' \n");
            closeFiles(c_fp, ml_fp);
            usage(true);
        } else if (line[0] == '\n' || line[0] == '#') {
            continue;
        }

        // Stores an array of pointers to space-seperated 
        // substrings of `line`
        char *line_substr[BUFSIZ / 2]; 
        splitBySpace(line, line_substr); // initialises `line_substr`
        
        // handles `program-item`s such as statements and function declarations
        // formats them and writes them to our temporary c file TEMPNAME
        handleProgramItem(line_substr, c_fp, ml_fp, declared_ids, ids_stored);
    }
    return c_fp;

}

int main(int argc, char * argv[]) {
    // What did Chris say he wanted the main function to be like?

    // Argument validation
    // prints usage msg via stderr stream
    if (argc < 2 ) usage(true); 
    //prints usage msg text via stdout stream if --help is called
    if (!strcmp("--help", argv[1])) usage(false); 

    // an array of pointers to structures
    // the structures contain an id for the identifier, and its type. 
    struct id_and_type *declared_ids[IDCOUNT]; 
    int ids_stored = 0; // keeps track of the number of identifiers added to `declared_ids`

    // ensures only valid input commandline arguments have been received 
    // if non-numerical values are encountered, `usage(true)` is called.
    // adds commandline args to "declared_identifiers"
    validateArgv(argc, argv, declared_ids, &ids_stored);

    // constructs a temporary C program `c_temp` from a valid ml file
    FILE * c_temp = constructC(argv, argc, declared_ids, ids_stored);

    // // compiles `c_path`, executes the file constructed, and then deletes 
    // // `c_path` and the output file.
    // compileAndExec(c_path);
    return 0;
}
