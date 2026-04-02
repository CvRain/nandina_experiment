module;
#include <thorvg-1/thorvg.h>
#include <SDL3/SDL.h>
#include <vector>
#include <memory>
#include <functional>

export module Nandina.Core;

export namespace Nandina {

    class NanComponent {
    public:
        virtual ~NanComponent() = default;

        // 1. 对象树管理
        auto add_child(std::unique_ptr<NanComponent> child) -> NanComponent* {
            child->parent_ = this;
            // 将子组件的绘图对象推入当前的场景容器中
            //scene_->push(child->get_paint());
            scene_.push(nullptr);

            children_.push_back(std::move(child));
            return children_.back().get();
        }

        // 2. 事件分发：从 Window 传导下来
        virtual void handle_event(const SDL_Event& event) {
            for (auto& child : children_) {
                child->handle_event(event);
            }
        }

        // 获取 ThorVG 的 Paint 对象（这里是 Scene，类似于一个组）
        auto get_paint() -> std::unique_ptr<tvg::Scene> {
            // 由于 ThorVG 的 push 需要转移所有权，我们在导出时需要 clone 或者特殊处理
            // 简单起见，这里我们直接暴露内部场景的指针
            return nullptr; // 实际实现中建议直接操作内部的 scene_ 成员
        }

        // 提供给派生类使用的场景容器
        tvg::Scene* scene() { return scene_.get(); }

    protected:
        NanComponent() : scene_(tvg::Scene::gen()) {}

        NanComponent* parent_ = nullptr;
        std::vector<std::unique_ptr<NanComponent>> children_;
        std::unique_ptr<tvg::Scene> scene_;
    };

} // namespace Nandina