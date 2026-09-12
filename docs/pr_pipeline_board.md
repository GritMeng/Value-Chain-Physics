# IPC Strategy & PR Pipeline Board (多进程协同看板)

This dashboard coordinates the strategic deployment and PR article publishing workflow between the user and AI processes.

---

## 🚀 Active Handshake & Task Registry

| Task ID | Component / Area | Description | Assigned Process | Status | Dependencies | Last Update |
|:---|:---|:---|:---|:---|:---|:---|
| **STR-01** | Strategy | Update `ipc_breakthrough_strategy.md` to ROIC-centric positioning | Process-A (Strategic) | `[ ] Pending` | None | 2026-06-18 |
| **DATA-02**| Financial Data | Verify DSI, Revenue, and Inventory data of the 6-company panel (TCL, CATL, etc.) from official PDFs | Process-B (Data Scraper) | `[ ] Pending` | CNINFO / HKEX Access | 2026-06-18 |
| **PR-03**  | PR Article | Draft "ROIC Verdict #1" (Why Digitalization fails to reduce DSI) targeting legacy APS | Process-A (PR) | `[ ] Pending` | DATA-02 | 2026-06-18 |
| **PR-04**  | LinkedIn | Draft English InMail pitches & comments for o9/Sanjiv Sidhu networking | Process-A (PR) | `[ ] Pending` | STR-01 | 2026-06-18 |
| **DASH-05**| Interactive UI | Set up Streamlit local ROIC Tree Calculator & visualizer | Process-B (Dev) | `[ ] Pending` | DATA-02 | 2026-06-18 |

---

## 📂 Folder Layout Guide

- `/docs/strategy/`: GTM strategies, targeting scripts.
- `/docs/pr_articles/`: Copy-ready HTML/Markdown files for WeChat & TMTPost.
- `/data/financials/`: Stock data extraction outputs, target company metrics database.
- `/tools/`: Scrapers, calculation utilities.

---

## 🤝 Coordination Protocol (握手规范)

1. **State Ownership**: When a process starts a task, it must update the status in this board to `[/] In Progress` and append its PID or AgentName.
2. **Conflict Prevention**: Do not modify files that another process is actively writing. Look at the `Last Update` and `Assigned Process`.
3. **Data Authenticity**: Every financial metric written to articles must have a corresponding verification log in `/data/financials/` linking to the source PDF page or official URL.
