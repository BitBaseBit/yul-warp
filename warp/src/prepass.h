#pragma once
#include <algorithm>
#include <cctype>
#include <iostream>
#include <locale>
#include <cassert>
#include <vector>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost;

bool isRuntimeObj(string str)
{
    return ((str.find('{')) != string::npos) && 
        (str.find("object") != string::npos) && 
        (str.find("_deployed") != string::npos);
}

vector<string> removeNonCode(vector<string> lines)
{
	for (size_t i = 0; i != lines.size(); ++i)	
	{
		if (lines[i].find("object") != string::npos)
		{
			return vector<string>(lines.begin() + i, lines.end());
		}
		else
		{
			continue;
		}
	}
	assert(0==1);
	return vector<string>();
}

vector<string> getRuntimeYul(vector<string> yulContents)
{
	vector<string> lines = removeNonCode(yulContents);
	int start = lines[0].find("\"");
	int end = lines[0].find("\"", start + 1);
	string objectName = lines[0].substr(start, end - start);
	string deployedObjName = "object " + objectName + "_deployed";
	// replace(deployedObjName.begin(), deployedObjName.end(), '\"', '\0');
	trim_left(deployedObjName);
	trim_right(deployedObjName);

	for (size_t i = 1; i != lines.size(); ++i)
	{
		string lineCopy = lines[i];
		// replace(lineCopy.begin(), lineCopy.end(), '\"', '\0');
        trim_left(lineCopy);
        trim_right(lineCopy);
		if (isRuntimeObj(lineCopy))
		{
            vector<string> runtimeObj = vector<string>(lines.begin() + i, lines.end());
            runtimeObj.insert(runtimeObj.begin(), lines[0]);
            return runtimeObj;
		}
	}
	return lines;
}


vector<string> splitStr(const string& str)
{
	vector<string> strings;

	string::size_type pos = 0;
	string::size_type prev = 0;
	while ((pos = str.find('\n', prev)) != string::npos)
	{
		strings.push_back(str.substr(prev, pos - prev));
		prev = pos + 1;
	}

	// To get the last substring (or only, if delimiter is not found)
	strings.push_back(str.substr(prev));

	return strings;
}