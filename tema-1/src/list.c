#include "list.h"

#include <assert.h>
#include <stdlib.h>

static struct list_node * list_node_create(void) {
    struct list_node *node = calloc(1, sizeof(*node));

    node->data = NULL;
    node->prev = NULL;
    node->next = NULL;

    return node;
}

static void * list_node_destroy(struct list_node *node) {
    assert(node != NULL);
    assert(node->prev == NULL);
    assert(node->next == NULL);

    void *data = node->data;

    free(node);

    return data;
}

struct list * list_create(void) {
    struct list *list = calloc(1, sizeof(*list));

    struct list_node *head = list->front = list_node_create();
    head->prev = NULL;

    struct list_node *tail = list->back = list_node_create();
    tail->next = NULL;

    head->next = tail;
    tail->prev = head;

    list->size = 0;

    return list;
}

void list_destroy(struct list *list) {
    assert(list != NULL);

    while (!list_empty(list)) {
        list_rm_fst(list);
    }

    free(list->front);
    free(list->back);

    free(list);
}

static struct list_node *
list_add_node(struct list *list,
              struct list_node *node,
              struct list_node *prev,
              struct list_node *next)
{
    assert(list != NULL);
    assert(node != NULL);
    assert(node->prev == NULL);
    assert(node->next == NULL);
    assert(prev != NULL);
    assert(prev->next == next);
    assert(next != NULL);
    assert(next->prev == prev);

    node->prev = prev;
    node->next = next;

    prev->next = node;
    next->prev = node;

    ++list->size;

    return node;
}

static struct list_node *
list_add_node_fst(struct list *list, struct list_node *node) {
    assert(list != NULL);
    assert(node != NULL);
    assert(node->prev == NULL);
    assert(node->next == NULL);

    return list_add_node(list, node, list->front, list_fst(list));
}

static struct list_node *
list_add_node_lst(struct list *list, struct list_node *node) {
    assert(list != NULL);
    assert(node != NULL);
    assert(node->prev == NULL);
    assert(node->next == NULL);

    return list_add_node(list, node, list_lst(list), list->back);
}

struct list_node * list_add_fst(struct list *list, void *data) {
    assert(list != NULL);

    struct list_node *node = list_node_create();
    node->data = data;

    return list_add_node_fst(list, node);
}

struct list_node * list_add_lst(struct list *list, void *data) {
    assert(list != NULL);

    struct list_node *node = list_node_create();
    node->data = data;

    return list_add_node_lst(list, node);
}

static struct list_node *
list_rm_node(struct list *list, struct list_node *node) {
    assert(list != NULL);
    assert(!list_empty(list));
    assert(node != NULL);
    assert(node->prev != NULL);
    assert(node->next != NULL);

    struct list_node *prev = node->prev;
    struct list_node *next = node->next;

    prev->next = next;
    next->prev = prev;

    --list->size;

    node->prev = NULL;
    node->next = NULL;

    return node;
}

static struct list_node * list_rm_node_fst(struct list *list) {
    assert(list != NULL);
    assert(!list_empty(list));

    return list_rm_node(list, list_fst(list));
}

static struct list_node * list_rm_node_lst(struct list *list) {
    assert(list != NULL);
    assert(!list_empty(list));

    return list_rm_node(list, list_lst(list));
}

void * list_rm(struct list *list, struct list_node *node) {
    assert(list != NULL);
    assert(!list_empty(list));
    assert(node != NULL);
    assert(node->prev != NULL);
    assert(node->next != NULL);

    return list_node_destroy(list_rm_node(list, node));
}

void * list_rm_fst(struct list *list) {
    assert(list != NULL);
    assert(!list_empty(list));

    return list_node_destroy(list_rm_node_fst(list));
}

void * list_rm_lst(struct list *list) {
    assert(list != NULL);
    assert(!list_empty(list));

    return list_node_destroy(list_rm_node_lst(list));
}

struct list_node * list_mk_fst(struct list *list, struct list_node *node) {
    assert(list != NULL);
    assert(!list_empty(list));
    assert(node != NULL);
    assert(node->prev != NULL);
    assert(node->next != NULL);

    return list_add_node_fst(list, list_rm_node(list, node));
}

struct list_node * list_mk_lst(struct list *list, struct list_node *node) {
    assert(list != NULL);
    assert(!list_empty(list));
    assert(node != NULL);
    assert(node->prev != NULL);
    assert(node->next != NULL);

    return list_add_node_lst(list, list_rm_node(list, node));
}
