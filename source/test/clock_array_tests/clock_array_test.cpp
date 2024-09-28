#include "commont.h"


// Test case for getting clock values
TEST_F(ClockArrayTest, simple_linear_calculation)
{
    dur_type dd{14};
    auto key=clk.tik_loc();
    SLEEP(dd);
    clk.tak();
    ASSERT_NEAR_REL(dd.count(), clk[key].time, rel_err);
}

TEST_F(ClockArrayTest, inlined_clocks) {
    dur_type dd{20};
    dur_type dd2{5};
    auto key1=clk.tik_loc();
    location_type key2;
    {
        SLEEP(dd);
        key2=clk.tik_loc();
        SLEEP(dd2);
        clk.tak();
    }
    clk.tak();
    ASSERT_NEAR_REL(dd.count()+dd2.count(), clk[key1].time, rel_err);
    ASSERT_NEAR_REL(dd2.count(), clk[key2].time, rel_err);
}


TEST_F(ClockArrayTest, loop_clock) {
    dur_type dd{20};
    constexpr size_t loop_count=10;
    location_type key;
    for (int i = 0; i < loop_count; ++i) {
        key=clk.tik_loc();
        SLEEP(dd);
        clk.tak();
    }
    ASSERT_NEAR_REL(dd.count()*loop_count, clk[key].time, rel_err);
}

TEST_F(ClockArrayTest, subsequent_section_clock) {
    dur_type dd{20};
    dur_type dd2{60};
    dur_type dd3{12};
    auto pair=clk.tik_loc_();
    location_type key=pair.second;
    SLEEP(dd);
    clk.tak();

    SLEEP(dd2);

    clk.tik(pair.first);
    SLEEP(dd3);
    clk.tak();


    ASSERT_NEAR_REL(dd.count()+dd3.count(), clk[key].time, rel_err);
}


TEST_F(ClockArrayTest, lambda_function_call) {
    dur_type dd{20};


    location_type key=clk.tik_loc();

    std::source_location ss;

    [&]()
    {
        auto pp=clk.tik_loc_();
        ss=pp.first;
        key=pp.second;
        SLEEP(dd);
        clk.tak();
    }();
    ASSERT_TRUE(std::string(ss.function_name()).contains(key[0]));

}

TEST_F(ClockArrayTest, mismatched_tik_tak) {
    ASSERT_THROW(clk.tak(),std::logic_error);
}


