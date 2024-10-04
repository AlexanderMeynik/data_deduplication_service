#ifndef SOURCE_SERVICE_COMMONT_H
#define SOURCE_SERVICE_COMMONT_H
#include <gtest/gtest.h>
#include "ClockArray.h"
#include <fff.h>
#include "../testUtils.h"
using namespace timing;

#define ASSERT_NEAR_REL(val1, val2, rel_error) ASSERT_NEAR(val1,val2,rel_error*val1/100)

#define ASSERT_HISTORY_EQ(index, function) ASSERT_EQ(fff.call_history[index],(void *)function);


#define COMMA ,
using dur_type=std::chrono::duration<int64_t, std::milli>;


static constexpr double abs_err = 1e-2;
static constexpr double rel_err = 1;
//crutch
#define MEASURE_TIME(block) \
    clk.tik();              \
    block                   \
    clk.tak();




#endif //SOURCE_SERVICE_COMMONT_H
