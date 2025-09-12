// Bennett Taylor betaylor
#include <stdio.h>
#include <stdlib.h>
#include "bits.h"
#include "mylist.h"

void createList(struct Node **head, char *fileName) {
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	struct Node *current = NULL;
	fp = fopen(fileName, "r");
	while (getline(&line, &len, fp) != -1) {
		int index = 0;
		while (index != -1) {
			if (line[index] == '\n') {
				line[index] = '\0';
				index = -2;
			}
			index++;
		}
        	struct Node *next = createNode(line);
		line = NULL;
		if (*head == NULL) {
			*head = next;
			current = next;
		}
		else {
			current->next = next;
			current = next;
		}
    	}
}

int main(int argc, char *argv[]) {
	unsigned int test = 19088743;
	printf("%u \n", BinaryMirror(test));

	struct Node *head = NULL;
	createList(&head, argv[1]);
	
	printList(head);

	printf("Now sorted: \n");
	
	head = mergeSortList(head);

	printList(head);

	freeList(head);
	return 0;
}
