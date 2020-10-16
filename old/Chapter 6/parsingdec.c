#include <stdio.h>
#include <stdlib.h>

#include "mpc.h" //written by books author

#include <readline/history.h>
#include <readline/readline.h>

// "char** argv" and "char **argv" both mean the same thing, a pointer to a character pointer
int main(int argc, char** argv) { 

	//Create parsers using mpc
	mpc_parser_t* Number	= mpc_new("number");
	mpc_parser_t* Operator	= mpc_new("operator");
	mpc_parser_t* Expr	= mpc_new("expr");
	mpc_parser_t* Lisp	= mpc_new("lisp");

	// define the parsers with the following language rules
	mpca_lang(MPCA_LANG_DEFAULT,
		"									\
			number	 : /-?[0-9]+/ | /-?[0-9]+[.][0-9]+/;						\
			operator : '+' | '-' | '*' | '/' | '%' |			\
			           \"add\" | \"sub\" | \"mul\" | \"div\" | \"mod\" ;	\
			expr	 : <number> | '(' <operator> <expr>+ ')' ;		\
			lisp	 : /^/ <operator> <expr>+ /$/ ;				\
		",
		Number, Operator, Expr, Lisp);

	// print version and how to exit
	puts("Lisp Interpreter for C, v0.0.0.0.1");
	puts("Press Ctrl+C to Exit\n");

	// loop will never end
	while (1){
	
		// output prompt and then read input
		char *input = readline("lisp> ");

		// add input to history
		add_history(input);		

		// attempt to parse user's input
		mpc_result_t r;
		if (mpc_parse("<stdin", input, Lisp, &r)) {
			// if successful print the AST (?)
			mpc_ast_print(r.output);
			mpc_ast_delete(r.output);
		} else {
			// else print the error
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		
		// free retrieved input
		free(input);
	}
	
	// undefine and delete our parsers
	mpc_cleanup(4, Number, Operator, Expr, Lisp);

	return EXIT_SUCCESS;
}
// what are these new editline headers and commands doing...
