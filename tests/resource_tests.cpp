#include <catch2/catch_test_macros.hpp>

#include "resource/backends/directory_backend.hpp"
#include "resource/backends/memory_backend.hpp"
#include "resource/backends/sqlite_backend.hpp"
#include "resource/resource_manager.hpp"

#include <sqlite3.h>

#include <filesystem>
#include <fstream>

using namespace nandina;

namespace
{
    [[nodiscard]] auto id(std::string_view value) -> resource::ResourceId {
        return *resource::ResourceId::parse(value);
    }

    struct TempDirectory {
        std::filesystem::path path = std::filesystem::temp_directory_path()
            / ("nandina-resource-" + resource::ResourceId::random().to_string());
        TempDirectory() { std::filesystem::create_directories(path); }
        ~TempDirectory() { std::error_code error; std::filesystem::remove_all(path, error); }
    };

    void exec(sqlite3* db, const char* sql) {
        char* message = nullptr;
        const int result = sqlite3_exec(db, sql, nullptr, nullptr, &message);
        if (message) { sqlite3_free(message); }
        REQUIRE(result == SQLITE_OK);
    }

    void create_database(const std::filesystem::path& path) {
        sqlite3* db = nullptr;
        REQUIRE(sqlite3_open(path.string().c_str(), &db) == SQLITE_OK);
        exec(db, "PRAGMA application_id=1312902724; PRAGMA user_version=1;");
        exec(db,
            "CREATE TABLE resources("
            "id BLOB PRIMARY KEY, resource_key TEXT NOT NULL UNIQUE, media_type TEXT NOT NULL,"
            "storage INTEGER NOT NULL, data BLOB, external_path TEXT, size INTEGER NOT NULL);"
            "CREATE TABLE aliases(alias TEXT PRIMARY KEY, resource_id BLOB NOT NULL);"
        );
        exec(db,
            "INSERT INTO resources VALUES("
            "X'00112233445546778899aabbccddeeff','fonts/default','font/ttf',0,X'010203',NULL,3);"
            "INSERT INTO aliases VALUES('fonts/ui',X'00112233445546778899aabbccddeeff');"
            "INSERT INTO resources VALUES("
            "X'10112233445546778899aabbccddeeff','shaders/basic','text/plain',1,NULL,'basic.glsl',4);"
        );
        REQUIRE(sqlite3_close(db) == SQLITE_OK);
    }
}

TEST_CASE("resource identity and keys are canonical", "[resource]") {
    const auto value = id("00112233-4455-4677-8899-aabbccddeeff");
    REQUIRE(value.to_string() == "00112233-4455-4677-8899-aabbccddeeff");
    REQUIRE_FALSE(value.is_nil());
    REQUIRE_FALSE(resource::ResourceId::parse("00112233-4455-4677-8899-AABBCCDDEEFF"));
    REQUIRE(resource::ResourceKey::parse("fonts/ui/default").has_value());
    REQUIRE_FALSE(resource::ResourceKey::parse("fonts/../secret"));
    REQUIRE_FALSE(resource::ResourceKey::parse("Fonts/Default"));
    const auto random = resource::ResourceId::random();
    REQUIRE((random.bytes()[6] & 0xf0U) == 0x40U);
    REQUIRE((random.bytes()[8] & 0xc0U) == 0x80U);
}

