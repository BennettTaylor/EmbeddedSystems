#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
 
 void *myFirstThread(void *p)
 {
     	sleep(1);
     	printf("I am the first thread \n");
     	return NULL;
}

void *mySecondThread(void *p)
{ 
     	printf("I am the second thread \n");
     	pthread_yield();
}

void *myNewThread(void *p)
{
	int integer;
	int index = 0;
	int power = 2;
	char binary[sizeof(int)];
	printf("Hello!\n");
	//Ask user to enter an integer
	printf("Enter an integer: ");
	scanf("%d", &integer);
	while (integer != 0) {
		itoa(integer % power, binary[sizeof(int) - index - 1], 10);
		power *= 2;
	}
	//print the integer in decimal, binary, and in hexadecimal form
	printf("Integer in decimal form: %d \n", integer);
	printf("Integer in binary form: %s \n", binary);
	printf("Integer in hexidecimal form: %x \n", integer);
}

int main() 
{
	pthread_t tid,tid2, tid3;
	
	printf("Program started...\n");
	
	pthread_create(&tid, NULL, myFirstThread, NULL);
	pthread_create(&tid2, NULL, mySecondThread, NULL);
	pthread_create(&tid3, NULL, myNewThread, NULL);
	
	pthread_join(tid, NULL);
	pthread_join(tid2, NULL);
	pthread_join(tid3, NULL);
	
	printf("All threads done.\n");
       	exit(0);
}
