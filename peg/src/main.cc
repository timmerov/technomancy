/*
Copyright (C) 2012-2017 tim cotter. All rights reserved.
*/

/**
Parsing Expression Grammar (peg) testing.
https://github.com/taocpp/PEGTL
yum install PEGTL-devel.x86_64
**/

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>
#include <aggiornamento/master.h>

#include <pegtl.hh>

namespace {
    class Peg {
    public:
        Peg() = default;
        Peg(const Peg &) = default;
        ~Peg() = default;

        void run() noexcept {
            LOG("Hello, World!");
            LOG("Goodbye, World!");
        }
    };
}

int main(
    int argc, char *argv[]
) noexcept {
    (void) argc;
    (void) argv;

    agm::log::init(AGM_TARGET_NAME ".log");

    Peg peg;
    peg.run();

    return 0;
}
