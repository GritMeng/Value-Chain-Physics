# 前端页面端到端测试（AionUi 内置浏览器 + chrome-devtools-mcp）

本目录的测试通过 **AionUi 内置浏览器**（aionui-browser MCP，底层为 chrome-devtools-mcp）
驱动真实前端页面，覆盖场景浏览、计划透视、交付承诺/回放、ITP-IOP、替代料、性能与历史六个视图。

## 前置条件

1. Demo 应用已启动：`npm run demo`（默认 http://127.0.0.1:3001）。
2. AionUi 浏览器面板已打开（MCP 桥接端口 `AIONUI_CDP_ACTIVE_PORT` 可用）。

## 运行

```bash
node test/browser/e2e.mjs
```

脚本在同一 MCP 会话内完成全部步骤；输出逐项 ✅/❌ 并通过退出码反映结果（0 = 全部通过）。

## 说明

- 内置浏览器只有一个固定标签页，脚本用 `list_pages` → `select_page` → `navigate_page`。
- React 受控输入必须使用原生 `value` setter 派发 `input` 事件，脚本中的 `setInputs()` 已封装。

## 覆盖的基线断言（与 CLI 回归一致）

| 场景 | 基线 |
|---|---|
| 交付承诺 | due=3 qty=30 → 承诺日 3、预留 3 工时、回滚 0 步 |
| 交付回放 | due=1 → 回滚 1 步、承诺日 2、时间轴 2 帧 |
| ITP/IOP | 配额 2 组、阻断 1 单、原因为「突破 ITP 配额上限」 |
| 替代料 | 三类 group=3 net_demand=12 → 分配 10、分配后在手 90（安全库存 10 未侵占） |
