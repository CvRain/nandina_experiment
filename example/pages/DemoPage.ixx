module;

#include <array>
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

// ── DemoPage ──────────────────────────────────────────────────────────────────
// A single-page reactive playground focused on manual validation:
//   • Counter state changes
//   • Computed derived summary
//   • EventSignal persistent + once subscriptions
//   • StateList-backed activity log
// ─────────────────────────────────────────────────────────────────────────────
export class DemoPage : public Nandina::Page {
public:
    static auto Create() -> std::unique_ptr<DemoPage>;

    auto build() -> Nandina::WidgetPtr override;

private:
    auto record_event(std::string message) -> void;

    Nandina::State<int> counter_{0};
    Nandina::State<std::string> headline_{"Reactive playground ready"};
    Nandina::State<std::string> status_text_{"Use the controls below to mutate state and inspect the log."};
    Nandina::State<int> persistent_signal_hits_{0};
    Nandina::State<int> once_signal_hits_{0};
    Nandina::State<int> last_signal_value_{0};

    Nandina::EventSignal<int> demo_signal_;
    Nandina::StateList<std::string> event_log_;
    Nandina::Computed<std::function<std::string()>> derived_summary_{
        std::function<std::string()>{
            [this] {
                const int value = counter_();
                return std::format("double={} | parity={} | abs={} | signal-last={}",
                                   value * 2,
                                   (value % 2 == 0) ? "even" : "odd",
                                   value < 0 ? -value : value,
                                   last_signal_value_());
            }
        }
    };

    Nandina::ScopedConnection signal_conn_;
    Nandina::ScopedConnection log_conn_;

    int next_signal_value_ = 1;
    static constexpr float kPageW = 640.0f;
    static constexpr float kPageH = 480.0f;
    static constexpr float kContentW = 592.0f;
    static constexpr float kButtonW = 136.0f;
    static constexpr float kButtonH = 34.0f;
    static constexpr std::size_t kVisibleLogEntries = 4;
    static constexpr std::size_t kMaxLogEntries = 10;
};

auto DemoPage::Create() -> std::unique_ptr<DemoPage> {
    auto self = std::make_unique<DemoPage>();
    self->initialize();
    return self;
}

auto DemoPage::record_event(std::string message) -> void {
    event_log_.push_back(std::move(message));
    while (event_log_.size() > kMaxLogEntries) {
        event_log_.remove_at(0);
    }
}

