# 资源驻留与页面生命周期（C5.3）

> 状态：实现完成，聚焦测试与 43/43 全量自动化通过；等待 Settings 实机复验。

## 1. 目标

页面实例生命周期与渲染资源生命周期必须彼此独立。Router 可以按导航语义销毁页面，字体图集
和图片纹理不应因此在每次访问时重复解码、上传；反过来，也不能为了保留纹理而永久保活页面
节点、响应式 scope、异步任务和回调。

本单元只管理窗口内资源驻留，不改变 Router 的页面语义：

- `push(B)` 保留并隐藏 A；`pop()` 销毁 B，恢复原来的 A 实例；
- 再次 `push(B)` 会创建新的 B；
- `replace(B)` 销毁旧页并创建新的 B；开启转场时旧页仅保留到淡出结束；
- Store 或应用服务承载跨页面业务状态，资源缓存不承担页面状态复用。

未来若需要 Angular `RouteReuseStrategy` 一类能力，应提供显式 route reuse policy，而不是默认
缓存所有页面。

## 2. 字体管线驻留

`FontPipelineCache` 仍以 family、weight、slant 和 atlas options 为键。Text 节点持有正在使用的
`FontPipeline` 强引用；窗口缓存额外保留最近使用的管线：

- 默认最多保留 16 条管线；
- 默认估算驻留预算为 128 MiB；
- raylib 估算按每个 atlas pixel 的 1 byte CPU alpha + 4 byte GPU RGBA 计算；
- 超过数量或预算时淘汰最近最少使用的缓存引用；仍被节点使用的管线继续有效；
- 淘汰后若没有其他使用者，`GlyphAtlasTexture` 立即调用 `destroy_texture()`；
- `clear()` 释放全部缓存引用，但不会使仍由活动节点持有的管线悬空。

预算约束的是不活动资源的缓存驻留，不是应用所有活动文字的硬上限。一个活动页面本身超过预算
时仍可正确渲染，只是不在页面销毁后继续驻留。

## 3. 图片纹理驻留

`CachedTexture` 是普通图片纹理的共享 RAII 所有者。最后一个引用释放时调用渲染设备的
`destroy_texture()`，不再把无法访问的纹理留到窗口关闭。

`TextureCache` 由 `NanWindow` 创建并通过 `NanSceneTree` 注入 Image：

- 文件纹理按路径与 `ImageLoadOptions` 建键；
- nanres 图片按稳定 `ResourceId`、media type 与 `ImageLoadOptions` 建键；
- crop、resize 或 tint 不同的派生纹理不会错误共享；
- 默认最多保留 64 条最近使用纹理的缓存引用，估算 RGBA 驻留预算为 128 MiB；
- LRU 淘汰只移除缓存引用，正在显示或被 keep-alive 页面持有的纹理保持有效；
- 未接入窗口缓存的低层 Image 仍使用 RAII 独占纹理，切换 source 或销毁节点时立即释放。

页面服务必须沿嵌套路由显式传递。创建子 `NanRouter` 时，将宿主 `PageContext` 的
`resources()`、`font_loader()` 和 `font_families()` 传入构造函数；否则子页面虽然仍可构建，
但 `BuildContext::resource_manager()` 为空，`res://` 图片会在首次绘制前被诊断为资源服务不可用。

当前 nanres 资源包在进程运行期间视为不可变快照。运行时替换同一 ResourceId 的开发工具需要
在替换后清理纹理缓存；热重载与资源 revision 键不属于 Linux 1.0 承诺。

## 4. 所有权与关闭顺序

渲染设备必须晚于所有 `CachedTexture` 和 `FontPipeline` 销毁。`NanWindow::close()` 顺序为：

1. 执行 `on_teardown()` 并清空 Router；
2. 分离场景根，使活动页面释放资源引用；
3. 清除 SceneTree 的字体与纹理缓存上下文；
4. 释放默认字体、字体 LRU 与图片 LRU；
5. 释放渲染设备并关闭原生窗口。

直接构造 `NanSceneTree`、`TextureCache` 或 `FontPipelineCache` 的低层调用者也必须保持同样的
设备后销毁约束。

## 5. 1.0 验收

自动化测试必须覆盖：相同资源跨节点生命周期只加载一次、加载选项隔离、LRU 淘汰后释放、
无缓存 Image 的 source 切换释放、字体管线在页面引用消失后仍可命中，以及超过限制后的安全
重建。Settings 手工验收需要反复切换 Components/About，确认第二次进入不再批量重建相同
字体图集，退出时所有缓存纹理在渲染设备之前释放。

后台图片解码、异步 GPU 上传、glyph atlas dirty-region 更新、预热清单、跨窗口共享、热重载
和详细运行时显存面板留给后续版本；它们不改变本文件定义的所有权边界。
