#pragma once
#include <iostream>
#include <string>

static int _pass = 0, _fail = 0;

#define CHECK(expr, msg) \
    do { \
        if (expr) { \
            ++_pass; \
            std::cout << "  PASS: " << (msg) << "\n"; \
        } else { \
            ++_fail; \
            std::cout << "  FAIL: " << (msg) << "\n"; \
        } \
    } while(0)

#define RUN_TESTS() \
    do { \
        std::cout << "\n" << _pass << " passed, " << _fail << " failed.\n"; \
        return _fail > 0 ? 1 : 0; \
    } while(0)
