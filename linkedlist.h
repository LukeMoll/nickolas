#include <stdlib.h>
#include <stdbool.h>
#ifndef LINKEDLIST_H_
#define LINKEDLIST_H_

typedef struct LinkedStringNode {
   char *value;
   struct LinkedStringNode *next;
} LinkedStringNode;

bool ll_create(struct LinkedStringNode** head, char *value);
bool ll_insert(struct LinkedStringNode* node, char *value);
bool ll_append(struct LinkedStringNode* head, char *value);
void ll_free(struct LinkedStringNode* head);
bool ll_remove(struct LinkedStringNode** head, size_t index);
char* ll_get(struct LinkedStringNode* head, size_t index);
struct LinkedStringNode* ll_fromArray(char *inputArray[], size_t size);

#endif // LINKEDLIST_H_