TEST_CASE("memory resources survive replacement and manager overlays", "[resource]") {
    const auto resource_id = id("00112233-4455-4677-8899-aabbccddeeff");
    auto base = std::make_shared<resource::MemoryBackend>("base");
    auto override = std::make_shared<resource::MemoryBackend>("override");
    auto old = base->insert(resource_id, resource::ResourceKey("fonts/default"), "font/ttf", {1, 2});
    REQUIRE(old.has_value());
    REQUIRE(base->add_alias(resource::ResourceKey("fonts/ui"), resource_id).has_value());
    REQUIRE(override->insert(resource_id, resource::ResourceKey("fonts/default"), "font/ttf", {9}).has_value());

    resource::ResourceManager manager;
    const auto base_mount = manager.mount(base, 0);
    const auto override_mount = manager.mount(override, 10);
    REQUIRE(manager.require(resource::ResourceKey("fonts/default"))->get()->bytes()[0] == 9);
    REQUIRE(manager.require(resource::ResourceKey("fonts/ui"))->get()->bytes()[0] == 1);
    REQUIRE(manager.unmount(override_mount));
    REQUIRE(manager.require(resource::ResourceKey("fonts/default"))->get()->bytes()[0] == 1);

    REQUIRE(base->insert(
        resource_id, resource::ResourceKey("fonts/default"), "font/ttf", {3},
        resource::InsertMode::replace
    ).has_value());
    REQUIRE((*old)->bytes()[0] == 1);
    REQUIRE(manager.unmount(base_mount));
    REQUIRE((*old)->bytes()[1] == 2);
}

TEST_CASE("directory backend loads manifest resources into owned handles", "[resource]") {
    TempDirectory temp;
    { std::ofstream file(temp.path / "asset.bin", std::ios::binary); file.write("abc", 3); }
    const auto resource_id = id("00112233-4455-4677-8899-aabbccddeeff");
    auto backend = resource::DirectoryBackend::open({
        .root = temp.path,
        .resources = {{
            .id = resource_id,
            .key = resource::ResourceKey("data/asset"),
            .relative_path = "asset.bin",
            .media_type = "application/octet-stream",
            .aliases = {resource::ResourceKey("data/default")},
            .expected_size = 3,
        }},
    });
    REQUIRE(backend.has_value());
    auto found = (*backend)->find(resource::ResourceKey("data/default"));
    REQUIRE(found.has_value());
    REQUIRE((**found)->size() == 3);
    std::filesystem::remove(temp.path / "asset.bin");
    (*backend).reset();
    REQUIRE((**found)->bytes()[2] == static_cast<std::uint8_t>('c'));
}

TEST_CASE("SQLite backend resolves blobs aliases and external files", "[resource][sqlite]") {
    TempDirectory temp;
    const auto database = temp.path / "resources.db";
    create_database(database);
    { std::ofstream file(temp.path / "basic.glsl", std::ios::binary); file.write("main", 4); }
    auto backend = resource::SQLiteBackend::open({.database = database});
    REQUIRE(backend.has_value());

    auto blob = (*backend)->find(resource::ResourceKey("fonts/ui"));
    REQUIRE(blob.has_value());
    REQUIRE((**blob)->key().value() == "fonts/default");
    REQUIRE((**blob)->storage() == resource::ResourceStorage::embedded_blob);
    REQUIRE((**blob)->bytes()[2] == 3);

    auto external = (*backend)->find(resource::ResourceKey("shaders/basic"));
    REQUIRE(external.has_value());
    REQUIRE((**external)->storage() == resource::ResourceStorage::external_file);
    REQUIRE((**external)->size() == 4);

    std::filesystem::remove(database);
    (*backend).reset();
    REQUIRE((**blob)->bytes()[0] == 1);
}

TEST_CASE("SQLite backend rejects non-Nandina databases", "[resource][sqlite]") {
    TempDirectory temp;
    const auto database = temp.path / "foreign.db";
    sqlite3* db = nullptr;
    REQUIRE(sqlite3_open(database.string().c_str(), &db) == SQLITE_OK);
    exec(db, "CREATE TABLE unrelated(value INTEGER);");
    REQUIRE(sqlite3_close(db) == SQLITE_OK);
    const auto backend = resource::SQLiteBackend::open({.database = database});
    REQUIRE_FALSE(backend.has_value());
    REQUIRE(backend.error().code == resource::ResourceErrorCode::malformed_backend);
}
