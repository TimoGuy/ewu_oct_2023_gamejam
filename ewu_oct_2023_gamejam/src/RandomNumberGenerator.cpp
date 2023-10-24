#include "RandomNumberGenerator.h"

#include <random>


namespace rng
{
    std::default_random_engine generator;

    float_t randomReal()
    {
        return randomRealRange(0.0f, 1.0f);
    }

    float_t randomRealRange(float_t min, float_t max)
    {
		std::uniform_real_distribution<float_t> distribution(min, max);
        return distribution(generator);
    }
}
