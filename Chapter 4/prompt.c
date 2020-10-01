#include <stdio.h>
#include <stdlib.h>

// declare a buffer for user input of size 2048
static char input[2048];

// "char** argv" and "char **argv" both mean the same thing, a pointer to a character pointer
int main(int argc, char** argv) { 

	// print version and how to exit

	puts("Lisp Interpreter for C, v0.0.0.0.1");
	puts("Press Ctrl+C to Exit\n");


	// loop will never end
	while (1){
	
		// output prompt
		fputs("lisp> ", stdout);

		// read	user input, maximum size 2048, i wonder what happens if 2049 chars are inputted
		fgets(input, 2048, stdin);
		// my guess is that fgets puts the null termination byte for me at the end of my array
		// and porbably taking care of a chance of overrun for me

		// simply repeats it back the input for now
		printf("Oh well, %s", input);				
	}

	return EXIT_SUCCESS;
}
