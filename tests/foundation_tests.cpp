//
// Foundation-layer UTF-8 tests.
//

#include "foundation/utf8.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>

using namespace nandina;

TEST_CASE("UTF-8 encodes Unicode scalar values", "[foundation][utf8]") {
    REQUIRE(foundation::utf8::encode(U'A') == "A");
    REQUIRE(foundation::utf8::encode(U'中') == "\xE4\xB8\xAD");
    REQUIRE(foundation::utf8::encode(U'🙂') == "\xF0\x9F\x99\x82");
    REQUIRE(foundation::utf8::encode(static_cast<char32_t>(0xD800)) == "\xEF\xBF\xBD");
}

TEST_CASE("UTF-8 helpers preserve codepoint boundaries", "[foundation][utf8]") {
    const std::string text = "A中🙂";

    REQUIRE(foundation::utf8::codepoint_count(text) == 3);
    REQUIRE(foundation::utf8::next_boundary(text, 0) == 1);
    REQUIRE(foundation::utf8::next_boundary(text, 1) == 4);
    REQUIRE(foundation::utf8::next_boundary(text, 4) == 8);
    REQUIRE(foundation::utf8::previous_boundary(text, 8) == 4);
    REQUIRE(foundation::utf8::previous_boundary(text, 4) == 1);
    REQUIRE(foundation::utf8::clamp_boundary(text, 2) == 1);
    REQUIRE(foundation::utf8::byte_offset_for_codepoints(text, 2) == 4);
}

TEST_CASE("UTF-8 decode preserves source byte ranges", "[foundation][utf8]") {
    const std::string text = "A中🙂";
    const auto decoded = foundation::utf8::decode(text);

    REQUIRE(decoded.size() == 3);
    REQUIRE(decoded[0].value == U'A');
    REQUIRE(decoded[0].byte_offset == 0);
    REQUIRE(decoded[0].byte_length == 1);
    REQUIRE(decoded[1].value == U'中');
    REQUIRE(decoded[1].byte_offset == 1);
    REQUIRE(decoded[1].byte_length == 3);
    REQUIRE(decoded[2].value == U'🙂');
    REQUIRE(decoded[2].byte_offset == 4);
    REQUIRE(decoded[2].byte_length == 4);

    const auto malformed = foundation::utf8::decode("\xFF" "A");
    REQUIRE(malformed.size() == 2);
    REQUIRE(malformed[0].value == foundation::utf8::replacement_character);
    REQUIRE(malformed[0].byte_length == 1);
    REQUIRE(malformed[1].value == U'A');
}

TEST_CASE("UTF-8 grapheme ranges follow UAX 29 boundaries", "[foundation][utf8][grapheme]") {
    const std::string combining = "a\xCC\x81" "b";
    const auto combining_ranges = foundation::utf8::grapheme_ranges(combining);
    REQUIRE(combining_ranges.size() == 2);
    REQUIRE(combining_ranges[0].offset == 0);
    REQUIRE(combining_ranges[0].length == 3);
    REQUIRE(combining_ranges[1].offset == 3);

    const std::string emoji_zwj = "👩‍💻";
    const auto emoji_ranges = foundation::utf8::grapheme_ranges(emoji_zwj);
    REQUIRE(emoji_ranges.size() == 1);
    REQUIRE(emoji_ranges.front().length == emoji_zwj.size());

    const std::string flag = "🇨🇳";
    const auto flag_ranges = foundation::utf8::grapheme_ranges(flag);
    REQUIRE(flag_ranges.size() == 1);
    REQUIRE(flag_ranges.front().length == flag.size());
}
