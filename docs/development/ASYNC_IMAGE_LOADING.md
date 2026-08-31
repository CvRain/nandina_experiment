# 异步图片加载（C5.4）

> 状态：基础链路与自动化测试完成；等待 Gallery 实机性能验收。

## 1. 目标与边界

大尺寸 PNG/JPEG 的首次解码、缩放不能阻塞 UI 帧。页面应立即完成布局并绘制固定尺寸占位块，
后台任务完成后再替换为真实纹理。该行为只改变首次加载调度，不改变 C5.3 定义的纹理键、LRU
驻留、页面生命周期或关闭顺序。

C5.4 覆盖打包资源的 CPU 解码与预处理。当前 `ResourceManager::require()` 仍在 UI 线程完成
SQLite/sidecar 读取；通常主要卡顿来自原图解码和缩放，但极慢磁盘或更大资源仍可能暴露同步
I/O。文件路径图片继续使用同步兼容路径。

## 2. 线程模型

```text
UI thread                      BackgroundExecutor              UI thread
---------                      ------------------              ---------
resolve res:// + bytes  ────>  decode/crop/resize/tint  ────>  upload RGBA texture
show placeholder               no GPU calls                    cache + repaint
```

- `IImageDecoder` 只产生紧密排列的 RGBA8 像素，可在后台线程运行；
- `IRenderDevice::create_rgba_texture()` 只在 UI/渲染线程调用；
- `TextureCache` 按资源 ID、media type 和预处理选项合并相同的 pending 请求；
- 完成后只上传一次纹理，所有等待的 `Image` 共享同一个 `CachedTexture`；
- 缓存销毁会使未回到 UI 的结果失效，后台任务不会访问已销毁的渲染设备；
- `Image` 用 generation 丢弃 source/options 改变后的过期完成回调。

## 3. Image 合约

`ImageLoadMode` 提供 `automatic`、`synchronous` 和 `asynchronous`。普通窗口注入异步服务后，
`automatic` 对打包资源使用异步路径；低层测试、无窗口缓存或不具备异步服务的调用者安全回退
到同步路径。

`ImageLoadState` 为 `idle → loading → ready|failed`。`set_placeholder_color()` 配置 loading/failed
期间的固定区域填充；显式 `set_size()` 可保证真实图片完成时不触发布局跳动。Gallery 明确使用
异步模式、`surface_variant` 占位色和目标上传尺寸。

## 4. 验收与后续单元

C5.4 自动化必须证明：首帧只提交后台任务并绘制占位块、后台阶段不上传 GPU、UI 回调阶段才
创建纹理，以及相同 pending 请求只解码/上传一次。实机验收需要确认首次进入 Gallery 可以立即
滚动或切换页面，四张图片随后逐步出现。

后续按以下顺序推进：

1. **C5.5 — I/O、取消与预算**：把 package/sidecar 读取也移入后台任务；增加真正的任务取消、
   并发解码数、encoded/decoded 临时内存预算和失败重试策略。
2. **C5.6 — 可见性与帧预算**：ScrollView 可见区域附近才请求图片；支持预取距离和优先级；
   按每帧纹理数量/字节预算分批上传，避免多个任务同时完成造成单帧尖峰。
3. **1.1 性能工具**：加载耗时、缓存命中率、pending 数、临时内存和 GPU 驻留诊断面板。

网络图片下载、渐进式 JPEG、磁盘缩略图缓存、跨窗口共享和热重载不属于 Linux 1.0 范围。
