#include "tools/list.h"

void list_init(list_t *list)
{
    list->first = list->last = (list_node_t *)0;
    list->count = 0;
}