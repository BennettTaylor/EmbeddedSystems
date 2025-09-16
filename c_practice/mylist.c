// Bennett Taylor betaylor
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mylist.h"
#include "bits.h"

// A linked list printing function helpful for debugging
void printList(struct Node *head) {
	struct Node *current = head;
	
	// Loop through list and print values stored in nodes
	int index = 0;
        while (current != NULL) {
		printf("Index: %d \n", index);
                printf("Unsigned int: %u \n", current->num);
		printf("Mirror int: %u \n", current->mirror);
                printf("ASCII: %s \n", current->ASCII);
		printf("Mirror ASCII: %s \n", current->mirrorASCII);
                printf("Binary: %s \n\n", current->binary);
		printf("Sequences: %u \n", current->sequences);
                current = current->next;
		index++;
        }
	return;
}

// Creates a node from an unsigned int's ASCII representation
struct Node *createNode(char *ASCII) {
	int bits = sizeof(unsigned int) * 8;

	// Allocating neccessary space for the node
	struct Node *node = (struct Node*)malloc(sizeof(struct Node));
	node->mirrorASCII = (char *) malloc(sizeof(char) * 12);

	// Assigning node values
	node->ASCII = ASCII;
	node->num = (unsigned int)atoi(ASCII);
	node->mirror = BinaryMirror(node->num);
	sprintf(node->mirrorASCII, "%u", node->mirror);
	node->binary = (char*)malloc(bits + 1);
	node->binary[bits] = '\0';
	node->sequences = CountSequence(node->num);

	// Building ASCII string to store the number's binary representation
	int index = 0;
	while (index < bits) {
		node->binary[bits - index - 1] = '0' + ((node->num & (1U << index)) != 0);
		index++;
	}
	return node;
}

// Function for comparing the mirror ASCII representation of two nodes
int compareNodes(struct Node *node1, struct Node *node2) {
	if (node1 == NULL) {
		return 0;
	}
	if (node2 == NULL) {
		return 1;
	}
	return strcmp(node1->mirrorASCII, node2->mirrorASCII) < 0;
}

// Used for  merging pre sorted linked lists
struct Node *mergeLists(struct Node *list1, struct Node *list2) {
	// Pointers for building the new linked list
	struct Node *sortedList;
	struct Node *temp;

	// Determine head of new list
	if (compareNodes(list1, list2)) {
		sortedList = list1;
		list1 = list1->next;
	} else {
		sortedList = list2;
		list2 = list2->next;
	}

	// Loop through list, adding nodes in sorted order
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
	// Pointers used for breaking list into sublists
	struct Node *slow;
	struct Node *fast;

	// Pointers used in the merge stage
	struct Node *sortedList1;
	struct Node *sortedList2;
	struct Node *sortedList;

	// Initialization
	fast = head;
	slow = head;
	int size = 0;
	if (head->next == NULL) {
		return head;
	}

	// Sets slow pointer to approximately half way through the list
	while (fast->next != NULL) {
		if (size % 2) {
			slow = slow->next;
		}
		fast = fast->next;
		size++;
	}

	// Breaks the list into two sublists
	fast = slow->next;
	slow->next = NULL;
	slow = head;

	// Recursive merge sort call
	sortedList1 = mergeSortList(slow);
	sortedList2 = mergeSortList(fast);

	// Merging sorted sublists
	sortedList = mergeLists(sortedList1, sortedList2);
	return sortedList;
}

// For freeing the linked list
void freeList(struct Node* head) {
	struct Node *current = head;
	// Loop through the list and free any allocated storage
	while (current != NULL) {
		head = current;
		free(current->ASCII);
		free(current->mirrorASCII);
		free(current->binary);
		current = current->next;
		free(head);
	}
	return;
}
