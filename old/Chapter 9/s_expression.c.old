#include <stdio.h>
#include <stdlib.h>
#include <math.h> //for power operator

#include "mpc.h" //written by books author

#include <readline/history.h>
#include <readline/readline.h>

typedef struct lval { // needs to be defined as lval before it can reference itself recursively
	int type;
	long num;
	//error and symbol type have string data
	char* err;
	char* sym;
	//pinter to list of lvals and count of size
	int count;
	struct lval** cell; // if it was one star it would be infinite it size
} lval;

// create enums for lisp value types
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR};

// construct pointer to new Number lval 
lval* lval_num(long x) {
	lval* v = malloc(sizeof(lval));;
	v->type = LVAL_NUM;
	v->num = x;
	return v;
}

// construct pointer to new Error lval
lval* lval_err(char* m) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_ERR;
	v->err = malloc(strlen(m) + 1);
	strcpy(v->err, m);
	return v;
};

// construct pointer to a new Symbol lval
lval* lval_sym(char* s) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->sym = malloc(strlen(s)+1); //the plus one is because strlen excludes the null terminating byte
	strcpy(v->sym, s);
	return v;
}

// construct pointer to empty S expr lval
lval* lval_sexpr(void) { //what
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

lval* lval_read(mpc_ast_t* t);

// delete lval* and free the memory
void lval_del(lval* v);

void lval_expr_print(lval* v, char open, char close);

//print an lval
void lval_print(lval* v);
//print an lval and append a newline
void lval_println(lval* v) { lval_print(v); putchar('\n'); }

lval eval(mpc_ast_t* t);
lval eval_op(lval x, char* op, lval y);

// "char** argv" and "char **argv" both mean the same thing, a pointer to a character pointer
int main(int argc, char** argv) { 

	//Create parsers using mpc
	mpc_parser_t* Number	= mpc_new("number");
	mpc_parser_t* Symbol    = mpc_new("symbol");
	mpc_parser_t* Sexpr	= mpc_new("sexpr");
	mpc_parser_t* Expr	= mpc_new("expr");
	mpc_parser_t* Lisp	= mpc_new("lisp");

	// define the parsers with the following language rules
	mpca_lang(MPCA_LANG_DEFAULT,
		"										\
			number	 : /-?[0-9]+/ ;							\
			symbol   : '+' | '-' | '*' | '/' | '%' | '^' |				\
			           \"add\" | \"sub\" | \"mul\" | \"div\" | \"mod\" | \"pow\" ;	\
			sexpr	 : '(' <expr>* ')' ;						\
			expr	 : <number> | <symbol> | <sexpr> ;				\
			lisp	 : /^/ <expr>* /$/ ;					\
		",
		Number, Symbol, Sexpr, Expr, Lisp);

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
			// if successful try reading in result and printing, then delete old tree
			lval* x = lval_read(r.output);
			lval_println(x);
			mpc_ast_delete(x);
		} else {
			// else print the error
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		
		// free retrieved input
		free(input);
	}
	
	// undefine and delete our parsers
	mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lisp);

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

void lval_expr_print(lval* v, char open, char close) {
	putchar(open);
	for (int i = 0; i < v->count; i++) {
		// print value contained within
		lval_print(v->cell[i]);
		
		// dont print trailing space if last element
		if (i != (v->count-1)) {
			putchar(' ');
		}
	}
	putchar(close);
}

void lval_print(lval* v) {
	switch (v->type) {
		case LVAL_NUM:		printf("%li", v->num); break;	
		case LVAL_ERR:		printf("Error: %s", v->err); break;	
		case LVAL_SYM:		printf("%s", v->)sym; break;	
		case LVAL_SEXPR:	lval_expr_print(v, '(', ')'); break;	
	}
}

lval* lval_read_num(mpc_ast_t* t) {
	errno = 0;
	long x = strtol(t->contents, NULL, 10);
	return errno != ERANGE ?
		lval_num(x) : lval_err("invalid number");
}

lval* lval_read(mpc_ast_t* t) {
	// if symbol or number return conversion to that type
	if (strstr(t->tag, "number")) { return lval_read_num(t); }
	if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

	// if root (>) or S expr then create empty list
	lval* x = NULL;
	if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
	if (strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }

	// fill this newly created list with any valid expression contained within
	for (int i = 0; i < t->children_num; i++) {
		if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
		if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
		if (strcmp(t->children[i]->tags, "regex") == 0) { continue; }
		x = lval_add(x, lval_read(t->children[i]));
	}

	return x;
}

lval* lval_add(lval* v, lval* x) {
	v->count++;
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	v->cell[v->count-1] = x;
	return v;
}

void lval_del(lval* v) { //for every malloc there should be a corresponding free
	
	switch (v->type) {
	
		// do nothing special for the number type
		case LVAL_NUM: break; 
		
		//for Err or SYM free the string data
		case LVAL_ERR: free(v->err); break;
		case LVAL_SYM: free(v->sym); break;

		// if S expr delete all elements inside
		case LVAL_SEXPR:
			for (int i = 0; i < v->count; i++) {
				lval_del(v->cell[i]);
			}
			// also free mem allocated to contain the pointers
			free(v->cell);
		break;
	}

	free(v); // free the memory allocated for the lval struct itself
}

