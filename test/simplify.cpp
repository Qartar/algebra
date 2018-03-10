// simplify.cpp
//

#include "expression.h"

#include <cstdio>
#include <iostream>

//------------------------------------------------------------------------------
int main()
{
    while (true) {
        std::string line; std::getline(std::cin, line);
        if (line == "") {
            return 0;
        }

        algebra::simplify(algebra::parse(line.c_str()), 32, 256);
    }
}
