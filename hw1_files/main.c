// Bennett Taylor betaylor
#include <stdio.h>
#include <stdlib.h>
#include "bits.h"
#include "mylist.h"

int main(int argc, char *argv[]) {
	unsigned int test = 19088743;
	printf("%u \n", BinaryMirror(test));
	
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	fp = fopen(argv[1], "r");
	while (getline(&line, &len, fp) != -1) {
        	printf("%s", line);
    	}

	return 0;
}
