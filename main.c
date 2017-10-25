#include <stdio.h>
#include "editline/readline.h"
#include <stdlib.h>
#include <assert.h>
#include "mpc.h"

#define INPUT_SIZE 2048
#define MAJOR_VERSION 0
#define MINOR_VERSION 1
#define GRAMMAR_LIMIT (10 * 1024)

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
double eval_op(double x, char* op, double y) {
    printf("debug: %s %lf  %lf\n", op, x, y);
    if (strcmp(op, "+") == 0) { return x + y; }
    if (strcmp(op, "-") == 0) { return x - y; }
    if (strcmp(op, "*") == 0) { return x * y; }
    if (strcmp(op, "/") == 0) { return x / y; }
    printf("Not implemented evaluation of operator: %s", op);
    return 1;
}

int quit(mpc_ast_t* t) {
    if (t->children_num > 1 && strstr(t->children[1]->contents, "exit")) {
        return 1;
    }
    return 0;
}

//evaluate parse tree
double eval(mpc_ast_t* t) {

    if(strstr(t->tag, "number") != NULL) {
        return atof(t->contents);
    }

    char* op = t->children[1]->contents;
    double x = eval(t -> children[2]);

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

            printf("%lf\n", eval(r.output));

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
