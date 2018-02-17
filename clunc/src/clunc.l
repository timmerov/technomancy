/*
lexical analyzer for clunc.
*/

REX_SL_SL_CMT   "//".*[\r\n]
REX_SL_ST_CMT   "/*"([^/]*[^*/][^*]*)*[*]+"/"
REX_WHITE       [ \t\r\n]+
REX_INTEGER     [0-9]+
REX_TOKEN       [_a-zA-Z][_a-zA-Z0-9]*
REX_STRING      ["]([^"]|(\\.))*["]
REX_SYMBOLS     [{}=;]

%option noinput nounput noyywrap
%option prefix="clunc_yy"

%{
#include "clunc.h"

#include "clunc.tab.h"
static char *unescape(char *s);
void track(char *cmt, char *s);
#define yylval clunc_yylval
%}

%%
    /* comments */
{REX_SL_SL_CMT}     track("//comment",   yytext); /* nothing */
{REX_SL_ST_CMT}     track("/*comment*/", yytext); /* nothing */

    /* white space */
{REX_WHITE}         track("whitespace", yytext); /* nothing */

    /* keywords */
"int"               track("kw:int",    yytext); return KEY_INT;
"string"            track("kw:string", yytext); return KEY_STRING;

	/* number */
{REX_INTEGER}       track("integer", yytext); yylval.s = strdup(yytext); return NUMBER;

    /* tokens */
{REX_TOKEN}         track("token",   yytext); yylval.s = strdup(yytext); return TOKEN;

    /* strings */
{REX_STRING}        track("string",  yytext); yylval.s = unescape(yytext); return STRING;

    /* symbols */
{REX_SYMBOLS}       track("symbol",  yytext); return yytext[0];

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
			case 't':   c = '\t';  break;  // tab
			case 'r':   c = '\r';  break;  // carriage return
			case 'n':   c = '\n';  break;  // new line
			case '\\':  c = '\\';  break;  // backslash
			case '"':   c = '\"';  break;  // double quote
			}
		}
		s[j] = c;
	}
	s[j] = 0;

	return s;
}

int g_start_line = 0;
int g_start_column = 0;
int g_end_line = 0;
int g_end_column = 0;
char *g_last_token = "";

void track(
	char *cmt,
	char *sin
) {
	g_last_token = sin;
	g_start_line = g_end_line;
	g_start_column = g_end_column;

	char ch;
	char *s = sin;
	while (ch = *s++) {
        if (ch == '\n') {
			++g_end_line;
			g_end_column = 0;
        } else {
			++g_end_column;
        }
	}

	/**/
	fprintf(stderr, "flex start=%d:%d end=%d:%d: %s: %s\n",
		g_start_line, g_start_column, g_end_line, g_end_column, cmt, sin);
	/**/
}
