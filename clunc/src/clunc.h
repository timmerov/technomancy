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
clunc_node *translation_units(clunc_node *head, clunc_node *tail);
clunc_node *class_declaration(const char *id, clunc_node *fields);

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

	void print() noexcept;
};
#endif
