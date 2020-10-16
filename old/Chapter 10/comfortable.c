#include <stdio.h>
#include <stdlib.h>
#include <math.h> //for power operator

#include "mpc.h" //written by books author

#include <readline/history.h>
#include <readline/readline.h>

#define LASSERT(args, cond, err) \
	if (!(cond)) { lval_del(args); return lval_err(err); }

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

/* A pointer to a new empty Qexpr lval */
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

lval* lval_add(lval* v, lval* x);

// delete lval* and free the memory
void lval_del(lval* v);

lval* lval_eval_sexpr(lval* v);
lval* lval_eval(lval* v);

lval* lval_pop(lval* v, int i); // takes an index i and removes it from v, then returns it, leaving the rest of v intact
lval* lval_take(lval* v, int i); // takes an index i from v and returns it, deleting all of v in the process

lval* builtin(lval* a, char* func);
lval* builtin_op(lval* a, char* op);

void lval_expr_print(lval* v, char open, char close);

// "char** argv" and "char **argv" both mean the same thing, a pointer to a character pointer
int main(int argc, char** argv) { 

	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Symbol = mpc_new("symbol");	
	mpc_parser_t* Sexpr  = mpc_new("sexpr");
	mpc_parser_t* Qexpr  = mpc_new("qexpr");
	mpc_parser_t* Expr   = mpc_new("expr");
	mpc_parser_t* Lisp   = mpc_new("lisp");

	mpca_lang(MPCA_LANG_DEFAULT,
		"                                 	                   			\
			number : /-?[0-9]+/ ;                             			\
  			symbol : \"list\" | \"head\" | \"tail\" | \"join\" | \"eval\" |		\
    				 '+' | '-' | '*' | '/' | '%' | '^' |				\
			         \"add\" | \"sub\" | \"mul\" | \"div\" | \"mod\" | \"pow\" ;	\
			sexpr  : '(' <expr>* ')' ;                         			\
			qexpr  : '{' <expr>* '}' ;                         			\
    			expr   : <number> | <symbol> | <sexpr> | <qexpr> ; 			\
    			lisp   : /^/ <expr>* /$/ ;                         			\
  		",
  		Number, Symbol, Sexpr, Qexpr, Expr, Lisp);

	// print version and how to exit
	puts("Xen: A Lisp Interpreter for C");
	puts("Version 0.0.0.0.6");
	puts("Press Ctrl+C to Exit\n");

	// loop will never end
	while (1){
	
		// output prompt and then read input
		char *input = readline("xen> ");

		// add input to history
		add_history(input);		

		// attempt to parse user's input
		mpc_result_t r;
		if (mpc_parse("<stdin", input, Lisp, &r)) {
			// if successful try reading in result and printing, then delete old tree
  			lval* x = lval_eval(lval_read(r.output));
			lval_println(x);
			lval_del(x);
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
	lval* result = builtin(v, f->sym);
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

lval* builtin_head(lval* a) {
	LASSERT(a, a->count == 1,
		"Function 'head' passed too many arguments!");
	LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
		"Function 'head' passed incorrect types!");
	LASSERT(a, a->cell[0]->count != 0,
		"Function 'head' passed {}!");

	lval* v = lval_take(a, 0);
	while (v->count > 1) { lval_del(lval_pop(v, 1)); }
	return v;
}

lval* builtin_tail(lval* a) {
	LASSERT(a, a->count == 1,
		"Function 'tail' passed too many arguments!");
	LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
		"Function 'tail' passed incorrect types!");
	LASSERT(a, a->cell[0]->count != 0,
		"Function 'tail' passed {}!");

	lval* v = lval_take(a, 0);
	lval_del(lval_pop(v, 0));
	return v;
}

lval* builtin_list(lval* a) {
	a->type = LVAL_QEXPR;
	return a;
}

lval* builtin_eval(lval* a) {
	LASSERT(a, a->count == 1,
		"Function 'eval' passed too many arguments!");
	LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
		"Function 'eval' passed incorrect types!");
	
	lval* x = lval_take(a, 0);
	x->type = LVAL_SEXPR;
	return lval_eval(x);
}

lval* lval_join(lval* x, lval* y) {
	// for each cell in 'y', add it to 'x'
	while (y->count) {
		x = lval_add(x, lval_pop(y, 0));
	}

	// delete the empty y and return x
	lval_del(y);
	return x;
}

lval* builtin_join(lval* a) {
	
	for (int i = 0; i < a->count; i++) {
		LASSERT(a, a->cell[i]->type == LVAL_QEXPR,
			"Function 'join' passed incorrect type.");
	}

	lval* x = lval_pop(a, 0);

	while (a->count) {
		x = lval_join(x, lval_pop(a, 0));
	}

	lval_del(a);
	return x;
}

lval* builtin(lval* a, char* func) {
	if (strcmp("list", func) == 0) { return builtin_list(a); }
	if (strcmp("head", func) == 0) { return builtin_head(a); }
	if (strcmp("tail", func) == 0) { return builtin_tail(a); }
	if (strcmp("join", func) == 0) { return builtin_join(a); }
	if (strcmp("eval", func) == 0) { return builtin_eval(a); }
	if (
		(strstr("+-*/%^", func)) || 
		(strcmp("add", func)) ||
		(strcmp("sub", func)) ||
		(strcmp("mul", func)) ||
		(strcmp("div", func)) ||
		(strcmp("mod", func)) ||
		(strcmp("pow", func)) 
	) { return builtin_op(a, func); }

	lval_del(a);
	return lval_err("Unknown Function!");
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
		case LVAL_NUM:   printf("%li", v->num); break;
		case LVAL_ERR:   printf("Error: %s", v->err); break;
    		case LVAL_SYM:   printf("%s", v->sym); break;
    		case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
    		case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
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
  
	if (strstr(t->tag, "number")) { return lval_read_num(t); }
  	if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }
  
  	lval* x = NULL;
  	if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); } 
  	if (strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }
  	if (strstr(t->tag, "qexpr"))  { x = lval_qexpr(); }
  
 	for (int i = 0; i < t->children_num; i++) {
		if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
   		if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
    		if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
    		if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
    		if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
    		x = lval_add(x, lval_read(t->children[i]));
  	}
  
  	return x;
}
 
//for every malloc there should be a corresponding free	
void lval_del(lval* v) {

	switch (v->type) {
		case LVAL_NUM: break;
    		case LVAL_ERR: free(v->err); break;
    		case LVAL_SYM: free(v->sym); break;

    		/* If Qexpr or Sexpr then delete all elements inside */
    		case LVAL_QEXPR:
    		case LVAL_SEXPR:
			for (int i = 0; i < v->count; i++) {
        		lval_del(v->cell[i]);
      			}
      			/* Also free the memory allocated to contain the pointers */
      			free(v->cell);
    		break;
  	}

  	free(v);
}
