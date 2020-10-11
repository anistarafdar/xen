#include <stdio.h>
#include <stdlib.h>
#include <math.h> //for power operator

#include "mpc.h" //written by books author

#include <readline/history.h>
#include <readline/readline.h>

typedef struct {
	int type;
	long num;
	int err;
} lval;

// create enums for lisp value types and error types for the struct above
enum { LVAL_NUM, LVAL_ERR };
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

// create new number type for lval 
lval lval_num(long x) {
	lval v;
	v.type = LVAL_NUM;
	v.num = x;
	return v;
};

// create new error type for lval
lval lval_err(int x) {
	lval v;
	v.type = LVAL_ERR;
	v.err = x;
	return v;
};

//print an lval
void lval_print(lval v);
//print an lval and append a newline
void lval_println(lval v) { lval_print(v); putchar('\n'); }

lval eval(mpc_ast_t* t);
lval eval_op(lval x, char* op, lval y);

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
			           \"add\" | \"sub\" | \"mul\" | \"div\" | \"mod\" | \"pow\" ;	\
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
			lval result = eval(r.output);
			lval_println(result);
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

lval eval(mpc_ast_t* t) {
	
	// if tagged as a number return it directly (base case)
	if (strstr(t->tag, "number")) {
		// check if there is an error in conversion
		errno = 0;
		long x = strtol(t->contents, NULL, 10);
		return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
	}
	
	// the operator is always the second child
	char* op = t->children[1]->contents;
	// store the third child in x
	lval x = eval(t->children[2]);

	// iterate the remaining children and combining
	int i = 3;
	while (strstr(t->children[i]->tag, "expr")) { // strstr returns 0 if the second string is not a substring of the first
		x = eval_op(x, op, eval(t->children[i]));
		i++;
	}

	return x;
}

// use operator string to see which operator to perform
lval eval_op(lval x, char* op, lval y) {
	// if either value is an error return it before trying to compute
	if (x.type == LVAL_ERR) { return x; }
	if (y.type == LVAL_ERR) { return y; }
	
	if ((strcmp(op, "+") == 0) || (strcmp(op, "add") == 0)) { return lval_num(x.num + y.num); }
	if ((strcmp(op, "-") == 0) || (strcmp(op, "sub") == 0)) { return lval_num(x.num - y.num); }
	if ((strcmp(op, "*") == 0) || (strcmp(op, "mul") == 0)) { return lval_num(x.num * y.num); }
	if ((strcmp(op, "/") == 0) || (strcmp(op, "div") == 0)) { 
		// if dividing by zero catch the error otherwise continue as normal
		return y.num == 0
			? lval_err(LERR_DIV_ZERO)
			: lval_num(x.num / y.num);
	}
	if ((strcmp(op, "%") == 0) || (strcmp(op, "mod") == 0)) { return lval_num(x.num % y.num); }
	if ((strcmp(op, "^") == 0) || (strcmp(op, "pow") == 0)) { return lval_num((int) pow((double) x.num, y.num)); }

	return lval_err(LERR_BAD_OP);
}

void lval_print(lval v) {
	switch (v.type) {
		//if its a number then print then break
		case LVAL_NUM: printf("%li", v.num); break;

		//if the type is an error
		case LVAL_ERR:
			//check the error type then print it
			if (v.err == LERR_DIV_ZERO) {
				printf("Error: Division by zero");
			}
			if (v.err == LERR_BAD_OP) {
				printf("Error: Invalid operator");
			}
			if (v.err == LERR_BAD_NUM) {
				printf("Error: Invalid number");
			}
		break;
	}
}


