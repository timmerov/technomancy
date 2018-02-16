/*
lexical analyzer for clunc.
*/

%option noinput nounput noyywrap
%option prefix="clunc_yy"

%{
#include "clunc.h"

//#define TSC_PRINT fprintf
#if !defined(TSC_PRINT)
#define TSC_PRINT(...)
#endif

#include "clunc.tab.h"
static char *unescape(char *s);
#define yylval clunc_yylval
%}

%%
    /* comments */
"//".*[\r\n]                   TSC_PRINT(stdout, "flex comment '%s'\n", yytext);  /* nothing */
"/*"([^/]*[^*/][^*]*)*[*]+"/"  TSC_PRINT(stdout, "flex comment '%s'\n", yytext);  /* nothing */

    /* white space */
[ \t\r\n]+              TSC_PRINT(stdout, "flex whitespace '%s'\n", yytext);  /* nothing */

    /* keywords */
"syntax"                TSC_PRINT(stdout, "flex keyword '%s'\n", yytext);  return SYNTAX;
"import"                TSC_PRINT(stdout, "flex keyword '%s'\n", yytext);  return IMPORT;
"package"               TSC_PRINT(stdout, "flex keyword '%s'\n", yytext);  return PACKAGE;
"message"               TSC_PRINT(stdout, "flex keyword '%s'\n", yytext);  return MESSAGE;
"enum"                  TSC_PRINT(stdout, "flex keyword '%s'\n", yytext);  return ENUM;
"repeated"              TSC_PRINT(stdout, "flex keyword '%s'\n", yytext);  return REPEATED;
"oneof"                 TSC_PRINT(stdout, "flex keyword '%s'\n", yytext);  return ONEOF;
"map"                   TSC_PRINT(stdout, "flex keyword '%s'\n", yytext);  return MAP;

	/* number */
[0-9]+                  TSC_PRINT(stdout, "flex number '%s'\n", yytext);   yylval.s = strdup(yytext);    return NUMBER;

    /* tokens */
[_a-zA-Z][_a-zA-Z0-9]*  TSC_PRINT(stdout, "flex token '%s'\n", yytext);    yylval.s = strdup(yytext);    return TOKEN;

    /* strings */
["]([^"]|(\\.))*["]     TSC_PRINT(stdout, "flex string '%s'\n", yytext);   yylval.s = unescape(yytext);  return STRING;
[']([^']|(\\.))*[']     TSC_PRINT(stdout, "flex string '%s'\n", yytext);   yylval.s = unescape(yytext);  return STRING;

    /* symbols */
[;{}=<>,.]              TSC_PRINT(stdout, "flex symbol '%s'\n", yytext);                                 return yytext[0];

%%

void clunc_scan_string(const char *s) {
	clunc_yy_scan_string(s);
}

/*
remove bounding quotes.
replace \-character with the special character.
convert "hello\n" ie '"' 'h' 'e' 'l' 'l' 'o' '\' 'n' '"' '\0'
to "hello\n" ie 'h' 'e' 'l' 'l' 'o' '\n' '\0'
*/
static char *unescape(char *sin) {
	int i, j, c;
	char quote;
	char *s;

	s = NULL;
	quote = sin[0];
	if (quote == '\'' || quote == '"') {
		int last;
		last = strlen(sin) - 1;
		if (sin[last] == quote) {
			s = strdup(sin+1);
			s[last-1] = 0;
		}
	}
	if (s == NULL) {
		s = strdup(sin);
	}

	if (s[0] == '"' && s[strlen(s)-1] == '"') {
		++s;
		s = strdup(s);
		s[strlen(s)-1] = 0;
	} else {
		s = strdup(s);
	}

	for (i=0, j=0; (c=s[i]); ++i, ++j) {
		if (c == '\\') {
			switch ((c = s[++i])) {
			case 't':  c = '\t'; break;
			case 'r':  c = '\r'; break;
			case 'n':  c = '\n'; break;
			case '\\': c = '\\'; break;
			case 'f':  c = '\f'; break;
			case 'v':  c = '\v'; break;
			case 'a':  c = '\a'; break;
			case '"':  c = '\"'; break;
			case '\'':  c = '\''; break;
			}
		}
		s[j] = c;
	}
	s[j] = 0;

	return s;
}
