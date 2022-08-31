#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "list.h"

int main (int argc, char *argv[]) {
    // Verify that two command line arguments are passed
    if (argc != 3) {
        printf("Wrong number of command line parameters supplied\nUsage: list_harness <input filename> <instruction>\n");
        exit(1);
    }

    // Open file
    FILE* f = fopen(argv[1], "r");
    if (f == NULL) {
        printf("Error opening file %s\n", argv[1]);
        exit(1);
    }

    // Initialize list
    list_t l;
    list_init(&l, datum_strcmp, free);

    // Read by line until EOF
    int fileNotEmpty = 0;
    char buffer[BUF_SIZE]; char trimmed[BUF_SIZE];
    while (fgets(buffer, BUF_SIZE, f) != NULL) {
        int i; int j = 0;
        fileNotEmpty = 1;
        // Remove all whitespace
        for (i = 0; i < BUF_SIZE; i++) {
            if (!isspace(buffer[i]) || buffer[i] == '\n') {
                trimmed[j] = buffer[i];
                j++;
            }
        }
        if (strcmp(argv[2], "echo") == 0) {
            // Echo trimmed line
            printf("%s", trimmed);
        } else if (strcmp(argv[2], "tail") == 0) {
            // add line to list
            list_insert_tail(&l, trimmed);
        } else if (strcmp(argv[2], "tail-remove") == 0) {
            // add line to list
            list_insert_tail(&l, trimmed);
        } else {
            printf("Unknown command supplied\n");
            exit(1);
        }
    }
    fclose(f);

    if (!fileNotEmpty) {
        printf("<empty>\n");
        exit(1);
    }

    if (strcmp(argv[2], "tail") == 0) {
        list_visit_items(&l, list_item_print);
    } else if (strcmp(argv[2], "tail-remove") == 0) {
        while (l.length > 0) {
            list_remove_head(&l); list_remove_head(&l); list_remove_head(&l);
            list_visit_items(&l, list_item_print);
            printf("----------------------------------\n");
        }
    }
    list_delete(&l);
    return 0;
}