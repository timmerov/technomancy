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
	kCluncFunction,
	kCluncDeclaration,
	kCluncStatement,
};

struct clunc_node;
typedef struct clunc_node clunc_node;
struct clunc_node {
	clunc_node *next;
	int what;
	clunc_node *child1;
	clunc_node *child2;
	const char *token1;
	const char *token2;
};

/** public api **/
clunc_node *clunc_load_file(const char *filename);
clunc_node *clunc_load_string(const char *str);
void clunc_free(clunc_node *clunc);

/** api for bison and flex **/
int clunc_yylex(void);
void clunc_scan_string(const char *s);

/** internal api **/
clunc_node *clunc_alloc(int what);

#ifdef __cplusplus
} /* extern C */
#endif

#ifdef __cplusplus
/** public api **/
class Clunc {
public:
	Clunc() = default;
	Clunc(const Clunc&) = delete;
	~Clunc() = default;

	std::string proto_fname_;
	std::string inc_fname_;
	std::string header_fname_;
	std::string source_fname_;
	std::string custom_fname_;

	int convert_proto_to_hh_cc() noexcept;
};
#endif
