/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

/**
reverse rank order voting.

from fixed data sets for generated data sets.
**/

#include "fixed.h"

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>

int main(
    int argc, char *argv[]
) noexcept {
    (void) argc;
    (void) argv;

    agm::log::init(AGM_TARGET_NAME ".log", false);

    Voting voting;
    voting.run();

    return 0;
}
