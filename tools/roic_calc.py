# -*- coding: utf-8 -*-
"""
Intelligent Planning & Control (IPC) - DuPont ROIC Optimizer
Calculates standard ROIC and projects the "Planning-Attributable ROIC Gap"
by optimizing inventory days (DSI) and reducing planning-driven operational waste.
"""

class ROICCalculator:
    def __init__(self, company_name, revenue, cogs, net_profit, inventory, accounts_receivable, fixed_assets, accounts_payable=0):
        self.company_name = company_name
        self.revenue = float(revenue)
        self.cogs = float(cogs)
        self.net_profit = float(net_profit)
        self.inventory = float(inventory)
        self.ar = float(accounts_receivable)
        self.fa = float(fixed_assets)
        self.ap = float(accounts_payable)
        
        # Calculate derived baseline metrics
        self.ebit = self.net_profit / 0.85 # Assume 15% effective tax rate for operating income proxy
        self.nopat = self.ebit * (1 - 0.15)
        
        # Invested Capital = Working Capital (Inventory + AR - AP) + Net Fixed Assets
        # AP is subtracted as it's interest-free supplier financing
        self.working_capital = self.inventory + self.ar - self.ap
        self.invested_capital = self.working_capital + self.fa
        
        # Baseline ROIC
        self.baseline_roic = (self.nopat / self.invested_capital) * 100 if self.invested_capital > 0 else 0
        self.baseline_dsi = (self.inventory / self.cogs) * 365 if self.cogs > 0 else 0
        
    def project_optimization(self, target_dsi=45.0, cogs_saving_ratio=0.01):
        """
        Projects ROIC after IPC optimization.
        - target_dsi: Target Days Sales of Inventory (default 45 days)
        - cogs_saving_ratio: Reduction in COGS from fewer write-downs, premium logistics, rush freight (default 1%)
        """
        # 1. Cash Unlocked from Inventory Optimization
        baseline_dsi = self.baseline_dsi
        if baseline_dsi <= target_dsi:
            optimized_inventory = self.inventory
            cash_unlocked = 0.0
            dsi_improvement = 0.0
        else:
            optimized_inventory = (target_dsi / 365.0) * self.cogs
            cash_unlocked = self.inventory - optimized_inventory
            dsi_improvement = baseline_dsi - target_dsi
            
        # 2. COGS reduction (unlocking margin)
        saved_cogs_costs = self.cogs * cogs_saving_ratio
        optimized_cogs = self.cogs - saved_cogs_costs
        
        # Increment EBIT/NOPAT from saved costs (assume 15% tax on savings)
        optimized_ebit = self.ebit + saved_cogs_costs
        optimized_nopat = optimized_ebit * (1 - 0.15)
        
        # 3. New Invested Capital (with lower inventory)
        optimized_working_capital = optimized_inventory + self.ar - self.ap
        optimized_invested_capital = optimized_working_capital + self.fa
        
        # Optimized ROIC
        optimized_roic = (optimized_nopat / optimized_invested_capital) * 100 if optimized_invested_capital > 0 else 0
        roic_gain = optimized_roic - self.baseline_roic
        
        return {
            "baseline_dsi": baseline_dsi,
            "target_dsi": target_dsi,
            "dsi_improvement": dsi_improvement,
            "baseline_roic": self.baseline_roic,
            "optimized_roic": optimized_roic,
            "roic_gain_pct": roic_gain,
            "cash_unlocked": cash_unlocked,
            "cost_savings": saved_cogs_costs,
            "baseline_invested_capital": self.invested_capital,
            "optimized_invested_capital": optimized_invested_capital
        }
        
    def generate_report_markdown(self, target_dsi=45.0, cogs_saving_ratio=0.01):
        res = self.project_optimization(target_dsi, cogs_saving_ratio)
        
        report = []
        report.append(f"### 📊 【价值链物理学 ROIC 诊断】{self.company_name}")
        report.append(f"")
        report.append(f"| 财务指标 (Baseline) | 基准值 | 优化后 (IPC 预期) | 变动 |")
        report.append(f"| :--- | :--- | :--- | :--- |")
        report.append(f"| **营业收入 (Revenue)** | {self.revenue/1e8:.2f} 亿 | {self.revenue/1e8:.2f} 亿 | - |")
        report.append(f"| **存货金额 (Inventory)** | {self.inventory/1e8:.2f} 亿 | {(self.inventory - res['cash_unlocked'])/1e8:.2f} 亿 | **-{res['cash_unlocked']/1e8:.2f} 亿 (-{res['cash_unlocked']/self.inventory*100:.1f}%)** |")
        report.append(f"| **存货周转天数 (DSI)** | {res['baseline_dsi']:.1f} 天 | {res['target_dsi']:.1f} 天 | **-{res['dsi_improvement']:.1f} 天** |")
        report.append(f"| **运营成本 (COGS)** | {self.cogs/1e8:.2f} 亿 | {(self.cogs - res['cost_savings'])/1e8:.2f} 亿 | -{res['cost_savings']/1e8:.2f} 亿 (-{cogs_saving_ratio*100:.1f}%) |")
        report.append(f"| **投入资本 (Invested Capital)** | {res['baseline_invested_capital']/1e8:.2f} 亿 | {res['optimized_invested_capital']/1e8:.2f} 亿 | -{res['cash_unlocked']/1e8:.2f} 亿 (-{res['cash_unlocked']/res['baseline_invested_capital']*100:.1f}%) |")
        report.append(f"| **资本回报率 (ROIC)** | **{res['baseline_roic']:.2f}%** | **{res['optimized_roic']:.2f}%** | **+{res['roic_gain_pct']:.2f}% (上升 {res['roic_gain_pct']*100:.0f} bps)** |")
        report.append(f"")
        report.append(f"> [!TIP]")
        report.append(f"> **诊断结论**：若通过 IPC 引擎消除「协同协奏障碍」，将 DSI 从 {res['baseline_dsi']:.1f} 天降至 {res['target_dsi']:.1f} 天，并减少 {cogs_saving_ratio*100:.1f}% 计划相关溢价采购/跌价损失，")
        report.append(f"> 可**释放沉淀资金 {res['cash_unlocked']/1e8:.2f} 亿元**，将企业 ROIC 提振 **{res['roic_gain_pct']:.2f} 个百分点**。")
        
        return "\n".join(report)

if __name__ == "__main__":
    import sys
    # Reconfigure stdout to handle UTF-8 printing in Windows terminals
    if hasattr(sys.stdout, 'reconfigure'):
        sys.stdout.reconfigure(encoding='utf-8')
        
    # Test with typical TCL Electronics metrics (2025 estimation)
    # Revenue: 80B HKD, COGS: 68B HKD, Net Profit: 1.2B HKD, Inventory: 13.2B HKD, AR: 10B HKD, AP: 12B HKD, FA: 6B HKD
    tcl = ROICCalculator(
        company_name="TCL 电子 (01070.HK)",
        revenue=800e8,
        cogs=680e8,
        net_profit=12e8,
        inventory=132e8,
        accounts_receivable=100e8,
        fixed_assets=60e8,
        accounts_payable=120e8
    )
    print("=== TCL ROIC Baseline ===")
    print(f"DSI: {tcl.baseline_dsi:.1f} days")
    print(f"ROIC: {tcl.baseline_roic:.2f}%")
    
    print("\n=== After IPC Optimization ===")
    print(tcl.generate_report_markdown(target_dsi=45.0, cogs_saving_ratio=0.01))
