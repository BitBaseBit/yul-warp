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

#pragma once

#include <libyul/AST.h>
#include <libyul/ASTForward.h>
#include <libyul/Dialect.h>

#include <liblangutil/ParserBase.h>
#include <liblangutil/Scanner.h>
#include <liblangutil/SourceLocation.h>

#include <map>
#include <memory>
#include <variant>
#include <vector>

using namespace solidity;
using namespace std;


namespace warp
{
struct LoopData
{
	std::string pre;
	std::string condition;
	std::string body;
	std::string post;
	std::string loopFuncSig;
	// Data to retun at the end of the loop
	std::string returnStatement;
	// statments that are semantically equivalent to
	// 'continue'
	std::string continueStatements;
	// return with a recursive call to itself
	std::string recursiveCall;
	std::string recursiveFuncAssign;
	std::string endLoopCheck;
};
class Parser: public langutil::ParserBase
{
public:
	string m_cairo_str = "";
	enum class ForLoopComponent
	{
		None,
		ForLoopPre,
		ForLoopCondition,
		ForLoopPost,
		ForLoopBody,
	};

	enum class UseSourceLocationFrom
	{
		Scanner,
		LocationOverride,
		Comments,
	};

	explicit Parser(langutil::ErrorReporter&						  _errorReporter,
					yul::Dialect const&								  _dialect,
					std::optional<solidity::langutil::SourceLocation> _locationOverride
					= {})
		: ParserBase(_errorReporter),
		  m_dialect(_dialect), m_locationOverride{_locationOverride
													  ? *_locationOverride
													  : langutil::SourceLocation{}},
		  m_debugDataOverride{}, m_useSourceLocationFrom{
									 _locationOverride
										 ? UseSourceLocationFrom::LocationOverride
										 : UseSourceLocationFrom::Scanner}
	{
	}

	/// Constructs a Yul parser that is using the source locations
	/// from the comments (via @src).
	explicit Parser(
		langutil::ErrorReporter&										 _errorReporter,
		yul::Dialect const&												 _dialect,
		std::optional<std::map<unsigned, std::shared_ptr<string const>>> _sourceNames)
		: ParserBase(_errorReporter),
		  m_dialect(_dialect), m_sourceNames{std::move(_sourceNames)},
		  m_debugDataOverride{yul::DebugData::create()},
		  m_useSourceLocationFrom{m_sourceNames.has_value()
									  ? UseSourceLocationFrom::Comments
									  : UseSourceLocationFrom::Scanner}
	{
	}
	string				  generateCairoFuncSigRetVars(const yul::TypedNameList& _retVars);
	string				  generateCairoFuncRetVars(const yul::TypedNameList& _retVars);
	tuple<string, string> generateCairoFuncArgs(const yul::TypedNameList& _params);
	string				  generateCairoFuncDef(const yul::FunctionDefinition& _funcDef);
	string				  generateCairoExpression(const yul::Expression&		  _expr,
												  const yul::VariableDeclaration& _varDecl);
	string				  generateCairoExpression(yul::Expression& _expr);
	string				  generateCarioAssigment(const yul::Assignment& _assign);
	string				  generateCairoFuncCall(const yul::FunctionCall& _func);

	/// Parses an inline assembly block starting with `{` and ending with `}`.
	/// @returns an empty shared pointer on error.
	tuple<unique_ptr<yul::Block>, string>
	parseInline(std::shared_ptr<langutil::Scanner> const& _scanner, string& ret);

	/// Parses an assembly block starting with `{` and ending with `}`
	/// and expects end of input after the '}'.
	/// @returns an empty shared pointer on error.
	std::unique_ptr<yul::Block> parse(langutil::CharStream& _charStream, string& ret);

protected:
	langutil::SourceLocation currentLocation() const override
	{
		if (m_useSourceLocationFrom == UseSourceLocationFrom::Scanner)
			return ParserBase::currentLocation();
		return m_locationOverride;
	}

	langutil::Token advance() override;

	void fetchSourceLocationFromComment();

	/// Creates a DebugData object with the correct source location set.
	std::shared_ptr<yul::DebugData const> createDebugData() const;

	/// Creates an inline assembly node with the current source location.
	template<class T>
	T createWithLocation() const
	{
		T r;
		r.debugData = createDebugData();
		return r;
	}

	vector<string> m_loopAccessibleVars;
	vector<string> m_loopAcessedUpperScopeVars;
	vector<string> m_loopFuncArgs;
	LoopData	   m_currentLoopData;
	string		   m_currFunName;
	string		   m_CurrentFunArgs;
	string		   m_CurrentFunArgsForCall;
	yul::Block	   parseBlock(string& ret, const string& _funName = "");
	string		   parseLoopBlock(string& ret, const string& _funName);
	yul::Statement parseStatement(string& ret, const string& _funName);
	string		   parseLoopStatement(string& _ret, const string& _funcName);
	string		   parseForLoop(string& ret, const string& _funcName);
	void 		   stringReplace(string& str, const string& find, const string& replace);
	yul::Case	   parseCase(string& ret, const string& check);
	string		   parseLoopCase();
	void		   addLoopAccessibleVars(const yul::VariableDeclaration& _vars);
	void		   addLoopAccessedVars(const yul::VariableDeclaration& _vars);
	string		   generateCairoLoopFunc(const std::string& _funcName);
	void		   getAuxLoopData(const std::string& _funcName);
	/// Parses a functional expression that has to push exactly one stack element
	yul::Expression parseExpression();
	/// Parses an elementary operation, i.e. a literal, identifier, instruction or
	/// builtin functian call (only the name).
	std::variant<yul::Literal, yul::Identifier> parseLiteralOrIdentifier();
	yul::VariableDeclaration					parseVariableDeclaration();
	yul::FunctionDefinition						parseFunctionDefinition(string& ret);
	yul::FunctionCall parseCall(std::variant<yul::Literal, yul::Identifier>&& _initialOp);
	yul::TypedName	  parseTypedName();
	yul::YulString	  expectAsmIdentifier();

	/// Reports an error if we are currently not inside the body part of a for
	/// loop.
	void checkBreakContinuePosition(string const& _which);

	static bool isValidNumberLiteral(string const& _literal);

private:
	yul::Dialect const& m_dialect;

	std::optional<std::map<unsigned, std::shared_ptr<std::string const>>> m_sourceNames;
	langutil::SourceLocation			  m_locationOverride;
	std::shared_ptr<yul::DebugData const> m_debugDataOverride;
	UseSourceLocationFrom m_useSourceLocationFrom = UseSourceLocationFrom::Scanner;
	ForLoopComponent	  m_currentForLoopComponent = ForLoopComponent::None;
	bool				  m_insideFunction = false;
};

} // namespace warp
