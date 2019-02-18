#include "tagger.hpp"

// generated
#include "testing/hello_tests.hpp"

// standard
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

unsigned Tagger::count(void* data) {
    if (counts_.find(data) == counts_.end()) {
        return 0u;
    }
    return counts_.at(data);
}

#ifdef DOCTEST_LIBRARY_INCLUDED
struct TestTag {
    int unused = 1;
    void* data = &unused;
    void* tag = nullptr;
};

TEST_CASE("[net] test Tagger") {
    Tagger tagger;

    TestTag data1{};
    TestTag data2 = data1;
    TestTag data3{};
    REQUIRE(data2.data == data1.data);

    // counts increment correctly
    {
        data1.tag = tagger.make_tag(TagLabel::rpc_finished, data1.data);
        CHECK(tagger.count(data1.data) == 1);

        data2.tag = tagger.make_tag(TagLabel::processing, data2.data);
        CHECK(tagger.count(data2.data) == 2);

        data3.tag = tagger.make_tag(TagLabel::rpc_call_requested_by_client, data3.data);
        CHECK(tagger.count(data3.data) == 1);
    }

    // data is correct
    {
        Tag tag{};
        unsigned count;

        std::tie(tag, count) = tagger.get_tag(data3.tag);
        CHECK(tag.label == TagLabel::rpc_call_requested_by_client);
        CHECK(tag.data == data3.data);
        CHECK(count == 0);

        std::tie(tag, count) = tagger.get_tag(data1.tag);
        CHECK(tag.label == TagLabel::rpc_finished);
        CHECK(tag.data == data1.data);
        CHECK(count == 1);

        std::tie(tag, count) = tagger.get_tag(data2.tag);
        CHECK(tag.label == TagLabel::processing);
        CHECK(tag.data == data2.data);
        CHECK(count == 0);
    }

    // invalid input
    {
        CHECK_THROWS(tagger.get_tag(nullptr));
        CHECK(tagger.count(nullptr) == 0);
    }
}
#endif

} // namespace detail
} // namespace net
