/*
		This file is part of solidity.

		solidity is free software: you can redistribute it and/or modify
		it under the terms of the GNU General Public License as published by
		the Free Software Foundation, either version 3 of the License, or
		(at your option) any later version.

		solidity is distributed in the hope that it will be useful,
		but WITHOUT ANY WARRANTY; without even the implied warranty of
		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
		GNU General Public License for more details.

		You should have received a copy of the GNU General Public License
		along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/
// SPDX-License-Identifier: GPL-3.0
/**
 * @author Christian <c@ethdev.com>
 * @date 2016
 * Solidity inline assembly parser.
 */

#include <range/v3/view/subrange.hpp>
namespace ranges_NOSTD = ranges;
#include <liblangutil/ErrorReporter.h>
#include <liblangutil/Exceptions.h>
#include <liblangutil/Scanner.h>
#include <libsolutil/Common.h>
#include <libsolutil/Visitor.h>
#include <libyul/AST.h>
#include <libyul/Exceptions.h>


#include <boost/algorithm/string.hpp>

#include "AsmParser.h"
#include <algorithm>
#include <intx/intx.hpp>
#include <regex>

using namespace std;
using namespace solidity;
using namespace solidity::util;
using namespace solidity::langutil;
using namespace solidity::yul;
using namespace warp;
using namespace intx;

const uint128 MAX_128 = ~uint128(0);

namespace warp
{
[[nodiscard]] shared_ptr<DebugData const> updateLocationEndFrom(
	shared_ptr<DebugData const> const& _debugData,
	langutil::SourceLocation const& _location)
{
	SourceLocation updatedLocation = _debugData->location;
	updatedLocation.end = _location.end;
	return make_shared<DebugData const>(updatedLocation);
}

optional<int> toInt(string const& _value)
{
	try
	{
		return stoi(_value);
	}
	catch (...)
	{
		return nullopt;
	}
}

string toUint256(string _literal)
{
	uint256 argVal = from_string<uint256>(_literal.c_str());
	div_result res = udivrem(argVal, MAX_128);
	auto low = to_string(res.rem) + ", ";
	auto high = to_string(res.quot) + ")";
	return "Uint256(" + low + high;
}

} // namespace

std::shared_ptr<DebugData const> warp::Parser::createDebugData() const
{
	switch (m_useSourceLocationFrom)
	{
	case UseSourceLocationFrom::Scanner:
		return DebugData::create(ParserBase::currentLocation());
	case UseSourceLocationFrom::LocationOverride:
		return DebugData::create(m_locationOverride);
	case UseSourceLocationFrom::Comments:
		return m_debugDataOverride;
	}
	solAssert(false, "");
}

unique_ptr<Block> warp::Parser::parse(CharStream& _charStream, string& ret)
{
	m_scanner = make_shared<Scanner>(_charStream);
	unique_ptr<Block> block;
	string cairo;
	std::tie(block, cairo) = parseInline(m_scanner, ret);
	expectToken(Token::EOS);
	return block;
}

tuple<unique_ptr<Block>, string>
warp::Parser::parseInline(std::shared_ptr<Scanner> const& _scanner, string& ret)
{
	m_recursionDepth = 0;

	_scanner->setScannerMode(ScannerKind::Yul);
	ScopeGuard resetScanner([&] { _scanner->setScannerMode(ScannerKind::Solidity); });

	try
	{
		m_scanner = _scanner;
		if (m_sourceNames)
			fetchSourceLocationFromComment();
		return tuple<
			unique_ptr<Block>,
			string>(make_unique<Block>(parseBlock(ret)), this->m_cairo_str);
	}
	catch (FatalError const&)
	{
		yulAssert(
			!m_errorReporter.errors().empty(),
			"Fatal error detected, but no error is reported.");
	}
	return tuple<unique_ptr<Block>, string>(nullptr, "");
}
void Parser::fetchSourceLocationFromComment()
{
	solAssert(m_sourceNames.has_value(), "");

	if (m_scanner->currentCommentLiteral().empty())
		return;

	static regex const lineRE = std::regex(
		R"~~~((^|\s*)@src\s+(-1|\d+):(-1|\d+):(-1|\d+)(\s+|$))~~~",
		std::regex_constants::ECMAScript | std::regex_constants::optimize);

	string const text = m_scanner->currentCommentLiteral();
	auto from = sregex_iterator(text.begin(), text.end(), lineRE);
	auto to = sregex_iterator();

	for (auto const& matchResult: ranges_NOSTD::make_subrange(from, to))
	{
		solAssert(matchResult.size() == 6, "");

		auto const sourceIndex = toInt(matchResult[2].str());
		auto const start = toInt(matchResult[3].str());
		auto const end = toInt(matchResult[4].str());

		auto const commentLocation = m_scanner->currentCommentLocation();
		m_debugDataOverride = DebugData::create();
		if (!sourceIndex || !start || !end)
			m_errorReporter.syntaxError(
				6367_error,
				commentLocation,
				"Invalid value in source location mapping. Could not parse location "
				"specification.");
		else if (sourceIndex == -1)
			m_debugDataOverride
				= DebugData::create(SourceLocation{*start, *end, nullptr});
		else if (!(sourceIndex >= 0
				   && m_sourceNames->count(static_cast<unsigned>(*sourceIndex))))
			m_errorReporter.syntaxError(
				2674_error,
				commentLocation,
				"Invalid source mapping. Source index not defined via @use-src.");
		else
		{
			shared_ptr<string const> sourceName
				= m_sourceNames->at(static_cast<unsigned>(*sourceIndex));
			solAssert(sourceName, "");
			m_debugDataOverride
				= DebugData::create(SourceLocation{*start, *end, move(sourceName)});
		}
	}
}

