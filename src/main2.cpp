#include<iostream>
#include<array>
#include "ClockArray.h"
#include <unordered_map>
#include <vector>
#include <thread>
#include <unistd.h>
using namespace std::chrono_literals;
using CLOCK =timing::chrono_clock_template<std::chrono::milliseconds>;
void func_1(CLOCK &cl)
{
    cl.tik();
    cl.tik();
    std::this_thread::sleep_for(120ms);
    cl.tak();
    std::this_thread::sleep_for(120ms);
    cl.tak();
}
void func_2(CLOCK &cl)
{
    std::this_thread::sleep_for(250ms);
    cl.tak();
}
int main(){
    CLOCK cl;
    cl.tik();
    for (int i = 0; i < 3; ++i) {
        cl.tik();
        std::this_thread::sleep_for(250ms);
        cl.tak();
        cl.tik();
        std::this_thread::sleep_for(120ms);
        cl.tak();
    }
    cl.tak();

/*func_2(cl);*/
for(const auto& p:cl)
    std::cout<<p.first[0]<<'\t'<<p.first[1]<<'\t'<<p.second<<'\n';
}
