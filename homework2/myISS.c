#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#define MEMORY_SIZE 256
#define REGISTER_COUNT 6

/* Memory data */
char data[MEMORY_SIZE] = {0};
bool initialized[MEMORY_SIZE] = {false};

/* Register data */
char registers[REGISTER_COUNT] = {0};
bool equal_flag = false;

/* Instruction data */
char instruction[MEMORY_SIZE] = {-1};
char arg1[MEMORY_SIZE] = {0};
char arg2[MEMORY_SIZE] = {0};
bool r_type[MEMORY_SIZE] = {false};

int first_instruction = 0;

/* Statistics */
unsigned instruction_count = 0;
unsigned cycle_count = 0;
unsigned cache_hits = 0;
unsigned memory_ops = 0;

int main(int argc, char *argv[]) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    /* Check for argument */
    if (argc != 2) {
        fprintf(stderr, "Usage: ./%s <file.assembly>\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Open file */
    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("Failed to open file");
        return EXIT_FAILURE;
    }

    /* Read instruction data */
    unsigned line_index = 0;
    unsigned instruction_address = 0;
    while ((read = getline(&line, &len, file)) != -1) {
        /* Get instruction address*/
        char *token = strtok(line, "\t ,[]\n");
        if(token) {
            instruction_address = atoi(token);
            if (line_index == 0) {
                first_instruction = instruction_address;
            }
        } else {
            printf("Error reading instruction address\n");
            break;
        }

        /* Get instruction */
        token = strtok(NULL, "\t ,[]\n");
        switch(token[0]) {
            case 'M': // MOV
                instruction[instruction_address] = 0;
                break;
            case 'A': // ADD
                instruction[instruction_address] = 1;
                break;
            case 'C': // CMP
                instruction[instruction_address] = 2;
                r_type[instruction_address] = true;
                break;
            case 'J': // JE/JMP
                if (token[1] == 'E') {
                    instruction[instruction_address] = 3;
                } else {
                    instruction[instruction_address] = 4;
                }
                break;
            case 'L': // LD
                instruction[instruction_address] = 5;
                r_type[instruction_address] = true;
                break;
            case 'S': // ST
                instruction[instruction_address] = 6;
                r_type[instruction_address] = true;
                break;
            default: // Invalid instruction
                instruction[instruction_address] = -1;
                break;
        }

        /* Get arguments */
        token = strtok(NULL, "\t ,[]\n");
        if (token) {
            if (instruction[instruction_address] != 3 && instruction[instruction_address] != 4) {
                    arg1[instruction_address] = atoi(&token[1]);
            } else {
                    arg1[instruction_address] = atoi(token);
            }
        } else {
                printf("Error reading arg1\n");
                break;
        } 

        if (instruction[instruction_address] != 3 && instruction[instruction_address] != 4) {
            token = strtok(NULL, "\t ,[]\n");
            if (token) {
                if (token[0] == 'R') {
                    arg2[instruction_address] = atoi(&token[1]);
                    r_type[instruction_address] = true; 
                } else {
                    arg2[instruction_address] = atoi(token);
                }
            } else {
                printf("Error reading arg2\n");
                break;
            }
        }

        line_index++;
    }

    instruction_address = first_instruction;

    /* Run simulation */
    while (instruction[instruction_address] != -1) {
        switch(instruction[instruction_address]) {
            case 0: // MOV
                registers[arg1[instruction_address] - 1] = arg2[instruction_address];
                break;
            case 1: // ADD
                if (r_type[instruction_address])
                    registers[arg1[instruction_address] - 1] += registers[arg2[instruction_address] - 1];
                else
                    registers[arg1[instruction_address] - 1] += arg2[instruction_address];
                break;
            case 2: // CMP
                equal_flag = (registers[arg1[instruction_address] - 1] == registers[arg2[instruction_address] - 1]);
                break;
            case 3: // JE
                if (equal_flag) {
                    instruction_address = arg1[instruction_address] - 1;
                    equal_flag = false;
                }
                break;
            case 4: // JMP
                instruction_address = arg1[instruction_address] - 1;
                break;
            case 5: // LD
                if (!initialized[registers[arg2[instruction_address] - 1]]) {
                    cycle_count += 48;
                    data[registers[arg2[instruction_address] - 1]] = 0;
                    initialized[registers[arg2[instruction_address] - 1]] = true;
                } else {
                    cache_hits++;
                }
                registers[arg1[instruction_address] - 1] = data[registers[arg2[instruction_address] - 1]];
                cycle_count += 1;
                memory_ops++;
                break;
            case 6: // ST
                if (!initialized[registers[arg1[instruction_address] - 1]]) {
                    cycle_count += 48;
                    initialized[registers[arg1[instruction_address] - 1]] = true;
                } else {
                    cache_hits++;
                }
                data[registers[arg1[instruction_address] - 1]] = registers[arg2[instruction_address] - 1];
                cycle_count += 1;
                memory_ops++;
                break;
            default: // Invalid instruction
                fprintf(stderr, "Error: Invalid instruction at index %d\n", instruction_address);
                instruction_address = -1;
                break;
        }
        cycle_count += 1;
        instruction_address++;
        instruction_count++;
    }

    printf("Total number of executed instructions: %u\n", instruction_count);
    printf("Total number of clock cycles: %u\n", cycle_count);
    printf("Number of hits to local memory: %u\n", cache_hits);
    printf("Total number of executed LD/ST instructions: %u\n", memory_ops);

    fclose(file);

    return EXIT_SUCCESS;
}