langutil::Token Parser::advance()
{
	auto const token = ParserBase::advance();
	if (m_useSourceLocationFrom == UseSourceLocationFrom::Comments)
		fetchSourceLocationFromComment();
	return token;
}


Block warp::Parser::parseBlock(string& ret, const string& _funName)
{
	RecursionGuard recursionGuard(*this);
	Block block = createWithLocation<Block>();
	expectToken(Token::LBrace);
	while (currentToken() != Token::RBrace)
		block.statements.emplace_back(this->parseStatement(ret, _funName));
	if (m_useSourceLocationFrom == UseSourceLocationFrom::Scanner)
		block.debugData = warp::updateLocationEndFrom(block.debugData, currentLocation());
	advance();
	return block;
}

// To Handle for loops
string warp::Parser::parseLoopBlock(string& ret, const string& _funName)
{
	RecursionGuard recursionGuard(*this);
	Block block = createWithLocation<Block>();
	string cairo_str;
	expectToken(Token::LBrace);
	while (currentToken() != Token::RBrace)
		cairo_str += this->parseLoopStatement(ret, _funName);
	if (m_useSourceLocationFrom == UseSourceLocationFrom::Scanner)
		block.debugData = warp::updateLocationEndFrom(block.debugData, currentLocation());
	advance();
	return cairo_str;
}

string warp::Parser::generateCairoFuncCall(const yul::FunctionCall& _func)
{
	string fName = _func.functionName.name.str();
	string cairo_str = fName + "(";
	vector<string> args_cairo;
	auto args = _func.arguments;
	if (args.empty())
	{
		return cairo_str + ")";
	}
	for (auto arg: args)
	{
		if (const auto argT(std::get_if<yul::Identifier>(&arg)); argT)
		{
			string argName = argT->name.str();
			cairo_str += argName + ", ";
		}
		else if (const auto argT(std::get_if<yul::Literal>(&arg)); argT)
		{
			cairo_str += toUint256(argT->value.str()) + ", ";
		}
		else if (const auto funcInner(std::get_if<yul::FunctionCall>(&arg)); funcInner)
		{
			string splitCall = "let (local tmp : Uint256) = "
							   + this->generateCairoFuncCall(*funcInner) + "\n";
			cairo_str += splitCall;
		}
	}
	return string(cairo_str.begin(), cairo_str.end() - 2) + ")";
}

string warp::Parser::generateCairoExpression(
	const yul::Expression& _expr, const yul::VariableDeclaration& _varDecl)
{
	string _cairo_str_lhs = "let (";
	string _cairo_str_rhs;
	auto numVarsLhs = _varDecl.variables.size();
	if (numVarsLhs == 1)
	{
		_cairo_str_lhs
			= "let (local " + _varDecl.variables[0].name.str() + " : Uint256) ";
	}
	else
	{
		for (auto var: _varDecl.variables)
		{
			auto v = *var.debugData;
			string name = var.name.str();
			_cairo_str_lhs += "local " + name + " : Uint256, ";
		}
	}
	_cairo_str_lhs = string(_cairo_str_lhs.begin(), _cairo_str_lhs.end() - 2) + ")";
	if (const auto func(std::get_if<yul::FunctionCall>(&_expr)); func)
	{
		_cairo_str_rhs = this->generateCairoFuncCall(*func);
	}
	else if (const auto ident(std::get_if<yul::Identifier>(&_expr)); ident)
	{
		_cairo_str_rhs = ident->name.str();
	}
	else if (const auto lit(std::get_if<yul::Literal>(&_expr)); lit)
	{
		_cairo_str_rhs += toUint256(lit->value.str());
	}
	string complete = _cairo_str_lhs + " = " + _cairo_str_rhs;
	return complete;
}

