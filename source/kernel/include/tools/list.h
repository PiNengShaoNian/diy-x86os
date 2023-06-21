#ifndef LIST_H
#define LIST_H

#include "comm/types.h"

typedef struct _list_node_t
{
    struct _list_node_t *pre;
    struct _list_node_t *next;
} list_node_t;

static inline void list_node_init(list_node_t *node)
{
    node->pre = node->next = (list_node_t *)0;
}

static inline list_node_t *list_node_pre(list_node_t *node)
{
    return node->pre;
}

static inline list_node_t *list_node_next(list_node_t *node)
{
    return node->next;
}

typedef struct _list_t
{
    list_node_t *first;
    list_node_t *last;
    int count;
} list_t;

void list_init(list_t *list);

static inline int list_is_empty(list_t *list)
{
    return list->count == 0;
}

static inline list_node_t *list_first(list_t *list)
{
    return list->first;
}

static inline list_node_t *list_last(list_t *list)
{
    return list->last;
}

static inline int list_count(list_t *list)
{
    return list->count;
}

void list_insert_first(list_t *list, list_node_t *node);
void list_insert_last(list_t *list, list_node_t *node);

list_node_t *list_remove_first(list_t *list);
list_node_t *list_remove(list_t *list, list_node_t *node);

#define offset_in_parent(parent_type, node_name) \
    ((uint32_t)(&(((parent_type *)0)->node_name)))

#define parent_addr(node, parent_type, node_name) \
    ((uint32_t)node - offset_in_parent(parent_type, node_name))

#define list_node_parent(node, parent_type, node_name) \
    ((parent_type *)(node ? parent_addr(node, parent_type, node_name) : 0))

#endif