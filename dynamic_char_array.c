#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// could use define to make the dynamic array usage easier
struct DynamCharArray {
    int size; // amount of space allocated
    int count; // number of characters
    char *string; // pointer to the (current) char array
};

void myPush(struct DynamCharArray *array, char s) {
    // appends char s to array
    if(array->count + 1 > array->size) { // reached max size
        // new array twice the size of the original
        char temp[array->size * 2];

        // copy everything over
        int j = 0;
        for(char *k = array->string; *k != '\0'; k++, j++) {
            temp[j] = *k;
        }
        temp[j + 1] = '\0';

        strcpy(array->string, temp);
        array->size *= 2;
    } 
    // the append step
    array->string[array->count] = s; 
    array->count += 1;
    array->string[array->count] = '\0'; // re-add null char 
}

char myPop(struct DynamCharArray *arr) {
    if(arr->count == 0) {
        fprintf(stderr, "DynamCharArray error: cannot pop empty list.");
        exit(EXIT_FAILURE);
    }
    char c = arr->string[arr->count - 1];
    arr->string[arr->count - 1] = '\0';
    arr->count -= 1;
    return c;
}

void myInsert(struct DynamCharArray *array, int i, char s) {
    if(i > array->count - 1 || i < 0) {
        // throw new IndexOutOfBounds Error
        fprintf(stderr, "index out of bounds \n");
        exit(EXIT_FAILURE);
    }
    array->string[i] = s;
}


int main(int argc, char *argv[]) {
    // usage: size, number of chars, char array
    char yes[3] = "11\0";
    struct DynamCharArray temp = {atoi(argv[1]), atoi(argv[2]), yes};
    // checking that this works
    // not tested much so far - suppied with commandline args `2 0` 
    for(int i = 0; i < 5; i++) {
        myPush(&temp, 'i');
        printf("%s\n", temp.string);
    }
    for(int i = 1; i < 4; i++) {
        myInsert(&temp, i, 'x');
    }
    printf("%s\n", temp.string);
    for(int i = 0; i < 5; i++) {
        char nothing = myPop(&temp);
        printf("%s\n", temp.string);
    }
    printf("Final array fields: \n size = %d \n count = %d \n", temp.size, temp.count);
}