/**
implement clunc programming language.
**/

#include "clunc.h"

#include <aggiornamento/log.h>

#include <iostream>

namespace {
} // namespace

clunc_node *clunc_load_file(
	const char *filename
) noexcept {
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

void clunc_print(
	clunc_node *cn
) noexcept {
	for (; cn; cn = cn->next_) {
		cn->print();
	}
}

clunc_node::clunc_node(
	int what
) noexcept :
	what_(what)
{
}

clunc_node::~clunc_node() noexcept {
	delete next_;
	delete child1_;
	free((void *) token1_);
}

void clunc_node::print() noexcept {
	switch (what_) {
	case kCluncUndefined:
		break;
	case kCluncClassDeclaration:
		std::cout << "class " << token1_ << " {" << std::endl;
		std::cout << "};" << std::endl;
		break;
	}
}

extern "C"
void start(
	clunc_node **proot,
	clunc_node *cn
) {
	LOG("");
	*proot = cn;
}

extern "C"
clunc_node *translation_units(
	clunc_node *head,
	clunc_node *tail
) {
	if (head) {
		//LOG("head");
		head->next_ = tail;
		return head;
	}
	//LOG("tail");
	return tail;
}

extern "C"
clunc_node *class_declaration(
	const char *id,
	clunc_node *fields
) {
	//LOG(id);
	auto cn = new(std::nothrow) clunc_node(kCluncClassDeclaration);
    cn->token1_ = id;
    cn->child1_ = fields;
	return cn;
}
