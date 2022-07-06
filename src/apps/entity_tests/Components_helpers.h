#pragma once


#include <string>
#include <vector>


struct ComponentA
{
    double d;
};


struct ComponentB
{
    std::string str;
};


struct ComponentC
{
    std::vector<int> vec;
};

struct ComponentEmpty
{
    // empty
};
