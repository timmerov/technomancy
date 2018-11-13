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

#include "peglib.h"

namespace {
    class Peg {
    public:
        Peg() = default;
        Peg(const Peg &) = default;
        ~Peg() = default;

        void run() noexcept {
            peg::parser parser(R"(
                EXPRESSION       <-  TERM (TERM_OPERATOR TERM)*
                TERM             <-  FACTOR (FACTOR_OPERATOR FACTOR)*
                FACTOR           <-  NUMBER / '(' EXPRESSION ')'

                TERM_OPERATOR    <-  < [-+] >
                FACTOR_OPERATOR  <-  < [/*] >
                NUMBER           <-  < [0-9]+ >

                %whitespace      <-  [ \t\r\n]*
            )");

            parser["EXPRESSION"]      = reduce;
            parser["TERM"]            = reduce;
            parser["TERM_OPERATOR"]   = toChar;
            parser["FACTOR_OPERATOR"] = toChar;
            parser["NUMBER"]          = toInt;

            const char expr[] = "1+2*3";
            int val = 0;
            bool result = parser.parse(expr, val);
            if (result) {
                LOG(expr<<" = "<<val);
            } else {
                LOG("syntax error");
            }
        }

        static char toChar(
            const peg::SemanticValues& sv
        ) {
            return static_cast<char>(*sv.c_str());
        }

        static int toInt(
            const peg::SemanticValues& sv
        ) {
            return atoi(sv.c_str());
        }

        static int reduce(
            const peg::SemanticValues& sv
        ) {
            auto result = sv[0].get<int>();
            int nsvs = sv.size();
            for (int i = 1; i < nsvs; i += 2) {
                char oper = sv[i].get<char>();
                int num = sv[i + 1].get<int>();
                switch (oper) {
                case '+':
                    result += num;
                    break;

                case '-':
                    result -= num;
                    break;

                case '*':
                    result *= num;
                    break;

                case '/':
                    result /= num;
                    break;
                }
            }
            return result;
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
