/**
implement clunc programming language.
**/

#include "clunc.h"

#include <iostream>

namespace {
} // namespace

extern "C"
clunc_node *clunc_alloc(
	int what
) {
	clunc_node *cn = (clunc_node *) calloc(1, sizeof(clunc_node));
	cn->next = nullptr;
	cn->what = what;
	cn->type1 = nullptr;  /// type, vector, map
	cn->type2 = nullptr;  /// map
	cn->name = nullptr;   /// include, namespace, class, enum, type, vector, map, union
	cn->value = nullptr;  /// enum
	cn->list = nullptr;   /// class, enum, union
	return cn;
}

extern "C"
void clunc_free(
	clunc_node *cn
) {
	if (cn == nullptr) {
		return;
	}

	clunc_free(cn->next);
	free((void *) cn->type1);
	free((void *) cn->type2);
	free((void *) cn->name);
	free((void *) cn->value);
	clunc_free(cn->list);
	free(cn);
}

extern "C"
clunc_node *clunc_load_file(const char *filename) {
	FILE *src_file = fopen(filename, "r");
	if (src_file == nullptr) {
		exit(1);
	}

	fseek(src_file, 0, SEEK_END);
	int size = ftell(src_file);

	char *src_ptr = (char *) calloc(1, size+1);
	if (src_ptr == nullptr) {
		exit(2);
	}

	fseek(src_file, 0, SEEK_SET);
	int bytes_read = fread(src_ptr, 1, size, src_file);
	if (bytes_read != size) {
		exit(3);
	}

	clunc_node *cn = clunc_load_string(src_ptr);

	free(src_ptr);
	fclose(src_file);

	return cn;
}
