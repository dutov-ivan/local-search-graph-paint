#pragma once
#include <random>
#include <iostream>
#include <memory>

// Forward declarations
struct Algorithm;
struct StateNode;

struct Init
{
    Init(unsigned int seed);
    std::mt19937 &getRng();

private:
    std::random_device rd_;
    std::mt19937 rng_;
};

// Global state
struct GlobalState
{
    std::unique_ptr<Algorithm> algorithm;
    std::unique_ptr<StateNode> initialStateNode;
    int iterationCount = 0;

    GlobalState();
    ~GlobalState(); // Custom destructor to handle incomplete types
};

// Declare the global objects defined elsewhere
extern Init init;
extern GlobalState globalState;
