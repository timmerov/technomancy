/**
parse protobuf files.
generate c++ classes suitable for a header file.
**/

#pragma once

#ifdef __cplusplus
#include <string>
#endif // __cplusplus


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

enum {
	kCluncUndefined,

	kCluncClassDeclaration,
	kCluncFieldDeclaration,
	kCluncStandardTypeSpecifier,
	kCluncIntLiteral,
	kCluncStringLiteral,
	kCluncFunctionDeclaration,
	kCluncAssignment,

	kCluncKeywordInt,
	kCluncKeywordString,

	kCluncBinaryOpEquals,
};

struct clunc_node;
typedef struct clunc_node clunc_node;

/** public api **/
clunc_node *clunc_load_string(const char *str);

/** api for bison and flex **/
int clunc_yylex(void);
void clunc_scan_string(const char *s);

/** grammar api **/
void start(clunc_node **proot, clunc_node *cn);
clunc_node *build_list(clunc_node *head, clunc_node *tail);
clunc_node *class_declaration(const char *id, clunc_node *fields);
clunc_node *field_declaration(const char *id, clunc_node *type, clunc_node *rhs);
clunc_node *standard_type_specifier(int type);
clunc_node *int_literal(int value);
clunc_node *string_literal(const char *s);
clunc_node *function_declaration(const char *id, clunc_node *type, clunc_node *statements);
clunc_node *assignment(const char *id, clunc_node *type, int op, clunc_node *rhs);

#ifdef __cplusplus
} /* extern C */
#endif

#ifdef __cplusplus
/** public api **/
clunc_node *clunc_load_file(const char *filename) noexcept;
std::string clunc_to_string(clunc_node *cn, int tabs = 0) noexcept;

struct clunc_node {
	clunc_node(int what) noexcept;
	~clunc_node() noexcept;

	clunc_node *next_ = nullptr;
	int what_ = kCluncUndefined;
	clunc_node *child1_ = nullptr;
	clunc_node *child2_ = nullptr;
	const char *token1_ = nullptr;
	int value1_ = 0;

	std::string toString(int tabs = 0) noexcept;
};
#endif
