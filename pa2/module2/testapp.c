#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#define DEVICE "/dev/simple_character_device"
#define BUFFER_SIZE 1024

int main () {
	char instruction, buffer[BUFFER_SIZE];
	int file = open(DEVICE, O_RDWR);
	while (1) {
		printf("\nR) Read \nW) Write \nE) Exit \nWrite Some Intructions Bruh\n\nEnter command: ");
		scanf("%c", &instruction);
		switch (instruction) {
			
			case 'R':
			case 'r':
				read(file, buffer, strlen(buffer));
				printf("Device output: %s\n", buffer);
				while (getchar() != '\n');
				break;

			case 'W':
			case 'w':
				printf("Enter your String Young Skywalker: ");
				scanf("%s", buffer);
				write(file, buffer, strlen(buffer));
				while (getchar() != '\n');
				break;
			
			case 'E':
			case 'e':
				return 0;
			default:
				while (getchar() != '\n');
		}
	}
	close(file);
	return 0;
}
