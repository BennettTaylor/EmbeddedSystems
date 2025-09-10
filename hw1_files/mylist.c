// Bennett Taylor betaylor
#include <stdlib.h>
#include "mylist.h"

struct Node *createNode(char *ASCII) {
	int bits = sizeof(unsigned int) * 8;
	struct Node *node = (struct Node*)malloc(sizeof(struct Node));
	node->ASCII = ASCII;
	node->num = (unsigned int)atoi(ASCII);
	node->binary = (char*)malloc(bits + 1);
	node->binary[bits] = '\0';

	int index = 0;
	while (index < bits) {
		node->binary[index] = '0' + ((node->num & (1U << index)) != 0);
		index++;
	}
	return node;
}

struct Node *mergeSortList(struct Node *head) {
	return NULL;
}


void freeList(struct Node* head) {
	return;
}
