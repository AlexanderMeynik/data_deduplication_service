#ifndef SOURCE_SERVICE_COMMONT_H
#define SOURCE_SERVICE_COMMONT_H
#include <gtest/gtest.h>
#include "../../service/common/ClockArray.h"
#include "../../../fff/fff.h"
using namespace timing;
using namespace std::chrono_literals;
#define SLEEP(dur) std::this_thread::sleep_for(dur)
#define ASSERT_NEAR_REL(val1, val2, rel_error) ASSERT_NEAR(val1,val2,rel_error*val1/100)
#define COMMA ,

//crutch
#define MEASURE_TIME(block) \
    clk.tik();              \
    block                   \
    clk.tak();
class ClockArrayTest : public ::testing::Test {
protected:
    using dur_type = std::chrono::duration<int64_t, std::milli>;
    chrono_clock_template<dur_type> clk;
    static constexpr double abs_err = 1e-2;
    static constexpr double rel_err = 1;
};

#endif //SOURCE_SERVICE_COMMONT_H