string warp::Parser::generateCairoExpression(yul::Expression& _expr)
{
	string _cairo_str = "";
	if (const auto func(std::get_if<yul::FunctionCall>(&_expr)); func)
	{
		_cairo_str = this->generateCairoFuncCall(*func);
		if (m_currentForLoopComponent == ForLoopComponent::ForLoopCondition)
		{
			string new_str = "let (local loop_check : Uint256) = " + _cairo_str + "\n";
			new_str += "if loop_check.low == 0:\n";
			return new_str;
		}
		else
			return _cairo_str;
	}
	else if (const auto ident(std::get_if<yul::Identifier>(&_expr)); ident)
	{
		_cairo_str = ident->name.str();
		if (m_currentForLoopComponent == ForLoopComponent::ForLoopCondition)
		{
			string new_str = "if " + _cairo_str + ".low == 0:\n";
			return new_str;
		}
		else
			return _cairo_str;
	}
	else if (const auto lit(std::get_if<yul::Literal>(&_expr)); lit)
	{
		_cairo_str = toUint256(lit->value.str());
		if (m_currentForLoopComponent == ForLoopComponent::ForLoopCondition)
		{
			string new_str = "if " + _cairo_str + " == 0:\n";
			return new_str;
		}
		else
			return _cairo_str;
	}
}

// first string is args for definition, second string for recursive loop if it exists.
tuple<string, string>
warp::Parser::generateCairoFuncArgs(const yul::TypedNameList& _params)
{
	string cairo_str;
	if (_params.empty())
	{
		cairo_str += ")";
		return tuple<string, string>(cairo_str, "");
	}
	else
	{
		for (auto param: _params)
		{
			cairo_str += param.name.str() + " : Uint256, ";
			this->m_CurrentFunArgsForCall += param.name.str() + ", ";
		}
		return tuple<
			string,
			string>(string(cairo_str.begin(), cairo_str.end() - 2) + ")", cairo_str);
	}
}

string warp::Parser::generateCairoFuncSigRetVars(const yul::TypedNameList& _retVars)
{
	string cairo_str;
	if (_retVars.empty())
	{
		cairo_str += ":";
		return cairo_str;
	}
	else
	{
		cairo_str += "-> ( ";
		for (auto var: _retVars)
		{
			cairo_str += var.name.str() + " : Uint256, ";
		}
		return string(cairo_str.begin(), cairo_str.end() - 2) + "):";
	}
}

string warp::Parser::generateCairoFuncRetVars(const yul::TypedNameList& _retVars)
{
	string cairo_str;
	if (_retVars.empty())
	{
		return "return ()";
	}
	else
	{
		cairo_str += "return (";
		for (auto var: _retVars)
		{
			cairo_str += var.name.str() + ", ";
		}
		return string(cairo_str.begin(), cairo_str.end() - 2) + ")";
	}
}

string warp::Parser::generateCairoFuncDef(const yul::FunctionDefinition& _funcDef)
{
	auto _funcName = _funcDef.name.str();
	string args;
	string loopArgs;
	string cairo_str = "func " + _funcName
					   + "{pedersen_ptr : HashBuiltin*, storage_ptr : Storage*, "
						 "memory_dict : DictAccess*, msize,range_check_ptr}(";
	std::tie(args, loopArgs) = generateCairoFuncArgs(_funcDef.parameters);
	cairo_str += args;
	this->m_CurrentFunArgs = loopArgs;
	cairo_str += this->generateCairoFuncSigRetVars(_funcDef.returnVariables);
	return cairo_str;
}

string warp::Parser::generateCarioAssigment(const yul::Assignment& _assign)
{
	string _cairo_str = "let (";
	if (_assign.variableNames.size() == 1)
	{
		_cairo_str
			+= "local " + _assign.variableNames[0].name.str() + " : Uint256)" + " = ";
		_cairo_str += this->generateCairoExpression(*_assign.value) + "\n";
		return _cairo_str;
	}
	else
	{
		string lhs = "let (";
		for (auto var: _assign.variableNames)
		{
			lhs += "local " + var.name.str() + ": Uint256, ";
		}
		lhs += string(_cairo_str.begin(), _cairo_str.end() - 2) + ") = ";
		string rhs = this->generateCairoExpression(*_assign.value) + "\n";
		_cairo_str = lhs + rhs;
		return _cairo_str;
	}
}