auto DemoPage::build() -> Nandina::WidgetPtr {
    auto root = Nandina::Column::Create();
    root->set_bounds(0.0f, 0.0f, kPageW, kPageH);
    root->set_background(28, 32, 46, 255);
    root->padding(24.0f).gap(12.0f);

    auto make_text_box = [](std::unique_ptr<Nandina::Label> label, float height) {
        auto box = Nandina::SizedBox::Create();
        box->set_background();
        box->height(height).child(std::move(label));
        return box;
    };

    auto make_button = [](std::string text, std::uint8_t r, std::uint8_t g, std::uint8_t b) {
        auto button = Nandina::Button::Create();
        button->set_bounds(0.0f, 0.0f, kButtonW, kButtonH);
        button->text(std::move(text));
        button->set_background(r, g, b);
        return button;
    };

    auto title = Nandina::Label::Create();
    title->set_bounds(0.0f, 0.0f, kContentW, 34.0f);
    title->font_size(24.0f).text_color(198, 214, 255);
    title->text("Reactive Playground");
    root->add(make_text_box(std::move(title), 34.0f));

    auto headline = Nandina::Label::Create();
    headline->set_bounds(0.0f, 0.0f, kContentW, 22.0f);
    headline->font_size(14.0f).text_color(148, 221, 182);
    headline->bind_text(headline_);
    root->add(make_text_box(std::move(headline), 22.0f));

    auto counter_value = Nandina::Label::Create();
    counter_value->set_bounds(0.0f, 0.0f, kContentW, 56.0f);
    counter_value->font_size(46.0f).text_color(236, 244, 255);
    counter_value->bind_text(counter_, [](int value) {
        return std::format("count = {}", value);
    });
    root->add(make_text_box(std::move(counter_value), 64.0f));

    auto derived_label = Nandina::Label::Create();
    derived_label->set_bounds(0.0f, 0.0f, kContentW, 22.0f);
    derived_label->font_size(13.0f).text_color(255, 210, 150);
    auto *derived_label_ptr = derived_label.get();
    root->add(make_text_box(std::move(derived_label), 22.0f));

    effect([this, derived_label_ptr] {
        derived_label_ptr->text(derived_summary_());
    });

    auto signal_stats = Nandina::Label::Create();
    signal_stats->set_bounds(0.0f, 0.0f, kContentW, 22.0f);
    signal_stats->font_size(13.0f).text_color(217, 188, 255);
    auto *signal_stats_ptr = signal_stats.get();
    root->add(make_text_box(std::move(signal_stats), 22.0f));

    effect([this, signal_stats_ptr] {
        signal_stats_ptr->text(std::format("signal persistent={} | once={} | last={}",
                                           persistent_signal_hits_(),
                                           once_signal_hits_(),
                                           last_signal_value_()));
    });

    auto status_label = Nandina::Label::Create();
    status_label->set_bounds(0.0f, 0.0f, kContentW, 22.0f);
    status_label->font_size(12.0f).text_color(180, 188, 205);
    status_label->bind_text(status_text_);
    root->add(make_text_box(std::move(status_label), 22.0f));

    auto counter_row = Nandina::Row::Create();
    counter_row->set_background(0, 0, 0, 0);
    counter_row->set_bounds(0.0f, 0.0f, kContentW, kButtonH);
    counter_row->gap(12.0f);

    auto decrement_button = make_button("-1", 196, 111, 111);
    decrement_button->on_click([this]() {
        counter_.set(counter_() - 1);
        headline_.set("Counter decremented");
        status_text_.set("State<int> changed and dependent labels re-rendered.");
        record_event(std::format("counter -> {}", counter_()));
    });
    counter_row->add(std::move(decrement_button));

    auto increment_button = make_button("+1", 102, 150, 236);
    increment_button->on_click([this]() {
        counter_.set(counter_() + 1);
        headline_.set("Counter incremented");
        status_text_.set("Computed summary should update together with the counter.");
        record_event(std::format("counter -> {}", counter_()));
    });
    counter_row->add(std::move(increment_button));

    auto reset_button = make_button("Reset", 116, 185, 129);
    reset_button->on_click([this]() {
        counter_.set(0);
        headline_.set("Counter reset");
        status_text_.set("State reset to zero.");
        record_event("counter reset to 0");
    });
    counter_row->add(std::move(reset_button));
    root->add(std::move(counter_row));

    auto action_row = Nandina::Row::Create();
    action_row->set_background(0, 0, 0, 0);
    action_row->set_bounds(0.0f, 0.0f, kContentW, kButtonH);
    action_row->gap(12.0f);

    auto banner_button = make_button("Cycle headline", 203, 147, 88);
    banner_button->on_click([this]() {
        static constexpr std::array<std::string_view, 4> messages{
            "Reactive playground ready",
            "Verifying State and Label binding",
            "Computed summary is live",
            "Event log is fed by StateList"
        };
        const auto index = static_cast<std::size_t>(
            (counter_() < 0 ? -counter_() : counter_()) % static_cast<int>(messages.size()));
        headline_.set(std::string{messages[index]});
        status_text_.set("String state updated via Label::bind_text.");
        record_event(std::format("headline -> {}", messages[index]));
    });
    action_row->add(std::move(banner_button));

    auto signal_button = make_button("Fire signal", 151, 103, 220);
    signal_button->on_click([this]() {
        demo_signal_.connect_once([this](int value) {
            once_signal_hits_.set(once_signal_hits_() + 1);
            status_text_.set("connect_once handler consumed the latest signal.");
            record_event(std::format("signal once received {}", value));
        });

        const int emitted_value = next_signal_value_++;
        demo_signal_.emit(emitted_value);
        headline_.set("Signal emitted");
        record_event(std::format("signal emitted {}", emitted_value));
    });
    action_row->add(std::move(signal_button));

    auto clear_log_button = make_button("Clear log", 120, 128, 148);
    clear_log_button->on_click([this]() {
        event_log_.clear();
        headline_.set("Log cleared");
        status_text_.set("StateList was reset.");
        record_event("log cleared");
    });
    action_row->add(std::move(clear_log_button));
    root->add(std::move(action_row));

    auto log_title = Nandina::Label::Create();
    log_title->set_bounds(0.0f, 0.0f, kContentW, 20.0f);
    log_title->font_size(13.0f).text_color(145, 213, 255);
    log_title->text("Activity log (StateList watcher)");
    root->add(make_text_box(std::move(log_title), 20.0f));

    auto log_count = Nandina::Label::Create();
    log_count->set_bounds(0.0f, 0.0f, kContentW, 18.0f);
    log_count->font_size(12.0f).text_color(158, 166, 184);
    auto *log_count_ptr = log_count.get();
    root->add(make_text_box(std::move(log_count), 18.0f));

    std::array<Nandina::Label *, kVisibleLogEntries> log_entries{};
    for (std::size_t index = 0; index < kVisibleLogEntries; ++index) {
        auto entry = Nandina::Label::Create();
        entry->set_bounds(0.0f, 0.0f, kContentW, 18.0f);
        entry->font_size(12.0f).text_color(220, 224, 233);
        entry->text("- (empty)");
        log_entries[index] = entry.get();
        root->add(make_text_box(std::move(entry), 18.0f));
    }

    auto refresh_log = [this, log_entries, log_count_ptr]() {
        log_count_ptr->text(std::format("entries: {} (showing latest {})",
                                        event_log_.size(),
                                        kVisibleLogEntries));

        for (std::size_t slot = 0; slot < kVisibleLogEntries; ++slot) {
            const std::size_t visible_count = event_log_.size() < kVisibleLogEntries
                                                  ? event_log_.size()
                                                  : kVisibleLogEntries;

            if (slot < visible_count) {
                const std::size_t source_index = event_log_.size() - visible_count + slot;
                log_entries[slot]->text(std::format("- {}", event_log_[source_index]));
            }
            else {
                log_entries[slot]->text("- (empty)");
            }
        }
    };

    log_conn_ = Nandina::ScopedConnection{
        event_log_.watch(refresh_log)
    };

    signal_conn_ = Nandina::ScopedConnection{
        demo_signal_.connect([this](int value) {
            persistent_signal_hits_.set(persistent_signal_hits_() + 1);
            last_signal_value_.set(value);
            status_text_.set("Persistent signal subscriber ran before the once-only subscriber count update settled.");
        })
    };

    refresh_log();
    record_event("playground initialized");

    return root;
}
