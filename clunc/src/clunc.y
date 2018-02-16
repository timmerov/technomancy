/*
grammar for clunc.
*/

%defines
%define api.prefix {clunc_yy}
%parse-param { clunc_node **pcn }
%debug

%{
#include "clunc.h"

#include <stdlib.h>
#include <string.h>

//#define TSC_PRINT fprintf
#if !defined(TSC_PRINT)
#define TSC_PRINT(...)
#endif

static void clunc_yyerror(clunc_node **pcn, const char *s) ;
#define ALIGN 64
%}

%union {
	const char *s;
	clunc_node *cn;
}

%token <s> KEY_INT
%token <s> KEY_STRING

%token <s> NUMBER
%token <s> STRING
%token <s> TOKEN

%type <cn> functions
%type <cn> function
%type <cn> declaration
%type <s> decltype

%%

start :
	functions {
		TSC_PRINT(stdout, "bison start functions\n");
		*pcn = $1;
	} ;

functions :
	function functions {
        TSC_PRINT(stdout, "bison functions function functions\n");
        if ($1) {
			$$ = $1;
			$$->next = $2;
		} else {
			$$ = $2;
		}
	} | {
        TSC_PRINT(stdout, "bison functions\n");
        $$ = NULL;
	} ;

function :
	declaration '{' '}' {
        TSC_PRINT(stdout, "bison function declaration { }\n");
        $$ = clunc_alloc(kCluncFunction);
		$$->child1 = $1;
	} ;

declaration :
	TOKEN decltype {
        TSC_PRINT(stdout, "bison declaration TOKEN decltype\n");
        $$ = clunc_alloc(kCluncDeclaration);
		$$->token1 = $1;
        $$->token2 = $2;
	} ;

decltype :
	KEY_INT {
        TSC_PRINT(stdout, "bison decltype KEY_INT\n");
        $$ = strdup("int");
	} | KEY_STRING {
        TSC_PRINT(stdout, "bison decltype KEY_STRING\n");
        $$ = strdup("string");
	} ;

%%

static void clunc_yyerror(clunc_node **pb, const char *s) {
	fprintf(stderr, "clunc:error: %s\n", s);
}

clunc_node *clunc_load_string(const char *str) {
    //yydebug = 1;
    clunc_node *cn = 0;
    clunc_scan_string(str);
    clunc_yyparse(&cn);
    return cn;
}
