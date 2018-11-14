/*
Copyright (C) 2012-2017 tim cotter. All rights reserved.
*/

/**
Parsing Expression Grammar (peg) testing.
https://github.com/yhirose/cpp-peglib
**/

#include "peglib.h"

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>

#include <cstdlib>


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

			pattern matching
				it's regex-like.
				these work as expected: . * + () []
				* is greedy.

			!pattern
				the input stream does not match the pattern.
				does not consume the input.
				hence this construct for quoted strings:
					string <- '"' (!'"' .)* '"'

			stupidity alert
				the ast method does not seem to record which rule was matched.
				the workaround is to define explicit rules for every sub-rule.
				after optimize(), the name of the rule may be in ast.original_name.
				seems to be the case when the sub-rule has one token.
			**/
			char grammar[] =
			/** protobuf statements **/
			R"(
				statements <- (
					syntax_statement /
					import_statement /
					package_statement /
					enum_statement /
					message_statement
				)*
				syntax_statement <- "syntax" '=' string ';'
				import_statement <- "import" string ';'
				package_statement <- "package" token ';'
			)"
			/** protobuf enum **/
			R"(
				enum_statement <- "enum" token '{' enum_decl* '}'
				enum_decl <- token '=' number ';'
			)"
			/** protobuf message **/
			R"(
				message_statement <- "message" token '{' field* '}'
				field <-
					type_decl /
					repeated_decl /
					oneof_decl /
					map_decl /
					message_statement /
					enum_statement

				type_decl <- type token '=' number ';'
				type <- token ('.' token)*

				repeated_decl <- "repeated" type token '=' number ';'
				oneof_decl <- "oneof" token '{' type_decl* '}'
				map_decl <- "map" '<' type ',' type '>' token '=' number ';'

			)"
			/** terminating symbols **/
			R"(
				%word <- token / number
				string <- < '"' (!'"' .)* '"' >
				token  <- < [a-zA-Z_][a-zA-Z0-9_]* >
				number <- < [0-9]+ >
			)"
			/** fold comments into whitespace **/
			R"(
				%whitespace <- (' ' / '\t' / end_of_line / comment)*
				comment <-
					"//" (!end_of_comment .)* end_of_comment /
					"/*" (!"*/" .)* "*/"
				end_of_comment <- end_of_line / end_of_file
				end_of_line <- '\r\n' / '\r' / '\n'
				end_of_file <- !.
			)";

			peg::parser parser(grammar);
			parser.enable_ast();

			const char proto[] = R"(
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
				// comment at end of file)";

			std::shared_ptr<peg::Ast> ast;
			bool result = parser.parse(proto, ast);
			if (result) {
				ast = peg::AstOptimizer(true).optimize(ast);
				auto s = peg::ast_to_s(ast);
				LOG(s);
				compile(*ast);
				LOG("success!");
			} else {
				LOG("syntax error");
			}
		}

		void compile(
			const peg::Ast& ast,
			int indent = 0
		) noexcept {
			std::string spaces(indent, ' ');
			indent += 2;

			std::cout<<spaces<<"name="<<ast.name<<" org="<<ast.original_name<<" token="<<ast.token<<std::endl;
			for (auto node : ast.nodes) {
				compile(*node, indent);
			}
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
