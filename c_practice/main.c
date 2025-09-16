// Bennett Taylor betaylor
#include <stdio.h>
#include <stdlib.h>
#include "bits.h"
#include "mylist.h"

// For generating the linked list from the input file
void createList(struct Node **head, char *fileName) {
	// Necessary variables for reading the file
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	struct Node *current = NULL;

	// Grab each line of the input file one by one
	fp = fopen(fileName, "r");
	while (getline(&line, &len, fp) != -1) {
		int index = 0;
		// Remove newline characters and terminate string
		while (index != -1) {
			if (line[index] == '\n') {
				line[index] = '\0';
				index = -2;
			}
			index++;
		}

		// Create node from line
        	struct Node *next = createNode(line);
		line = NULL;

		// Build linked list
		if (*head == NULL) {
			*head = next;
			current = next;
		}
		else {
			current->next = next;
			current = next;
		}
    	}

	fclose(fp);
	return;
}

// Used to output linked list to specified file
void outputList(struct Node *head, char *fileName) {
	FILE *fp;
        struct Node *current = head;
        fp = fopen(fileName, "w");

	// Print the mirror and the sequence count for each number
	while (current != NULL) {
		fprintf(fp, "%s\t%d\n", current->mirrorASCII, current->sequences);
		current = current->next;
	}

	fclose(fp);
	return;
}

// Entry point
int main(int argc, char *argv[]) {
	// Argument error checking
	if (argc != 3) {
		printf("ERROR: Enter arguements for input and output files\n");
		return 1;
	}

	// Create list, sort, output to specified file, and free memory
	struct Node *head = NULL;
	createList(&head, argv[1]);
	head = mergeSortList(head);
	outputList(head, argv[2]);
	freeList(head);

	return 0;
}
