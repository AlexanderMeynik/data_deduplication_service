#include "commont.h"


class ClockArrayTest : public ::testing::Test {
protected:
    chronoClockTemplate<durType> clk;
};

TEST_F(ClockArrayTest, simple_linear_calculation) {
    durType dd{14};
    auto key = clk.tikLoc();
    SLEEP(dd);
    clk.tak();
    ASSERT_NEAR_REL(dd.count(), clk[key].time, relErr);
}

TEST_F(ClockArrayTest, inlined_clocks) {
    durType dd{20};
    durType dd2{5};
    auto key1 = clk.tikLoc();
    locationType key2;
    {
        SLEEP(dd);
        key2 = clk.tikLoc();
        SLEEP(dd2);
        clk.tak();
    }
    clk.tak();
    ASSERT_NEAR_REL(dd.count() + dd2.count(), clk[key1].time, relErr);
    ASSERT_NEAR_REL(dd2.count(), clk[key2].time, relErr);
}


TEST_F(ClockArrayTest, loop_clock) {
    durType dd{20};
    constexpr size_t loop_count = 10;
    locationType key;
    for (int i = 0; i < loop_count; ++i) {
        key = clk.tikLoc();
        SLEEP(dd);
        clk.tak();
    }
    ASSERT_NEAR_REL(dd.count() * loop_count, clk[key].time, relErr);
}

TEST_F(ClockArrayTest, subsequent_section_clock) {
    durType dd{20};
    durType dd2{60};
    durType dd3{12};
    auto pair = clk.tikLoc2();
    locationType key = pair.second;
    SLEEP(dd);
    clk.tak();

    SLEEP(dd2);

    clk.tik(pair.first);
    SLEEP(dd3);
    clk.tak();

    ASSERT_NEAR_REL(dd.count() + dd3.count(), clk[key].time, relErr);
}


TEST_F(ClockArrayTest, lambda_function_call) {
    durType dd{20};

    locationType key = clk.tikLoc();

    std::source_location ss;

    [&]() {
        auto pp = clk.tikLoc2();
        ss = pp.first;
        key = pp.second;
        SLEEP(dd);
        clk.tak();
    }();
    ASSERT_TRUE(std::string(ss.function_name()).contains(key[0]));
}

TEST_F(ClockArrayTest, mismatched_tik_tak) {
    ASSERT_THROW(clk.tak(), std::logic_error);
}


