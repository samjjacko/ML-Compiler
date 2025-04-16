//  CITS2002 Project 1 2024
//  Student1:   23360799   Samuel Jackson
//  Platform:   Apple

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>

// these numbers were procured from the CSSE help forum. 
#define BUFSIZE 1024
#define IDCOUNT 50
#define IDSIZE 12
// long random filename to avoid deleting/overwriting meaningful files
// if a user has a file named "tempqwertyuytrewertytrertyuytrtyu.c" in their 
// current directory, they deserve to have it deleted :)
#define TEMPNAME "tempqwertyuytrewertytrertyuytrtyu.c"

// Prints a standard commandline utility program usage message.
// If `err`, the usage message is printed as an error, and the
// program terminates with EXIT_FAILURE, deleting the 
// temporary c file constructed. 
void usage(bool err) {

    // If --help is the only cmd argument
    FILE * output = stdout;
    char char_one = '@';
    int exit_status = EXIT_SUCCESS;

    if(err){
        output = stderr;
        char_one = '!';
        exit_status = EXIT_FAILURE;
    }

    fprintf(output, "%c Usage: ./runml.out path/to/file.ml arg0 arg1 arg2 [...] \n", char_one);
    fprintf(output, "%c arg0, arg1, ... must be integers (e.g. 1, 3000, ...), ", char_one);
    fprintf(output, "or floating-point numbers (e.g. 3.1415). \n");
    fprintf(output, "%c Args can be signed. \n", char_one);
    remove(TEMPNAME); // deletes the temporary file constructed
    exit(exit_status);
}

// closes active files
void closeFiles(FILE * c_fp, FILE * ml_fp) {
    fclose(c_fp);
    fclose(ml_fp);
}

/* Helper function to split a char array by a given delimiter. 
Inspired by `strtok()`. 
Splits a string into several substrings, and fills `buffer`
with those substrings. If a '#' or '\n' is encountered anywhere, 
the current substring is returned and the function terminates.*/
void splitByDelim(char *line, char *buffer[BUFSIZE], char sep) {
    char *temp = malloc(strlen(line));
    strcpy(temp, line); // so we can chop the line up
    // we will move this pointer around, and point it to the start
    // of different strings
    char *ptr = temp;
    // true if not on a seperator character
    bool onsubstr = false;
    int substr_count = 0;
    for(int i = 0; temp[i] != '\0'; i++) {
        if(temp[i] == sep) { // substring located
            // we've come off a substring onto the seperator
            if(onsubstr) { 
                temp[i] = '\0'; // make the end of the substring at index i
                buffer[substr_count] = ptr; // points to the start of this substring
                ptr = temp + i + 1; // index after the seperator
                onsubstr = false;
                substr_count++;
            } else {
                // two seperators in a row - skip over them 
                ptr++; 
            }
        } else if (temp[i] == '#'|| temp[i] == '\n') {
                // no point in checking the rest of the line
                temp[i] = '\0';
                break;
        } else { // normal char 
            onsubstr = true;
        }
    }
    // if the string is terminated without a new seperator at the end, 
    // the last substring is added. 
    buffer[substr_count] = ptr; 
    // adds this to make looping over it later easier
    buffer[substr_count + 1] = "\0";
}

struct scope_ids {
    char *ids[IDCOUNT]; // an array of identifier strings for this scope
    int id_count; // the number of identifiers added to this scope
};

// Stores the global scope and function scope structures 
struct scope_info {
    // a list of identifiers in the global scope, 
    // and the count of identifiers added to the global scope
    struct scope_ids glo_scope; 
    // a list of identifiers in the immediate function scope
    // is reconstructed when function declaration is encountered
    struct scope_ids func_scope;
    // true if inside a local function, and false if 
    // in global scope
    bool in_func;
};

/* Stores function attributes */
struct func_attrs {
    char *type; // a string to the return type of the current function
    char *header; // the function identifier and arguments
    char *identif; // the name of the function - added to global scope once function terminates
    char **body; // pointer to array of lines
    int size; // number of lines allocated for this function
    int linecount; // number of lines of this function's body
} def_func = {.type = "void", .size = 4, .linecount = 0};

