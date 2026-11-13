// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
// Always evaluates, including under NDEBUG. Test setup and checks must never
// disappear with the production assertion configuration.
#define TEST_REQUIRE(condition) do { if (!(condition)) { \
 fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#condition); exit(EXIT_FAILURE); \
} } while (0)
static inline void test_posix_result(int result,const char *operation) {
 if(result<0){fprintf(stderr,"FAIL %s: %s\n",operation,strerror(errno));exit(EXIT_FAILURE);}
}
