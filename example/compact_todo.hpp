//
// Compact Todo — recommended single-page application example.
//

#ifndef NANDINA_EXPERIMENT_EXAMPLE_COMPACT_TODO_HPP
#define NANDINA_EXPERIMENT_EXAMPLE_COMPACT_TODO_HPP

#include "app/nan_page.hpp"
#include "app/nan_store.hpp"
#include "reactive/signal.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::examples::compact_todo
{
    struct Task {
        std::uint64_t id = 0;
        std::string title;
        bool completed = false;

        auto operator==(const Task&) const -> bool = default;
    };

    class Store final: public app::NanStore {
    public:
        explicit Store(reactive::Graph& graph);

        [[nodiscard]] auto add(std::string_view title) -> bool;
        void toggle(std::uint64_t id);
        void remove(std::uint64_t id);

        reactive::Signal<std::vector<Task>> tasks;

    private:
        std::uint64_t next_id_ = 3;
    };

    [[nodiscard]] auto build(app::PageContext& context) -> std::shared_ptr<scene::NanNode2D>;
} // namespace nandina::examples::compact_todo

#endif // NANDINA_EXPERIMENT_EXAMPLE_COMPACT_TODO_HPP
