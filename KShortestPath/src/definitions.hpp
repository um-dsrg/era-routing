#ifndef definitions_hpp
#define definitions_hpp

#include <cstdint>

using id_t = uint32_t;
using identifier_t = uint64_t; // FIXME: Replace this with id_t
using linkCost_t = double;
using linkCapacity_t = double;

#ifdef MY_DEBUG
#define LOG_MSG(x) do { std::cerr << x << std::endl; } while (0)
#else
#define LOG_MSG(x)
#endif

#endif /* definitions_hpp */
