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