/* Returns true if a given identifer `iden` has been declared 
and is in the currently active scope. */
bool idInScope(char *iden, struct scope_info *scopes) {
    if(scopes->in_func) { // we can check function scope too
        for(int i = 0; i < scopes->func_scope.id_count; i++) {
            if(!strcmp(iden, scopes->func_scope.ids[i])) {
                return true;
            }
        }
    } // if the loop terminates or we're not currently  
    // in function scope, check global scope too
    for(int i = 0; i < scopes->glo_scope.id_count; i++) {
        if(!strcmp(iden, scopes->glo_scope.ids[i])) {
            return true;
        }
    }
    // if we still haven't encountered anything
    return false;
}

// adds required headerfiles
void addPreDirectives(char * argv[], FILE * c_fp, int argc) {
    // adds the headerfile required for `printf`
    char include[] = "#include <stdio.h> \n";
    // Ensures correct type formatting when printing
    char print_macro[] = "#define print(a) (int) (a) == (a) ? fprintf(stdout, \"%d\\n\", (int) (a)): fprintf(stdout, \"%6f\\n\", (a)); \n";
    
    // a char array to contain all required preprocessor directives
    char pre_dir[strlen(include) + strlen(print_macro)];
    sprintf(pre_dir, "%s%s", include, print_macro);
    if (fputs(pre_dir, c_fp) == EOF) {
        fprintf(stderr, "@ Unexpected error writing to file %s \n", TEMPNAME);
    }
}

// defines `argname` `value` at the top of TEMPNAME
void defineArg(char argname[], char value[], FILE * c_fp) {
    char directive[BUFSIZE];
    // defines all commandline arguments at top of temporary C file.
    sprintf(directive, "#define %s %s\n", argname, value);
    if (fputs(directive, c_fp) == EOF) {
        fprintf(stderr, "@ Unexpected error writing to file %s \n", TEMPNAME);
    }
}

/* Returns true if a string `maybe_number` is entirely numerical. */
bool isNumerical(char maybe_number[]) {
    int per_count = 0; // stores the number of periods encountered
    int iterations = 0;
    char *j = maybe_number;
        if (*j == '-') j++; // skip the negative sign
        
    for(; *j != '\0'; j++, iterations++) {
        if (*j == '.') {
            // if we've already encountered '.', this can't be valid
            if (per_count) {
                return false;
            }
            per_count++; // increment per_count otherwise
        } else if (!isdigit(*j)) { // non-numeric char
            return false;
        }
    }
    if (!iterations){
        return false; // we have a weird input like " - "
    }
    // survived the tests; it's a number
    return true;
}

// iterates through argv, adding valid arguments to `scope_ids[0]`, and terminating 
// if any arguments are non-numerical
void validateArgv(int argc, char * argv[], struct scope_info *scopes, FILE * c_fp) {
    for(int i = 0; i < argc - 2; i++) {
        // checks if the current argument string is a number
        if (!isNumerical(argv[i + 2])) { 
            // non-numeric arg encountered - program terminates accordingly.
            fclose(c_fp);
            // remove(TEMPNAME);
            usage(true);
        }

        // ASSUMPTION - our arg{number} ID mustn't exceed IDSIZE
        // e.g.: we aren't going to get arg100000000. 
        char *argname = malloc(IDSIZE + 1); 
        sprintf(argname, "arg%d", i);
        // Adds the identifier name to the global scope
        // increments the number of global scope identifiers
        scopes->glo_scope.ids[i] = argname;
        scopes->glo_scope.id_count += 1;

        // writes this arg to the top of the temporary c file
        defineArg(argname, argv[i + 2], c_fp);
    }
}

// adds an identifier to a scope depending on the active scope, 
// and increments the number of identifiers in that scope. 
void addToScope(struct scope_info *scopes, char * iden) {
    char *temp = malloc(strlen(iden));
    strcpy(temp, iden);

    if(scopes->in_func) {
        int count = scopes->func_scope.id_count; 
        scopes->func_scope.ids[count] = temp;
        scopes->func_scope.id_count++;
    } else {
        int count = scopes->glo_scope.id_count;
        scopes->glo_scope.ids[count] = temp;
        scopes->glo_scope.id_count++;
    }
}


/* Checks that a given identifier follows the requirements supplied in the 
project description. That is, terminates if the identifier contains 
numbers or exceeds the allocated IDSIZE. */
void validateIdentifier(char *identif) {
    for (int i = 0; identif[i] != '\0'; i++) {
        if(isdigit(identif[i]) || i == IDSIZE) {
            fprintf(stderr, "! Error: invalid identifier \"%s\".\n", identif);
            fprintf(stderr, "! Identifiers cannot contain numbers, or exceed 12 chars.\n");
            usage(true);
        }
    }
}

