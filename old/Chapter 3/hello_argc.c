#include <stdio.h>
#include <stdlib.h>

void printHello(char* name, int i){
	while(i > 0){
		printf("Hello, %s\n", name);
		--i;
	}
}

int main(int argc, char** argv) {
	printHello(argv[1], atoi(argv[2]));

	return EXIT_SUCCESS;
}