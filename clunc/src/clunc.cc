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
	free((void *) token2_);
}

void clunc_node::print() noexcept {
	switch (what_) {
	case kCluncUndefined:
		break;
	case kCluncClassDeclaration:
		std::cout << "class " << token1_ << " {" << std::endl;
		clunc_print(child1_);
		std::cout << "};" << std::endl;
		break;
	case kCluncFieldDeclaration:
		std::cout << "    ";
		child1_->print();
		std::cout << " " << token1_;
		if (token2_) {
			std::cout << " = ";
			if (child1_->what_ == kCluncStandardTypeSpecifier
			&& child1_->value1_ == kCluncKeywordString) {
				std::cout << "\"" << token2_ << "\"";
			} else {
				std::cout << token2_;
			}
		}
		std::cout << ";" << std::endl;
		break;
	case kCluncStandardTypeSpecifier:
		switch (value1_) {
		case kCluncKeywordInt:
			std::cout << "int";
			break;
		case kCluncKeywordString:
			std::cout << "std::string";
			break;
		}
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
clunc_node *build_list(
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

extern "C"
clunc_node *field_declaration(
	const char *id,
	clunc_node *type,
	const char *value
) {
	//LOG(id);
	auto cn = new(std::nothrow) clunc_node(kCluncFieldDeclaration);
    cn->token1_ = id;
    cn->child1_ = type;
    cn->token2_ = value;
	return cn;
}

extern "C"
clunc_node *standard_type_specifier(
	int type
) {
	//LOG(id);
	auto cn = new(std::nothrow) clunc_node(kCluncStandardTypeSpecifier);
	cn->value1_ = type;
	return cn;
}
