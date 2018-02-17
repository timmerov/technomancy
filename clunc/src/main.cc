/*
Copyright (C) 2018 tim cotter. All rights reserved.
*/

/**
**/

#include "clunc.h"

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>

namespace {
static const char kTestString[] = R"(
///////////////////////////////
/**
clunc test sequences
**/
main int {
	x int = 1;
	s string = "hello";
	y int = x;
}
// end of file comment
///////////////////////////////
)";

	std::string cluncToString(
		clunc_node *cn
	) noexcept {
		std::string s;
        if (cn == nullptr) {
			return s;
		}
		switch (cn->what) {
		case kCluncFunction:
			s += "[Fn]:\n";
			s += cluncToString(cn->child1);
			s += " {\n";
			for (auto it = cn->child2; it; it = it->next) {
				s += cluncToString(it);
			}
			s += "}\n";
			break;
		case kCluncDeclaration:
			s += cn->token1;
			s += " ";
			s += cn->token2;
			break;
		case kCluncStatement:
			s += "\t";
			s += cluncToString(cn->child1);
			s += " = ";
			s += cn->token1;
			s += ";\n";
			LOG(s);
			break;
		}
		return s;
	}
}

int main(
    int argc, char *argv[]
) noexcept {
    (void) argc;
    (void) argv;

    agm::log::init(AGM_TARGET_NAME ".log");

    LOG(kTestString);

    auto cn = clunc_load_string(kTestString);

	for (auto it = cn; it; it = it->next) {
		auto str = cluncToString(it);
		LOG(str);
	}

	clunc_free(cn);

    return 0;
}
