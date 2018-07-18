#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "linkedlist.h"

bool ll_create(LinkedStringNode** head, char *value) {
    // double pointer important!
    *head = (LinkedStringNode*)malloc(sizeof(LinkedStringNode));
    if(NULL == (*head)) {// typically as one statement
        free(*head); // ¯\_(ツ)_/¯
        return false;
    }
    // else...
    (*head)->value = malloc((strlen(value)+1)*sizeof(char));
    if(NULL == (*head)->value) {
        free((*head)->value);
        return false;
    }
    strcpy((*head)->value, value);
    (*head)->next = NULL;
    return true;
}

bool ll_insert(LinkedStringNode* node, char *value) {
    LinkedStringNode* new = (LinkedStringNode*)malloc(sizeof(LinkedStringNode));
    if(NULL == new) {// typically as one statement
        free(new); // ¯\_(ツ)_/¯
        return false;
    }
    // else...
    new->value = malloc((strlen(value)+1)*sizeof(char));
    if(NULL == new->value) {
        free(new->value);
        return false;
    }
    strcpy(new->value, value);
    new->next = node->next; // not NULL by default
    node->next = new;
    return true;
}

bool ll_append(LinkedStringNode* head, char *value) {
    if(NULL != head->next) {
        return ll_append(head->next, value);
    }
    else {
        return ll_insert(head, value);
    }
}

void ll_free(LinkedStringNode* head) {
    if(NULL == head) {
        return;
    }
    ll_free(head->next);
    free(head);
}

bool ll_remove(LinkedStringNode** head, size_t index) {
    if(index == 0) {
        (*head) = (*head)->next;
        return true;
    }
    else if(index == 1) {
        if(NULL == (*head)->next) {
            return false;
        }
        // else
        (*head)->next = (*head)->next->next;
        return true;
    }
    else {
        if(NULL == (*head)->next) {
            return false;
        }
        return ll_remove(&((*head)->next), index -1);
    }
}

char* ll_get(LinkedStringNode* head, size_t index) {
    if(index == 0) {
        return head->value;
    }
    else {
        return (NULL == head->next)?false:ll_get(head->next, index-1);
    }
}

LinkedStringNode* ll_fromArray(char *inputArray[], size_t size) {
    size_t n = size/sizeof(inputArray[0]); // Doesn't handle len(array) == 0
    LinkedStringNode *head, *current;
    ll_create(&head, inputArray[0]);
    current = head;
    for(size_t i = 1; i < n; i++) {
        ll_append(current, inputArray[i]);
        current = current->next;
    } 
    return head;
}
