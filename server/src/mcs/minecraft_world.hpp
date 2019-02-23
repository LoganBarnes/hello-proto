#pragma once

// generated
#include <minecraft/components.pb.h>
#include <minecraft/requests.pb.h>
#include <minecraft/world.pb.h>

// standard
#include <tuple>
#include <unordered_set>

namespace detail {

template <class T>
void hash_combine(std::size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

} // namespace detail

namespace std {

template <>
struct hash<std::tuple<int, int, int>> {
    size_t operator()(const std::tuple<int, int, int>& t) const {
        size_t seed = 0;
        detail::hash_combine(seed, std::get<0>(t));
        detail::hash_combine(seed, std::get<1>(t));
        detail::hash_combine(seed, std::get<2>(t));
        return seed;
    }
};

} // namespace std

namespace mcs {

class MinecraftWorld {
public:
    MinecraftWorld();

    minecraft::Errors add_adjacent_block(const minecraft::Block& existing_block,
                                         const minecraft::BlockFace& adjacent_direction,
                                         minecraft::Block* adj_block);

    const std::unordered_set<std::tuple<int, int, int>>& blocks() const;

private:
    std::unordered_set<std::tuple<int, int, int>> blocks_;
};

} // namespace mcs