/* Adds the identifier of a function (e.g. "foo") to global scope, 
and formats/adds to the `identif` and `header` fields of the 
func_attrs struct `func`. */
void constructFuncHeader(char *substrs[], struct scope_info *scopes, struct func_attrs *func) {
    // check that the function identifier is actually valid 
    validateIdentifier(substrs[1]);
    char argbuffer[BUFSIZE*2]; // because we're adding commas, a type, and a space to each argument
    argbuffer[0] = '\0'; // incase the function has no parameters
    char * argpos = argbuffer;
    int j = 0;
    for(int i = 2; strcmp(substrs[i], "\0"); i++, j++) {
        // checks that function arguments are well behaved
        validateIdentifier(substrs[i]);
        // writes a parameter to argbuffer at position argpos
        sprintf(argpos, "float %s, ", substrs[i]); 
        // Writes the function parameter to the function's scope
        addToScope(scopes, substrs[i]);
        argpos += 8 + strlen(substrs[i]); // 8 for "float , "
        j += 7 + strlen(substrs[i]);
    }
    argbuffer[j - 2] = '\0'; // chops off the last comma onwards
    char *function_header = malloc(BUFSIZE);
    // everything but the return type
    sprintf(function_header, "%s(%s) { \n", substrs[1], argbuffer); 
    char * identif = malloc(strlen(substrs[1]));
    strcpy(identif, substrs[1]);
    func->identif = identif;
    func->header = function_header;
}

// Looks through an array from a specified starting index (startpos)
// and substitutes undefined identifiers with `0.0`, before returning the array
// Substitutes `0.0` for undeclared variables in these cases:
//    identifier "<-" expression
// |  print  expression
// |  return expression
// replaces any undeclared identifiers with 0.0, and returns the expression. 
char * substituteArgs(int start, char *line, struct scope_info *scopes) {
    // case handling
    if(line[start] == '\0' || line[start] == '\n' || line[start] == '#') {
        return "\0"; // ignore this substring
    }
    for(int i = 0; line[i] == ' '; i++) { // skip any initial spaces
        start++;
    }
    char *outstr = malloc(BUFSIZ);
    int j = 0;
    for(char *i = &line[start]; *i != '\0' && *i != '\n'; i++) {
        if(isalnum(*i)) {
            char iden[IDSIZE];
            // adds chars to iden until an identifier is encountered
            int k = 0;
            for(; isalnum(*i) && *i != '\0' && *i != '\n'; k++, i++) {
                iden[k] = *i;
            } 
            iden[k] = '\0'; // making sure it's well behaved
            // once this loop terminates, we have an identifier
            char *outpos = outstr;
            outpos += j;
            if(isNumerical(iden) || idInScope(iden, scopes)) {
                // we're safe to add it to our output string
                sprintf(outpos, "%s", iden); 
                j += k;
            } else{
                if(k > 2) {
                    if(isdigit(iden[3])) { // we have an `arg{number}`, e.g. a cmd arg
                        sprintf(outpos, "%s", iden);
                        j += k;
                    } else {
                        outstr[j] = '0';
                        j++;
                    }
                } else { // id not in scope - substitute zero 
                    outstr[j] = '0';
                    j++;
                }
            } 
            i--; // so we don't get ahead of ourselves in the loop above
        } else if (*i == '#') {
            return outstr; // we must have a valid line
        } else { // we have '(', ')', '+', etc. we can add this to our output str
            outstr[j] = *i;
            j++;        
        }
    }
    return outstr;
}

/* Dynamically adds a line to the "body" char *array[] of a given 
function struct. If we exceed more lines than size allocated, we 
double the space allocated.*/
void addFuncLine (struct func_attrs *func, char *line) {
    if(func->linecount == 0) { // construct new 2d array
        func->body = malloc(4 * sizeof(char*));
    }
    int lines = func->linecount;
    if(lines == func->size) { // need to increase size
        func->size *= 2;
        // chars are universally one byte
        char **newbody = malloc(func->size * 2);
        for(int i = 0; i < lines; i++) {
            newbody[i] = func->body[i];
        }
        free(func->body);
        func->body = newbody;
    }
    func->body[lines] = line;
    func->linecount += 1;
}

