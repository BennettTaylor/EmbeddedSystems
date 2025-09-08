#include <stdio.h>
#include <stdlib.h>
#include <math.h>

unsigned int mirror(unsigned int input) {
	int power = 1;
	int powerInv = 31;
	unsigned int output = 0;
	while (powerInv != 0) {
		if (input % (int)pow(2, power)) {
			output += pow(2, powerInv);
			input -= (int)pow(2, power - 1);		
		}
		power++;
		powerInv--;
	}
	return output;
}

int main() {
	unsigned int test = 19088743;
	printf("%u \n", mirror(test));

	FILE *fp;
	fp = fopen("input.txt", "r");
	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        	printf("%s", buffer);
    	}
	return 0;
}
