/*
grammar for clunc.
*/

%defines
%define api.prefix {clunc_yy}
%parse-param { clunc_node **pb }
%debug

%{
#include "clunc.h"

#include <stdlib.h>
#include <string.h>

//#define TSC_PRINT fprintf
#if !defined(TSC_PRINT)
#define TSC_PRINT(...)
#endif

static void clunc_yyerror(clunc_node **pb, const char *s) ;
#define ALIGN 64
%}

%union {
	const char *s;
	clunc_node *pb;
}

%token <s> SYNTAX
%token <s> IMPORT
%token <s> PACKAGE
%token <s> MESSAGE
%token <s> ENUM
%token <s> REPEATED
%token <s> ONEOF
%token <s> MAP

%token <s> NUMBER
%token <s> STRING
%token <s> TOKEN

%type <pb> statements
%type <pb> statement
%type <pb> message_statement
%type <pb> field_list
%type <pb> field_decl
%type <pb> type_decl
%type <s>  type
%type <pb> type_list
%type <pb> enum_list
%type <pb> enum_decl

%%

start :
	statements {
		TSC_PRINT(stdout, "bison start statements\n");
		*pb = $1;
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
	}

statement :
	SYNTAX '=' STRING ';' {
        TSC_PRINT(stdout, "bison statement SYNTAX = STRING ;\n");
        $$ = NULL;
	} |
	IMPORT STRING ';' {
        TSC_PRINT(stdout, "bison statement IMPORT STRING ;\n");
        $$ = clunc_alloc(kCluncInclude);
		$$->name = $2;
	} |
	PACKAGE TOKEN ';' {
        TSC_PRINT(stdout, "bison statement PACKAGE TOKEN ;\n");
        $$ = clunc_alloc(kCluncNamespace);
		$$->name = $2;
	} |
	message_statement {
        TSC_PRINT(stdout, "bison statement message_statement\n");
        $$ = $1;
	} |
	ENUM TOKEN '{' enum_list '}' {
        TSC_PRINT(stdout, "bison statement ENUM TOKEN { enum_list }\n");
        $$ = clunc_alloc(kCluncEnum);
		$$->name = $2;
        $$->list = $4;
	} ;

message_statement :
	MESSAGE TOKEN '{' field_list '}' {
        TSC_PRINT(stdout, "bison statement MESSAGE TOKEN { field_list }\n");
        $$ = clunc_alloc(kCluncClass);
		$$->name = $2;
        $$->list = $4;
	} ;

field_list :
	field_decl field_list {
        TSC_PRINT(stdout, "bison field_list field_decl field_list\n");
        $$ = $1;
        $$->next= $2;
	} | {
        TSC_PRINT(stdout, "bison field_list\n");
        $$ = NULL;
	}

field_decl :
	type_decl {
        TSC_PRINT(stdout, "bison field_decl type_decl ;\n");
        $$ = $1;
	} |
	REPEATED type TOKEN '=' NUMBER ';' {
        TSC_PRINT(stdout, "bison field_decl REPEATED type TOKEN = NUMBER ;\n");
        $$ = clunc_alloc(kCluncVector);
        $$->type1 = $2;
        $$->name = $3;
	} |
	ONEOF TOKEN '{' type_list '}' {
        TSC_PRINT(stdout, "bison field_decl ONEOF TOKEN { type_list }\n");
        $$ = clunc_alloc(kCluncUnion);
        $$->name = $2;
        $$->list = $4;
	} |
	message_statement {
        TSC_PRINT(stdout, "bison field_decl message_statement\n");
        $$ = $1;
	} |
	MAP '<' type ',' type '>' TOKEN '=' NUMBER ';' {
        TSC_PRINT(stdout, "bison field_decl MAP < type , type > TOKEN = NUMBER ;\n");
        $$ = clunc_alloc(kCluncMap);
        $$->type1 = $3;
        $$->type2 = $5;
        $$->name  = $7;
	} ;

type_list :
	type_decl type_list {
        TSC_PRINT(stdout, "bison type_list enum_decl type_list\n");
        $$ = $1;
        $$->next = $2;
	} | {
        TSC_PRINT(stdout, "bison type_list\n");
        $$ = NULL;
	}

type_decl :
	type TOKEN '=' NUMBER ';' {
        TSC_PRINT(stdout, "bison type_decl type TOKEN = NUMBER ; 2\n");
        $$ = clunc_alloc(kCluncType);
        $$->type1 = $1;
        $$->name  = $2;
	} ;

type :
	TOKEN '.' type {
        TSC_PRINT(stdout, "bison type TOKEN '.' type\n");
		int len;
		len = strlen($1) + 1 + strlen($3) + 1;
		$$ = calloc(1, len);
		sprintf((char *) $$, "%s.%s", $1, $3);
		free((void *) $1);
		free((void *) $3);
	} |
    TOKEN {
        TSC_PRINT(stdout, "bison type TOKEN\n");
        $$ = $1;
	} ;

enum_list :
	enum_decl enum_list {
        TSC_PRINT(stdout, "bison enum_list enum_decl enum_list 2\n");
        $$ = $1;
        $$->next = $2;
	} | {
        TSC_PRINT(stdout, "bison enum_list\n");
        $$ = NULL;
	}

enum_decl :
	TOKEN '=' NUMBER ';' {
        TSC_PRINT(stdout, "bison enum_decl TOKEN = NUMBER ;\n");
        $$ = clunc_alloc(kCluncValue);
        $$->name = $1;
        $$->value = $3;
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
