#include <stdio.h>
#include <stdlib.h>
#include <math.h> //for power operator

#include "mpc.h" //written by books author

#include <readline/history.h>
#include <readline/readline.h>

#define LASSERT(args, cond, fmt, ...) \
  if (!(cond)) { lval* err = lval_err(fmt, ##__VA_ARGS__); lval_del(args); return err; }

#define LASSERT_TYPE(func, args, index, expect) \
  LASSERT(args, args->cell[index]->type == expect, \
    "Function '%s' passed incorrect type for argument %i. Got %s, Expected %s.", \
    func, index, ltype_name(args->cell[index]->type), ltype_name(expect))

#define LASSERT_NUM(func, args, num) \
  LASSERT(args, args->count == num, \
    "Function '%s' passed incorrect number of arguments. Got %i, Expected %i.", \
    func, args->count, num)

#define LASSERT_NOT_EMPTY(func, args, index) \
  LASSERT(args, args->cell[index]->count != 0, \
    "Function '%s' passed {} for argument %i.", func, index);

// forward declarions

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

// Lisp Value

enum {  LVAL_ERR, LVAL_NUM,   LVAL_SYM,
	LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR };

char* ltype_name(int t) {
  switch(t) {
    case LVAL_FUN: return "Function";
    case LVAL_NUM: return "Number";
    case LVAL_ERR: return "Error";
    case LVAL_SYM: return "Symbol";
    case LVAL_SEXPR: return "S-Expression";
    case LVAL_QEXPR: return "Q-Expression";
    default: return "Unknown";
  }
}

typedef lval*(*lbuiltin)(lenv*, lval*);

struct lval { // needs to be defined as lval before it can reference itself recursively
	int type;

	long num;
	char* err;
	char* sym;
	lbuiltin fun;

	//pointer to list of lvals and count of size
	int count;
	lval** cell; // if it was one star it would be infinite it size
};

struct lenv {
	int count;
	char** syms;
	lval** vals;
};

// construct pointer to new Number, Error, Symbol, Fun, and empty S expr or Q expr lval 
lval* lval_num(long x) {
	lval* v = malloc(sizeof(lval));;
	v->type = LVAL_NUM;
	v->num = x;
	return v;
}

lval* lval_err(char* fmt, ...) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  
  /* Create a va list and initialize it */
  va_list va;
  va_start(va, fmt);
  
  /* Allocate 512 bytes of space */
  v->err = malloc(512); // make sure error messages arent longer than 512
  
  /* printf the error string with a maximum of 511 characters */
  vsnprintf(v->err, 511, fmt, va);
  
  /* Reallocate to number of bytes actually used */
  v->err = realloc(v->err, strlen(v->err)+1);
  
  /* Cleanup our va list */
  va_end(va);
  
  return v;
}

lval* lval_sym(char* s) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->sym = malloc(strlen(s)+1); //the plus one is because strlen excludes the null terminating byte
	strcpy(v->sym, s);
	return v;
}

lval* lval_fun(lbuiltin func) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_FUN;
	v->fun = func;
	return v;
}