Statement warp::Parser::parseStatement(string& _ret, const string& _funcName)
{
	RecursionGuard recursionGuard(*this);
	switch (currentToken())
	{
	case Token::Let:
	{
		// using Expression = std::variant<FunctionCall, Identifier, Literal>;
		auto _varDecl = parseVariableDeclaration();
		auto expr = *_varDecl.value;
		m_cairo_str += this->generateCairoExpression(expr, _varDecl) + "\n";
		this->addLoopAccessibleVars(_varDecl);
		return _varDecl;
	}
	case Token::Function:
	{
		auto _funcDef = this->parseFunctionDefinition(_ret);
		this->m_currFunName = "";
		this->m_CurrentFunArgs = "";
		this->m_CurrentFunArgsForCall = "";
		this->m_loopAccessibleVars.clear();
		return _funcDef;
	}
	case Token::LBrace:
		return parseBlock(_ret);
	case Token::If:
	{
		If _if = createWithLocation<If>();
		this->m_cairo_str += "if ";
		advance();
		_if.condition = make_unique<Expression>(parseExpression());
		this->m_cairo_str
			+= this->generateCairoExpression(*_if.condition) + ".low == 1:\n";
		_if.body = parseBlock(_ret);
		this->m_cairo_str += "end\n";
		if (m_useSourceLocationFrom == UseSourceLocationFrom::Scanner)
			_if.debugData = warp::
				updateLocationEndFrom(_if.debugData, _if.body.debugData->location);
		return Statement{move(_if)};
	}
	case Token::Switch:
	{
		Switch _switch = createWithLocation<Switch>();
		advance();
		_switch.expression = make_unique<Expression>(parseExpression());
		string check = this->generateCairoExpression(*_switch.expression);
		this->m_cairo_str += "if " + check + ".low == ";
		while (currentToken() == Token::Case)
			_switch.cases.emplace_back(parseCase(_ret, check));
		if (currentToken() == Token::Default)
		{
			this->m_cairo_str += "else:\n";
			_switch.cases.emplace_back(parseCase(_ret, ""));
		}
		if (m_useSourceLocationFrom == UseSourceLocationFrom::Scanner)
			_switch.debugData = warp::updateLocationEndFrom(
				_switch.debugData, _switch.cases.back().body.debugData->location);
		this->m_cairo_str += "end\n";
		return Statement{move(_switch)};
	}
	case Token::For:
	{
		this->m_cairo_str = parseForLoop(_ret, this->m_currFunName) + this->m_cairo_str;
		return Statement{};
	}
	case Token::Break:
	{
		Statement stmt{createWithLocation<Break>()};
		checkBreakContinuePosition("break");
		advance();
		return stmt;
	}
	case Token::Continue:
	{
		Statement stmt{createWithLocation<Continue>()};
		checkBreakContinuePosition("continue");
		advance();
		return stmt;
	}
	case Token::Leave:
	{
		Statement stmt{createWithLocation<Leave>()};
		if (!m_insideFunction)
			m_errorReporter.syntaxError(
				8149_error,
				currentLocation(),
				"Keyword \"leave\" can only be used inside a function.");
		advance();
		return stmt;
	}
	default:
		break;
	}

	// Options left:
	// Expression/FunctionCall
	// Assignment
	variant<Literal, Identifier> elementary(parseLiteralOrIdentifier());

	switch (currentToken())
	{
	case Token::LParen:
	{
		Expression expr = parseCall(std::move(elementary));
		this->m_cairo_str += this->generateCairoExpression(expr) + "\n";
		return ExpressionStatement{debugDataOf(expr), move(expr)};
	}
	case Token::Comma:
	case Token::AssemblyAssign:
	{
		Assignment assignment;
		assignment.debugData = debugDataOf(elementary);

		while (true)
		{
			if (!holds_alternative<Identifier>(elementary))
			{
				auto const token = currentToken() == Token::Comma ? "," : ":=";

				fatalParserError(
					2856_error,
					string("Variable name must precede \"") + token + "\""
						+ (currentToken() == Token::Comma ? " in multiple assignment."
														  : " in assignment."));
			}

			auto const& identifier = std::get<Identifier>(elementary);

			if (m_dialect.builtin(identifier.name))
				fatalParserError(
					6272_error,
					"Cannot assign to builtin function \"" + identifier.name.str()
						+ "\".");

			assignment.variableNames.emplace_back(identifier);

			if (currentToken() != Token::Comma)
				break;

			expectToken(Token::Comma);

			elementary = parseLiteralOrIdentifier();
		}

		expectToken(Token::AssemblyAssign);

		assignment.value = make_unique<Expression>(parseExpression());
		if (m_useSourceLocationFrom == UseSourceLocationFrom::Scanner)
			assignment.debugData = warp::updateLocationEndFrom(
				assignment.debugData, locationOf(*assignment.value));

		return Statement{move(assignment)};
	}
	default:
		fatalParserError(6913_error, "Call or assignment expected.");
		break;
	}

	yulAssert(false, "");
	return {};
}

void warp::Parser::addLoopAccessibleVars(const VariableDeclaration& _vars)
{
	for (auto var: _vars.variables)
	{
		this->m_loopAccessibleVars.emplace_back(var.name.str());
	}
}

void warp::Parser::addLoopAccessedVars(const VariableDeclaration& _vars)
{
	for (auto var: _vars.variables)
	{
		this->m_loopAccessibleVars.emplace_back(var.name.str());
	}
}

