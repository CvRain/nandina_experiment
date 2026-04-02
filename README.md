# Nandina

**简单而独立的 C++ 组件库框架（实验阶段）**

Nandina 是一个基于现代 C++26 的 GUI 组件框架实验项目，目标是在 C++ 桌面端提供接近前端框架（React / Angular / Svelte）的开发体验：无宏、响应式状态、组合式组件、Design Token 主题、跨平台渲染。

---

## ✨ 特性目标

- 🚫 **零宏**：所有 API 均为纯 C++ 模板，无预处理器魔法
- ⚡ **响应式状态**：`State<T>` / `Computed<F>` / `Effect` / `EffectScope`，自动依赖追踪
- 🧩 **组合优于继承**：Action 单元（HoverAction / PressAction / FocusAction）组合复杂组件
- 🎨 **Design Token**：ThemeTokens 统一颜色、间距、圆角、字体
- 🌍 **跨平台渲染**：WebGPU > OpenGL > 软件光栅化，后端对组件透明

---

## 🛠 技术栈

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

## 🚀 快速开始

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

## 📚 文档

| 文档 | 说明 |
| :-- | :-- |
| [docs/design.md](docs/design.md) | 完整设计文档（架构、响应式、布局、文字渲染等） |
| [docs/development-plan.md](docs/development-plan.md) | 开发计划与里程碑任务清单（M1–M5） |
| [docs/reactive-design.md](docs/reactive-design.md) | Nandina.Reactive 模块详细设计 |

---

## 📊 当前状态

| 里程碑 | 内容 | 状态 |
| :-- | :-- | :-- |
| M0 | 窗口 + Button 组合 + 事件分发 + ThorVG SwCanvas | ✅ 完成 |
| M1 | Reactive 模块 + Label + dirty 剪枝 | 🚧 进行中 |
| M2 | Row / Column / Stack 布局容器 | ⬜ 待开始 |
| M3 | ThemeTokens + Design Token + Variant | ⬜ 待开始 |
| M4 | FreeType + HarfBuzz 文字渲染 | ⬜ 待开始 |
| M5 | 组件库扩展 + Yoga 布局引擎 | ⬜ 待开始 |

---

## 🗺 路线图

```
M0 (done) → M1 (reactive + Label) → M2 (layout) → M3 (style) → M4 (text) → M5 (component lib + Yoga)
```

---

## 📄 许可证

本项目为个人实验项目，许可证待定。
