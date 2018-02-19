/*
lexical analyzer for clunc.
*/

REX_COMMENT1    "//".*[\r\n]
REX_COMMENT2   "/*"([^/]*[^*/][^*]*)*[*]+"/"
REX_WHITE       [ \t\r\n]+
REX_CONSTANT    [0-9]+
REX_IDENTIFIER  [_a-zA-Z][_a-zA-Z0-9]*
REX_LITERAL     ["]([^"]|(\\.))*["]
REX_SYMBOLS     [{}();,]

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
{REX_COMMENT1}      track("//comment",   yytext); /* nothing */
{REX_COMMENT2}      track("/*comment*/", yytext); /* nothing */

    /* white space */
{REX_WHITE}         track(NULL, yytext); /* nothing */

    /* keywords */
"int"               track("keyword:int",    yytext); yylval.i = kCluncKeywordInt;     return KEY_INT;
"string"            track("keyword:string", yytext); yylval.i = kCluncKeywordString;  return KEY_STRING;

	/* numbers tokens strings */
{REX_CONSTANT}      track("integer", yytext); yylval.i = atoi(yytext);     return CONSTANT;
{REX_IDENTIFIER}    track("token",   yytext); yylval.s = strdup(yytext);   return IDENTIFIER;
{REX_LITERAL}       track("string",  yytext); yylval.s = unescape(yytext); return LITERAL;

	/* operators */
"="                 track("=",   yytext); yylval.i = kCluncKeywordAssign;      return ASSIGN;
"*="                track("*=",  yytext); yylval.i = kCluncKeywordMulAssign;   return MUL_ASSIGN;
"/="                track("/=",  yytext); yylval.i = kCluncKeywordDivAssign;   return DIV_ASSIGN;
"%="                track("%=",  yytext); yylval.i = kCluncKeywordModAssign;   return MOD_ASSIGN;
"+="                track("+=",  yytext); yylval.i = kCluncKeywordAddAssign;   return ADD_ASSIGN;
"-="                track("-=",  yytext); yylval.i = kCluncKeywordSubAssign;   return SUB_ASSIGN;
"<<="               track("<<=", yytext); yylval.i = kCluncKeywordLeftAssign;  return LEFT_ASSIGN;
">>="               track(">>=", yytext); yylval.i = kCluncKeywordRightAssign; return RIGHT_ASSIGN;
"&="                track("&=",  yytext); yylval.i = kCluncKeywordAndAssign;   return AND_ASSIGN;
"^="                track("^=",  yytext); yylval.i = kCluncKeywordXorAssign;   return XOR_ASSIGN;
"|="                track("|=",  yytext); yylval.i = kCluncKeywordOrAssign;    return OR_ASSIGN;
"||"                track("||",  yytext); yylval.i = kCluncKeywordLogOr;       return LOG_OR;
"&&"                track("&&",  yytext); yylval.i = kCluncKeywordLogAnd;      return LOG_AND;

    /* symbols */
{REX_SYMBOLS}       track("symbol",  yytext); yylval.i = yytext[0];  return yytext[0];

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

/**
convert '\n' to '\\' 'n'
convert '\t' to 1..4 spaces
**/
char *translate_string(
	char *sin
) {
	char ch;
	char *s;
	char *t;
	int len;
	int i;
	int j;

	len = 1;
	for (s = sin; ch = *s; ++s) {
		switch (ch) {
		case '\t':
			len += 4;
			break;
		case '\n':
			len += 2;
			break;
		default:
			len += 1;
			break;
		}
	}
	t = malloc(len);
	len = 0;
	for (i = 0, j = 0; ; ++i) {
		ch = sin[i];
		t[j] = ch;
		if (ch == 0) {
			break;
		}
		switch (ch) {
		case '\t':
			switch (j % 4) {
			case 0:
				t[j++] = ' ';
			case 3:
				t[j++] = ' ';
			case 2:
				t[j++] = ' ';
			case 1:
				t[j++] = ' ';
			}
			break;
		case '\n':
			t[j++] = '\\';
			t[j++] = 'n';
			break;
		default:
			++j;
			break;
		}
	}
	return t;
}

void track(
	char *cmt,
	char *sin
) {
	g_last_token = sin;
	g_start_line = g_end_line;
	g_start_column = g_end_column;

	char ch;
	char *s;
	for (s = sin; ch = *s; ++s) {
        if (ch == '\n') {
			++g_end_line;
			g_end_column = 0;
        } else {
			++g_end_column;
        }
	}

	if (0 && cmt) {
		char *t;
		t = translate_string(sin);
		fprintf(stderr, "flex start=%d:%d end=%d:%d: %s: %s\n",
			g_start_line, g_start_column, g_end_line, g_end_column, cmt, t);
		free((void *) t);
	}
}
