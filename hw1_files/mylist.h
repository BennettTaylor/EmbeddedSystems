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
struct Node *mergeSortList(struct Node *);
void freeList(struct Node*);

#endif
