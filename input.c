#include <stdio.h>
#include <stdlib.h>
#include "ParserCombinatorLibrary/src/mpc.h"

#ifdef _WIN32 
#include <string.h>

static char buffer[2048];

char* readline(char* prompt) {
	fputs(prompt, stdout);
	fgets(input, 2048, stdin);
	char* cpy = malloc(strlen(buffer)+1);
	strcpy(cpy, buffer);
	cpy[strlen(cpy)-1] = '\0';
	return cpy;
}

void add_history(char* unused) {}

#else
#include <editline.h>
#endif

enum { LISPVAL_NUM, LISPVAL_ERR, LISPVAL_SYM, LISPVAL_SYMEXP };

typedef struct {
    int type;
    long num;
    char* err;
    char* sym;
    int count;
    struct lispval** cell;
} lispval;

lispval* lispval_num(long x) {
    lispval* v = malloc(sizeof(lispval));
    v->type = LISPVAL_NUM;
    v->num = x;
    return v;
}

lispval* lispval_err(char* m) {
    lispval* v = malloc(sizeof(lispval));
    v->type = LISPVAL_ERR;
    v->err = malloc(strlen(m)+1);
    strcpy(v->err, m);
    return v;
}

lispval* lispval_sym(char* s) {
    lispval* v = malloc(sizeof(lispval));
    v->type = LISPVAL_SYM;
    v->sym = malloc(sizeof(s)+1);
    strcpy(v->sym, s);
    return v;
}

lispval* lispval_symexp(void) {
    lispval* v = malloc(sizeof(lispval));
    v->type = LISPVAL_SYMEXP;
    v->count = 0;
    v->cell = NULL;
    return v;
}

void lispval_del(lispval* v) {
    switch (v->type) {
        case LISPVAL_NUM: break;
        case LISPVAL_ERR: free(v->err); break;
        case LISPVAL_SYM: free(v->sym); break;
        case LISPVAL_SYMEXP: 
                       for (int i = 0; i < v->count; i++) {
                           lispval_del(v->cell[i]);
                       }
                       free(v->cell);
        break;
    }
    free(v);
}

lispval* lispval_add(lispval* v, lispval* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lispval*) * v->count);
    v->cell[v->count-1] = x;
    return v;
}

lispval* lispval_read_num(mpc_ast_t* t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno !=ERANGE ?
        lispval_num(x) : lispval_err("Invalid number");
}

lispval* lispval_read(mpc_ast_t* t) { 
    if (strstr(t->tag, "number")) { return lispval_read_num(t);}
    if (strstr(t->tag, "symbol")) { return lispval_sym(t->contents);}
    
    lispval* x = NULL;
    if (strcmp(t->tag, ">")==0) { x = lispval_symexp();}
    if (strstr(t->tag, "symexpression")) { x = lispval_symexp();}

    for (int i = 0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(")==0) { continue; }
        if (strcmp(t->children[i]->contents, ")")==0) { continue; }
        if (strcmp(t->children[i]->tag, "regex")==0) { continue; }
        x = lispval_add(x, lispval_read(t->children[i]));
    }

    return x;
}

void lispval_print(lispval* v);

void lispval_expr_print(lispval* v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {
        lispval_print(v->cell[i]);
        if (i != (v->count-1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

void lispval_print(lispval* v) {
    switch (v->type) {
        case LISPVAL_NUM: printf ("%li", v->num); break;
        case LISPVAL_ERR: printf("Error: %s",v->err); break;
        case LISPVAL_SYM: printf("%s",v->sym); break;
        case LISPVAL_SYMEXP: lispval_expr_print(v, '(', ')'); break;
    }
}

void lispval_println(lispval* v) {
    lispval_print(v);
    putchar('\n');
}

/*lispval evaluate_op(lispval x, char* op, lispval y) {

    if (x.type == LISPVAL_ERR) { return x; }
    if (y.type == LISPVAL_ERR) { return y; }

    if(strcmp(op, "+") == 0) {return lispval_num(x.num + y.num);}
    if(strcmp(op, "-") == 0) {return lispval_num(x.num - y.num);}
    if(strcmp(op, "*") == 0) {return lispval_num(x.num * y.num);}
    if(strcmp(op, "/") == 0) {return y.num == 0 ? lispval_err(LISPERR_DIV_ZERO) : lispval_num(x.num / y.num); }
    if(strcmp(op, "%") == 0) {return lispval_num(x.num % y.num);}
    
    return lispval_err(LISPERR_INVALID_OP);
}

lispval evaluate(mpc_ast_t* t) {

    if(strstr(t->tag, "number")) {
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lispval_num(x) : lispval_err(LISPERR_INVALID_NUM);
    }
    
    char* op = t->children[1]->contents;

    lispval x = evaluate(t->children[2]);

    int i = 3;
    while (strstr(t->children[i]->tag, "expression")) {
        x = evaluate_op(x, op, evaluate(t->children[i]));
        i++;
    }

    return x;
}*/

int main(int argc, char** argv) {

    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* SymExpression = mpc_new("symexpression");
    mpc_parser_t* Expression = mpc_new("expression");
    mpc_parser_t* Basedlisp = mpc_new("basedlisp");

    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                                \
          number        : /-?[0-9]+/ ;                                   \
          symbol        : '+' | '-' | '*' | '/' | '%' ;                  \
          symexpression : '(' <expression>* ')' ;                        \
          expression    : <number> | '(' <operator> <expression>+ ')' ;  \
          basedlisp     : /^/ <operator> <expression>+ /$/ ;             \
        ",
            Number, Symbol, SymExpression, Expression, Basedlisp);

	puts("Basedlisp version 0.0.0.0.3");
	puts("Press Ctrl+c to Exit\n");

	while (1) {
		char* input = readline("basedlisp> ");
		add_history(input);
		mpc_result_t r;
        if (mpc_parse("<stdin>", input, Basedlisp, &r)) {
            mpc_ast_print(r.output);
            lispval* x  = lispval_read(r.output);
            lispval_println(x);
            lispval_del(x);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
		free(input);
	}

    mpc_cleanup(5, Number, Symbol, SymExpression, Expression, Basedlisp);

	return 0;
}