string warp::Parser::parseLoopStatement(string& _ret, const string& _funcName)
{
	RecursionGuard recursionGuard(*this);
	switch (currentToken())
	{
	case Token::Let:
	{
		auto _varDecl = parseVariableDeclaration();
		auto expr = *_varDecl.value;
		string cairo_str = this->generateCairoExpression(expr, _varDecl) + "\n";
		return cairo_str;
	}
	case Token::Function:
	{
		yulAssert(false, "can't define a function in a loop");
		return "";
	}
	case Token::LBrace:
		return parseLoopBlock(_ret, _funcName);
	case Token::If:
	{
		If _if = createWithLocation<If>();
		string cairo_str = "if ";
		advance();
		_if.condition = make_unique<Expression>(parseExpression());
		cairo_str += this->generateCairoExpression(*_if.condition) + ".low == 1:\n";
		cairo_str += parseLoopBlock(_ret, _funcName);
		cairo_str += "end\n";
		return cairo_str;
	}
	case Token::Switch:
	{
		Switch _switch = createWithLocation<Switch>();
		advance();
		_switch.expression = make_unique<Expression>(parseExpression());
		string check = this->generateCairoExpression(*_switch.expression);
		string cairo_str = "if " + check + ".low == ";
		while (currentToken() == Token::Case)
			cairo_str += this->parseLoopCase();
		if (currentToken() == Token::Default)
		{
			cairo_str += "else:\n";
			cairo_str += this->parseLoopCase();
		}
		if (m_useSourceLocationFrom == UseSourceLocationFrom::Scanner)
			_switch.debugData = warp::updateLocationEndFrom(
				_switch.debugData, _switch.cases.back().body.debugData->location);
		cairo_str += "end\n";
		return cairo_str;
	}
	case Token::For:
	{
		return this->parseForLoop(_ret, this->m_currFunName);
	}
	case Token::Break:
	{
		Statement stmt{createWithLocation<Break>()};
		checkBreakContinuePosition("break");
		advance();
		return "break\n";
	}
	case Token::Continue:
	{
		Statement stmt{createWithLocation<Continue>()};
		checkBreakContinuePosition("continue");
		advance();
		return "continue";
	}
	case Token::Leave:
	{
		Statement stmt{createWithLocation<Leave>()};
		if (!m_insideFunction)
			m_errorReporter.syntaxError(
				8149_error,
				currentLocation(),
				"Keyword \"leave\" can only be used inside a function.");
		advance();
		return "leave";
	}
	default:
		break;
	}

	// Options left:
	// Expression/FunctionCall
	// Assignment
	variant<Literal, Identifier> elementary(parseLiteralOrIdentifier());

	switch (currentToken())
	{
	case Token::LParen:
	{
		Expression expr = parseCall(std::move(elementary));
		string cairo_str = this->generateCairoExpression(expr) + "\n";
		return cairo_str;
	}
	case Token::Comma:
	case Token::AssemblyAssign:
	{
		Assignment assignment;
		assignment.debugData = debugDataOf(elementary);

		while (true)
		{
			if (!holds_alternative<Identifier>(elementary))
			{
				auto const token = currentToken() == Token::Comma ? "," : ":=";

				fatalParserError(
					2856_error,
					std::string("Variable name must precede \"") + token + "\""
						+ (currentToken() == Token::Comma ? " in multiple assignment."
														  : " in assignment."));
			}

			auto const& identifier = std::get<Identifier>(elementary);

			if (m_dialect.builtin(identifier.name))
				fatalParserError(
					6272_error,
					"Cannot assign to builtin function \"" + identifier.name.str()
						+ "\".");

			assignment.variableNames.emplace_back(identifier);

			if (currentToken() != Token::Comma)
				break;

			expectToken(Token::Comma);

			elementary = parseLiteralOrIdentifier();
		}

		expectToken(Token::AssemblyAssign);

		assignment.value = make_unique<Expression>(parseExpression());
		if (m_useSourceLocationFrom == UseSourceLocationFrom::Scanner)
			assignment.debugData = warp::updateLocationEndFrom(
				assignment.debugData, locationOf(*assignment.value));

		string cairo_str = this->generateCarioAssigment(assignment) + "\n";
		for (auto var: assignment.variableNames)
		{
			m_loopAcessedUpperScopeVars.emplace_back(var.name.str());
		}
		return cairo_str;
	}
	default:
		fatalParserError(6913_error, "Call or assignment expected.");
		break;
	}

	yulAssert(false, "");
	return "";
}

