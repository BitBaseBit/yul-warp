#include "library.h"

#include <iostream>

#include <libsolidity/interface/Version.h>

using namespace std;

void hello() {
    cout << "Hi! The solidity version is " << solidity::frontend::VersionNumber << endl;
}

