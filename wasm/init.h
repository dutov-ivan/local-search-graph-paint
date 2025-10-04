#pragma once
#include <random>
#include <iostream>

struct Init
{
    Init(unsigned int seed);
    std::mt19937 &getRng();

private:
    std::random_device rd_;
    std::mt19937 rng_;
};

// Declare the global object defined elsewhere
extern Init init;
