#include <stdio.h>
#include <stdlib.h>

void printHello(char* name, int i){
	while(i > 0){
		printf("Hello, %s", name);
		--i;
	}
}

int main(int argc, char** argv) {
	printHello(argv[1], 5);

	return EXIT_SUCCESS;
}
