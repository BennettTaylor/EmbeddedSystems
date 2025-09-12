// Bennett Taylor betaylor
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mylist.h"

void printList(struct Node *head) {
	struct Node *current = head;
	int index = 0;
        while (current != NULL) {
		printf("Index: %d \n", index);
                printf("Unsigned int: %u \n", current->num);
                printf("ASCII: %s \n", current->ASCII);
                printf("Binary: %s \n\n", current->binary);
                current = current->next;
		index++;
        }
}

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

int compareNodes(struct Node *node1, struct Node *node2) {
	if (node1 == NULL) {
		return 0;
	}
	if (node2 == NULL) {
		return 1;
	}
	return strcmp(node1->ASCII, node2->ASCII) < 0;
}

struct Node *mergeLists(struct Node *list1, struct Node *list2) {
	struct Node *sortedList;
	struct Node *temp;
	if (compareNodes(list1, list2)) {
		sortedList = list1;
		list1 = list1->next;
	} else {
		sortedList = list2;
		list2 = list2->next;
	}
	temp = sortedList;
	while (list1 != NULL || list2 != NULL) {
		if (compareNodes(list1, list2)) {
			temp->next = list1;
			list1 = list1->next;
		} else {
			temp->next = list2;
			list2 = list2->next;
		}
		temp = temp->next;
	}
	return sortedList;
}

struct Node *mergeSortList(struct Node *head) {
	struct Node *slow;
	struct Node *fast;
	struct Node *sortedList1;
	struct Node *sortedList2;
	struct Node *sortedList;
	fast = head;
	slow = head;
	int size = 0;
	if (head->next == NULL) {
		return head;
	}
	while (fast->next != NULL) {
		if (size % 2) {
			slow = slow->next;
		}
		fast = fast->next;
		size++;
	}
	fast = slow->next;
	slow->next = NULL;
	slow = head;
	sortedList1 = mergeSortList(slow);
	sortedList2 = mergeSortList(fast);
	sortedList = mergeLists(sortedList1, sortedList2);
	return sortedList;
}


void freeList(struct Node* head) {
	return;
}
