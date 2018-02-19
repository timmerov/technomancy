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
	int i;
	const char *s;
	clunc_node *cn;
}

%token<i> KEY_INT
%token<i> KEY_STRING

%token<i> CONSTANT
%token<s> STRING_LITERAL
%token<s> IDENTIFIER

%token<i> ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN SUB_ASSIGN
%token<i> LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN XOR_ASSIGN OR_ASSIGN
%token<i> LOG_OR LOG_AND LOG_NOT
%token<i> OR_OP XOR_OP AND_OP NOT_OP
%token<i> EQ_OP NE_OP LT_OP GT_OP LE_OP GE_OP
%token<i> LEFT_OP RIGHT_OP
%token<i> ADD_OP SUB_OP MUL_OP DIV_OP MOD_OP
%token<i> INC_OP DEC_OP

%type <cn> translation_units
%type <cn> translation_unit
%type <cn> class_declaration
%type <cn> field_declarations
%type <cn> field_declaration
%type <cn> type_specifier
%type <cn> function_declaration
%type <cn> statements
%type <cn> statement
%type <cn> expression
%type <i> binary_operator
%type <cn> unary_expression
%type <i> unary_operator
%type <cn> postfix_expression
%type <i> postfix_operator
%type <cn> primary_expression

%%

/**
classes and functions
**/
start
	: translation_units { start(proot, $1); }
	;

/**
classes and functions
**/
translation_units
	: translation_unit translation_units { $$ = build_list($1, $2); }
	| { $$ = NULL; }
	;

/**
classes and functions
**/
translation_unit
	: class_declaration { $$ = $1; }
	| function_declaration { $$ = $1; }
	;

/**
message ( ... )
**/
class_declaration
	: IDENTIFIER '(' field_declarations ')' { $$ = class_declaration($1, $3); }
	;

/**
comma separated list of expressions.
trailing comma is allowed.
**/
field_declarations
	: field_declaration { $$ = $1; }
	| field_declaration ',' { $$ = $1; }
	| field_declaration ',' field_declarations { $$ = build_list($1, $3); }
	| { $$ = NULL; }
	;

field_declaration
	: IDENTIFIER type_specifier '=' const_expression { $$ = field_declaration($1, $2, $4); }
	| IDENTIFIER type_specifier { $$ = field_declaration($1, $2, NULL); }
	;

type_specifier
	: standard_type_specifier { $$ = standard_type_specifier($1); }
	;

function_declaration
	: IDENTIFIER type_specifier '{' statements '}' { $$ = function_declaration($1, $2, $4); }
	;

statements
	: statement statements { $$ = build_list($1, $2); }
	| { $$ = NULL; }
	;

statement
	: expression ';'
	/** if else for switch **/
	;

expression
	: unary_expression
	| unary_expression binary_operator expression
	;

binary_operator
	: ASSIGN
	| MUL_ASSIGN
	| DIV_ASSIGN
	| MOD_ASSIGN
	| ADD_ASSIGN
	| SUB_ASSIGN
	| LEFT_ASSIGN
	| RIGHT_ASSIGN
	| AND_ASSIGN
	| XOR_ASSIGN
	| OR_ASSIGN
	| LOG_OR
	| LOG_AND
	| OR_OP
	| XOR_OP
	| AND_OP
	| EQ_OP
	| NE_OP
	| LT_OP
	| GT_OP
	| LE_OP
	| GE_OP
	| LEFT_OP
	| RIGHT_OP
	| ADD_OP
	| SUB_OP
	| MUL_OP
	| DIV_OP
	| MOD_OP
	;

unary_expression
	: postfix_expression
	| unary_operator unary_expression
	/** sizeof **/
	;

unary_operator
	: ADD_OP
	| SUB_OP
	| NOT_OP
	| LOG_NOT
	| INC_OP
	| DEC_OP
	;

postfix_expression
	: primary_expression
	| postfix_expression postfix_operator
	;

postfix_operator
	: INC_OP
	| DEC_OP
	;

primary_expression
	: IDENTIFIER
	| CONSTANT { $$ = int_literal($1); }
	| STRING_LITERAL { $$ = string_literal($1); }
	| '(' expression ')' { $$ = $2 }
	;

%%


#if 0
assignment
	: IDENTIFIER type_specifier ';' { $$ = assignment($1, $2, 0, NULL); }
	| IDENTIFIER type_specifier assignment_op expression ';' { $$ = assignment($1, $2, $3, $4); }
	| IDENTIFIER assignment_op expression ';' { $$ = assignment($1, NULL, $2, $3); }
	;

assignment_op
	: '=' { $$ = kCluncBinaryOpEquals; }
	;

standard_type_specifier
	: KEY_INT
	| KEY_STRING
	;

const_expression
	: CONSTANT { $$ = int_literal($1); }
	| LITERAL  { $$ = string_literal($1); }
	;

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
