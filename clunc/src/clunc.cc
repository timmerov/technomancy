/**
implement clunc programming language.
**/

#include "clunc.h"

#include <aggiornamento/log.h>

#include <iostream>

namespace {
	std::string toSpaces(
		int tabs
	) noexcept {
		std::string s(2*tabs, ' ');
		return std::move(s);
	}
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

std::string clunc_to_string(
	clunc_node *cn,
	int tabs
) noexcept {
	std::string s;
	for (; cn; cn = cn->next_) {
		s += cn->toString(tabs);
	}
	return std::move(s);
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
	delete child2_;
	free((void *) token1_);
}

std::string clunc_node::toString(
	int tabs
) noexcept {
	std::string s;
	switch (what_) {
	case kCluncUndefined:
		break;

	case kCluncClassDeclaration:
		s += toSpaces(tabs);
		s += "class ";
		s += token1_;
		s += " {\n";
		s += clunc_to_string(child1_, tabs+1);
		s += toSpaces(tabs);
		s += "};\n";
		break;

	case kCluncFieldDeclaration:
		s += toSpaces(tabs);
		s += child1_->toString();
		s += " ";
		s += token1_;
		if (child2_) {
			s += " = ";
			s += child2_->toString();
		}
		s += ";\n";
		break;

	case kCluncStandardTypeSpecifier:
		switch (value1_) {
		case kCluncKeywordInt:
			s = "int";
			break;
		case kCluncKeywordString:
			s = "std::string";
			break;
		}
		break;

	case kCluncIntLiteral:
		s += std::to_string(value1_);
		break;

	case kCluncStringLiteral:
		s += "\"";
		/// tk tsc todo: escape tabs and crs.
		s += token1_;
		s += "\"";
		break;
	}
	return std::move(s);
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
	clunc_node *rhs
) {
	//LOG(id);
	auto cn = new(std::nothrow) clunc_node(kCluncFieldDeclaration);
    cn->token1_ = id;
    cn->child1_ = type;
    cn->child2_ = rhs;
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

extern "C"
clunc_node *int_literal(
	int value
) {
	//LOG(id);
	auto cn = new(std::nothrow) clunc_node(kCluncIntLiteral);
	cn->value1_ = value;
	return cn;
}

extern "C"
clunc_node *string_literal(
	const char *str
) {
	//LOG(id);
	auto cn = new(std::nothrow) clunc_node(kCluncStringLiteral);
	cn->token1_ = str;
	return cn;
}
