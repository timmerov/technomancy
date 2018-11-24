/**
compile clunc source files to c++.
**/

#include "clunc.h"

#include <aggiornamento/log.h>
#include <peg/peglib.h>

#include <sstream>


namespace {
	void dumpAst(
		std::stringstream& ss,
		const peg::Ast& ast,
		int indent
	) noexcept {
		std::string spaces(indent, ' ');
		indent += 2;

		ss<<spaces
			<<"name="<<ast.name<<"/"<<ast.original_name
			<<" choice="<<ast.choice<<"/"<<ast.original_choice
			<<" token="<<ast.token<<std::endl;

		for (auto&& node_sp : ast.nodes) {
			auto& node = *node_sp;
			dumpAst(ss, node, indent);
		}
	}
}

std::string clunc::compile(
	const std::string& source
) noexcept {
	char grammar[] =
	/** protobuf statements **/
	R"(
		statements <- statement*
		statement <-
			class_statement /
			function_statement

		class_statement <-
			token '(' ')' ';' /
			token '(' class_declaration* ')' ';'
		class_declaration <-
			token '=' value ';' /     # x = 1;
			token token '=' value ';' # x int = 2;
		value <- number / string

		function_statement <- token token '{' action* '}'

		action <-
			token token ';' /                 # x int;
			token token '=' expression ';' /  # x int = 1;
			token '=' expression ';'          # x = 1;

		expression <- value

		%word <- token / number
		string <- '"' < (!'"' .)* > '"'
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

	peg::parser parser;
	try {
		parser.load_grammar(grammar);
	} catch (...) {
		LOG("=ERR= failed to parse grammar.");
		assert(false);
	}
	parser.enable_ast();

	std::shared_ptr<peg::Ast> ast;
	bool result = parser.parse(source.c_str(), ast);
	if (result == false) {
		LOG("syntax error");
		assert(false);
	}
	//ast = peg::AstOptimizer(true).optimize(ast);

	std::stringstream ss;
	if (ast) {
		dumpAst(ss, *ast, 0);
	}

	return std::move(ss.str());
}
