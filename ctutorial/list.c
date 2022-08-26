#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "list.h"

// Ctor for list
void list_init(list_t *l, int (*compare)(const void *key, const void *with), void (*datum_delete)(void *datum)) {
    // Dummy head and tail nodes for doubly linked list
    list_item_t* head = malloc(sizeof(list_item_t));
    list_item_t* tail = malloc(sizeof(list_item_t));
    head->next = tail;
    tail->pred = head;

    l->head = head;
    l->tail = tail;
    l->length = 0;
    l->compare = compare;
    l->datum_delete = datum_delete;
}

int main (int argc, char *argv[]) {
    // Verify that two command line arguments are passed
    if (argc != 3) {
        printf("Wrong number of command line parameters supplied\n");
        exit(-1);
    }

    if (strcmp(argv[2], "echo") == 0) {
        // Open file
        FILE* f = fopen(argv[1], "r");
        if (f == NULL) {
            printf("Error opening file %s\n", argv[1]);
        }

        // Read by line until EOF
        char buffer[40]; char trimmed[40];
        while (fgets(buffer, 40, f) != NULL) {
            int i;
            int j = 0;

            // Remove all whitespace
            for (i = 0; i < 40; i++) {
                if (!isspace(buffer[i]) || buffer[i] == '\n') {
                    trimmed[j] = buffer[i];
                    j++;
                }
            }
            // Echo trimmed line
            printf(trimmed);
        }
    } else if (strcmp(argv[2], "tail") == 0) {
        // TODO
    } else if (strcmp(argv[2], "tail-remove") == 0) {
        // TODO
    } else {
        printf("Unknown command supplied");
        exit(-1);
    }
}