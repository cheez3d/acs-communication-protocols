#ifndef LIST_H
#define LIST_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

struct list_node {
    void *data;

    struct list_node *prev;
    struct list_node *next;
};

struct list {
    struct list_node *front;
    struct list_node *back;

    size_t size;
};

struct list * list_create(void);

void list_destroy(struct list *list);

static inline bool list_empty(struct list *list) {
    assert(list != NULL);

    return list->size == 0;
}

static inline struct list_node * list_fst(struct list *list) {
    assert(list != NULL);

    return list->front->next;
}

static inline struct list_node * list_lst(struct list *list) {
    assert(list != NULL);

    return list->back->prev;
}

struct list_node * list_add_fst(struct list *list, void *data);
struct list_node * list_add_lst(struct list *list, void *data);

void * list_rm(struct list *list, struct list_node *node);
void * list_rm_fst(struct list *list);
void * list_rm_lst(struct list *list);

struct list_node * list_mk_fst(struct list *list, struct list_node *node);
struct list_node * list_mk_lst(struct list *list, struct list_node *node);

#endif // LIST_H