Case Parser::parseCase(string& ret, const string& check)
{
	RecursionGuard recursionGuard(*this);
	Case _case = createWithLocation<Case>();
	if (currentToken() == Token::Default)
		advance();
	else if (currentToken() == Token::Case)
	{
		advance();
		variant<yul::Literal, yul::Identifier> literal = parseLiteralOrIdentifier();
		_case.value
			= make_unique<yul::Literal>(std::get<yul::Literal>(std::move(literal)));
		m_cairo_str += _case.value->value.str() + ":\n";
	}
	else
		yulAssert(false, "Case or default case expected.");
	_case.body = parseBlock(ret);
	if (m_useSourceLocationFrom == UseSourceLocationFrom::Scanner)
		_case.debugData = warp::
			updateLocationEndFrom(_case.debugData, _case.body.debugData->location);
	return _case;
}

string warp::Parser::parseLoopCase()
{
	RecursionGuard recursionGuard(*this);
	Case _case = createWithLocation<Case>();
	string cairo_str;
	if (currentToken() == Token::Default)
		advance();
	else if (currentToken() == Token::Case)
	{
		advance();
		variant<yul::Literal, yul::Identifier> literal = parseLiteralOrIdentifier();
		_case.value
			= make_unique<yul::Literal>(std::get<yul::Literal>(std::move(literal)));
		cairo_str += _case.value->value.str() + ":\n";
	}
	else
		yulAssert(false, "Case or default case expected.");
	cairo_str += parseLoopBlock(cairo_str, this->m_currFunName);
	if (m_useSourceLocationFrom == UseSourceLocationFrom::Scanner)
		_case.debugData = warp::
			updateLocationEndFrom(_case.debugData, _case.body.debugData->location);
	return cairo_str;
}


std::string warp::Parser::generateCairoLoopFunc(const std::string& _funcName)
{
	std::string loopFuncDefSig
		= "func " + _funcName
		  + "_recLoop{pedersen_ptr : HashBuiltin*, storage_ptr : Storage*, "
			"memory_dict : DictAccess*, msize,range_check_ptr}("
		  + this->m_CurrentFunArgs;
	std::string retStrSig;
	std::string retStr = "return (";
	std::string recfuncCall = _funcName + "_recLoop(" + this->m_CurrentFunArgsForCall;
	std::string assign = "let (local ";
	for (auto var: m_loopAcessedUpperScopeVars)
	{
		loopFuncDefSig += var + " : Uint256, ";
		retStrSig += var + " : Uint256, ";
		assign += var + " : Uint256, ";
		retStr += var + ", ";
		recfuncCall += var + ", ";
	}
	std::string loopFuncDefSig_1
		= std::string(loopFuncDefSig.begin(), loopFuncDefSig.end() - 2) + ") ";
	std::string retStrSig_
		= "-> (" + std::string(retStrSig.begin(), retStrSig.end() - 2) + "):\n";
	std::string loopFuncDefSig_2 = loopFuncDefSig_1 + retStrSig_;

	std::string retStr_ = std::string(retStr.begin(), retStr.end() - 2) + ")\n";
	this->m_currentLoopData.returnStatement = retStr_;
	std::string funCheck = this->m_currentLoopData.condition + retStr_ + "else:\n";
	std::string almostCompleteFunc = loopFuncDefSig_2 + funCheck
									 + this->m_currentLoopData.body
									 + this->m_currentLoopData.post;
	std::string recursiveCallReturn
		= "return " + std::string(recfuncCall.begin(), recfuncCall.end() - 2) + ") ";
	this->m_currentLoopData.recursiveCall = recursiveCallReturn;
	std::string recursiveCallAssign
		= std::string(recfuncCall.begin(), recfuncCall.end() - 2) + ")\n";
	std::string assign_
		= std::string(assign.begin(), assign.end() - 2) + ") = " + recursiveCallAssign;
	std::string completeFunc = almostCompleteFunc + recursiveCallReturn + "\nend\nend\n";
	this->m_cairo_str += assign_;
	return completeFunc;
}

void warp::Parser::stringReplace(string& str, const string& find, const string& replace)
{
	size_t index = 0;
	size_t increment = replace.size();
	while (true)
	{
		/* Locate the substring to replace. */
		index = str.find(find, index);
		if (index == std::string::npos)
			break;

		/* Make the replacement. */
		str.replace(index, find.size(), replace);

		/* Advance index forward so the next iteration doesn't pick it up as well. */
		index += increment;
	}
}

