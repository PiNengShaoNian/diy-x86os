#include "tools/list.h"

void list_init(list_t *list)
{
    list->first = list->last = (list_node_t *)0;
    list->count = 0;
}

void list_insert_first(list_t *list, list_node_t *node)
{
    node->next = list->first;
    node->pre = (list_node_t *)0;
    if (list_is_empty(list))
    {
        list->first = list->last = node;
    }
    else
    {
        list->first->pre = node;
        list->first = node;
    }

    list->count++;
}

void list_insert_last(list_t *list, list_node_t *node)
{
    node->next = (list_node_t *)0;
    node->pre = list->last;

    if (list_is_empty(list))
    {
        list->first = list->last = node;
    }
    else
    {
        list->last->next = node;
        list->last = node;
    }

    list->count++;
}

/**
 * 移除指定链表的头部
 * @param list 操作的链表
 * @return 链表的第一个结点
 */
list_node_t *list_remove_first(list_t *list)
{
    // 表项为空，返回空
    if (list_is_empty(list))
    {
        return (list_node_t *)0;
    }

    // 取第一个结点
    list_node_t *remove_node = list->first;

    // 将first往表尾移1个，跳过刚才移过的那个，如果没有后继，则first=0
    list->first = remove_node->next;
    if (list->first == (list_node_t *)0)
    {
        // node为最后一个结点
        list->last = (list_node_t *)0;
    }
    else
    {
        // 非最后一结点，将后继的前驱清0
        remove_node->next->pre = (list_node_t *)0;
    }

    // 调整node自己，置0，因为没有后继结点
    remove_node->next = remove_node->pre = (list_node_t *)0;

    // 同时调整计数值
    list->count--;
    return remove_node;
}

/**
 * 移除指定链表的中的表项
 * 不检查node是否在结点中
 */
list_node_t *list_remove(list_t *list, list_node_t *remove_node)
{
    // 如果是头，头往前移
    if (remove_node == list->first)
    {
        list->first = remove_node->next;
    }

    // 如果是尾，则尾往回移
    if (remove_node == list->last)
    {
        list->last = remove_node->pre;
    }

    // 如果有前，则调整前的后继
    if (remove_node->pre)
    {
        remove_node->pre->next = remove_node->next;
    }

    // 如果有后，则调整后往前的
    if (remove_node->next)
    {
        remove_node->next->pre = remove_node->pre;
    }

    // 清空node指向
    remove_node->pre = remove_node->next = (list_node_t *)0;
    --list->count;
    return remove_node;
}
