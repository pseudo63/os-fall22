#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "list.h"
#define BUF_SIZE 40

void list_item_print (void *v) {
    list_item_t *item = (list_item_t *) v;
    printf("%s", (char *)(item->datum));
}

int datum_strcmp (const void *key, const void *with) {
    return strcmp((const char *)key, (const char *)with);
}

// Ctor for list
void list_init (list_t *l, int (*compare)(const void *key, const void *with), void (*datum_delete)(void *datum)) {
    // Dummy head and tail nodes for doubly linked list
    list_item_t* head = malloc(sizeof(list_item_t));
    list_item_t* tail = malloc(sizeof(list_item_t));
    head->next = tail;
    head->pred = NULL;
    tail->pred = head;
    tail->next = NULL;

    l->head = head;
    l->tail = tail;
    l->length = 0;
    l->compare = compare;
    l->datum_delete = datum_delete;
}

// Dtor for list
void list_delete (list_t *l) {
    while (l->length > 0) {
        list_remove_head(l);
    }
    free(l->head);
    free(l->tail);
}

void list_visit_items (list_t *l, void (*visitor)(void *v)) {
    list_item_t *curr = l->head->next;
    while (curr != l->tail) {
        visitor(curr);
        curr = curr->next;
    }
}

void list_insert_tail (list_t *l, void *v) {
    list_item_t *new = malloc(sizeof(list_item_t));
    new->datum = malloc(BUF_SIZE * sizeof(char));
    strcpy(new->datum, v);
    new->pred = l->tail->pred;
    new->next = l->tail;
    l->tail->pred->next = new;
    l->tail->pred = new;
    l->length++;
}

void list_remove_head (list_t *l) {
    if (l->length > 0) {
        list_item_t *del = l->head->next;
        l->head->next = del->next;
        del->next->pred = l->head;
        l->datum_delete(del->datum);
        free(del);
        l->length--;
    }
}

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
    char buffer[BUF_SIZE]; char trimmed[BUF_SIZE];
    while (fgets(buffer, BUF_SIZE, f) != NULL) {
        int i;
        int j = 0;
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
            printf("Unknown command supplied");
            exit(1);
        }
    }
    fclose(f);

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