string warp::Parser::parseForLoop(string& ret, const string& _funcName)
{
	RecursionGuard recursionGuard(*this);

	ForLoopComponent outerForLoopComponent = m_currentForLoopComponent;

	yul::ForLoop forLoop = createWithLocation<yul::ForLoop>();
	expectToken(Token::For);
	// Should be empty because of expression splitting
	m_currentForLoopComponent = ForLoopComponent::ForLoopPre;
	this->m_currentLoopData.pre = parseLoopBlock(ret, _funcName);

	m_currentForLoopComponent = ForLoopComponent::ForLoopCondition;
	auto conditionExpression = parseExpression();
	this->m_currentLoopData.condition = generateCairoExpression(conditionExpression);
	m_currentForLoopComponent = ForLoopComponent::ForLoopPost;
	this->m_currentLoopData.post = parseLoopBlock(ret, _funcName);

	m_currentForLoopComponent = ForLoopComponent::ForLoopBody;
	this->m_currentLoopData.body = parseLoopBlock(ret, _funcName);

	string cairo_loop = this->generateCairoLoopFunc(_funcName);

	m_currentForLoopComponent = outerForLoopComponent;
	string break_replace = this->m_currentLoopData.returnStatement;
	string continue_replace
		= this->m_currentLoopData.post + this->m_currentLoopData.recursiveCall + "\n";
	this->stringReplace(cairo_loop, "break", break_replace);
	this->stringReplace(cairo_loop, "continue", continue_replace);
	return cairo_loop;
}

Expression Parser::parseExpression()
{
	RecursionGuard recursionGuard(*this);

	variant<Literal, Identifier> operation = parseLiteralOrIdentifier();
	return visit(
		GenericVisitor{
			[&](Identifier& _identifier) -> Expression
			{
				if (currentToken() == Token::LParen)
					return parseCall(std::move(operation));
				if (m_dialect.builtin(_identifier.name))
					fatalParserError(
						7104_error,
						_identifier.debugData->location,
						"Builtin function \"" + _identifier.name.str()
							+ "\" must be called.");
				return move(_identifier);
			},
			[&](Literal& _literal) -> Expression { return move(_literal); }},
		operation);
}

variant<Literal, Identifier> Parser::parseLiteralOrIdentifier()
{
	RecursionGuard recursionGuard(*this);
	switch (currentToken())
	{
	case Token::Identifier:
	{
		Identifier identifier{createDebugData(), YulString{currentLiteral()}};
		advance();
		return identifier;
	}
	case Token::StringLiteral:
	case Token::HexStringLiteral:
	case Token::Number:
	case Token::TrueLiteral:
	case Token::FalseLiteral:
	{
		LiteralKind kind = LiteralKind::Number;
		switch (currentToken())
		{
		case Token::StringLiteral:
		case Token::HexStringLiteral:
			kind = LiteralKind::String;
			break;
		case Token::Number:
			if (!isValidNumberLiteral(currentLiteral()))
				fatalParserError(4828_error, "Invalid number literal.");
			kind = LiteralKind::Number;
			break;
		case Token::TrueLiteral:
		case Token::FalseLiteral:
			kind = LiteralKind::Boolean;
			break;
		default:
			break;
		}

		Literal literal{
			createDebugData(),
			kind,
			YulString{currentLiteral()},
			kind == LiteralKind::Boolean ? m_dialect.boolType : m_dialect.defaultType};
		advance();
		if (currentToken() == Token::Colon)
		{
			expectToken(Token::Colon);
			if (m_useSourceLocationFrom == UseSourceLocationFrom::Scanner)
				literal.debugData
					= warp::updateLocationEndFrom(literal.debugData, currentLocation());
			literal.type = expectAsmIdentifier();
		}

		return literal;
	}
	case Token::Illegal:
		fatalParserError(
			1465_error, "Illegal token: " + to_string(m_scanner->currentError()));
		break;
	default:
		fatalParserError(1856_error, "Literal or identifier expected.");
	}
	return {};
}

VariableDeclaration Parser::parseVariableDeclaration()
{
	RecursionGuard recursionGuard(*this);
	VariableDeclaration varDecl = createWithLocation<VariableDeclaration>();
	expectToken(Token::Let);
	while (true)
	{
		varDecl.variables.emplace_back(parseTypedName());
		if (currentToken() == Token::Comma)
			expectToken(Token::Comma);
		else
			break;
	}
	if (currentToken() == Token::AssemblyAssign)
	{
		expectToken(Token::AssemblyAssign);
		varDecl.value = make_unique<Expression>(parseExpression());
		if (m_useSourceLocationFrom == UseSourceLocationFrom::Scanner)
			varDecl.debugData = warp::
				updateLocationEndFrom(varDecl.debugData, locationOf(*varDecl.value));
	}
	else if (m_useSourceLocationFrom == UseSourceLocationFrom::Scanner)
		varDecl.debugData = warp::updateLocationEndFrom(
			varDecl.debugData, varDecl.variables.back().debugData->location);

	return varDecl;
}

