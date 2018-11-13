/*
Copyright (C) 2012-2017 tim cotter. All rights reserved.
*/

/**
Parsing Expression Grammar (peg) testing.
https://github.com/yhirose/cpp-peglib
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
            peg::parser parser(R"grammar(
                statements <- statement*
                statement  <-
                    "syntax" '=' STRING ';' /
                    "import" STRING ';' /
                    "package" TOKEN ';' /
                    message_statement /
                    enum_statement

                message_statement <- "message" TOKEN '{' field* '}'
                field <-
                    type_decl /
                    "repeated" type TOKEN '=' NUMBER ';' /
                    "oneof" TOKEN '{' type_decl* '}' /
                    "map" '<' type ',' type '>' TOKEN '=' NUMBER ';' /
                    message_statement /
                    enum_statement

                type_decl <- type TOKEN '=' NUMBER ';'
                type <- TOKEN ('.' TOKEN)*

                enum_statement <- "enum" TOKEN '{' enum_decl* '}'
                enum_decl <- TOKEN '=' NUMBER ';'

                STRING <- < '"' (!["] .)* '"' >
                TOKEN  <- < [a-zA-Z_][a-zA-Z0-9_]* >
                NUMBER <- < [0-9]+ >

                %whitespace <- [ \t\r\n]*
            )grammar");

            parser["statements"] = nop;
            parser["statement"] = nop;
            parser["message_statement"] = nop;
            parser["field"] = nop;
            parser["type_decl"] = nop;
            parser["type"] = nop;
            parser["enum_statement"] = nop;
            parser["enum_decl"] = nop;
            parser["STRING"] = nop;
            parser["TOKEN"] = nop;
            parser["NUMBER"] = nop;

            const char expr[] = R"expr(
                syntax = "proto3";
                import "path/to/some/file";
                package cerebras;
                message CigarConfig {
                    int x = 1;
                    double y = 2;
                    google.protobuf.any z = 3;
                    repeated int.int.int a = 4;
                    oneof fred {
                        int b = 5;
                        double c = 6;
                    }
                    map<string, string> d = 7;
                    message sub {
                        int x = 8;
                    }
                    enum Wilma {
                        f = 0;
                        g = 1;
                    }
                }
                enum Fred {
                    d = 0;
                    e = 1;
                }
            )expr";

            std::string val;
            bool result = parser.parse(expr, val);
            if (result) {
                LOG("success!");
                LOG("val="<<val);
            } else {
                LOG("syntax error");
            }
        }

        static std::string nop(
            const peg::SemanticValues& sv
        ) {
            (void) sv;
            std::string str("w00t!");
            return std::move(str);
        }

        static char toChar(
            const peg::SemanticValues& sv
        ) {
            return sv.str()[0];
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
