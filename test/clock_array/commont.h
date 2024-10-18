#ifndef SOURCE_SERVICE_COMMONT_H
#define SOURCE_SERVICE_COMMONT_H

#include <gtest/gtest.h>
#include <fff.h>

#include "clockArray.h"

using namespace timing;
using namespace std::chrono_literals;

#define SLEEP(dur) std::this_thread::sleep_for(dur)
#define ASSERT_NEAR_REL(val1, val2, rel_error) ASSERT_NEAR(val1,val2,rel_error*val1/100)
#define ASSERT_HISTORY_EQ(index, function) ASSERT_EQ(fff.call_history[index],(void *)function);
#define COMMA ,

using durType = std::chrono::duration<int64_t, std::milli>;

static constexpr double absErr = 1e-2;
static constexpr double relErr = 1;

#endif //SOURCE_SERVICE_COMMONT_H
