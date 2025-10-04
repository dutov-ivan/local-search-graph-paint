#include "init.h"

Init::Init(unsigned int seed)
    : rng_(seed)
{
    std::cout << "RNG initialized with seed " << seed << std::endl;
}

std::mt19937 &Init::getRng()
{
    return rng_;
}

// Define the global object
Init init(42);
