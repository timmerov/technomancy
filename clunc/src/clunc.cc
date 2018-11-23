/**
compile clunc source files to c++.
**/

#include "clunc.h"

#include <aggiornamento/log.h>
#include <peg/peglib.h>


std::string clunc::compile(
	const std::string& source
) noexcept {
	char grammar[] =
	/** protobuf statements **/
	R"(
		statements <- statement*
		statement <- token token '=' number ';'

		%word <- token / number
		string <- < '"' (!'"' .)* '"' >
		token  <- < [a-zA-Z_][a-zA-Z0-9_]* >
		number <- < [0-9]+ >

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

	std::string str;
	std::shared_ptr<peg::Ast> ast;
	bool result = parser.parse(source.c_str(), ast);
	if (result) {
		ast = peg::AstOptimizer(true).optimize(ast);
		str = peg::ast_to_s(ast);
		LOG("success!");
	} else {
		LOG("syntax error");
		assert(false);
	}

	return std::move(str);
}
