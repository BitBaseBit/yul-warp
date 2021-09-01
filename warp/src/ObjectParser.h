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
 * Parser for Yul code and data object container.
 */

#pragma once

#include <libyul/Dialect.h>
#include <libyul/Object.h>
#include <liblangutil/EVMVersion.h>
#include <libyul/backends/evm/EVMDialect.h>
#include <libyul/YulString.h>

#include <liblangutil/ErrorReporter.h>
#include <liblangutil/ParserBase.h>

#include <libsolutil/Common.h>

#include <memory>

namespace solidity::langutil
{
class Scanner;
}

using namespace solidity;

namespace warp
{
/**
 * Yul object parser. Invokes the inline assembly parser.
 */
class ObjectParser: public langutil::ParserBase
{
public:
	langutil::ErrorList _errList = langutil::ErrorList{};
	langutil::ErrorReporter errRep = langutil::ErrorReporter(_errList); 
	langutil::EVMVersion _ver = langutil::EVMVersion().london();
	yul::EVMDialectTyped _dialect = yul::EVMDialectTyped(_ver, true);
	std::string m_cairo;
	explicit ObjectParser()
		: ParserBase(errRep), m_dialect(_dialect)
	{
	}

	/// Parses a Yul object.
	/// Falls back to code-only parsing if the source starts with `{`.
	/// @param _reuseScanner if true, do check for end of input after the last `}`.
	/// @returns an empty shared pointer on error.
	std::tuple<std::shared_ptr<yul::Object>, std::string>
	parse(std::shared_ptr<langutil::Scanner> const& _scanner, bool _reuseScanner, std::string& ret);

	std::optional<yul::SourceNameMap> const& sourceNameMapping() const noexcept { return m_sourceNameMapping; }

private:
	std::optional<yul::SourceNameMap> tryParseSourceNameMapping() const;
	std::shared_ptr<yul::Object> parseObject(yul::Object* _containingObject, std::string& ret);
	std::tuple<std::shared_ptr<yul::Block>, std::string> parseCode(std::string& ret);
	std::tuple<std::shared_ptr<yul::Block>, std::string> parseBlock(std::string& ret);
	void parseData(yul::Object& _containingObject);

	/// Tries to parse a name that is non-empty and unique inside the containing object.
	yul::YulString parseUniqueName(yul::Object const* _containingObject);
	void addNamedSubObject(yul::Object& _container, yul::YulString _name, std::shared_ptr<yul::ObjectNode> _subObject);

	yul::Dialect const& m_dialect;

	std::optional<yul::SourceNameMap> m_sourceNameMapping;
};

}
