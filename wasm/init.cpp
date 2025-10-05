#include "init.h"
#include "algorithms.h" // Include for complete types

Init::Init(unsigned int seed)
    : rng_(seed)
{
    std::cout << "RNG initialized with seed " << seed << std::endl;
}

std::mt19937 &Init::getRng()
{
    return rng_;
}

// GlobalState implementations
GlobalState::GlobalState() : algorithm(nullptr), initialStateNode(nullptr) {}

GlobalState::~GlobalState() = default; // Now we have complete types available

// Define the global objects
Init init(42);
GlobalState globalState;
