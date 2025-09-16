# C Practice
The program source files found in this directory were created to practice foundational C programming constructs. This includes bit manipulation, file I/O, makefiles, linked lists, arguments, and project structure.

## Implementation
In bits.c two functions BinaryMirror and CountSequence are implemented to find the Binary mirror and sequence frequency. They both take advantage of common bit manipulation techniques used in c.

In mylist.c functions for creating a linked list to hold all relevant data about the input integers are implemented. To sort the list by the binary mirror's ASCII representation, merge sort is used.

In main.c file I/O and linked list generation is handled.

## Usage
To cretae the MyBitsApp executable simply run the 'make' command, which will generate the executable and some intermediate object files. To remove the generate files run 'make clean'

The MyBitsApp executable takes in two arguments, the first being an input file and the second a output file. As an example:

./MyBitsApp input.txt output.txt

The input should contain a list of unsigned integers in base 10 ASCII representation. As an example: 

1731349335  
859977072  
1229999959  
4672819  
1866880619  
1331328837  
1164661861  
1651534660  
1331771717  
8009047  
1818578768  
1431588199  
1414551882  
3295863  
1248094837  

The program will take these unsigned integers, find the 010 sequence frequecy within the binary representation, generate their binary mirrors, sort by binary mirror ASCII representation, and output these to the specified output file. The output for the example about would be:

1385826858  10  
183674422	5   
246170316	4  
2728167154	5  
2733529842	5  
2787825314	8  
2924881490	7  
3434275328	3  
3597296374	4  
3870436010	9  
3937164800	5  
3939650790	5  
3941730962	8  
3998370816	4  
584453702	4  
