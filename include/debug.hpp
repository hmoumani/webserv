#pragma once

#include <fstream>
#include <iostream>

#define DEBUG

#ifdef DEBUG
std::ostream &dout = std::cerr;
#else
std::ofstream dev_null("/dev/null");
std::ostream &dout = dev_null;
#endif
