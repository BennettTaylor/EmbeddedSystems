#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void printManPage();

int main(int argc, char **argv) {
	/* If there is one arguement, handle -l behavior */
	if (argc == 2) {
		/* Check for correct arguement */
		if (strcmp(argv[1], "-l") != 0) {
			printManPage();
			return -1;
		}

		/* Attempt to open mytimer device file */
		FILE *pFile;
		pFile = fopen("/dev/mytimer", "r+");
		if (pFile == NULL) {
			fputs("mytimer module isn't loaded at /dev/mytimer\n", stderr);
			return -1;
		}

		/* Read timers */
		char line[129];
		char *message;
		int line_index = 0;
		int count;
		int active;
		int seconds;
		while(fgets(line, 129, pFile) != NULL) {
			if (line_index == 0) {
				count = atoi(line);	
			}
			else if (line_index == 1) {
				active = atoi(line);
			}
			else if (!(line_index % 2)) {
				seconds = atoi(line);
			}
			else if (line_index % 2) {
				message = strtok(line, "\n");
				printf("%s %d\n", message, seconds);
			}
			line_index++;
		}
	}
	/* If there are two arguments, handle -m behavior */
	else if (argc == 3) {
		/* Check for correct arguement */
                if (strcmp(argv[1], "-m") != 0) {
                        printManPage();
                        return -1;
                }

		/* Check [COUNT] arguement */
                int count = atoi(argv[2]);
                if ((count < 1) || (count > 5)) {
                        printManPage();
                        printf("[COUNT] must be an integer between 1 and 5 (inclusive)\n");
                        return -1;
                }

		/* Attempt to open mytimer device file */
                FILE *pFile;
                pFile = fopen("/dev/mytimer", "r+");
                if (pFile == NULL) {
                        fputs("mytimer module isn't loaded at /dev/mytimer\n", stderr);
                        return -1;
                }


		/* Build module message */
		char input[3];
		int n;
		n = sprintf(input, "%d\n", count);
                
                /* Give module message */
                fputs(input, pFile);

	}
	/* If there are three arguments, handle -s behavior */
	else if (argc == 4) {
		/* Check for correct arguement */
                if (strcmp(argv[1], "-s") != 0) {
                        printManPage();
                        return -1;
                }
		
		/* Check [SEC] arguement */
		int seconds = atoi(argv[2]);
		if (seconds < 1 || seconds > 86400) {
			printManPage();
			printf("[SEC] must be an integer between 1 and 86,400 (inclusive)\n");
			return -1;
		}

		/* Check [MSG] arguement */
		if (strlen(argv[3]) > 128) {
			printManPage();
                        printf("[MSG] must be no longer than 128 characters\n");
			return -1;
		}

		/* Attempt to open mytimer device file */
                FILE *pFile;
                pFile = fopen("/dev/mytimer", "r+");
                if (pFile == NULL) {
                        fputs("mytimer module isn't loaded at /dev/mytimer\n", stderr);
                        return -1;
                }

		/* Read timers */
                char line[129];
                char *message;
                int line_index = 0;
                int count;
                int active;
                int sec;
                while(fgets(line, 129, pFile) != NULL) {
                        if (line_index == 0) {
                                count = atoi(line);
                        }
                        else if (line_index == 1) {
                                active = atoi(line);
				if (active == count) {
					/* Check for any updates */
					line_index++;
					int found = 0;
					while (fgets(line, 129, pFile) != NULL) {
						if (!(line_index % 2)) {
                                			sec = atoi(line);
                        			}
                        			else if (line_index % 2) {
                                			message = strtok(line, "\n");
                                			if (strcmp(message, argv[3]) == 0) {
                                        			printf("The timer %s was updated!\n", message);
                                        			found = 1;
								break;
                                			}
                        			}
						line_index++;
					}
					if (!found) {
						printf("%d timer(s) already exist(s)!\n", active);
                                        	return 0;
					}
				}
                        }
                        else if (!(line_index % 2)) {
                                sec = atoi(line);
                        }
                        else if (line_index % 2) {
                                message = strtok(line, "\n");
				if (strcmp(message, argv[3]) == 0) {
					printf("The timer %s was updated!\n", message);
					break;
				}
                        }
                        line_index++;
                }
		
		/* Build module message */
		char input[138];
		int n;

		n = sprintf(input, "0\n%d\n%s\n", seconds, argv[3]);
		

		/* Give module message */
		fputs(input, pFile);
	}
	/* If there are not 1, 2, or 3 arguements there is an error */
	else {
		printManPage();
		return -1;
	}

	return 0;
}

void printManPage() {
	printf("Error: invalid use.\n");
	printf("To list currently registered timers use: ./ktimer -l\n");
	printf("To register a new timer use: ./ktimer -s [SEC] [MSG]\n");
	printf("To change the number of active timers supported use: ./ktimer -m [COUNT]\n");
}
