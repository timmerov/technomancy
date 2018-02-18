/*
grammar for clunc.
*/

%defines
%define api.prefix {clunc_yy}
%parse-param { clunc_node **proot }
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

%token<s> KEY_INT
%token<s> KEY_STRING

%token<s> NUMBER
%token<s> STRING
%token<s> TOKEN

%type <cn> translation_units
%type <cn> translation_unit
%type <cn> class_declaration

%type <s> identifier

%%

start
	: translation_units { start(proot, $1); }
	;

translation_units
	: translation_unit translation_units { $$ = translation_units($1, $2); }
	| { $$ = NULL; }
	;

translation_unit
	: class_declaration { $$ = $1; }
	| function_declaration { $$ = NULL; }
	;

class_declaration
	: identifier '(' field_declarations ')' { $$ = class_declaration($1, NULL); }
	;

field_declarations
	: field_declaration field_declarations
	| { }
	;

field_declaration
	: identifier type_specifier '=' const_expression ';'
	| identifier type_specifier ';'
	;

function_declaration
	: identifier type_specifier '{' statements '}'
	;

statements
	: statement statements
	| { }
	;

statement
	: assignment
	;

assignment
	: identifier type_specifier ';'
	| identifier type_specifier assignment_op expression ';'
	| identifier assignment_op expression ';'
	;

assignment_op
	: '='
	;

expression /*tbd*/
	: const_expression
	;

identifier
	: TOKEN
	;

type_specifier
	: KEY_INT
	| KEY_STRING
	;

const_expression
	: NUMBER
	| STRING
	;

%%


#if 0
%type <s> translation_units
%type <s> translation_unit
%type <s> class_declaration
%type <s> field_declarations
%type <s> field_declaration
%type <s> function_declaration
%type <s> statements
%type <s> statement
%type <s> assignment
%type <ch> assignment_op
%type <s> expression
%type <s> identifier
%type <s> type_specifier
%type <s> const_expression

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
	declaration '{' statements '}' {
        TSC_PRINT(stdout, "bison function declaration { }\n");
        $$ = clunc_alloc(kCluncFunction);
		$$->child1 = $1;
		$$->child2 = $3;
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

statements :
	statement statements {
        TSC_PRINT(stdout, "bison statements statement statements\n");
        if ($1) {
			$$ = $1;
			$$->next = $2;
		} else {
			$$ = $2;
		}
	} | {
        TSC_PRINT(stdout, "bison statements\n");
        $$ = NULL;
	} ;

statement :
	declaration '=' expression ';' {
        TSC_PRINT(stdout, "bison statement declaration = expression ;\n");
        $$ = clunc_alloc(kCluncStatement);
		$$->child1 = $1;
		$$->token1 = $3;
	} ;

expression :
	NUMBER {
        TSC_PRINT(stdout, "bison expression NUMBER\n");
        $$ = $1;
	} | STRING {
        TSC_PRINT(stdout, "bison expression STRING\n");
        $$ = $1;
	} | TOKEN {
        TSC_PRINT(stdout, "bison expression TOKEN\n");
        $$ = $1;
	} ;
#endif

extern int g_start_line;
extern int g_start_column;
extern int g_end_line;
extern int g_end_column;
extern char *g_last_token;

static void clunc_yyerror(
	clunc_node **cn,
	const char *s
) {
	fprintf(stderr, "clunc:error:%d:%d:%d:%d %s near %s\n",
		g_start_line, g_start_column, g_end_line, g_end_column, s, g_last_token);
}

clunc_node *clunc_load_string(
	const char *str
) {
	//yydebug = 1;
	clunc_node *cn = 0;
	clunc_scan_string(str);
	clunc_yyparse(&cn);
	return cn;
}
