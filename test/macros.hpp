#pragma once

#include <gtest/gtest.h>


#if defined __cpp_exceptions || defined __EXCEPTIONS || defined _CPPUNWIND
#define USE_EXCEPTIONS
#endif

#if __cplusplus >= 201703L
    #define NODISCARD [[nodiscard]]
#else
    #if defined(__GNUC__) && (__GNUC__ >= 4) // clang also defines these
        #define NODISCARD __attribute__((warn_unused_result))
    #elif defined(_MSC_VER) && (_MSC_VER >= 1700)
        #define NODISCARD _Check_return_
    #else
        #define NODISCARD
    #endif
#endif
