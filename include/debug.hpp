#pragma once

#include <fstream>
#include <iostream>

#define DEBUG FALSE

#ifdef DEBUG
std::ostream &dout = std::cerr;
#else
std::ofstream dev_null("/dev/null");
std::ostream &dout = dev_null;
#endif
