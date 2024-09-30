#include "commont.h"

#define FAKE_FUNC_CLOCK_ARR
DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(timepoint_type,time_getter);


FAKE_VALUE_FUNC(location_type ,src_to_loc_type,std::source_location);

FAKE_VALUE_FUNC(double ,double_cast,timepoint_type,timepoint_type);


#define FFF_FAKES_LIST(FAKE)            \
  FAKE(double_cast)                    \
  FAKE(src_to_loc_type)                   \
  FAKE(time_getter)



class ClockArrayInterfaceTest : public ::testing::Test {
public:
public:
    void SetUp() override
    {
        FFF_FAKES_LIST(RESET_FAKE);
        FFF_RESET_HISTORY();
    }
protected:
    using custom_clog=timing::ClockArray<double,timepoint_type ,time_getter,src_to_loc_type,double_cast>;

    custom_clog clk;

};

TEST_F(ClockArrayInterfaceTest, tik_function_call_chain)
{

    custom_clog clk;
    clk.tik();
    ASSERT_EQ(src_to_loc_type_fake.call_count,1);
    ASSERT_EQ(time_getter_fake.call_count,1);

    ASSERT_HISTORY_EQ(0, src_to_loc_type);
    ASSERT_HISTORY_EQ(1, time_getter);
}

TEST_F(ClockArrayInterfaceTest, tik_loc_function_call_chain)
{

    custom_clog clk;
    clk.tik_loc();
    ASSERT_EQ(src_to_loc_type_fake.call_count,2);
    ASSERT_EQ(time_getter_fake.call_count,1);
    ASSERT_HISTORY_EQ(0, src_to_loc_type);
    ASSERT_HISTORY_EQ(1, time_getter);
    ASSERT_HISTORY_EQ(2, src_to_loc_type);

   }


TEST_F(ClockArrayInterfaceTest, tak_function_call_chain)
{

    custom_clog clk;
    clk.tik();
    clk.tak();
    ASSERT_EQ(src_to_loc_type_fake.call_count,2);
    ASSERT_EQ(time_getter_fake.call_count,2);
    ASSERT_EQ(double_cast_fake.call_count,1);


    ASSERT_HISTORY_EQ(0, src_to_loc_type);
    ASSERT_HISTORY_EQ(1, time_getter);
    ASSERT_HISTORY_EQ(2, src_to_loc_type);
    ASSERT_HISTORY_EQ(3, time_getter);
    ASSERT_HISTORY_EQ(4, double_cast);
}


TEST_F(ClockArrayInterfaceTest, file_location_proper_pass)
{
    src_to_loc_type_fake.custom_fake=get_file_state;
    auto source_location=clk.tik_loc();



    auto return_vals=src_to_loc_type_fake.return_val_history[1];
    auto arguments_val=src_to_loc_type_fake.arg0_history[1];

    ASSERT_EQ(source_location, return_vals);
    ASSERT_EQ(source_location, get_file_state(arguments_val));
    src_to_loc_type_reset();

}