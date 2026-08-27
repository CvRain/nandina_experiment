# 桌面文本编辑（C4）

> 状态：自动化实现完成（42/42）；Linux committed CJK 手工验收待完成。

## 1. 范围

C4 在现有 grapheme-safe caret、选择、双向文本布局和 committed `TextInputEvent` 基础上补齐
桌面单行编辑命令：全选、复制、剪切、粘贴、撤销和重做。富文本、多行编辑、系统级原生
pre-edit 绘制与候选框定位不在 1.0 基本闭环内。

## 2. 平台边界

`scene::IClipboard` 是 UTF-8 文本剪贴板的最小平台边界；`NanSceneTree` 持有非 owning 服务，
测试可注入内存实现，`NanWindow` 注入 raylib 的 `GetClipboardText` / `SetClipboardText` 后端。
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

raylib 当前窗口边界只提供 `GetCharPressed` 的 committed Unicode codepoint 队列，没有公开
原生 pre-edit 文本、候选列表或候选框位置 API。因此 1.0 支持已提交 CJK 文本输入，并将原生
pre-edit/候选框定位明确记录为 1.x 限制，不伪装为已实现。

Linux 手工验收：

1. 在受支持的桌面会话启动 Settings 示例并聚焦 Profile name。
2. 使用系统输入法提交 `中文输入`，确认文本、caret、字体回退和水平滚动正常。
3. 使用 Ctrl+A/C/X/V/Z/Y 与 Ctrl+Shift+Z 验证命令；切换只读/禁用测试夹具时确认不可变。
4. 记录 X11/Wayland、桌面环境、输入法与观察到的 pre-edit/候选框限制。

自动化验收覆盖 UTF-8/CJK committed 输入、选择剪贴板、撤销/重做、redo 分支失效、
Ctrl/Super 键位以及只读/禁用不变性。

自动化验证：`meson compile -C buildDir`、`meson test -C buildDir --print-errorlogs`
（42/42）与 `git diff --check` 均通过。C4 只有在完成并记录上述 Linux 手工验收后才关闭。
