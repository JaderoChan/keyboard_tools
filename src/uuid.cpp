#include "uuid.hpp"

#include <random>

namespace kbdt
{

namespace details
{

static uint64_t genUuid()
{
    std::random_device rd;
    std::mt19937_64 mt(rd());
    std::uniform_int_distribution<uint64_t> dis;
    return dis(mt);
}

uint64_t uuid()
{
    static auto uuid = genUuid();
    return uuid;
}

} // namespace details

} // namespace kbdt
