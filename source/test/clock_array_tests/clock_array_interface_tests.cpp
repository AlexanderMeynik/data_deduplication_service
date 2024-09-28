#include "commont.h"

/*#include <functional>

*//* Configure FFF to use std::function, which enables capturing lambdas *//*
#define CUSTOM_FFF_FUNCTION_TEMPLATE(RETURN, FUNCNAME, ...) \
    std::function<RETURN (__VA_ARGS__)> FUNCNAME*/

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(double,time_getter);


FAKE_VALUE_FUNC(location_type ,func_name,std::source_location);
//todo move this to common
//todo set up scripts

FAKE_VALUE_FUNC(double ,double_cast,double,double);


#define FFF_FAKES_LIST(FAKE)            \
  FAKE(double_cast)                    \
  FAKE(func_name)                   \
  FAKE(time_getter)

using custom_clog=timing::ClockArray<double,double,time_getter,func_name,double_cast>;


TEST_F(ClockArrayTest, tik_function_call_chain)
{

    custom_clog clk;
    clk.tik();
    ASSERT_EQ(func_name_fake.call_count,1);
    ASSERT_EQ(time_getter_fake.call_count,1);
    FFF_FAKES_LIST(RESET_FAKE);
    FFF_RESET_HISTORY();
}

TEST_F(ClockArrayTest, tik_loc_function_call_chain)
{

    custom_clog clk;
    clk.tik_loc();
    ASSERT_EQ(func_name_fake.call_count,2);
    ASSERT_EQ(time_getter_fake.call_count,1);
    FFF_FAKES_LIST(RESET_FAKE);
    FFF_RESET_HISTORY();
}


TEST_F(ClockArrayTest, tak_function_call_chain)
{

    custom_clog clk;
    clk.tik();//todo check call history
    clk.tak();
    ASSERT_EQ(func_name_fake.call_count,2);
    ASSERT_EQ(time_getter_fake.call_count,2);
    ASSERT_EQ(double_cast_fake.call_count,1);
    FFF_FAKES_LIST(RESET_FAKE);
    FFF_RESET_HISTORY();
}