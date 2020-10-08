#include <stdio.h>
#include <stdlib.h>

#include <readline/history.h>
#include <readline/readline.h>

// "char** argv" and "char **argv" both mean the same thing, a pointer to a character pointer
int main(int argc, char** argv) { 

	// print version and how to exit
	puts("Lisp Interpreter for C, v0.0.0.0.1");
	puts("Press Ctrl+C to Exit\n");

	// loop will never end
	while (1){
	
		// output prompt and then read input
		char *input = readline("lisp> ");

		// add input to history
		add_history(input);		

		// simply echos back input 
		printf("Oh well, %s to you too!\n", input);

		// free retrieved input
		free(input);
	}

	return EXIT_SUCCESS;
}
// what are these new editline headers and commands doing...