lval* lval_sexpr(void) { 
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

lval* lval_qexpr(void) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_QEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

// cread and delete lenv structs
lenv* lenv_new(void) {
	lenv* e = malloc(sizeof(lenv));
	e->count = 0;
	e->syms = NULL;
	e->vals = NULL;
	return e;
}

// delete lval* and free the memory
void lval_del(lval* v);

void lenv_del(lenv* e) {
	for (int i = 0; i < e->count; i++) {
		free(e->syms[i]);
		lval_del(e->vals[i]);
	}
	free(e->syms);
	free(e->vals);
	free(e);
}

lval* lval_read(mpc_ast_t* t);
lval* lval_copy(lval* v);
//print an lval
void lval_print(lval* v);
//print an lval and append a newline
void lval_println(lval* v) { lval_print(v); putchar('\n'); }

lval* lval_add(lval* v, lval* x);

lval* lval_eval(lenv* e, lval* v);
lval* lval_eval_sexpr(lenv* e, lval* v);

lval* lval_pop(lval* v, int i); // takes an index i and removes it from v, then returns it, leaving the rest of v intact
lval* lval_take(lval* v, int i); // takes an index i from v and returns it, deleting all of v in the process

lval* builtin(lenv* e, lval* a, char* func);
lval* builtin_op(lenv* e, lval* a, char* op);
lval* builtin_def(lenv* e, lval* a);

void lval_expr_print(lval* v, char open, char close);

lval* lenv_get(lenv* e, lval* k);
void lenv_put(lenv* e, lval* k, lval* v);
void lenv_add_builtins(lenv* e);
// "char** argv" and "char **argv" both mean the same thing, a pointer to a character pointer
int main(int argc, char** argv) { 

	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Symbol = mpc_new("symbol");	
	mpc_parser_t* Sexpr  = mpc_new("sexpr");
	mpc_parser_t* Qexpr  = mpc_new("qexpr");
	mpc_parser_t* Expr   = mpc_new("expr");
	mpc_parser_t* Lisp   = mpc_new("lisp");

	mpca_lang(MPCA_LANG_DEFAULT,
		"                                             			\
			number : /-?[0-9]+/ ;                  			\
  			symbol : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;		\
			sexpr  : '(' <expr>* ')' ;                         	\
			qexpr  : '{' <expr>* '}' ;             			\
    			expr   : <number> | <symbol> | <sexpr> | <qexpr> ;	\
    			lisp   : /^/ <expr>* /$/ ;                         	\
  		",
  		Number, Symbol, Sexpr, Qexpr, Expr, Lisp);

	// print version and how to exit
	puts("Xen: A Lisp Interpreter for C");
	puts("Version 0.0.0.0.6");
	puts("Press Ctrl+C to Exit\n");

	lenv* e = lenv_new();
	lenv_add_builtins(e);
	
	while (1){
	
		char *input = readline("xen> ");
		add_history(input);		

		mpc_result_t r;
		if (mpc_parse("<stdin", input, Lisp, &r)) {
		
  			lval* x = lval_eval(e, lval_read(r.output));
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
	
	lenv_del(e);
	
	// undefine and delete our parsers
	mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lisp);

	return EXIT_SUCCESS;
}

lval* lval_eval_sexpr(lenv* e, lval *v) {
	
	// evaluate children
	for (int i = 0; i < v->count; i++) {
		v->cell[i] = lval_eval(e, v->cell[i]);
	}
	
	// error checking
	for (int i = 0; i < v->count; i++) {
		if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
	}

	// empty expr
	if (v->count == 0) { return v; }
	
	// single expression
	if (v->count == 1) { return lval_take(v, 0); }

	// ensure first element is function
	lval* f = lval_pop(v, 0);
	if (f->type != LVAL_FUN) {
		lval_del(f); lval_del(v);
		return lval_err("First element is not a function!");
	}

	// call function to get result
	lval* result = f->fun(e, v);
	lval_del(f);
	return result;
}

lval* lval_eval(lenv* e, lval* v) {
	if (v->type == LVAL_SYM) {
		lval* x = lenv_get(e, v);
		lval_del(v);
		return x;
	}
	if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(e, v); }
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

lval* builtin_op(lenv* e, lval* a, char* op) {
  
  	for (int i = 0; i < a->count; i++) {
		LASSERT_TYPE(op, a, i, LVAL_NUM);
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
		
		lval_del(y);
	}

	lval_del(a);
	return x;
}

lval* builtin_add(lenv* e, lval* a) {
  return builtin_op(e, a, "+");
}

lval* builtin_sub(lenv* e, lval* a) {
  return builtin_op(e, a, "-");
}

lval* builtin_mul(lenv* e, lval* a) {
  return builtin_op(e, a, "*");
}

lval* builtin_div(lenv* e, lval* a) {
  return builtin_op(e, a, "/");
}

lval* builtin_head(lenv* e, lval* a) {
  LASSERT_NUM("head", a, 1);
  LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
  LASSERT_NOT_EMPTY("head", a, 0);
  
  lval* v = lval_take(a, 0);  
  while (v->count > 1) { lval_del(lval_pop(v, 1)); }
  return v;
}

lval* builtin_tail(lenv* e, lval* a) {
  LASSERT_NUM("tail", a, 1);
  LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
  LASSERT_NOT_EMPTY("tail", a, 0);

  lval* v = lval_take(a, 0);  
  lval_del(lval_pop(v, 0));
  return v;
}

lval* builtin_list(lenv* e, lval* a) {
	a->type = LVAL_QEXPR;
	return a;
}

lval* builtin_eval(lenv* e, lval* a) {
  LASSERT_NUM("eval", a, 1);
  LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);
  
  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(e, x);
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

lval* builtin_join(lenv* e, lval* a) {
	
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

lval* builtin_def(lenv* e, lval* a) {
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
    "Function 'def' passed incorrect type!");

  /* First argument is symbol list */
  lval* syms = a->cell[0];

  /* Ensure all elements of first list are symbols */
  for (int i = 0; i < syms->count; i++) {
    LASSERT(a, syms->cell[i]->type == LVAL_SYM,
      "Function 'def' cannot define non-symbol");
  }

  /* Check correct number of symbols and values */
  LASSERT(a, syms->count == a->count-1,
    "Function 'def' cannot define incorrect "
    "number of values to symbols");

  /* Assign copies of values to symbols */
  for (int i = 0; i < syms->count; i++) {
    lenv_put(e, syms->cell[i], a->cell[i+1]);
  }

  lval_del(a);
  return lval_sexpr();
}

/*
lval* builtin(lenv* e, lval* a, char* func) {
	if (strcmp("list", func) == 0) { return builtin_list(a); }
	if (strcmp("head", func) == 0) { return builtin_head(a); }
	if (strcmp("tail", func) == 0) { return builtin_tail(a); }
	if (strcmp("join", func) == 0) { return builtin_join(a); }
	if (strcmp("eval", func) == 0) { return builtin_eval(a); }
	if (
		(strstr("/+-*", func)) || 
		(strcmp("add", func)) ||
		(strcmp("sub", func)) ||
		(strcmp("mul", func)) ||
		(strcmp("div", func))
	) { return builtin_op(a, func); }

	lval_del(a);
	return lval_err("Unknown Function!");
}
*/
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
		case LVAL_FUN:	 printf("<function>"); break;
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

lval* lval_copy(lval* v) {
	
	lval* x = malloc(sizeof(lval));
	x->type = v->type;

	switch (v->type) {
		// copy functions and numbers directly
		case LVAL_FUN: x->fun = v->fun; break;
		case LVAL_NUM: x->num = v->num; break;
		
		// copy strings using malloc and strcpy
		case LVAL_ERR:
			x->err = malloc(strlen(v->err) + 1);
			strcpy(x->err, v->err); break;
		case LVAL_SYM:
			x->sym = malloc(strlen(v->sym) + 1);
			strcpy(x->sym, v->sym); break;
		
		// copy lists by copying each sub expression
		case LVAL_SEXPR:
		case LVAL_QEXPR:
			x->count = v->count;
			x->cell = malloc(sizeof(lval*) * x->count);
			for (int i = 0; i < x->count; i++) {
				x->cell[i] = lval_copy(v->cell[i]);
			}
		break;
	}

	return x;
}
 
//for every malloc there should be a corresponding free	
void lval_del(lval* v) {

	switch (v->type) {
		case LVAL_NUM: break;
    		case LVAL_ERR: free(v->err); break;
    		case LVAL_SYM: free(v->sym); break;
		case LVAL_FUN: break;
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

lval* lenv_get(lenv* e, lval* k) {
	// iterate over all items in environment
	for (int i = 0; i < e->count; i++) {
		// if stored string matches the symbol string return a copy of the value
		if (strcmp(e->syms[i], k->sym) == 0) {
			return lval_copy(e->vals[i]);
		}
	}
	// if no symbol found return error
	return lval_err("unbound symbol!", k->sym);
}

void lenv_put(lenv* e, lval* k, lval* v) {
	// iterate over all items in environment to see if the varaible exists already
	for (int i = 0; i < e->count; i++) {
		// if variable is found delete item at that position
		// and replace it with variable supplied by user
		if (strcmp(e->syms[i], k->sym) == 0) {
			lval_del(e->vals[i]);
			e->vals[i] = lval_copy(v);
			return;
		}
	}

	// if no existing entry found allocate space for a new entry
	e->count++;
	e->vals = realloc(e->vals, sizeof(lval*) * e->count);
	e->syms = realloc(e->syms, sizeof(char*) * e->count);
	// copy contents of lval and symbol string into new location
	e->vals[e->count-1] = lval_copy(v);
	e->syms[e->count-1] = malloc(strlen(k->sym)+1);
	strcpy(e->syms[e->count-1], k->sym);
}

void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
	lval* k = lval_sym(name);
	lval* v = lval_fun(func);
	lenv_put(e, k, v);
	lval_del(k); lval_del(v);
}

void lenv_add_builtins(lenv* e) {
  /* Variable Functions */
  lenv_add_builtin(e, "def", builtin_def);
  
  /* List Functions */
  lenv_add_builtin(e, "list", builtin_list);
  lenv_add_builtin(e, "head", builtin_head);
  lenv_add_builtin(e, "tail", builtin_tail);
  lenv_add_builtin(e, "eval", builtin_eval);
  lenv_add_builtin(e, "join", builtin_join);
  
  /* Mathematical Functions */
  lenv_add_builtin(e, "+", builtin_add);
  lenv_add_builtin(e, "-", builtin_sub);
  lenv_add_builtin(e, "*", builtin_mul);
  lenv_add_builtin(e, "/", builtin_div);
}
