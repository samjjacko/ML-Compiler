#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// implementing a stack with a linked list

// broken 

struct Node
{
    char cur;
    struct Node *prev;
    struct Node *next;
};

struct Stack {
    struct Node start;
    struct Node end;
};

struct Stack newStack(char c) {
    struct Node start = {c, NULL, NULL};
    struct Node end = {'\0', &start, NULL};
    start.next = &end;
    struct Stack newst = {start, end};
    return newst;
}

void push(struct Stack *st, char c) {
    st->end.cur = c; // push
    struct Node newend = {'\0', &(st->end), NULL};
    st->end.next = &newend;
    st->end = newend; // set the stack's endmost node to this
}

int main(int argc, char *argv[]) {
    struct Stack temp = newStack('i'); // must create a stack with one element
    for(int i = 0; i < 4; i++) {
        push(&temp, 'a' + i);
    }
    struct Node n = temp.start;
    while(n.next != NULL) {
        printf("%c\n", n.cur);
        n = *n.next;
    }
}