FunctionDefinition Parser::parseFunctionDefinition(string& _ret)
{
	RecursionGuard recursionGuard(*this);

	if (m_currentForLoopComponent == ForLoopComponent::ForLoopPre)
		m_errorReporter.syntaxError(
			3441_error,
			currentLocation(),
			"Functions cannot be defined inside a for-loop init block.");

	ForLoopComponent outerForLoopComponent = m_currentForLoopComponent;
	m_currentForLoopComponent = ForLoopComponent::None;

	FunctionDefinition funDef = createWithLocation<FunctionDefinition>();
	expectToken(Token::Function);
	funDef.name = expectAsmIdentifier();
	expectToken(Token::LParen);
	while (currentToken() != Token::RParen)
	{
		funDef.parameters.emplace_back(parseTypedName());
		if (currentToken() == Token::RParen)
			break;
		expectToken(Token::Comma);
	}
	expectToken(Token::RParen);
	if (currentToken() == Token::RightArrow)
	{
		expectToken(Token::RightArrow);
		while (true)
		{
			funDef.returnVariables.emplace_back(parseTypedName());
			if (currentToken() == Token::LBrace)
				break;
			expectToken(Token::Comma);
		}
	}
	bool preInsideFunction = m_insideFunction;
	m_insideFunction = true;
	m_cairo_str += this->generateCairoFuncDef(funDef) + "\n";
	this->m_currFunName = funDef.name.str();
	funDef.body = this->parseBlock(_ret, funDef.name.str());
	m_cairo_str += this->generateCairoFuncRetVars(funDef.returnVariables) + "\n";
	m_cairo_str += "end\n\n";
	m_insideFunction = preInsideFunction;
	if (m_useSourceLocationFrom == UseSourceLocationFrom::Scanner)
		funDef.debugData = warp::
			updateLocationEndFrom(funDef.debugData, funDef.body.debugData->location);

	m_currentForLoopComponent = outerForLoopComponent;
	return funDef;
}

FunctionCall Parser::parseCall(variant<Literal, Identifier>&& _initialOp)
{
	RecursionGuard recursionGuard(*this);

	if (!holds_alternative<Identifier>(_initialOp))
		fatalParserError(9980_error, "Function name expected.");

	FunctionCall ret;
	ret.functionName = std::move(std::get<Identifier>(_initialOp));
	ret.debugData = ret.functionName.debugData;

	expectToken(Token::LParen);
	if (currentToken() != Token::RParen)
	{
		ret.arguments.emplace_back(parseExpression());
		while (currentToken() != Token::RParen)
		{
			expectToken(Token::Comma);
			ret.arguments.emplace_back(parseExpression());
		}
	}
	if (m_useSourceLocationFrom == UseSourceLocationFrom::Scanner)
		ret.debugData = warp::updateLocationEndFrom(ret.debugData, currentLocation());
	expectToken(Token::RParen);
	return ret;
}

TypedName Parser::parseTypedName()
{
	RecursionGuard recursionGuard(*this);
	TypedName typedName = createWithLocation<TypedName>();
	typedName.name = expectAsmIdentifier();
	if (currentToken() == Token::Colon)
	{
		expectToken(Token::Colon);
		if (m_useSourceLocationFrom == UseSourceLocationFrom::Scanner)
			typedName.debugData
				= warp::updateLocationEndFrom(typedName.debugData, currentLocation());
		typedName.type = expectAsmIdentifier();
	}
	else
		typedName.type = m_dialect.defaultType;

	return typedName;
}

YulString Parser::expectAsmIdentifier()
{
	YulString name{currentLiteral()};
	if (currentToken() == Token::Identifier && m_dialect.builtin(name))
		fatalParserError(
			5568_error,
			"Cannot use builtin function name \"" + name.str()
				+ "\" as identifier name.");
	// NOTE: We keep the expectation here to ensure the correct source location
	// for the error above.
	expectToken(Token::Identifier);
	return name;
}

void Parser::checkBreakContinuePosition(string const& _which)
{
	switch (m_currentForLoopComponent)
	{
	case ForLoopComponent::None:
		m_errorReporter.syntaxError(
			2592_error,
			currentLocation(),
			"Keyword \"" + _which + "\" needs to be inside a for-loop body.");
		break;
	case ForLoopComponent::ForLoopPre:
		m_errorReporter.syntaxError(
			9615_error,
			currentLocation(),
			"Keyword \"" + _which + "\" in for-loop init block is not allowed.");
		break;
	case ForLoopComponent::ForLoopPost:
		m_errorReporter.syntaxError(
			2461_error,
			currentLocation(),
			"Keyword \"" + _which + "\" in for-loop post block is not allowed.");
		break;
	case ForLoopComponent::ForLoopBody:
		break;
	}
}

bool Parser::isValidNumberLiteral(string const& _literal)
{
	try
	{
		// Try to convert _literal to u256.
		[[maybe_unused]] auto tmp = u256(_literal);
	}
	catch (...)
	{
		return false;
	}
	if (boost::starts_with(_literal, "0x"))
		return true;
	else
		return _literal.find_first_not_of("0123456789") == string::npos;
}
