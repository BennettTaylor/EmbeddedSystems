// Bennett Taylor betaylor
#ifndef MYLIST
#define MYLIST

// Linked list node definition
typedef struct Node {
  char *ASCII;
  unsigned int num;
  char* binary;
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
