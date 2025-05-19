#include "uuid.hpp"
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <chrono>
#include <random>

std::string
GenerateUuid()
{
        std::stringstream uuid;

        std::random_device rd;
        std::mt19937 rng(rd());
        std::uniform_int_distribution<> udist;

        auto now = std::chrono::system_clock::now();
        auto since_epoch = now.time_since_epoch();
        auto nanos_since_epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(since_epoch);

        // TODO: Seed 'rng' differently. It doesn't really
        //       make sense to seed RNG with time, and also
        //       use timestamp afterwards.
        rng.seed(static_cast<unsigned int>(nanos_since_epoch.count()));

        uuid << std::hex;
        
        uuid << udist(rng) << "_" << udist(rng) << "_" << nanos_since_epoch.count();

        // TODO: Consider hashing the whole thing

        return uuid.str();
}
