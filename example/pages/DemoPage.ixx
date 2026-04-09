module;

#include <format>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

export module DemoPage;

import Nandina.Core;
import Nandina.Window;
import Nandina.Layout;
import Nandina.Components.Label;
import Nandina.Types;
// ── DemoPage ──────────────────────────────────────────────────────────────────
// Demonstrates the three new reactive primitives added in this milestone:
//
//   1. Prop<T>       — static vs reactive property wrapper on a Label
//   2. StateList<T>  — observable list: push / update / remove reflected in UI
//   3. Upgraded Signal — connect_once and ScopedConnection
//
// Layout: three sections stacked vertically inside a FreeWidget.
//   • Section A (top):    Prop demo — static label + reactive label + toggle button
//   • Section B (middle): StateList demo — dynamic item labels + Add/Remove buttons
//   • Section C (bottom): Signal demo   — EventSignal + connect_once counter label
// ─────────────────────────────────────────────────────────────────────────────
export class DemoPage : public Nandina::Page {
public:
    static auto Create() -> std::unique_ptr<DemoPage>;

    auto build() -> Nandina::WidgetPtr override;

private:
    // ── Prop demo state ───────────────────────────────────────────────────────
    Nandina::State<std::string> reactive_text_{"(reactive initial)"};
    int toggle_count_ = 0;

    // ── StateList demo state ──────────────────────────────────────────────────
    Nandina::StateList<std::string> item_list_;

    // ── Signal demo state ─────────────────────────────────────────────────────
    Nandina::EventSignal<int> demo_signal_;
    int signal_fire_count_ = 0; // total fires
    int once_received_count_ = 0; // fires caught by connect_once slots

    // Persistent subscriptions kept alive for the page lifetime:
    //   prop_conn_       — Prop<string>::on_change (Section A)
    //   list_watch_conn_ — StateList fine-grained on_change (Section B)
    //   scoped_conn_     — EventSignal persistent subscription (Section C)
    Nandina::ScopedConnection prop_conn_;
    Nandina::ScopedConnection list_watch_conn_;
    Nandina::ScopedConnection scoped_conn_;

    // Layout helpers
    static constexpr float kPageW = 640.0f;
    static constexpr float kPageH = 480.0f;
    static constexpr float kMarginX = 24.0f;
    static constexpr float kLabelH = 24.0f;
    static constexpr float kBtnW = 130.0f;
    static constexpr float kBtnH = 32.0f;
};

auto DemoPage::Create() -> std::unique_ptr<DemoPage> {
    auto self = std::make_unique<DemoPage>();
    self->initialize();
    return self;
}

