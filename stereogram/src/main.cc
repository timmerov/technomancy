/*
Copyright (C) 2012-2016 tim cotter. All rights reserved.
*/

/**
stereogram example.
**/

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>


int main(
    int argc, char *argv[]
) throw() {
    (void) argc;
    (void) argv;

    agm::log::init(AGM_TARGET_NAME ".log");

    LOG("Hello, World!");
    LOG("Goodbye, World!");

    return 0;
}
