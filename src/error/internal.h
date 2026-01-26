#ifndef ALOHA_ERROR_INTERNAL_H_
#define ALOHA_ERROR_INTERNAL_H_

#include "../frontend/location.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#define ALOHA_ICE(msg)                                                           \
    do                                                                           \
    {                                                                            \
        std::cerr << "\n\033[1;31mICE\033[0m at " << __FILE__ << ":" << __LINE__ \
                  << " in " << __func__ << ":\n  " << (msg) << "\n";             \
        std::abort();                                                            \
    } while (0)

#define ALOHA_UNIMPLEMENTED()                                                  \
    do                                                                         \
    {                                                                          \
        std::cerr << "\n\033[1;33mUNIMPLEMENTED\033[0m at " << __FILE__ << ":" \
                  << __LINE__ << " in " << __func__ << "\n";                   \
        std::abort();                                                          \
    } while (0)

#define ALOHA_UNREACHABLE()                                                  \
    do                                                                       \
    {                                                                        \
        std::cerr << "\n\033[1;31mUNREACHABLE\033[0m at " << __FILE__ << ":" \
                  << __LINE__ << " in " << __func__ << "\n";                 \
        std::abort();                                                        \
    } while (0)

#endif // ALOHA_ERROR_INTERNAL_H_