/* Using the information supplied in the func_attrs struct `func`, 
a the function is written to the c file TEMPNAME. */
void writeFunc(struct func_attrs *func, FILE * c_fp) {
    char func_line[BUFSIZE];
    // writes `{type} {identifier}({args}) { \n` to `func_line`
    sprintf(func_line, "%s %s", func->type, func->header);
    if (fputs(func_line, c_fp) == EOF) { // writes function header to C file
        fprintf(stderr, "@ Unexpected error writing to file %s \n", TEMPNAME);
    }
    for(int i = 0; i < func->linecount; i++) {
        if (fputs(func->body[i], c_fp) == EOF) { // writes each line of the function
            fprintf(stderr, "@ Unexpected error writing to file %s \n", TEMPNAME);
        }
    }
}

/* Validates a given assignment statement's identifier (supplied in substrs[0]; the 
first space-seperated substring of a .ml line), and then formats the assignment 
statement to valid c syntax, before writing the statement to `outline`. */
void assignment(char *substrs[], struct scope_info *scopes, char line[], char *outline) {
    char *ptr = substrs[0];
    for(int i = 0; substrs[0][i] != '\0'; i++) {
        if (substrs[0][i] == '\t') ptr++;
        if(isdigit(substrs[0][i]) || i == 12) {
            fprintf(stderr, "! Error: invalid identifier \"%s\".\n", substrs[0]);
            fprintf(stderr, "! Identifiers cannot contain numbers, or exceed 12 chars.\n");
            usage(true);
        }
    }
    addToScope(scopes, ptr); // adds the assigned identifier to the active scope 
    char *assign_exp = substituteArgs(strlen(substrs[0]) + 4, line, scopes);
    sprintf(outline, "float %s = %s;\n", ptr, assign_exp); 
}

/* Adds a statement (function calls, print statements, and return statements
for non-main functions) to the func_attr struct `func`. */
void handleStat(char *substrs[], struct scope_info *scopes, char *line, struct func_attrs *func) {
    char *outline = malloc(BUFSIZE);
    if (!strcmp(substrs[0], "print") || !strcmp(substrs[0], "\tprint")){
        //"print" is 5 letters long
        char *print_exp = substituteArgs(strlen(substrs[0]), line, scopes);
        sprintf(outline, "\tprint(%s)\n", print_exp);
        addFuncLine(func, outline);
    } else if (!strcmp(substrs[1], "<-")) {
        // adds identifier to active scope
        // writes the formatted line to `outline`
        assignment(substrs, scopes, line, outline);
        addFuncLine(func, outline); 
    } else if (substrs[0][0] == '#' || substrs[0][0] == '\n'){
        return;
    } else { 
        if (!strcmp(substrs[0], "return")) {
            fprintf(stderr, "! Error: invalid return statement in global scope.\n");
            usage(true);
        }
        // we must have a function call ()
        char temp[IDSIZE]; 
        int siz = strlen(substrs[0]);
        int i = 0;
        // don't need to check for '\n' or '#' - these are handled
        // by splitByDelim()
        for(; substrs[0][i] != '('; i++) {
            if(substrs[0][i] == '\0') {
                fprintf(stderr, "! Error: invalid statement \"%s\". \n", substrs[0]);
                fprintf(stderr, "! Function calls must follow the format `{name}({args})`\n");
                usage(true);
            }
            if (i + 1 == siz) {
                // throw a tantrum - invalid functioncall
            } else if (i > IDSIZE) {
                // function doesn't exist - we would've complained earlier
            } // that's a problem, we're writing the whole thing into it 
            temp[i] = substrs[0][i];
        }
        char *func_args = substituteArgs(i, line, scopes); // include left bracket - hence
        temp[i] = '\0';
        if (idInScope(temp, scopes)) {
            sprintf(outline, "%s%s;\n", temp, func_args); 
            addFuncLine(func, outline); // had a silly bug here
        } else {
            fprintf(stderr, "! Error: undeclared function \"%s\". \n", temp);
            usage(true);
        }
    }
}