auto DemoPage::build() -> Nandina::WidgetPtr {
    auto root = std::make_unique<Nandina::FreeWidget>();
    root->move_to(0.0f, 0.0f).resize(kPageW, kPageH);
    root->set_background(0, 0, 0, 0);

    float y = 12.0f;
    const float contentW = kPageW - kMarginX * 2.0f;

    // ── Page title ────────────────────────────────────────────────────────────
    {
        auto lbl = Nandina::Label::Create();
        lbl->set_bounds(kMarginX, y, contentW, 32.0f);
        lbl->font_size(22.0f).text_color(180, 210, 255);
        lbl->text("Reactive Demo: Prop / StateList / Signal");
        root->add_child(std::move(lbl));
        y += 38.0f;
    }

    // ════════════════════════════════════════════════════════════════
    // Section A — Prop<std::string> demo
    // ════════════════════════════════════════════════════════════════
    {
        auto hdr = Nandina::Label::Create();
        hdr->set_bounds(kMarginX, y, contentW, kLabelH);
        hdr->font_size(14.0f).text_color(150, 230, 150);
        hdr->text("A) Prop<string>: static vs reactive");
        root->add_child(std::move(hdr));
        y += kLabelH + 4.0f;

        // Static Prop (never changes after construction)
        Nandina::Prop<std::string> static_prop{std::string{"[static] Hello, Nandina!"}};

        auto static_lbl = Nandina::Label::Create();
        static_lbl->set_bounds(kMarginX, y, contentW * 0.6f, kLabelH);
        static_lbl->font_size(13.0f).text_color(220, 220, 220);
        static_lbl->text(static_prop.get()); // read static value once
        root->add_child(std::move(static_lbl));
        y += kLabelH + 4.0f;

        // Reactive Prop — bound to reactive_text_
        Nandina::Prop<std::string> reactive_prop{reactive_text_};

        auto reactive_lbl = Nandina::Label::Create();
        reactive_lbl->set_bounds(kMarginX, y, contentW * 0.6f, kLabelH);
        reactive_lbl->font_size(13.0f).text_color(255, 220, 160);
        reactive_lbl->text(reactive_prop.get());
        auto *reactive_lbl_ptr = reactive_lbl.get();
        root->add_child(std::move(reactive_lbl));
        y += kLabelH + 4.0f;

        // Prop::on_change() presents a uniform subscription API regardless of whether
        // the prop is static or reactive. For a reactive prop it delegates to the
        // underlying State's on_change(); reactive_prop may go out of scope after wiring
        // because the Connection (stored in prop_conn_) is anchored to reactive_text_.
        prop_conn_ = Nandina::ScopedConnection{
            reactive_prop.on_change([reactive_lbl_ptr](const std::string& v) {
                reactive_lbl_ptr->text(v);
            })
        };

        // Toggle button — cycles through a few strings to show reactivity
        auto toggle_btn = Nandina::Button::Create();
        toggle_btn->set_bounds(kMarginX, y, kBtnW, kBtnH);
        toggle_btn->text("Toggle reactive text");
        toggle_btn->set_background(100, 140, 200);
        toggle_btn->on_click([this]() {
            const std::vector<std::string> options{
                "(reactive initial)",
                "Prop is reactive!",
                "State changed!",
                "Hello from toggle"
            };
            toggle_count_ = (toggle_count_ + 1) % static_cast<int>(options.size());
            reactive_text_.set(options[static_cast<std::size_t>(toggle_count_)]);
        });
        root->add_child(std::move(toggle_btn));
        y += kBtnH + 10.0f;
    }

    // ════════════════════════════════════════════════════════════════
    // Section B — StateList<std::string> demo
    // ════════════════════════════════════════════════════════════════
    {
        auto hdr = Nandina::Label::Create();
        hdr->set_bounds(kMarginX, y, contentW, kLabelH);
        hdr->font_size(14.0f).text_color(150, 230, 150);
        hdr->text("B) StateList<string>: push / update / remove");
        root->add_child(std::move(hdr));
        y += kLabelH + 4.0f;

        // We render up to 3 item labels side by side, then show a count.
        constexpr std::size_t kMaxVisible = 3;
        constexpr float kItemW = 140.0f;
        constexpr float kItemGap = 6.0f;

        // Pre-create label slots (reused as items change)
        std::array<Nandina::Label *, kMaxVisible> item_ptrs{};
        for (std::size_t i = 0; i < kMaxVisible; ++i) {
            auto lbl = Nandina::Label::Create();
            lbl->set_bounds(kMarginX + static_cast<float>(i) * (kItemW + kItemGap),
                            y, kItemW, kLabelH);
            lbl->font_size(12.0f).text_color(255, 200, 120);
            lbl->text("—");
            item_ptrs[i] = lbl.get();
            root->add_child(std::move(lbl));
        }
        y += kLabelH + 4.0f;

        // Count label: shows total count and "+N more" when list exceeds kMaxVisible
        auto count_lbl = Nandina::Label::Create();
        count_lbl->set_bounds(kMarginX, y, contentW * 0.7f, kLabelH);
        count_lbl->font_size(12.0f).text_color(180, 180, 180);
        count_lbl->text("items: 0");
        auto *count_lbl_ptr = count_lbl.get();
        root->add_child(std::move(count_lbl));
        y += kLabelH + 4.0f;

        // Fine-grained subscription with on_change(): Updated events only touch the
        // changed slot (O(1)); all other mutations re-sync all visible slots from the
        // current list. The callback also fires automatically for the seeded push_backs.
        list_watch_conn_ = Nandina::ScopedConnection{
            item_list_.on_change([this, item_ptrs, count_lbl_ptr](const Nandina::ListChange<std::string>& ch) {
                if (ch.kind == Nandina::ListChangeKind::Updated) {
                    if (ch.index < kMaxVisible && ch.value) {
                        item_ptrs[ch.index]->text(*ch.value);
                    }
                } else {
                    for (std::size_t i = 0; i < kMaxVisible; ++i) {
                        item_ptrs[i]->text(i < item_list_.size() ? item_list_[i] : "—");
                    }
                }
                const std::size_t total = item_list_.size();
                count_lbl_ptr->text(total > kMaxVisible
                    ? std::format("items: {}  (+{} more not shown)", total, total - kMaxVisible)
                    : std::format("items: {}", total));
            })
        };

        // Seed a couple of items — the on_change callback fires for each push_back
        item_list_.push_back("Alpha");
        item_list_.push_back("Beta");

        // Buttons row: Push / Update[0] / Remove Last
        constexpr float kBtnGap = 8.0f;
        float bx = kMarginX;

        auto push_btn = Nandina::Button::Create();
        push_btn->set_bounds(bx, y, kBtnW, kBtnH);
        push_btn->text("Push item");
        push_btn->set_background(80, 160, 100);
        // Use item_list_.size() each time so the label is always unique.
        push_btn->on_click([this]() {
            item_list_.push_back(std::format("Item {}", item_list_.size() + 1));
        });
        root->add_child(std::move(push_btn));
        bx += kBtnW + kBtnGap;

        auto update_btn = Nandina::Button::Create();
        update_btn->set_bounds(bx, y, kBtnW, kBtnH);
        update_btn->text("Update [0]");
        update_btn->set_background(80, 130, 180);
        update_btn->on_click([this]() {
            if (!item_list_.empty()) {
                item_list_.update_at(0, "[updated]");
            }
        });
        root->add_child(std::move(update_btn));
        bx += kBtnW + kBtnGap;

        auto remove_btn = Nandina::Button::Create();
        remove_btn->set_bounds(bx, y, kBtnW, kBtnH);
        remove_btn->text("Remove last");
        remove_btn->set_background(180, 80, 80);
        remove_btn->on_click([this]() {
            if (!item_list_.empty()) {
                item_list_.remove_at(item_list_.size() - 1);
            }
        });
        root->add_child(std::move(remove_btn));
        y += kBtnH + 10.0f;
    }

    // ════════════════════════════════════════════════════════════════
    // Section C — Upgraded Signal: connect_once + ScopedConnection
    // ════════════════════════════════════════════════════════════════
    {
        auto hdr = Nandina::Label::Create();
        hdr->set_bounds(kMarginX, y, contentW, kLabelH);
        hdr->font_size(14.0f).text_color(150, 230, 150);
        hdr->text("C) Signal: connect_once + ScopedConnection");
        root->add_child(std::move(hdr));
        y += kLabelH + 4.0f;

        // Status label shows: "fires: N  once-caught: M"
        auto status_lbl = Nandina::Label::Create();
        status_lbl->set_bounds(kMarginX, y, contentW * 0.7f, kLabelH);
        status_lbl->font_size(13.0f).text_color(230, 200, 255);
        status_lbl->text("fires: 0  once-caught: 0");
        auto *status_lbl_ptr = status_lbl.get();
        root->add_child(std::move(status_lbl));
        y += kLabelH + 4.0f;

        // Persistent subscription: receives every emit and updates the label.
        // signal_fire_count_ is pre-incremented in on_click before emit(), so v
        // already reflects the new total. once_received_count_ lags by one slot-fire
        // order; the connect_once callback (registered before each emit) corrects it.
        scoped_conn_ = Nandina::ScopedConnection{
            demo_signal_.connect([this, status_lbl_ptr](int v) {
                status_lbl_ptr->text(std::format("fires: {}  once-caught: {}  (last={})",
                                                 v, once_received_count_, v));
            })
        };

        // Fire button — pre-increments the fire counter, registers a connect_once slot,
        // then emits. The once slot fires after the persistent one, bumps
        // once_received_count_, and re-renders the label so both counters are in sync.
        auto fire_btn = Nandina::Button::Create();
        fire_btn->set_bounds(kMarginX, y, kBtnW, kBtnH);
        fire_btn->text("Fire signal");
        fire_btn->set_background(140, 80, 180);
        fire_btn->on_click([this, status_lbl_ptr]() {
            demo_signal_.connect_once([this, status_lbl_ptr](int v) {
                ++once_received_count_;
                status_lbl_ptr->text(std::format("fires: {}  once-caught: {}  (last={})",
                                                 v, once_received_count_, v));
            });
            demo_signal_.emit(++signal_fire_count_);
        });
        root->add_child(std::move(fire_btn));
    }

    return root;
}
