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
x int = 0;
y string = 1;

/*message (
	x int = 0,
	s string = "",
)*/

/*main int {
	x int;
	y int = 1;
	s string = "hello";
	z = 2;
}*/
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

    auto cpp = clunc::compile(kTestString);

	LOG("output:");
	std::cout<<"/***********************/"<<std::endl;
	std::cout<<kTestString;
	std::cout<<"/***********************/"<<std::endl;
	std::cout<<cpp;
	std::cout<<"/***********************/"<<std::endl;

    return 0;
}
