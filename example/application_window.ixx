module;

#include <format>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

export module ApplicationWindow;

import Nandina.Core;
import Nandina.Window;
import Nandina.Layout;
import Nandina.Components.Label;
import Nandina.Types;

// ── CounterPage ───────────────────────────────────────────────────────────────
// Demonstrates State + Effect reactive binding inside a Page.
// Pressing "+1" / "-1" updates a count_, which the Effect propagates to the label.
// Uses FlexLayout (Expanded + Spacer) to demonstrate the two-pass layout algorithm.
// ─────────────────────────────────────────────────────────────────────────────
class CounterPage : public Nandina::Page {
public:
    static auto Create() -> std::unique_ptr<CounterPage>;

    auto build() -> Nandina::WidgetPtr override;

private:
    Nandina::State<int> count_{0};
};

auto CounterPage::Create() -> std::unique_ptr<CounterPage> {
    auto self = std::make_unique<CounterPage>();
    self->initialize();
    return self;
}

auto CounterPage::build() -> Nandina::WidgetPtr {
    constexpr float page_width = 640.0f;
    constexpr float page_height = 480.0f;
    constexpr float title_x = 40.0f;
    constexpr float title_y = 24.0f;
    constexpr float title_width = 560.0f;
    constexpr float title_height = 48.0f;
    constexpr float number_width = 240.0f;
    constexpr float number_height = 64.0f;
    constexpr float number_x = (page_width - number_width) * 0.5f;
    constexpr float number_y = (page_height - number_height) * 0.5f;
    constexpr float button_width = 120.0f;
    constexpr float button_height = 60.0f;
    constexpr float button_gap = 16.0f;
    constexpr float button_row_width = button_width * 2.0f + button_gap;
    constexpr float button_row_x = (page_width - button_row_width) * 0.5f;
    constexpr float button_row_y = number_y + number_height + 24.0f;

    auto root = std::make_unique<Nandina::FreeWidget>();
    root->move_to(0.0f, 0.0f).resize(page_width, page_height);
    root->set_background(0, 0, 0, 0);

    auto title_label = Nandina::Label::Create();
    title_label->set_background(0, 0, 0, 0);
    title_label->set_bounds(title_x, title_y, title_width, title_height);
    title_label->font_size(24.0f).text_color(200, 200, 255);
    title_label->text("Nandina Counter");
    root->add_child(std::move(title_label));

    auto number_label = Nandina::Label::Create();
    number_label->set_background(0, 0, 0, 0);
    number_label->set_bounds(number_x, number_y, number_width, number_height);
    number_label->font_size(48.0f).text_color(220, 240, 232);
    number_label->text("0");
    auto* number_label_ptr = number_label.get();
    root->add_child(std::move(number_label));

    this->effect([this, number_label_ptr]() {
        number_label_ptr->text(std::format("{}", count_.get()));
    });


    auto decrease_button = Nandina::Button::Create();
    decrease_button->set_bounds(button_row_x, button_row_y, button_width, button_height);
    decrease_button->text("-1");
    decrease_button->set_background(234, 153, 156);
    decrease_button->on_click([this]() {
        count_.set(count_.get() - 1);
    });

    root->add_child(std::move(decrease_button));

    auto increase_button = Nandina::Button::Create();
    increase_button->set_bounds(button_row_x + button_width + button_gap,
                                button_row_y,
                                button_width,
                                button_height);
    increase_button->text("+1");
    increase_button->set_background(234, 153, 156);
    increase_button->on_click([this]() {
        count_.set(count_.get() + 1);
    });


    root->add_child(std::move(increase_button));

    return root;
}

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
class DemoPage : public Nandina::Page {
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
    int signal_fire_count_ = 0;         // total fires
    int once_received_count_ = 0;       // fires caught by connect_once slots

    // Persistent subscriptions kept alive for the page lifetime
    Nandina::ScopedConnection scoped_conn_;
    Nandina::ScopedConnection list_watch_conn_;

    // Layout helpers
    static constexpr float kPageW    = 640.0f;
    static constexpr float kPageH    = 480.0f;
    static constexpr float kMarginX  = 24.0f;
    static constexpr float kLabelH   = 24.0f;
    static constexpr float kBtnW     = 130.0f;
    static constexpr float kBtnH     = 32.0f;
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
        Nandina::Prop<std::string> static_prop{ std::string{"[static] Hello, Nandina!"} };

        auto static_lbl = Nandina::Label::Create();
        static_lbl->set_bounds(kMarginX, y, contentW * 0.6f, kLabelH);
        static_lbl->font_size(13.0f).text_color(220, 220, 220);
        static_lbl->text(static_prop.get());   // read static value once
        root->add_child(std::move(static_lbl));
        y += kLabelH + 4.0f;

        // Reactive Prop — bound to reactive_text_
        Nandina::Prop<std::string> reactive_prop{ reactive_text_ };

