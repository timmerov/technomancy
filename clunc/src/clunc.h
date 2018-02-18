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

	kCluncKeywordInt,
	kCluncKeywordString,
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
clunc_node *field_declaration(const char *id, clunc_node *type, const char *value);
clunc_node *standard_type_specifier(int);

#ifdef __cplusplus
} /* extern C */
#endif

#ifdef __cplusplus
/** public api **/
clunc_node *clunc_load_file(const char *filename) noexcept;
void clunc_print(clunc_node *cn) noexcept;

struct clunc_node {
	clunc_node(int what) noexcept;
	~clunc_node() noexcept;

	clunc_node *next_ = nullptr;
	int what_ = kCluncUndefined;
	clunc_node *child1_ = nullptr;
	const char *token1_ = nullptr;
	const char *token2_ = nullptr;
	int value1_ = 0;

	void print() noexcept;
};
#endif
