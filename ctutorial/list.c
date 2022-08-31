#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "list.h"

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
