// proj_vk.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "demo.h"
#include "debug.h"

int main(int argc, const char** argv)
{
    if (argc < 2) {
        vkz::message(vkz::info, "Usage: %s [mesh]\n", argv[0]);
        return 1;
    }

    DemoMain();
    return 0;
}
