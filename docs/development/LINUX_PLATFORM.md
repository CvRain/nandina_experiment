# Linux 平台契约（C5）

> 状态：C5.1 窗口指标/DPI 与 C5.2 系统偏好契约已实现；当前电脑初步验收通过，等待第二台
> 电脑联合复验。本文记录 Linux 1.0 的真实边界，不代表 Windows 或 macOS 支持。

## 1. 支持栈

Linux 1.0 使用 raylib 渲染与 raylib/GLFW 桌面窗口。当前 vendored raylib 构建启用 GLFW
X11 后端，因此在 Wayland 桌面中通过 XWayland 运行，并不是原生 Wayland 客户端。

- XWayland 与原生 Wayland 应用交换文本时依赖 `wl-clipboard`；失败时回退 raylib/GLFW
  剪贴板。
- committed Unicode/CJK 输入可用；pre-edit 文本和候选框 caret 定位不是 1.0 能力。
- 原生 Wayland、Windows 和 macOS 平台适配不属于 Linux 1.0 支持承诺。

## 2. 进程与资源路径

`NanApplicationConfig::for_process()` 在 Linux 通过 `/proc/self/exe` 解析绝对可执行文件路径，
并捕获 `HOME`、`XDG_CONFIG_HOME`、`XDG_DATA_HOME`、`XDG_DATA_DIRS`、
`XDG_STATE_HOME`、`XDG_CACHE_HOME`。资源查找顺序是：

1. `<executable-directory>/resources`；
2. `$XDG_DATA_HOME/<application-id>`，未设置时为
   `$HOME/.local/share/<application-id>`；
3. `$XDG_DATA_DIRS/<application-id>` 中的每个目录；
4. 去重后的 `/usr/local/share/<application-id>` 与 `/usr/share/<application-id>`。

`PlatformResourceLocator` 同时形成一份应用可复用的平台路径快照，并由
`NanApplication::platform_paths()` 暴露：

- config：`$XDG_CONFIG_HOME/<application-id>`，回退
  `$HOME/.config/<application-id>`；
- data：`$XDG_DATA_HOME/<application-id>`，回退
  `$HOME/.local/share/<application-id>`；
- state：`$XDG_STATE_HOME/<application-id>`，回退
  `$HOME/.local/state/<application-id>`；
- cache：`$XDG_CACHE_HOME/<application-id>`，回退
  `$HOME/.cache/<application-id>`。

配置、可变数据、状态、缓存与只读应用资源是不同语义；应用不应把 `settings.json` 交给
`nanres`。用户级 portable 部署可将可执行文件放在 `~/.local/bin`，将 `resources.db` 与
`external/` 一起放在 data 根，同时在 data 根下用应用自有子目录保存可变数据。资源逻辑 key
不会因物理目录改变而变化。`resource-location.json` 只属于构建树，移动资源包时不得复制。

所有 HOME/XDG 单目录值以及 `XDG_DATA_DIRS` 的每个条目都必须是绝对路径；无效配置在启动
时产生明确错误。Linux 1.0 运行环境必须提供绝对 `HOME`。应用 ID 必须是单段规范
`ResourceKey`，可执行文件路径必须是绝对路径。框架不检查当前用户是否为 root，也不替
安装器选择 `/usr` 或 `~/.local`。

Linux 1.0 只承诺构建树运行与 executable-relative/XDG data portable staging。系统 SDK
安装和 deb/rpm/Flatpak/Snap/AppImage 不属于本契约；未来 macOS/Windows 实现应替换平台路径
适配器，而不是让应用代码散布条件宏。

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
`on_teardown()`，清空 router，分离场景根，清除字体/纹理上下文，释放默认字体、字体 LRU、
图片纹理 LRU，再释放渲染设备，清除 clipboard 服务，最后关闭原生窗口。这样所有 RAII
纹理都能在设备有效期间调用 `destroy_texture()`。析构函数仅为异常路径提供已打开窗口的
兜底关闭。应用析构首先停止并 join 后台执行器，再销毁资源管理器与挂载后端，避免仍在读取
`res://` 的任务观察到已销毁资源；详细资源所有权见 `RESOURCE_RESIDENCY.md`。

## 5. 系统外观与动效

Linux 1.0 不承诺自动观察桌面设置。`ThemeManager` 是平台无关核心，`system` 表示“最后由
应用宿主注入的平台偏好快照”，不是内部 DBus、GTK、portal 或命令轮询：

- `ThemePreference::system` 读取 `SystemPreferences::appearance`，未注入时回退 light；
- `MotionPreference::system` 读取 `SystemPreferences::reduced_motion`，未注入时回退 full
  motion；
- 宿主通过 `set_system_preferences()` 原子更新两项，最多发布一次 revision；
- `set_system_appearance()` 与 `set_system_reduced_motion()` 是分项事件的便利入口；
- 应用显式选择 light/dark 或 full/reduced 时，系统快照仍会保存，但不会触发无效 revision。

未来 Linux adapter 可以从 xdg-desktop-portal/DBus 获取真实状态后调用这个边界，不需要改变
控件、动画或主题解析层。1.0 没有安装该 adapter，因此 Settings 选择 `system` 时使用上述
默认回退；这一限制必须保留在发布说明中。

## 6. Settings 手工验收

在最终提交本单元前记录桌面、会话类型、显示倍率和观察结果：

1. 在 1x 显示配置启动 Settings，确认窗口、文字、图片与 pointer hit target 正常。
2. 在非 1x 显示配置重新启动，确认文字清晰、尺寸不翻倍、caret/选择/clip 与控件对齐。
3. 两种倍率下拖动调整窗口，确认响应式布局、50% 控件宽度、输入与语义焦点保持一致。
4. 使用纯键盘遍历并操作控件，分别切换 light/dark 与 reduced/full motion；重新选择
   `System` 时确认 1.0 无 adapter 的回退为 light/full，而不是暗示自动跟随桌面。
5. 使用 fcitx5 提交 CJK，并复验应用内外复制粘贴；候选框位置按已知限制记录。
6. 关闭窗口并再次启动，确认 teardown 无崩溃、资源与剪贴板服务没有残留故障。
7. 在 Components/About 间反复切换，确认第二次进入不再成批重建相同 1024×1024 字体图集，
   About Logo 不重复上传；窗口关闭时缓存纹理全部在渲染设备之前释放。

自动化覆盖 1x、2x、resize 后重新计算、整数 framebuffer 舍入、非法尺寸和非均匀倍率策略；
第二台设备的联合实机记录补齐前，整个 C5 不标记完成。

### 6.1 2026-08-29 初步实机记录

- 项目负责人在当前电脑运行修改后的 Settings，窗口、布局、文字与整体显示体验正常，未发现
  DPI 或 resize 相关异常。
- 本轮允许提交 C5.1 作为第一台设备的可复验基线。
- 第二台电脑仍需按上述清单复验，尤其记录实际 framebuffer scale，并与 C5.2 系统外观与
  动效契约一并确认；在该记录补齐前不关闭整个 C5。
