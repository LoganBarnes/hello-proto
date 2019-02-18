#pragma once

#include <memory>
#include <unordered_map>

namespace net {
namespace detail {

enum class TagLabel {
    rpc_call_requested_by_client,
    processing,
    rpc_finished,
};

struct Tag {
    TagLabel label;
    void* data;
};

/**
 * @brief
 */
class Tagger {
public:
    void* make_tag(TagLabel label, void* data);
    std::pair<Tag, unsigned> get_tag(void* tag_id);

    unsigned count(void* data);

private:
    std::unordered_map<void*, std::unique_ptr<Tag>> tags_;
    std::unordered_map<void*, unsigned> counts_;
};

} // namespace detail
} // namespace net
