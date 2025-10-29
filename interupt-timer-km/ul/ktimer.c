#define _GNU_SOURCE

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#define BUF_LEN 256
#define MSG_LEN 129

void printManPage();
void sighandler(int);

char message[MSG_LEN];
int fd;

int main(int argc, char **argv) {
	char read_buf[BUF_LEN];
	/* If there is one arguement, handle -l behavior */
	if (argc == 2) {
		/* Check for -r to remove timers */
		if (strcmp(argv[1], "-r") == 0) {
			/* Attempt to open mytimer dev file */
                	fd = open("/dev/mytimer", O_WRONLY);
                	if (fd == -1) {
                        	printf("mytimer module isn't loaded at /dev/mytimer.\n");
                        	close(fd);
                        	return -1;
                	}

                	/* Build module message */
                	char input[1];
                	int n = 1;
                	int result;
                	input[0] = '\0';

                	/* Give module message */
                	result = write(fd, input, n);

                	if (result == -1) {
                        	printf("Error writing to /dev/mytimer\n");
                	} else if (result == 0) {
                        	printf("Wrote 0 bytes to /dev/mytimer\n");
                	}
			
			return 0;
                }

		/* Check for correct arguement */
		if (strcmp(argv[1], "-l") != 0) {
			printManPage();
			return -1;
		}

		/* Attempt to open mytimer proc file */
		fd = open("/proc/mytimer", O_RDONLY);
		if (fd == -1) {
			printf("mytimer module isn't loaded at /dev/mytimer.\n");
			close(fd);
			return -1;
		}

		/* Read timers */
		char *token;
		int result;
		int line_index = 0;

		result = read(fd, read_buf, BUF_LEN - 1);
		if (result == -1) {
			printf("Read failed\n");
		} else if (result == 0) {
			printf("No information found in /proc/mytimer.\n");
		} else {
			read_buf[result] = '\0';
		}

		result = close(fd);
                if (result == -1) {
                        printf("Failed to close file\n");
                }
		
		/* Line 0 is module name, do nothing */
		token = strtok(read_buf, "\n");

		/* Loop through remaining lines */
		while (token != NULL) {
			/* Print seconds */
			if (line_index == 6) {
				printf("%s ", token);
			} 
			/* Print message */
			else if (line_index == 7) {
				printf("%s\n", token);
				line_index = 3;
			}
			line_index++;
			token = strtok(NULL, "\n");
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
	
		/* Attempt to open mytimer dev file */
                fd = open("/dev/mytimer", O_WRONLY);
                if (fd == -1) {
                        printf("mytimer module isn't loaded at /dev/mytimer.\n");
                        close(fd);
                        return -1;
                }

		/* Build module message */
		char input[3];
		int n;
		int result;
		n = sprintf(input, "%d\n", count);
                
                /* Give module message */
                result = write(fd, input, n);

		if (result == -1) {
			printf("Error writing to /dev/mytimer\n");
		} else if (result == 0) {
			printf("Wrote 0 bytes to /dev/mytimer\n");
		}
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

		/* Attempt to open mytimer dev file */
                fd = open("/dev/mytimer", O_RDWR);
                if (fd == -1) {
                        printf("mytimer module isn't loaded at /dev/mytimer.\n");
                        close(fd);
                        return -1;
                }

		/* Attempt to open mytimer proc file */
                int fd_proc;
                fd_proc = open("/proc/mytimer", O_RDONLY);
                if (fd_proc == -1) {
                        printf("mytimer module isn't loaded at /dev/mytimer.\n");
                        close(fd_proc);
                        return -1;
                }

		/* Read timers */
                char read_buf[BUF_LEN];
		char *token;
                int result;
                int line_index = 0;
		int count;
		int active;
		int update = 0;

                result = read(fd_proc, read_buf, BUF_LEN - 1);
                if (result == -1) {
                        printf("Read failed\n");
                } else if (result == 0) {
                        printf("No information found in /proc/mytimer.\n");
                } else {
                        read_buf[result] = '\0';
                }

		result = close(fd_proc);
                if (result == -1) {
                        printf("Failed to close file\n");
                }

                /* Line 0 is module name, do nothing */
                token = strtok(read_buf, "\n");

                /* Loop through remaining lines */
                while (token != NULL) {
			/* Get active timer count */
			if (line_index == 2) {
				active = atoi(token);
			}

			/* Get number of timers supported */
			if (line_index == 3) {
				count = atoi(token);
			}

			/* Check for updated message */
                        if (line_index == 7) {
                                if (strcmp(token, argv[3]) == 0) {
					printf("The timer %s was updated!\n", token);
					update = 1;
				}
                                line_index = 3;
                        }
                        line_index++;
                        token = strtok(NULL, "\n");
                }

		/* Don't add timer if not supported */
		if (!update) {
			if (active >= count) { 
                                printf("Cannot add another timer!\n"); 
                                return 0; 
                        } 
		}
		
		int oflags;
		struct sigaction action;

		/* Setup signal handler */
		if (!update) {
                	memset(&action, 0, sizeof(action));
                	action.sa_handler = sighandler;
                	action.sa_flags = SA_SIGINFO;
                	sigemptyset(&action.sa_mask);
                	sigaction(SIGIO, &action, NULL);
                	fcntl(fd, F_SETOWN, getpid());
                	oflags = fcntl(fd, F_GETFL);
                	fcntl(fd, F_SETFL, oflags | FASYNC);
		}

		/* Build module message */
		char input[138];
		int n;

		n = sprintf(input, "0\n%d\n%s\n", seconds, argv[3]);
		

		/* Give module message */
                result = write(fd, input, n);

                if (result == -1) {
                        printf("Error writing to /dev/mytimer\n");
                } else if (result == 0) {
                        printf("Wrote 0 bytes to /dev/mytimer\n");
                }
		
		/* Wait for signal */
		if (!update) {
			strcpy(message, argv[3]);
			pause();
		}
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

void sighandler(int signo)
{
	/* Attempt to open tty dev file */
	int result;
	result = close(fd);
	if (result == -1) {
        	printf("Failed to close file\n");
        }

        fd = open("/dev/ttyAMA0", O_WRONLY);
        if (fd == -1) {
                printf("ktimer can't access /dev/ttyAMA0.\n");
                close(fd);
                return;
        }

	/* Write to tty device file */
	result = write(fd, message, sizeof(message));
	if (result == -1) {
                printf("Error writing to /dev/ttyAMA0\n");
        } else if (result == 0) {
                printf("Wrote 0 bytes to /dev/ttyAMA0\n");
        }

	/* Print extra newline character */
	char newline[] = "\n";
	result = write(fd, newline, sizeof(newline));
        if (result == -1) {
                printf("Error writing to /dev/ttyAMA0\n");
        } else if (result == 0) {
                printf("Wrote 0 bytes to /dev/ttyAMA0\n");
        }

	/* Close file, exit */
	result = close(fd);
        if (result == -1) {
                printf("Failed to close file\n");
        }
	return;
}
