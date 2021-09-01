#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <math.h>

#include <liblangutil/CharStream.h>
#include <liblangutil/ErrorReporter.h>
#include <libsolidity/ast/AST.h>
#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/parsing/Parser.h>

#include "src/Parser.h"
#include "src/library.h"
#include "src/prepass.h"
#include "src/vendor/intx/include/intx/intx.hpp"

using namespace std;
using namespace solidity;
using namespace intx;

string slurpFile(string_view path)
{
	constexpr size_t BUF_SIZE = 4096;

	string	 result;
	ifstream is{path.data()};
	is.exceptions(ifstream::badbit);
	string buf(BUF_SIZE, '\0');
	while (is.read(buf.data(), BUF_SIZE))
		result.append(buf, 0, is.gcount());
	result.append(buf, 0, is.gcount());

	return result;
}

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		cerr << "USAGE: " << argv[0] << " SOLIDITY_CONTRACT" << endl;
		return 1;
	}

	string contractPath = argv[1];

	langutil::ErrorList		errors;
	langutil::ErrorReporter errorReporter{errors};
	frontend::Parser		parser{errorReporter, langutil::EVMVersion()};

	string		   contractContents = slurpFile(contractPath);
	string const&  yulContract = slurpFile("/home/greg/dev/warp-cpp/AsmParser/warp/loops.yul");
	string const&  sourceName = "loops.yul";
	vector<string> lines = splitStr(yulContract);
	vector<string> dep = getRuntimeYul(lines);
	string		   ret = warp::parseAndAnalyze(sourceName, yulContract);
	cout << ret << endl;
	ofstream gen_cairo;
	gen_cairo.open("gen.cairo");
	gen_cairo << ret << endl;
	gen_cairo.close();
	system("cairo-format -i gen.cairo");

	langutil::CharStream charStream{contractContents, contractPath};
	auto				 sourceUnit = parser.parse(charStream);
	return 0;
}
