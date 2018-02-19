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
/*message (
	x int = 0,
	s string = "",
)*/

main int {
	x int;
	y int = 1;
	s string = "hello";
	z = 2;
}
// end of file comment
///////////////////////////////
)";
}

int main(
    int argc, char *argv[]
) noexcept {
    (void) argc;
    (void) argv;

    agm::log::init(AGM_TARGET_NAME ".log");

    LOG(kTestString);

    auto cn = clunc_load_string(kTestString);

	LOG("output:");
	std::cout << "/***********************/" << std::endl;
	auto s = clunc_to_string(cn);
	std::cout << s;
	std::cout << "/***********************/" << std::endl;

	//delete cn;

    return 0;
}
