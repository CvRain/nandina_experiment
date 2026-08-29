# Linux 平台契约（C5）

> 状态：C5.1 窗口指标/DPI 自动化已实现，等待 Settings 实机验收；C5.2 系统外观与动效
> 语义仍待收紧。本文记录 Linux 1.0 的真实边界，不代表 Windows 或 macOS 支持。

## 1. 支持栈

Linux 1.0 使用 raylib 渲染与 raylib/GLFW 桌面窗口。当前 vendored raylib 构建启用 GLFW
X11 后端，因此在 Wayland 桌面中通过 XWayland 运行，并不是原生 Wayland 客户端。

- XWayland 与原生 Wayland 应用交换文本时依赖 `wl-clipboard`；失败时回退 raylib/GLFW
  剪贴板。
- committed Unicode/CJK 输入可用；pre-edit 文本和候选框 caret 定位不是 1.0 能力。
- 原生 Wayland、Windows 和 macOS 平台适配不属于 Linux 1.0 支持承诺。

## 2. 进程与资源路径

`NanApplicationConfig::for_process()` 在 Linux 通过 `/proc/self/exe` 解析绝对可执行文件路径，
并捕获 `HOME`、`XDG_DATA_HOME`、`XDG_DATA_DIRS`、`XDG_CACHE_HOME`。资源查找顺序是：

1. `<executable-directory>/resources`；
2. `$XDG_DATA_HOME/<application-id>`，未设置时为
   `$HOME/.local/share/<application-id>`；
3. `$XDG_DATA_DIRS/<application-id>` 中的每个目录；
4. 去重后的 `/usr/local/share/<application-id>` 与 `/usr/share/<application-id>`。

用户数据根沿用第 2 项；缓存根为 `$XDG_CACHE_HOME/<application-id>`，未设置时为
`$HOME/.cache/<application-id>`。当对应 XDG 变量未设置时，Linux 1.0 运行环境必须提供
`HOME`。应用 ID 必须是单段规范 `ResourceKey`，可执行文件路径必须是绝对路径。

## 3. 窗口、DPI 与坐标空间

`WindowConfig::high_dpi` 默认开启，并在创建窗口前设置 raylib 的
`FLAG_WINDOW_HIGHDPI`。每帧开始时，`NanWindow` 只采集一次窗口指标：

- screen size：`GetScreenWidth/Height()`，窗口与布局使用的 screen units；
- framebuffer size：`GetRenderWidth/Height()`，渲染目标的 physical pixels；
- framebuffer scale：physical pixels / screen unit，分别计算 X/Y 两轴。

`make_window_metrics()` 是不访问窗口后端的纯函数，负责校验尺寸并形成同一帧快照。
viewport 映射、根布局和 `DrawContext` 共用该快照，resize 后会在下一帧重新计算，避免布局
和绘制读取不同时间点的尺寸。

三个倍率不能混用：

1. `logical_to_screen` 只来自可选固定设计视口；
2. `screen_to_physical` 只来自 framebuffer/DPI；
3. 用户界面或字体缩放在布局前作用于 token，不属于窗口坐标变换。

当前字体与像素吸附接口只接受一个统一 physical scale。X/Y 倍率在 1% 相对误差内视为
整数 framebuffer 舍入；真正非均匀时，指标会将 `framebuffer_scale_is_uniform` 设为 false，
并以较小轴作为保守的 `screen_to_physical`。非均匀 framebuffer 不是 Linux 1.0 保证能力。

## 4. Resize 与 Teardown

resize 不改变设计视口或用户界面缩放：响应式窗口使用新的 screen size 重新布局；固定设计
视口使用新的 screen size 重新计算 contain/cover 映射。输入逆映射、语义 bounds 与绘制根变换
仍共享同一个 viewport mapping。

正常退出由 `NanApplication::run()` 调用 `NanWindow::close()`。顺序为：执行窗口
`on_teardown()`，清空 router，分离场景根，释放文本/font cache 与渲染设备，清除 clipboard
服务，最后关闭原生窗口。析构函数仅为异常路径提供已打开窗口的兜底关闭。

## 5. 系统外观与动效现状

当前 `ThemeManager` 没有 Linux 平台观察器。`ThemePreference::system` 只读取最后一次通过
`set_system_appearance()` 注入的快照，未注入时为 light；`MotionPreference::system` 只读取
`set_system_reduced_motion()`，未注入时为 full motion。现有 `ThemePreference::system` 注释仍有
“自动跟随 OS”的过度承诺，必须在 C5.2 修改 API 注释和文档，或实现真实平台 adapter，之后
才能关闭 C5。

## 6. Settings 手工验收

在最终提交本单元前记录桌面、会话类型、显示倍率和观察结果：

1. 在 1x 显示配置启动 Settings，确认窗口、文字、图片与 pointer hit target 正常。
2. 在非 1x 显示配置重新启动，确认文字清晰、尺寸不翻倍、caret/选择/clip 与控件对齐。
3. 两种倍率下拖动调整窗口，确认响应式布局、50% 控件宽度、输入与语义焦点保持一致。
4. 使用纯键盘遍历并操作控件，分别切换 light/dark 与 reduced/full motion。
5. 使用 fcitx5 提交 CJK，并复验应用内外复制粘贴；候选框位置按已知限制记录。
6. 关闭窗口并再次启动，确认 teardown 无崩溃、资源与剪贴板服务没有残留故障。

自动化覆盖 1x、2x、resize 后重新计算、整数 framebuffer 舍入、非法尺寸和非均匀倍率策略；
实机结果未记录前，C5.1 不标记完成。

### 6.1 2026-08-29 初步实机记录

- 项目负责人在当前电脑运行修改后的 Settings，窗口、布局、文字与整体显示体验正常，未发现
  DPI 或 resize 相关异常。
- 本轮允许提交 C5.1 作为第一台设备的可复验基线。
- 第二台电脑仍需按上述清单复验，尤其记录实际 framebuffer scale，并与 C5.2 系统外观与
  动效契约一并确认；在该记录补齐前不关闭整个 C5。
