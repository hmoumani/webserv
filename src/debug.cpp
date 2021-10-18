#include "debug.hpp"


#ifdef DEBUG
    std::ostream &debug = std::cerr;
#else
    std::ofstream dev_null("/dev/null");
    std::ostream &debug = dev_null;
#endif
