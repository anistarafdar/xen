#include <stdio.h>
#include <stdlib.h>
#include <math.h> //for power operator

#include "mpc.h" //written by books author

#include <readline/history.h>
#include <readline/readline.h>

long eval(mpc_ast_t* t);

long eval_op(long x, char* op, long y);

// "char** argv" and "char **argv" both mean the same thing, a pointer to a character pointer
int main(int argc, char** argv) { 

	//Create parsers using mpc
	mpc_parser_t* Number	= mpc_new("number");
	mpc_parser_t* Operator	= mpc_new("operator");
	mpc_parser_t* Expr	= mpc_new("expr");
	mpc_parser_t* Lisp	= mpc_new("lisp");

	// define the parsers with the following language rules
	mpca_lang(MPCA_LANG_DEFAULT,
		"										\
			number	 : /-?[0-9]+/ ;							\
			operator : '+' | '-' | '*' | '/' | '%' | '^' |				\
			           \"add\" | \"sub\" | \"mul\" | \"div\" | \"mod\" | \"pow\" ;		\
			expr	 : <number> | '(' <operator> <expr>+ ')' ;			\
			lisp	 : /^/ <operator> <expr>+ /$/ ;					\
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
			// if successful evaluate the AST, then delete old tree
			long result = eval(r.output);
			printf("%li\n", result);
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

long eval(mpc_ast_t* t) {
	
	// if tagged as a number return it directly (base case)
	if (strstr(t->tag, "number")) {
		return atoi(t->contents);
	}
	
	// the operator is always the second child
	char* op = t->children[1]->contents;

	// store the third child in x
	long x = eval(t->children[2]);

	// iterate the remaining children and combining
	int i = 3;
	while (strstr(t->children[i]->tag, "expr")) { // strstr returns 0 if the second string is not a substring of the first
		x = eval_op(x, op, eval(t->children[i]));
		i++;
	}

	return x;
}

// use operator string to see which operator to perform
long eval_op(long x, char* op, long y) {
	if ((strcmp(op, "+") == 0) || (strcmp(op, "add") == 0)) { return x + y; }
	if ((strcmp(op, "-") == 0) || (strcmp(op, "sub") == 0)) { return x - y; }
	if ((strcmp(op, "*") == 0) || (strcmp(op, "mul") == 0)) { return x * y; }
	if ((strcmp(op, "/") == 0) || (strcmp(op, "div") == 0)) { return x / y; }
	if ((strcmp(op, "%") == 0) || (strcmp(op, "mod") == 0)) { return x % y; }
	if ((strcmp(op, "^") == 0) || (strcmp(op, "pow") == 0)) { return (int) pow((double) x, y); }

	return 0;
}
