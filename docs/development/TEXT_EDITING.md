# 桌面文本编辑（C4）

> 状态：C4 完成；自动化、GNOME Wayland + fcitx5 输入及 Hyprland 剪贴板验收通过。

## 1. 范围

C4 在现有 grapheme-safe caret、选择、双向文本布局和 committed `TextInputEvent` 基础上补齐
桌面单行编辑命令：全选、复制、剪切、粘贴、撤销和重做。富文本、多行编辑、系统级原生
pre-edit 绘制与候选框定位不在 1.0 基本闭环内。

## 2. 平台边界

`scene::IClipboard` 是 UTF-8 文本剪贴板的最小平台边界；`NanSceneTree` 持有非 owning 服务，
测试可注入内存实现。`NanWindow` 在 Linux Wayland 会话优先通过 `wl-paste` / `wl-copy`
访问 compositor 剪贴板，命令不可用或失败时回退 raylib 的 `GetClipboardText` /
`SetClipboardText`；其他平台继续使用 raylib 后端。Wayland 支持配置应安装 `wl-clipboard`。
控件和编辑 primitive 不直接包含 raylib 类型或调用平台 API。

`scene::EditCommand` 表达平台无关意图：`select_all/copy/cut/paste/undo/redo`。桌面键位在
`EditableText` 输入适配层映射：Ctrl 或 Super + A/C/X/V/Z/Y，Shift+Ctrl/Super+Z 也表示
redo。复制和全选在只读状态可用；剪切、粘贴、撤销、重做和普通文本输入在只读或禁用状态
不得修改值。

## 3. 历史模型

每个已提交的文本输入、粘贴、剪切、Backspace 或 Delete 是一个撤销单元。快照包含 UTF-8
值、caret affinity 与选择范围；新编辑清空 redo 栈，程序化 `set_value` 重置历史且不发布
用户变更。撤销/重做通过同一个 change 通知路径发布一次最终值，历史上限为 100 个快照。

## 4. IME 与 CJK 约束

当前 raylib 构建使用 GLFW X11 后端，在 GNOME Wayland 中经 XWayland 运行。它通过
`GetCharPressed` 提供 committed Unicode codepoint 队列，但 GLFW 的 X11 input context 使用
`XIMPreeditNothing | XIMStatusNothing`，没有向 Nandina 暴露 pre-edit 文本、候选列表或 caret
spot。因此 1.0 支持已提交 CJK 文本输入，并将原生 pre-edit/候选框定位明确记录为 1.x 限制，
不伪装为已实现。直接切换 raylib 到原生 Wayland 也不能自动补齐 Wayland text-input 协议，且
可能破坏当前可用的 fcitx5 committed 输入，所以不作为本次剪贴板修复。

Linux 手工验收：

1. 在受支持的桌面会话启动 Settings 示例并聚焦 Profile name。
2. 使用系统输入法提交 `中文输入`，确认文本、caret、字体回退和水平滚动正常。
3. 使用 Ctrl+A/C/X/V/Z/Y 与 Ctrl+Shift+Z 验证命令；切换只读/禁用测试夹具时确认不可变。
4. 记录 X11/Wayland、桌面环境、输入法与观察到的 pre-edit/候选框限制。

自动化验收覆盖 UTF-8/CJK committed 输入、选择剪贴板、撤销/重做、redo 分支失效、
Ctrl/Super 键位以及只读/禁用不变性；Linux 聚焦测试覆盖 Wayland 会话检测、UTF-8 命令输出、
stdin 写入和命令失败。

### 4.1 2026-08-28 手工记录

- 环境：GNOME Wayland，fcitx5；raylib/GLFW 实际经 XWayland 运行。
- 已通过：Profile name 可顺利提交中文；Ctrl+A、Ctrl+Z、Ctrl+Y 正常。
- 二次复验已通过：Ctrl+Shift+Z 正常。
- 修复前失败：无法把其他应用中的文本通过 Ctrl+V 粘贴进 TextField。原因是 X11 selection
  后端不能在该会话中可靠读取 Wayland clipboard；现已增加 `wl-paste` / `wl-copy` 桥接。
- 首次修复复验失败：Ctrl+C/X 不能产生可粘贴文本，Ctrl+X 也不删除选择。真实调试确认平台
  桥接可以直接读写 Hyprland clipboard；实际根因是 `TextField` 内嵌的 `EditableText` 不是
  场景树节点，快捷键处理从自身查询 `NanSceneTree` 时总是得到空 clipboard。修复后由宿主
  `TextField` 显式传入场景树 clipboard，并增加针对完整 `TextField` 的 C/X/V 回归测试。
- 代理端到端验证：在真实 Hyprland/XWayland Settings 窗口注入 Ctrl+A/C、Ctrl+A/X、Ctrl+V，
  并通过 `wl-paste` 验证应用内恢复及外部→应用→外部 UTF-8 往返均通过。
- 项目负责人复验通过：Ctrl+C/X、应用内 Ctrl+V 和应用内外复制粘贴均可用。
- 已知限制：输入法候选框出现在窗口外并与窗口左侧对齐，而不是跟随 Profile name caret。
  这是上述 XIM input style 和缺失 caret spot API 的结果，记为 1.x 限制，不阻塞 committed
  CJK 输入的 1.0 基本可用目标。

C4 已关闭。原生 pre-edit/候选框定位继续作为 1.x 明确限制跟踪。

自动化验证：`meson compile -C buildDir`、`meson test -C buildDir widget --print-errorlogs`、
`meson test -C buildDir --print-errorlogs`（43/43）与 `git diff --check` 均通过。
