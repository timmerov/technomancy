/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

/**
hello world example.
**/

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>

const double kJoint = 0.3213;
const double kNotJoint = 1.0 - kJoint;
const double kPopulation = 209128094.0;
const double kWealth = 123800000000000.0;

void subdivide(
    double population,
    double wealth,
    int depth
) {
    double rich_people = population * kJoint;
    double rich_wealth = wealth * kNotJoint;
    double rich_average = rich_wealth / rich_people;
    double rich_center = rich_people / 2.0;
    double poor_people = population * kNotJoint;
    double poor_wealth = wealth * kJoint;
    double poor_average = poor_wealth / poor_people;
    double poor_center = rich_people + poor_people/2.0;

    --depth;
    if (depth > 0) {
        subdivide(rich_people, rich_wealth, depth);
    } else {
        LOG(rich_center<<" "<<rich_average);
    }
    LOG(poor_center<<" "<<poor_average);
}

int main(
    int argc, char *argv[]
) noexcept {
    (void) argc;
    (void) argv;

    agm::log::init(AGM_TARGET_NAME ".log");

    //LOG("Hello, World!");

    subdivide(kPopulation, kWealth, 16);
    LOG(kPopulation<<" "<<0.0);

    //LOG("Goodbye, World!");

    return 0;
}
