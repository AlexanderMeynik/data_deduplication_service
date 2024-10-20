#include "commont.h"
#include <Leopard/ThreadPool.h>


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
    auto pair = clk.tikPair();
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
        auto pp = clk.tikPair();
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

TEST_F(ClockArrayTest, test_parralel_clock)
{
    constexpr size_t N=10;
    using perType=std::chrono::milliseconds::period;
    constexpr std::chrono::duration<long,perType> default_dur{50};
    constexpr auto timeout=default_dur*N*5;
    std::atomic<size_t> gcount = 1;
    hmthrp::ThreadPool tp{N};
    auto    waitJob =
            [this,&default_dur,&gcount]
            <typename Ret = long, typename Period = perType>(auto begin,auto end)
                    {
                auto wait=default_dur*gcount++;
                auto loc=clk.tikLoc();
                SLEEP(wait);
                clk.tak();
                return std::make_pair(loc,wait);
            };
    auto futures= tp.parallel_loop(std::size_t(0), N, waitJob);

    int periodsWaited=1;
    while (!std::all_of(futures.begin(), futures.end(), [](auto &ft) {
        return ft.wait_for(0s) == std::future_status::ready;
    })) {
        if(periodsWaited * default_dur >= timeout)
        {
            ASSERT_TRUE(false) << "Timeout time exceeded";
            tp.shutdown();
        }
        std::this_thread::sleep_for(default_dur);
        periodsWaited++;
    }

    for (size_t i = 0; i <N ; ++i) {
        auto res=futures[i].get();
        ASSERT_NEAR_REL(clk[res.first].time,res.second.count(),relErr);
    }
    tp.shutdown();

}


