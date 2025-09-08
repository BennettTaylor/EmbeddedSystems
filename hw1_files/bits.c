// Bennett Taylor betaylor
#include <math.h>

// BinaryMirror implementation
unsigned int BinaryMirror(unsigned int input) {
	// Variables for keeping track of position and binary mirror
	unsigned int index = 0;
	unsigned int bound = sizeof(unsigned int) * 8;
	unsigned int mirror = 0;

	// Traverse bits in input, starting with lowest order bit
	while (index < bound) {
		// Add mirrored bit to binary mirror if bit is found
		int bit = (input & (1U << index)) != 0;
		if (bit) {
			mirror += (1U << bound - index - 1);
		}

		index++;
	}

	// Return binary mirror
        return mirror;
}

// CountSequence implemenetation
unsigned int CountSequence(unsigned int input) {
	// Variables for keeping track of position and pattern match count
	unsigned int index = 0;
	unsigned int bound = sizeof(unsigned int) * 8;
	unsigned int count = 0;

	// Traverse bits in input, starting with lowest order bit
	while (index < bound - 2) {
		// Generate bits for next set using a bitmask
		int bit1 = (input & (1U << index)) != 0;
		int bit2 = (input & (1U << index + 1)) != 0;
		int bit3 = (input & (1U << index + 2)) != 0;
		
		// Check for pattern
		if (!bit1 && bit2 && !bit3) {
			count++;
		}

		index++;
	}

	// Return pattern match count
	return count;
}
