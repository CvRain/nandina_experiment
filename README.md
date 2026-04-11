# Nandina

**简单而独立的 C++ 组件库框架（实验阶段）**

Nandina 是一个基于现代 C++26 的 GUI 组件框架实验项目，目标是在 C++ 桌面端提供接近前端框架（React / Angular / Svelte / Angular Signals）的开发体验：无宏、响应式状态、组合式组件、Design Token 主题、跨平台渲染。

---

## 项目方向

- **零宏**：所有 API 均为纯 C++ 表达式，不依赖 `moc` 一类预处理步骤
- **现代响应式**：`State<T>` / `ReadState<T>` / `Computed<F>` / `Effect` 负责值语义和自动依赖追踪
- **结构化集合更新**：`StateList<T>` 保留插入、删除、移动、更新等精确变更语义，适合树、列表、文件管理器
- **组合优于继承**：Action 单元（HoverAction / PressAction / FocusAction）组合复杂组件
- **跨平台渲染**：WebGPU > OpenGL > 软件光栅化，渲染后端对组件代码透明

---

## 技术栈

| 层 | 选型 | 说明 |
| :-- | :-- | :-- |
| 语言 | C++26 + Modules | 零头文件，概念约束 |
| 窗口/事件 | SDL3 | 跨平台，轻量稳定 |
| 矢量渲染 | ThorVG | 软件 / OpenGL / WebGPU 后端 |
| 文字 | FreeType + HarfBuzz | Unicode 字形光栅化 + 文本整形（M4） |
| 布局 | Row/Column/Stack → Yoga（M5） | 先自实现，后接入 Yoga |
| 构建 | CMake + Ninja + CMakePresets | C++26 Modules 扫描 |
| 包管理 | vcpkg manifest 模式 | 声明式依赖，CI 可复现 |

---

## 快速开始

### Arch Linux（系统包）

```bash
sudo pacman -S cmake ninja sdl3 thorvg

git clone https://github.com/CvRain/nandina_experiment.git
cd nandina_experiment

cmake --preset debug
cmake --build build/debug
./build/debug/bin/NandinaExperiment
```

### 使用 vcpkg（跨平台）

```bash
git clone https://github.com/CvRain/nandina_experiment.git
cd nandina_experiment

# 初始化 vcpkg（如已有全局 vcpkg 可跳过）
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh

cmake --preset debug-vcpkg
cmake --build build/debug-vcpkg
./build/debug-vcpkg/bin/NandinaExperiment
```

---

## 响应式模型速览

Nandina 当前明确保留两套互补的响应式模型：

1. `State<T>`
适合单值状态、派生值和自动 UI 绑定。读取时自动建立依赖，写入时自动触发相关 `Effect` / `Computed`。

2. `StateList<T>`
适合列表、树、文件节点集合等结构化数据。它提供精确的 `Inserted / Removed / Updated / Moved / Reset` 变更语义，而不是简单把整个集合当成一个普通值。

3. `tracked_*` 桥接接口
当你只需要粗粒度派生值时，可以通过 `tracked_size()`、`tracked_empty()`、`tracked_items()`、`version()` 把 `StateList<T>` 显式桥接回 `Computed` / `Effect` 世界。

```cpp
State<std::string> search_text{""};
StateList<std::string> files;

Computed match_count{[&] {
	const auto& items = files.tracked_items();
	auto keyword = search_text();

	return std::ranges::count_if(items, [&](const std::string& file) {
		return keyword.empty() || file.contains(keyword);
	});
}};
```

推荐规则：

- 单值状态用 `State<T>`
- 结构化集合用 `StateList<T>`
- 只在需要粗粒度派生时使用 `tracked_*`

---

## 批量更新

Reactive 模块现在提供最小 `batch(...)` 能力，用于把多个连续 `State::set()` 合并成一次依赖刷新，避免同一个 effect 在中间状态上重复重跑。

```cpp
batch([&] {
	selected_id.set("file-a");
	preview_name.set("file-a");
	preview_loading.set(true);
	status_text.set("Loading preview...");
});
```

这个能力当前有意保持克制：

- 主要用于 `State<T>` 驱动的自动依赖刷新
- 同一批次内对同一 effect / computed invalidator 去重
- 不尝试合并 `StateList<T>` 的细粒度 diff 事件

这更接近 UI commit/batch，而不是完整数据库事务。

---

## 文档

| 文档 | 说明 |
| :-- | :-- |
| [docs/design.md](docs/design.md) | 完整设计文档（架构、响应式、布局、文字渲染等） |
| [docs/development-plan.md](docs/development-plan.md) | 开发计划与里程碑任务清单（M1–M5） |
| [docs/reactive-design.md](docs/reactive-design.md) | Nandina.Reactive 模块详细设计 |
| [docs/reactive-primitives.md](docs/reactive-primitives.md) | Prop、StateList、Signal、tracked_*、batch 的使用说明 |

---

## 验证

构建应用：

```bash
cmake --preset debug-vcpkg
cmake --build --preset debug-vcpkg
```

响应式回归测试：

```bash
ctest --test-dir build/debug-vcpkg --output-on-failure
```

当前仓库包含 `NandinaReactiveSmoke`，覆盖：

- Effect 去重与异常安全
- Computed 惰性重算与失败重试
- `ReadState<T>` / `Prop<T>` 只读输入模型
- `EventSignal` 清理与重抛语义
- `StateList<T>` 的桥接接口与通知语义
- `batch(...)` 的最小批量刷新语义

---

## 当前状态

| 里程碑 | 内容 | 状态 |
| :-- | :-- | :-- |
| M0 | 窗口 + Button 组合 + 事件分发 + ThorVG SwCanvas | ✅ 完成 |
| M1 | Reactive 模块 + Label + dirty 剪枝 | 🚧 进行中 |
| M2 | Row / Column / Stack 布局容器 | ⬜ 待开始 |
| M3 | ThemeTokens + Design Token + Variant | ⬜ 待开始 |
| M4 | FreeType + HarfBuzz 文字渲染 | ⬜ 待开始 |
| M5 | 组件库扩展 + Yoga 布局引擎 | ⬜ 待开始 |

---

## 路线图

```
M0 (done) → M1 (reactive + Label) → M2 (layout) → M3 (style) → M4 (text) → M5 (component lib + Yoga)
```

---

## 许可证

本项目为个人实验项目，许可证待定。
