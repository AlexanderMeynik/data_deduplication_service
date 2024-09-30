
#include "testClasses.h";
#define FileServiceTest
//todo compare files/directories



TEST_F(ServiceFileTest,test_created_db_acess)
{

}

int main(int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);
    google::SetVLOGLevel("*",3);
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);


    return RUN_ALL_TESTS();
}