#include "background_reaper.h"

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <memory>
#include <thread>

#include <sys/time.h>

// 对象析构时睡眠 100 毫秒
struct StupidObject
{
    StupidObject() = default;

    ~StupidObject()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "析构..";
        fflush(stdout);
    }
};

struct State
{
    ~State() = default;

    StupidObject data_;
};

int main(int, char **)
{
    timeval start;
    timeval end;
    {
        gettimeofday(&start, nullptr);
        for (int i = 0; i < 10; i++)
        {
            auto s = std::make_unique<State>();
        }
        gettimeofday(&end, nullptr);
        std::cout << std::endl;
        std::cout << ((end.tv_sec - start.tv_sec) * 1000.) + ((end.tv_usec - start.tv_usec) / 1000.) << "ms" << std::endl;
        std::cout << std::endl;
    }

    std::cout << std::endl;
    std::cout << "========" << std::endl;
    std::cout << std::endl;

    auto reaper = [](State *ptr) {
        delete ptr;
    };
    auto bg_reaper = std::make_unique<BackgroundReaper<State, decltype(reaper)>>(std::move(reaper));

    auto deleter = [&](State *ptr) {
        bg_reaper->Reap(ptr);
    };

    {
        gettimeofday(&start, nullptr);
        // 循环创建由 bg_reaper 负责析构的智能指针
        for (int i = 0; i < 10; i++)
        {
            auto s = std::unique_ptr<State, decltype(deleter)>(new State, deleter);
        }
        bg_reaper->Notify();
        gettimeofday(&end, nullptr);

        std::cout << std::endl;
        std::cout << ((end.tv_sec - start.tv_sec) * 1000.) + ((end.tv_usec - start.tv_usec) / 1000.) << " ms" << std::endl;
        std::cout << std::endl;
    }
}
