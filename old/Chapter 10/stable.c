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
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR };

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

// a pointer to a new empty Qexpr lval
lval* lval_qexpr(void) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_QEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

lval* lval_read(mpc_ast_t* t);

//print an lval
void lval_print(lval* v);
//print an lval and append a newline
void lval_println(lval* v) { lval_print(v); putchar('\n'); }

// delete lval* and free the memory
void lval_del(lval* v);

lval* lval_eval_sexpr(lval* v);
lval* lval_eval(lval* v);

lval* lval_pop(lval* v, int i); // takes an index i and removes it from v, then returns it, leaving the rest of v intact
lval* lval_take(lval* v, int i); // takes an index i from v and returns it, deleting all of v in the process

lval* builtin_op(lval* a, char* op);

void lval_expr_print(lval* v, char open, char close);

// "char** argv" and "char **argv" both mean the same thing, a pointer to a character pointer
int main(int argc, char** argv) { 

	//Create parsers using mpc
	mpc_parser_t* Number	= mpc_new("number");
	mpc_parser_t* Symbol    = mpc_new("symbol");
	mpc_parser_t* Sexpr	= mpc_new("sexpr");
	mpc_parser_t* Qexpr	= mpc_new("qexpr");
	mpc_parser_t* Expr	= mpc_new("expr");
	mpc_parser_t* Lisp	= mpc_new("lisp");

	// define the parsers with the following language rules
	mpca_lang(MPCA_LANG_DEFAULT,
		"										\
			number	: /-?[0-9]+/ ;							\
			symbol	: '+' | '-' | '*' | '/' | '%' | '^' |				\
			           \"add\" | \"sub\" | \"mul\" | \"div\" | \"mod\" | \"pow\" ;	\
			sexpr	: '(' <expr>* ')' ;						\
			qexpr	: '{' <expr>* '}' ;						\
			expr	: <number> | <symbol> | <sexpr> ;				\
			lisp	: /^/ <expr>* /$/ ;						\
		",
		Number, Symbol, Sexpr, Qexpr, Expr, Lisp);

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
			lval* x = lval_eval(lval_read(r.output));
			lval_println(x);
			lval_del(x);
		} else {
			// else print the error
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		
		// free retrieved input
		free(input);
	}
	
	// undefine and delete our parsers
	mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lisp);

	return EXIT_SUCCESS;
}

lval* lval_eval_sexpr(lval *v) {
	
	// evaluate children
	for (int i = 0; i < v->count; i++) {
		v->cell[i] = lval_eval(v->cell[i]);
	}
	
	// error checking
	for (int i = 0; i < v->count; i++) {
		if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
	}

	// empty expr
	if (v->count == 0) { return v; }
	
	// single expression
	if (v->count == 1) { return lval_take(v, 0); }

	// ensure first element is symbol
	lval* f = lval_pop(v, 0);
	if (f->type != LVAL_SYM) {
		lval_del(f); lval_del(v);
		return lval_err("S-expression Does not start with symbol!");
	}

	// call builtin with operator
	lval* result = builtin_op(v, f->sym);
	lval_del(f);
	return result;
}

lval* lval_eval(lval* v) {
	// evaluate S expressions
	if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(v); }
	// all other lval types remain the same
	return v;
}

lval* lval_pop(lval* v, int i) {
	// find the item at 'i'
	lval* x = v->cell[i];

	// shift memory after the item at 'i' over the top
	memmove(&v->cell[i], &v->cell[i+1], 
		sizeof(lval*) * (v->count-i-1));

	// decrease the count of items in the list
	v->count--;

	// reallocate the memory used
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	return x;
}

lval* lval_take(lval* v, int i) {
	lval* x = lval_pop(v, i);
	lval_del(v);
	return x;
}

lval* builtin_op(lval* a, char* op) {
	
	// ensure all arguements are numbers
	for (int i = 0; i < a->count; i++) {
		if (a->cell[i]->type != LVAL_NUM) {
			if (a->cell[i]->type != LVAL_NUM) {
				lval_del(a);
				return lval_err("Cannot operate on non-number");
			}
		}
	}

	// pop the first element
	lval* x = lval_pop(a, 0);

	// if no arguments and sub then perform unary negation
	if ((strcmp(op, "-") == 0) && (a->count == 0)) {
		x->num = -x->num;
	}

	// while there are still elements remaining
	while (a->count > 0) {
		
		//pop the next element
		lval* y = lval_pop(a, 0);

		if ((strcmp(op, "+") == 0) || (strcmp(op, "add") == 0)) { x->num += y->num; }
		if ((strcmp(op, "-") == 0) || (strcmp(op, "sub") == 0)) { x->num -= y->num; }
		if ((strcmp(op, "*") == 0) || (strcmp(op, "mul") == 0)) { x->num *= y->num; }
		if ((strcmp(op, "/") == 0) || (strcmp(op, "div") == 0)) { 
			if (y->num == 0) {
				lval_del(x); lval_del(y);
				x = lval_err("Division By Zero!"); break;
		}
		x->num /= y->num;
		}
		if ((strcmp(op, "%") == 0) || (strcmp(op, "mod") == 0)) { x->num %= y->num; }
		if ((strcmp(op, "^") == 0) || (strcmp(op, "pow") == 0)) { x->num = (int) pow((double)x->num, y->num); }

		
		lval_del(y);
	}

	lval_del(a);
	return x;
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
		case LVAL_SYM:		printf("%s", v->sym); break;	
		case LVAL_SEXPR:	lval_expr_print(v, '(', ')'); break;	
		case LVAL_QEXPR:	lval_expr_print(v, '{', '}'); break;
	}
}

lval* lval_read_num(mpc_ast_t* t) {
	errno = 0;
	long x = strtol(t->contents, NULL, 10);
	return errno != ERANGE ?
		lval_num(x) : lval_err("invalid number");
}

lval* lval_add(lval* v, lval* x) {
	v->count++;
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	v->cell[v->count-1] = x;
	return v;
}

lval* lval_read(mpc_ast_t* t) {
	// if symbol or number return conversion to that type
	if (strstr(t->tag, "number")) { return lval_read_num(t); }
	if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

	// if root (>) or S expr or Q expr then create empty list
	lval* x = NULL;
	if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
	if (strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }
	if (strstr(t->tag, "qexpr"))  { x = lval_qexpr(); }

	// fill this newly created list with any valid expression contained within
	for (int i = 0; i < t->children_num; i++) {
		if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
		if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
		if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
		if (strcmp(t->children[i]->contents, "{") == 0) { continue; }

		if (strcmp(t->children[i]->tag, "regex") == 0) { continue; }
		x = lval_add(x, lval_read(t->children[i]));
	}

	return x;
}

void lval_del(lval* v) { //for every malloc there should be a corresponding free
	
	switch (v->type) {
	
		// do nothing special for the number type
		case LVAL_NUM: break; 
		
		//for Err or SYM free the string data
		case LVAL_ERR: free(v->err); break;
		case LVAL_SYM: free(v->sym); break;

		// if S or Q expr delete all elements inside
		case LVAL_QEXPR:
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

