#include <boost/config.hpp>

#define LIBRARY_API extern "C" BOOST_SYMBOL_EXPORT

LIBRARY_API char const* test()
{
    return "loaded library version two";
}
