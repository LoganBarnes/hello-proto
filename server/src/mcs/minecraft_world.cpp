#include "minecraft_world.hpp"

namespace mcs {
namespace {

std::tuple<int, int, int> from_proto(const minecraft::IVec3& vec) {
    return std::make_tuple(vec.x(), vec.y(), vec.z());
}

minecraft::Block adjacent_block(const minecraft::Block& existing_block,
                                const minecraft::BlockFace& adjacent_direction) {
    minecraft::Block result = existing_block;

    switch (adjacent_direction) {

    case minecraft::POS_X:
        result.mutable_position()->set_x(result.position().x() + 1);
        break;
    case minecraft::POS_Y:
        result.mutable_position()->set_y(result.position().y() + 1);
        break;
    case minecraft::POS_Z:
        result.mutable_position()->set_z(result.position().z() + 1);
        break;
    case minecraft::NEG_X:
        result.mutable_position()->set_x(result.position().x() - 1);
        break;
    case minecraft::NEG_Y:
        result.mutable_position()->set_y(result.position().y() - 1);
        break;
    case minecraft::NEG_Z:
        result.mutable_position()->set_z(result.position().z() - 1);
        break;

    case minecraft::BlockFace_INT_MIN_SENTINEL_DO_NOT_USE_:
        break;
    case minecraft::BlockFace_INT_MAX_SENTINEL_DO_NOT_USE_:
        break;
    }

    return result;
}

} // namespace

MinecraftWorld::MinecraftWorld() {
    // Start with a single block at the origin
    blocks_.emplace(std::make_tuple(0, 0, 0));
}

minecraft::Errors MinecraftWorld::add_adjacent_block(const minecraft::Block& existing_block,
                                                     const minecraft::BlockFace& adjacent_direction,
                                                     minecraft::Block* adj_block) {

    auto block_tuple = from_proto(existing_block.position());

    if (blocks_.find(block_tuple) == blocks_.end()) {
        minecraft::Errors errors{};
        errors.set_error_message("Existing block does not exist");
        return errors;
    }

    *adj_block = adjacent_block(existing_block, adjacent_direction);
    auto adj_tuple = from_proto(adj_block->position());

    if (blocks_.find(adj_tuple) != blocks_.end()) {
        minecraft::Errors errors{};
        errors.set_error_message("Adjacent block already exists");
        return errors;
    }

    blocks_.emplace(adj_tuple);

    return {};
}

const std::unordered_set<std::tuple<int, int, int>>& MinecraftWorld::blocks() const {
    return blocks_;
}

} // namespace mcs
