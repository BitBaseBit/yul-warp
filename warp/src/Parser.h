// SPDX-License-Identifier: GPL-3.0
/**
 * @author Christian <c@ethdev.com>
 * @date 2016
 * Solidity inline assembly parser.
 */

#pragma once


#include "ObjectParser.h"
#include <liblangutil/EVMVersion.h>
#include <liblangutil/Scanner.h>
#include <libyul/AssemblyStack.h>
#include <libyul/backends/evm/EVMDialect.h>

#include <memory>
#include <vector>

using namespace solidity;
using namespace std;

namespace warp
{
string parseAndAnalyze(std::string const& _sourceName, std::string const& _source)
{
	langutil::EVMVersion _version = langutil::EVMVersion().london();
	auto _charStream = make_unique<langutil::CharStream>(_source, _sourceName);
	std::shared_ptr<langutil::Scanner> scanner
		= make_shared<langutil::Scanner>(*_charStream);
	std::string retStr;
	std::shared_ptr<yul::Object> _parserResult;
	std::tie(_parserResult, retStr) = warp::ObjectParser().parse(scanner, false, retStr);
	return retStr;
}

} // end namespace warp
