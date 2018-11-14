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
			/**
			peg quick start.

			rule <- first / second / third / fourth
				if the first rule matches, the second will never be tried.
				order is important.

			single and double quotes.
				there's no difference as far as the peg parser is concerned.
				i use single quotes for characters.
				and double quotes for strings.

			rule <- < token >
				the angle brackets indicate leaf surrounded by white space.

			%whitespace
				special rule to define whitespace.
				this example include c++ style comments.

			%word
				it's a bit strange to have to define this.
				it prevents run-on tokens.
					int x = 1; // good
					intx = 1; // parses fine without %word
				strange but true.

			regular expression syntax
				it's regex-like.
				these work as expected: . * + () []
				* is greedy.

			!pattern
				the input stream does not match the pattern.
				does not consume the input.
				hence this construct for quoted strings:
					string <- '"' (!["] .)* '"'
			**/
            peg::parser parser(R"grammar(
                statements <- statement*
                statement  <-
                    "syntax" '=' string ';' /
                    "import" string ';' /
                    "package" token ';' /
                    message_statement /
                    enum_statement

                message_statement <- "message" token '{' field* '}'
                field <-
                    type_decl /
                    "repeated" type token '=' number ';' /
                    "oneof" token '{' type_decl* '}' /
                    "map" '<' type ',' type '>' token '=' number ';' /
                    message_statement /
                    enum_statement

                type_decl <- type token '=' number ';'
                type <- token ('.' token)*

                enum_statement <- "enum" token '{' enum_decl* '}'
                enum_decl <- token '=' number ';'

                %word <- string / token / number
                string <- < '"' (!["] .)* '"' >
                token  <- < [a-zA-Z_][a-zA-Z0-9_]* >
                number <- < [0-9]+ >

                %whitespace <- ([ \t\r\n] / comment)*
                comment <-
					"//" (!end_of_comment .)* end_of_comment /
					"/*" (!"*/" .)* "*/"
				end_of_comment <- end_of_line / end_of_file
				end_of_line <- '\r\n' / '\r' / '\n'
				end_of_file <- !.
            )grammar");

            parser["statements"] = nop;
            parser["statement"] = nop;
            parser["message_statement"] = nop;
            parser["field"] = nop;
            parser["type_decl"] = nop;
            parser["type"] = nop;
            parser["enum_statement"] = nop;
            parser["enum_decl"] = nop;
            parser["string"] = nop;
            parser["token"] = nop;
            parser["number"] = nop;

            const char expr[] = R"expr(
				// comment one
				/* comment two */
				/*
				multi-line comment
				*/
				/** tricky comment **/
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
            // comment at end of file)expr";

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