/* Handles global statements such as print statements, function declarations, 
declaration of globally scoped variables, etc. 
Returns statements in the global scope to be immediately written into TEMPFILE. 
If the statement cannot be printed because it requires later information, 
it is added to the relevant `func_attrs` struct's body, and "\0" is returned. */
char * handleGlobStat(char *substrs[], char line[], struct scope_info *scopes, struct func_attrs *func, struct func_attrs *main_func) {
    if(line[0] == '\n' || line[0] == '#') return "\0";
    if(!strcmp(substrs[0], "function")) {
        scopes->in_func = true;
        if(func->body != NULL) {
            free(func->body);
            // "reset" the local function scope
            scopes->func_scope.id_count = 0;
        } 
        *func = def_func; // initialises `func` to a new default
        constructFuncHeader(substrs, scopes, func);
    } else if (!strcmp(substrs[1], "<-")) {
        char *outline = malloc(BUFSIZE);
        assignment(substrs, scopes, line, outline);
        return outline; // to be printed directly to global scope
    } else if (line[0] == ' ' || line[0] == '\t') {
        fprintf(stderr, "! Syntax error: lines outside function scope cannot start with ' ' or '\\t' \n");
        usage(true); 
    } else { // function call or print statement
        handleStat(substrs, scopes, line, main_func);
    }
    return "\0";
}

// initialises the main scope struct
struct scope_info* initScopes() {
    // stores globally scoped identifiers
    struct scope_ids glo_scope;
    // stores identifiers in local function scope
    struct scope_ids func_scope;
    glo_scope.id_count = 0; func_scope.id_count = 0;
    struct scope_info *scopes = malloc(sizeof(struct scope_info));
    // sets the default active scope to "false", e.g. in global scope
    struct scope_info defau = {glo_scope, func_scope, false};
    *scopes = defau;
    return scopes;
}


/* Constructs the default parameters for the struct representing the main 
function: `main_func`.*/
void configureMain(struct func_attrs *main_func) {
    main_func->type = "int";
    main_func->header = "main(){\n";
}


/* Loops over the .ml file provided by the path name argument, and passes 
each line to several helper functions depending on whether the line is 
indented, a function declaration etc. 
Writes to the c file TEMPNAME, pointed to by `c_fp`. */
void constructC(char * argv[], int argc, struct scope_info *scopes, FILE *c_fp) {
    // temporary c file to be written to and executed
    // the .ml file provided by the path commandline argument
    FILE *ml_fp = fopen(argv[1], "r");
    if (ml_fp == NULL) {
        fprintf(stderr, "! Invalid file path - this .ml file doesn't exist\n");
        closeFiles(c_fp, ml_fp);
        usage(true);
    }

    // adds required preprocessor directives, such as headerfiles and definitions
    addPreDirectives(argv, c_fp, argc); 
    char line[BUFSIZE]; // to store the line read from the .ml file

    // main func will store lines required in the main function
    // e.g. globally scoped print statements, function calls, etc
    struct func_attrs main_func = def_func;
    configureMain(&main_func);
    // `func` simply holds the contents of a function while it is constructed
    // and is overwritten when a new function is encountered
    struct func_attrs func = def_func;

    // main loop
    char *substrs[BUFSIZE/12];  // holds substrings of a given line
    while(fgets(line, sizeof line, ml_fp) != NULL) {
        // case handling
        if (line[0] == '\n' || line[0] == '#') {
            continue;
        }
        // splits the line into space seperated substrings - if '#' is encountered in a substring, 
        // the left hand side of the '#' (from the last delimiter) becomes the final substr
        splitByDelim(line, substrs, ' '); 
        if (!scopes->in_func) { // in global scope
            // Adds returns globally scoped statements, unless they're print
            // statements, (which are added to the main function `main_func`), 
            // or function declarations (a `func` instance is constructed).
            // Any statement to be added to a function's body causes a return value of "\0". 
            char *globStat = handleGlobStat(substrs, line, scopes, &func, &main_func);
            if (!strcmp(globStat, "\0")) { // we have a function declaration 
                continue;
            } else { // globally scoped statement
                if (fputs(globStat, c_fp) == EOF) { 
                    // writes the globally scoped statement straight to the c file
                    fprintf(stderr, "@ Unexpected error writing to file %s \n", TEMPNAME);
                }
            }
        } else { // in function scope
            // function body has finished - no return statement
            if (line[0] != ' ' && line[0] != '\t') { 
                addFuncLine(&func, "}\n"); // adds the closing bracket for the function
                writeFunc(&func, c_fp); // writes the function to TEMPNAME
                
                scopes->in_func = false; // we are now outside of function scope
                // add the function identifier to global scope, so we 
                // can successfully reference it later 
                addToScope(scopes, func.identif); 

                // handles the new statement outside of the function scope
                handleStat(substrs, scopes, line, &main_func);

            } else if (!strcmp(substrs[0], "\treturn") || !strcmp(substrs[0], "return")) { 
                // checks arguments and substitutes 0 for undeclared args
                // sets `ret_exp` to the expression to be returned
                char * ret_exp = substituteArgs(strlen(substrs[0]), line, scopes);
                char func_line[BUFSIZE];
                // constructing the full return statement
                sprintf(func_line, "\treturn %s;}\n", ret_exp);
                // adds the return statement to the function's body
                addFuncLine(&func, func_line);
                
                // if we have a return statement, the only possible return type
                // for this function is "float". 
                func.type = "float";
                writeFunc(&func, c_fp); 
                scopes->in_func = false;
                addToScope(scopes, func.identif); 

            // user has attempted to nest functions 
            } else if (!strcmp(substrs[0], "function") || !strcmp(substrs[0], "\tfunction")) {
                fprintf(stderr, "! Error: nested functions are not supported in ML. \n");
                usage(true);
            // we must have a print statement, assignment, or functioncall 
            // inside this function
            } else {
                // writes the statement to the body of `func`. 
                handleStat(substrs, scopes, line, &func);
            }
        }
    }
    // once all lines from our ml file have been read, 
    // we write the main function at the bottom of the C file. 
    writeFunc(&main_func, c_fp);
    // writes the closing bracket at the end of the last body statement
    // from `main_func`. 
    if (fputs("}\n", c_fp) == EOF) { 
        fprintf(stderr, "@ Unexpected error writing to file %s \n", TEMPNAME);
    }
    fclose(ml_fp);
}

