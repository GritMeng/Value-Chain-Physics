# -*- coding: utf-8 -*-
"""
IPC Diagnosis Runner
Loads financial parameters from command line or interactive input,
calculates ROIC optimization using roic_calc.py, and outputs the PR-ready markdown report.
"""
import sys
import os

# Ensure the parent directory is in the path to import roic_calc
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from roic_calc import ROICCalculator

def run_interactive():
    # Set stdout to UTF-8
    if hasattr(sys.stdout, 'reconfigure'):
        sys.stdout.reconfigure(encoding='utf-8')
        
    print("==================================================")
    print("   Value Chain Physics - IPC ROIC Diagnosis Tool  ")
    print("==================================================")
    
    try:
        company = input("请输入公司/证券名称 (例如: 立讯精密 002475.SZ): ")
        revenue = float(input("请输入营业收入 (元, 例如 2000e8): "))
        cogs = float(input("请输入主营业务成本/COGS (元, 例如 1800e8): "))
        net_profit = float(input("请输入净利润 (元, 例如 120e8): "))
        inventory = float(input("请输入期末存货金额 (元, 例如 300e8): "))
        ar = float(input("请输入应收账款及票据 (元, 例如 250e8): "))
        fa = float(input("请输入固定资产净额 (元, 例如 150e8): "))
        ap = float(input("请输入应付账款及票据 (元, 例如 200e8, 没有输0): "))
        
        target_dsi = float(input("请输入目标存货周转天数 (天, 默认 45): ") or 45)
        cogs_saving = float(input("请输入计划节省比例 (%, 默认 1.0%): ") or 1.0) / 100.0
    except ValueError as e:
        print(f"输入格式错误: {e}")
        return

    calc = ROICCalculator(
        company_name=company,
        revenue=revenue,
        cogs=cogs,
        net_profit=net_profit,
        inventory=inventory,
        accounts_receivable=ar,
        fixed_assets=fa,
        accounts_payable=ap
    )
    
    report_md = calc.generate_report_markdown(target_dsi=target_dsi, cogs_saving_ratio=cogs_saving)
    
    # Save report to docs/pr_articles/
    output_dir = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "docs", "pr_articles")
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
        
    safe_name = "".join([c for c in company if c.isalnum() or c in (' ', '_')]).rstrip().replace(" ", "_")
    output_path = os.path.join(output_dir, f"{safe_name}_roic_report.md")
    
    with open(output_path, "w", encoding="utf-8") as f:
        f.write(report_md)
        
    print("\n==================================================")
    print(f"✅ 诊断报告生成成功！")
    print(f"保存路径: {output_path}")
    print("==================================================\n")
    print(report_md)

if __name__ == "__main__":
    run_interactive()
