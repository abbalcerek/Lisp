#include <stdio.h>
#include "editline/readline.h"
#include <stdlib.h>
#include <assert.h>
#include "mpc.h"

#define INPUT_SIZE 2048
#define MAJOR_VERSION 0
#define MINOR_VERSION 1
#define GRAMMAR_LIMIT (10 * 1024)

enum {LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM};
enum {LVAL_NUM, LVAL_ERR};

typedef struct {
    int type;
    double num;
    int err;
} lval;

lval lval_num(double x) {
    lval v;
    v.type = LVAL_NUM;
    v.num = x;
    return v;
}

lval lval_err(int x) {
    lval v;
    v.type = LVAL_ERR;
    v.err = x;
    return v;
}

void lval_print(lval v) {
    switch (v.type) {
        case LVAL_NUM:
            printf("%lf", v.num);
            break;
        case LVAL_ERR:
            if (v.err == LERR_DIV_ZERO) {
                printf("Error: Division by Zero!\n");
            } else if (v.err == LERR_BAD_OP) {
                printf("Error: Invalid Operation!");
            } else if (v.err == LERR_BAD_NUM) {
                printf("Error: Invalid Number!");
            }
            break;
    }
}

void lval_println(lval v) { lval_print(v); putchar('\n'); }

static char input[INPUT_SIZE];

//read grammer
void read_language_definition(char* lang_definition, char* grammar_file) {
    FILE *f = fopen(grammar_file, "r");

    fseek(f, 0L, SEEK_END);
    assert(ftell(f) < GRAMMAR_LIMIT);
    rewind(f);

    fread(lang_definition, 1, GRAMMAR_LIMIT, f);
    fclose(f);
}

//evaluate
lval eval_op(lval x, char* op, lval y) {
//    printf("debug: %s %lf  %lf\n", op, x, y);
    if (x.type == LVAL_ERR) { return x; }
    if (y.type == LVAL_ERR) { return y; }
    if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
    if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
    if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
    if (strcmp(op, "/") == 0) {

        return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
    }
    printf("Not implemented evaluation of operator: %s", op);
    return lval_err(LERR_BAD_OP);
}

int quit(mpc_ast_t* t) {
    if (t->children_num > 1 && strstr(t->children[1]->contents, "exit")) {
        return 1;
    }
    return 0;
}

//evaluate parse tree
lval eval(mpc_ast_t* t) {

    if(strstr(t->tag, "number") != NULL) {

        errno = 0;
        double x = strtod(t->contents, NULL);
        return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }

    char* op = t->children[1]->contents;
    lval x = eval(t -> children[2]);

    int i = 3;

    while (strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;

}


int main(int argc, char** argv) {

    //create parser
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Exit = mpc_new("exit");
    mpc_parser_t* Lispy = mpc_new("lispy");

    char lang_definition[GRAMMAR_LIMIT];
    read_language_definition(lang_definition, "grammar1.txt");

    puts(lang_definition);

    mpca_lang(MPCA_LANG_DEFAULT, lang_definition,
    Number, Operator, Expr, Exit, Lispy);

    //print hello message
    printf("Lisp version %d.%d\n", MAJOR_VERSION, MINOR_VERSION);
    puts("Press Ctrl+c or type `exit` to Exit");

    //main repel loop

    for (;;) {

        char* input = readline("lisp> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            mpc_ast_print(r.output);

            if (quit(r.output)) {
                puts("Good bye!!!");
                break;
            }

            lval_println(eval(r.output));

            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        //free user input buffer
        free(input);

    }

    //clean up parsers
    mpc_cleanup(5, Number, Operator, Expr, Exit, Lispy);

    return 0;

}
