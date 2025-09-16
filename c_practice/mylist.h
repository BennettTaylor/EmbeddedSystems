// Bennett Taylor betaylor
#ifndef MYLIST
#define MYLIST

// Linked list node definition
typedef struct Node {
	// Includes data about a number and it's binary mirror
	char *ASCII;
	unsigned int num;
	char* binary;
	unsigned int mirror;
	char *mirrorASCII;
	// Holds the frequency of "010" sequences found in the binary representation
	unsigned int sequences;
	struct Node *next;
} Node;

// Function declarations
struct Node *createNode(char *);
void printList(struct Node *);
int compareNodes(struct Node *, struct Node *);
struct Node *mergeLists(struct Node *, struct Node *);
struct Node *mergeSortList(struct Node *);
void freeList(struct Node*);

#endif
