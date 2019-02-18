#include "tagger.hpp"

#include <cassert>

namespace net {
namespace detail {

void* Tagger::make_tag(TagLabel label, void* data) {
    auto tag = std::make_unique<Tag>(Tag{label, data});
    void* tag_id = tag.get();
    tags_.emplace(tag_id, std::move(tag));
    counts_[data] += 1;
    return tag_id;
}

std::pair<Tag, unsigned> Tagger::get_tag(void* tag_id) {
    Tag tag = *tags_.at(tag_id);
    tags_.erase(tag_id);

    counts_.at(tag.data) -= 1;
    unsigned count = counts_.at(tag.data);

    if (count == 0) {
        counts_.erase(tag.data);
    }

    return std::make_pair(tag, count);
}

bool Tagger::has_data(void* data) {
    return counts_.find(data) != counts_.end();
}

} // namespace detail
} // namespace net