void compileAndExec(FILE * c_fp) {
    char cmd[BUFSIZ];
    int c_name_len = strlen(TEMPNAME);
    char output[c_name_len + 3];
    strcpy(output, TEMPNAME);
    char *ptr = &output[c_name_len - 2];
    // copies the name TEMPNAME but explicitly changes the file
    // extension to ".out". 
    sprintf(ptr, ".out");
    // formatting the instructions for the c compiler
    sprintf(cmd, "cc -std=c11 -o ./%s ./%s", output, TEMPNAME);
    fclose(c_fp);
    system(cmd);

    pid_t pid=fork();
    if (pid == 0) {  // child process
        char *argv[] = {};
        execv(output, argv);
        exit(127); // command could not be found
    }
    else { 
        waitpid(pid,0,0); // wait for child to terminate
        // delete helper files
        remove(TEMPNAME);
        remove(output);
    }
}

int main(int argc, char * argv[]) {
    // Argument validation
    // prints usage msg via stderr stream
    if (argc < 2) usage(true); 
    //prints usage msg text via stdout stream if --help is called
    if (!strcmp("--help", argv[1])) usage(false); 

    // initialises a struct to store identifiers and the number of identifiers 
    // for each scope, as well as the currently active scope. 
    struct scope_info* scopes = initScopes();

    // constructs a temporary c file which we write into using `constructC`. 
    FILE *c_fp = fopen(TEMPNAME, "w"); 
    if (c_fp == NULL) {
        fprintf(stderr, "! Error: could not construct temporary c file\n");
    }
    // Validates and adds cmd arguments to `scopes->glo_scope.ids` 
    // that is, adds commandline arguments to the global scope
    // and defines them at the top of our temporary C file
    // terminates if invalid arguments are encountered
    validateArgv(argc, argv, scopes, c_fp);

    // constructs a temporary C program `c_temp` from a valid ml file
    constructC(argv, argc, scopes, c_fp);
    
    // compiles `c_path`, executes the file constructed, and then deletes 
    // `TEMPNAME` and the output file produced by cc.
    compileAndExec(c_fp);
    return 1;
}

// ASSUMPTION: 
/* we cannot have something like
    ```
    hi <- 3 + 5000
    x <- hi + 3
    ```
in the global scope, because "hi" is not a constant. 
To do this sort of operation, it is expected a user will use a function instead
*/

// should double triple check everything