        auto reactive_lbl = Nandina::Label::Create();
        reactive_lbl->set_bounds(kMarginX, y, contentW * 0.6f, kLabelH);
        reactive_lbl->font_size(13.0f).text_color(255, 220, 160);
        reactive_lbl->text(reactive_prop.get());
        auto* reactive_lbl_ptr = reactive_lbl.get();
        root->add_child(std::move(reactive_lbl));
        y += kLabelH + 4.0f;

        // Keep the reactive Prop alive via an Effect that mirrors changes to the label.
        // Prop::on_change returns a Connection when reactive; we store it in scoped_conn_.
        // (We also subscribe via Effect for demonstration.)
        this->effect([this, reactive_lbl_ptr]() {
            // Effect re-runs automatically when reactive_text_ changes.
            reactive_lbl_ptr->text(reactive_text_.get());
        });

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
        std::array<Nandina::Label*, kMaxVisible> item_ptrs{};
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

        // Count label
        auto count_lbl = Nandina::Label::Create();
        count_lbl->set_bounds(kMarginX, y, contentW * 0.5f, kLabelH);
        count_lbl->font_size(12.0f).text_color(180, 180, 180);
        count_lbl->text("items: 0");
        auto* count_lbl_ptr = count_lbl.get();
        root->add_child(std::move(count_lbl));
        y += kLabelH + 4.0f;

        // Helper lambda to refresh the visible slots from item_list_
        auto refresh_slots = [this, item_ptrs, count_lbl_ptr]() {
            for (std::size_t i = 0; i < kMaxVisible; ++i) {
                if (i < item_list_.size()) {
                    item_ptrs[i]->text(item_list_[i]);
                } else {
                    item_ptrs[i]->text("—");
                }
            }
            count_lbl_ptr->text(std::format("items: {}", item_list_.size()));
        };

        // Subscribe with watch() (coarse-grained) — just rebuild all visible slots.
        // Store in a ScopedConnection so the subscription lives for the page lifetime.
        list_watch_conn_ = Nandina::ScopedConnection{
            item_list_.watch([refresh_slots]() { refresh_slots(); })
        };

        // Seed a couple of items
        item_list_.push_back("Alpha");
        item_list_.push_back("Beta");
        refresh_slots();  // sync initial state after subscriptions

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
        auto* status_lbl_ptr = status_lbl.get();
        root->add_child(std::move(status_lbl));
        y += kLabelH + 4.0f;

        // ScopedConnection: persistent subscription that updates the status label.
        scoped_conn_ = Nandina::ScopedConnection{
            demo_signal_.connect([this, status_lbl_ptr](int v) {
                ++signal_fire_count_;
                status_lbl_ptr->text(std::format("fires: {}  once-caught: {}  (last={})",
                                                  signal_fire_count_, once_received_count_, v));
            })
        };

        // Fire button — emits demo_signal_ with an incrementing value.
        // Before each emit, registers a connect_once slot so we can count
        // how many fires reach a one-shot listener.
        auto fire_btn = Nandina::Button::Create();
        fire_btn->set_bounds(kMarginX, y, kBtnW, kBtnH);
        fire_btn->text("Fire signal");
        fire_btn->set_background(140, 80, 180);
        fire_btn->on_click([this]() {
            // Register a one-shot listener just before firing — it fires once then auto-disconnects.
            demo_signal_.connect_once([this](int) {
                ++once_received_count_;
            });
            demo_signal_.emit(signal_fire_count_ + 1);
        });
        root->add_child(std::move(fire_btn));
    }

    return root;
}

// ── ApplicationWindow ─────────────────────────────────────────────────────────
// Uses the multi-layer RenderLayer architecture:
//   Layer 0 (World)   — dark background FreeWidget
//   Layer 1 (UI)      — RouterView with DemoPage (showcasing new primitives)
//   Layer 2 (Overlay) — semi-transparent badge in the bottom-right corner
// ─────────────────────────────────────────────────────────────────────────────
export class ApplicationWindow final : public Nandina::WindowController {
protected:
    auto setup() -> void override;

    [[nodiscard]] auto initial_size() const -> std::pair<int, int> override;

    [[nodiscard]] auto title() const -> std::string_view override;

private:
    Nandina::Router router_;
};

void ApplicationWindow::setup() {
    set_background_color(35, 38, 52);

    // Push DemoPage as the startup page so all three new features are visible.
    router_.push<DemoPage>();
    auto view = Nandina::RouterView::Create(router_);
    view->set_bounds(0.0f, 0.0f,
                     static_cast<float>(window_width()),
                     static_cast<float>(window_height()));
    set_content(std::move(view));
}

std::pair<int, int> ApplicationWindow::initial_size() const {
    return {640, 480};
}

std::string_view ApplicationWindow::title() const {
    return "Nandina — Prop / StateList / Signal Demo";
}

