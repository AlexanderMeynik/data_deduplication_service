#include <iostream>

#include <ostream>
#include "./FileUtils/ServiceFileInterface.h"
#include <sstream>

int main() {
    std::stringstream ss{"1111111111111111"
                         "2222222222222222"
                         "3333333333333333"
                         "4444444444444444"
                         "5555555555555555"
                         "6666666666666666"
                         "7777777777777777"
                         "8888888888888888"
                         "9999999999999999"
                         "1111111111111111"
                         "2222222222222222"
                         "3333333333333333"
                         "4444444444444444"
                         "5555555555555555"
                         "6666666666666666"
                         "7777777777777777"
                         "8888888888888888"
                         "9999999999999999"
                         "1111111111111111"
                         "2222222222222222"
                         "3333333333333333"
                         "4444444444444444"
                         "5555555555555555"
                         "6666666666666666"
                         "7777777777777777"
                         "8888888888888888"
                         "9999999999999999"
                         "1111111111111111"
                         "2222222222222222"
                         "3333333333333333"
                         "4444444444444444"
                         "5555555555555555"};
    buf64 buf;
    ss >> buf;


    return 0;
}