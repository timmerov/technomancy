/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

/**
process pictures of the moon.
**/

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>


int main(
    int argc, char *argv[]
) noexcept {
    (void) argc;
    (void) argv;

    agm::log::init(AGM_TARGET_NAME ".log");

    LOG("Hello, World!");
    LOG("Goodbye, World!");

    return 0;
}
