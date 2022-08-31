// List.h header file
#ifndef LIST
#define LIST
#define BUF_SIZE 42

typedef struct list_item {
    struct list_item *pred, *next;
    void *datum;
} list_item_t;


typedef struct list {
    list_item_t *head, *tail;
    unsigned length;
    int (*compare)(const void *key, const void *width);
    void (*datum_delete)(void *);
} list_t;

void list_item_print(void *v);
int datum_strcmp(const void *key, const void *with);

void list_init(list_t *l, int (*compare)(const void *key, const void *with), void (*datum_delete)(void *datum));
void list_delete(list_t *l);
void list_visit_items(list_t *l, void (*visitor)(void *v));
void list_insert_tail(list_t *l, void *v);
void list_remove_head(list_t *l);

#endif