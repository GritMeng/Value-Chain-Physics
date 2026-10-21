# -*- coding: utf-8 -*-
import os
import matplotlib.pyplot as plt
import matplotlib.patches as patches

# Configure Matplotlib fonts for Windows Chinese rendering
plt.rcParams['font.sans-serif'] = ['Microsoft YaHei', 'SimHei', 'Arial Unicode MS', 'Segoe UI', 'DejaVu Sans']
plt.rcParams['axes.unicode_minus'] = False

def draw_exact_visio_flowchart(output_path):
    # Canvas setup: Spacious canvas with exact engineering grid background
    fig, ax = plt.subplots(figsize=(26, 38), dpi=300)
    bg_color = '#ffffff'
    ax.set_facecolor(bg_color)
    fig.patch.set_facecolor(bg_color)
    ax.set_xlim(0, 100)
    ax.set_ylim(-35, 115)
    ax.axis('off')

    # Draw Visio/ProcessOn Blueprint Grid lines
    for x in range(0, 101, 2):
        ax.plot([x, x], [-35, 115], color='#e2e8f0', lw=0.5, zorder=0)
    for y in range(-35, 116, 2):
        ax.plot([0, 100], [y, y], color='#e2e8f0', lw=0.5, zorder=0)
    for x in range(0, 101, 10):
        ax.plot([x, x], [-35, 115], color='#cbd5e1', lw=0.9, zorder=0)
    for y in range(-35, 116, 10):
        ax.plot([0, 100], [y, y], color='#cbd5e1', lw=0.9, zorder=0)

    # Style Palettes
    c_blue_io = {'fill': '#3b82f6', 'border': '#1d4ed8', 'text': '#ffffff'}
    c_green_algo = {'fill': '#10b981', 'border': '#047857', 'text': '#ffffff'}
    c_orange_dec = {'fill': '#f97316', 'border': '#c2410c', 'text': '#ffffff'}
    c_purple_goal = {'fill': '#8b5cf6', 'border': '#6d28d9', 'text': '#ffffff'}

    # 1. Top-Left Legend (图例) Box
    legend_box = patches.FancyBboxPatch((2, 100), 22, 12, boxstyle="square,pad=0.4",
                                         facecolor='#f8fafc', edgecolor='#94a3b8', linewidth=1.5, zorder=2)
    ax.add_patch(legend_box)

    # Legend Parallelogram
    poly_p = patches.Polygon([[4, 109.5], [10, 109.5], [8, 107.5], [2, 107.5]],
                             facecolor=c_blue_io['fill'], edgecolor=c_blue_io['border'], lw=1.2, zorder=3)
    ax.add_patch(poly_p)
    ax.text(6, 108.5, "输入或输出数据", color='#ffffff', fontsize=7.5, fontweight='bold', ha='center', va='center', zorder=4)

    # Legend Diamond
    poly_d = patches.Polygon([[5, 106], [8, 104.5], [5, 103], [2, 104.5]],
                             facecolor=c_orange_dec['fill'], edgecolor=c_orange_dec['border'], lw=1.2, zorder=3)
    ax.add_patch(poly_d)
    ax.text(5, 104.5, "判定", color='#ffffff', fontsize=8, fontweight='bold', ha='center', va='center', zorder=4)

    # Legend Rectangle
    rect_l = patches.Rectangle((2, 100.5), 6, 1.8, facecolor=c_green_algo['fill'], edgecolor=c_green_algo['border'], lw=1.2, zorder=3)
    ax.add_patch(rect_l)
    ax.text(5, 101.4, "逻辑算法", color='#ffffff', fontsize=8, fontweight='bold', ha='center', va='center', zorder=4)

    # Legend Arrows
    ax.annotate('', xy=(20, 108.5), xytext=(12, 108.5),
                arrowprops=dict(arrowstyle="->", color="#2563eb", lw=1.5, mutation_scale=12), zorder=3)
    ax.text(14, 109.5, "走向", color='#2563eb', fontsize=8, fontweight='bold', zorder=4)

    ax.annotate('', xy=(20, 104.5), xytext=(12, 104.5),
                arrowprops=dict(arrowstyle="->", color="#000000", lw=1.5, mutation_scale=12), zorder=3)
    ax.text(14, 105.5, "关联", color='#000000', fontsize=8, fontweight='bold', zorder=4)


    # Shape Helpers
    def draw_parallelogram(x, y, w, h, text, font_size=8, bg='#3b82f6', border='#1d4ed8'):
        dx = 1.5
        pts = [[x - w/2 + dx, y + h/2], [x + w/2 + dx, y + h/2],
               [x + w/2 - dx, y - h/2], [x - w/2 - dx, y - h/2]]
        poly = patches.Polygon(pts, facecolor=bg, edgecolor=border, lw=1.3, zorder=3)
        ax.add_patch(poly)
        lines = text.split('\n')
        if len(lines) == 1:
            ax.text(x, y, text, color='#ffffff', fontsize=font_size, fontweight='bold', ha='center', va='center', zorder=4)
        else:
            ax.text(x, y + 0.6, lines[0], color='#ffffff', fontsize=font_size, fontweight='bold', ha='center', va='center', zorder=4)
            ax.text(x, y - 0.7, lines[1], color='#ffffff', fontsize=font_size - 0.5, fontweight='bold', ha='center', va='center', zorder=4)

    def draw_rect(x, y, w, h, text, font_size=8.5, bg='#10b981', border='#047857'):
        rect = patches.Rectangle((x - w/2, y - h/2), w, h, facecolor=bg, edgecolor=border, lw=1.3, zorder=3)
        ax.add_patch(rect)
        lines = text.split('\n')
        if len(lines) == 1:
            ax.text(x, y, text, color='#ffffff', fontsize=font_size, fontweight='bold', ha='center', va='center', zorder=4)
        else:
            ax.text(x, y + 0.6, lines[0], color='#ffffff', fontsize=font_size, fontweight='bold', ha='center', va='center', zorder=4)
            ax.text(x, y - 0.7, lines[1], color='#ffffff', fontsize=font_size - 0.5, fontweight='bold', ha='center', va='center', zorder=4)

    def draw_diamond(x, y, w, h, text, font_size=8, bg='#f97316', border='#c2410c'):
        pts = [[x, y + h/2], [x + w/2, y], [x, y - h/2], [x - w/2, y]]
        poly = patches.Polygon(pts, facecolor=bg, edgecolor=border, lw=1.3, zorder=3)
        ax.add_patch(poly)
        lines = text.split('\n')
        if len(lines) == 1:
            ax.text(x, y, text, color='#ffffff', fontsize=font_size, fontweight='bold', ha='center', va='center', zorder=4)
        else:
            ax.text(x, y + 0.5, lines[0], color='#ffffff', fontsize=font_size, fontweight='bold', ha='center', va='center', zorder=4)
            ax.text(x, y - 0.6, lines[1], color='#ffffff', fontsize=font_size - 0.5, fontweight='bold', ha='center', va='center', zorder=4)

    def draw_oval(x, y, w, h, text, font_size=9, bg='#8b5cf6', border='#6d28d9'):
        ellipse = patches.Ellipse((x, y), w, h, facecolor=bg, edgecolor=border, lw=1.5, zorder=3)
        ax.add_patch(ellipse)
        ax.text(x, y, text, color='#ffffff', fontsize=font_size, fontweight='bold', ha='center', va='center', zorder=4)

    def draw_arrow(x1, y1, x2, y2, label="", color="#2563eb", linestyle="-"):
        ax.annotate('', xy=(x2, y2), xytext=(x1, y1),
                    arrowprops=dict(arrowstyle="->", color=color, lw=1.4, ls=linestyle, mutation_scale=12), zorder=5)
        if label:
            mx, my = (x1 + x2)/2, (y1 + y2)/2
            ax.text(mx + 0.8, my + 0.4, label, color=color, fontsize=8, fontweight='bold', zorder=6)

    def draw_poly_arrow(points, label="", color="#2563eb"):
        xs, ys = zip(*points)
        ax.plot(xs, ys, color=color, lw=1.4, zorder=5)
        ax.annotate('', xy=(points[-1][0], points[-1][1]), xytext=(points[-2][0], points[-2][1]),
                    arrowprops=dict(arrowstyle="->", color=color, lw=1.4, mutation_scale=12), zorder=5)
        if label:
            mx, my = points[1][0], points[1][1]
            ax.text(mx + 0.8, my + 0.4, label, color=color, fontsize=8, fontweight='bold', zorder=6)


    # ==================== LAYOUT OF THE COMPLETE PROCESS ====================

    # Row 1: Inputs & Top Level (y = 108)
    draw_parallelogram(35, 108, 14, 3.2, "供应计划")
    draw_parallelogram(52, 98, 12, 3.0, "有效的需求")
    draw_parallelogram(64, 98, 10, 3.0, "单层BOM")
    draw_parallelogram(74, 98, 8, 3.0, "约束")
    draw_parallelogram(84, 98, 10, 3.0, "物料约束")
    draw_parallelogram(94, 98, 8, 3.0, "供应源")

    # Row 2: Tactical Algorithm & Expansion (y = 86)
    draw_rect(35, 86, 14, 3.5, "节拍拆分订单算法")
    draw_rect(60, 86, 14, 3.5, "供应链网络\n模型算法")

    # Connect top inputs to algorithms
    draw_arrow(35, 106.4, 35, 87.75, "", "#2563eb")
    draw_poly_arrow([[52, 96.5], [52, 92], [60, 92], [60, 87.75]], "", "#2563eb")
    draw_poly_arrow([[64, 96.5], [64, 92], [60, 92], [60, 87.75]], "", "#2563eb")
    draw_poly_arrow([[74, 96.5], [74, 92], [60, 92], [60, 87.75]], "", "#2563eb")
    draw_poly_arrow([[84, 96.5], [84, 92], [60, 92], [60, 87.75]], "", "#2563eb")
    draw_poly_arrow([[94, 96.5], [94, 92], [60, 92], [60, 87.75]], "", "#2563eb")

    # Row 3: Parallel Gate & Network Output (y = 76)
    draw_diamond(35, 76, 12, 3.2, "并行规划")
    draw_parallelogram(60, 76, 16, 3.2, "附带约束的\n供应链网络")
    draw_arrow(35, 84.25, 35, 77.6, "", "#2563eb")
    draw_arrow(60, 84.25, 60, 77.6, "", "#2563eb")

    # Row 4: Demand Prioritization (y = 66)
    draw_rect(35, 66, 14, 3.5, "需求排序")
    draw_arrow(35, 74.4, 35, 67.75, "", "#2563eb")

    # Row 5: Supply Demand Matching (y = 56)
    draw_rect(60, 56, 16, 3.5, "供应需求匹配")
    draw_parallelogram(85, 66, 12, 3.0, "约束容量")
    draw_rect(76, 56, 14, 3.5, "约束可用量算法\n(ATP)")
    draw_parallelogram(90, 56, 10, 3.0, "需求约束\nPegging")
    draw_parallelogram(68, 48, 12, 3.0, "可用约束")

    draw_arrow(60, 74.4, 60, 57.75, "", "#2563eb")
    draw_poly_arrow([[35, 64.25], [35, 56], [52, 56]], "当前需求", "#2563eb")

    # Row 6: Match List & Inventory ATP (y = 44)
    draw_parallelogram(60, 44, 16, 3.2, "需求供应\n匹配列表")
    draw_parallelogram(68, 36, 12, 3.0, "可用的库存")
    draw_rect(76, 36, 14, 3.5, "库存可用量算法\n(ATP)")
    draw_parallelogram(90, 44, 8, 3.0, "库存")
    draw_parallelogram(90, 36, 10, 3.0, "计划到料")
    draw_parallelogram(90, 28, 12, 3.0, "需求供给\nPegging")

    draw_arrow(60, 54.25, 60, 45.6, "", "#2563eb")
    draw_arrow(76, 54.25, 68, 49.5, "", "#2563eb")
    draw_arrow(76, 54.25, 90, 57.5, "", "#000000")
    draw_arrow(85, 64.5, 76, 57.75, "", "#2563eb")

    # Row 7: Forward Alignment (y = 34)
    draw_rect(40, 34, 14, 3.5, "清空F和其对应\n的计划数据")
    draw_parallelogram(52, 34, 12, 3.0, "相应需求\n再当前需求")
    draw_rect(60, 34, 16, 3.5, "Forward齐套算法")
    draw_parallelogram(60, 24, 18, 3.5, "备料齐套数量日期\n即物料开始日期")
    draw_rect(60, 14, 16, 3.5, "Forward约束分配\n算法")
    draw_parallelogram(60, 3, 18, 3.5, "父阶物料对应约束的\n开始日期和结束日期\n即父阶物料的可用日期")

    draw_arrow(60, 42.4, 60, 35.75, "", "#2563eb")
    draw_arrow(60, 32.25, 60, 25.75, "", "#2563eb")
    draw_arrow(60, 22.25, 60, 15.75, "", "#2563eb")
    draw_arrow(60, 12.25, 60, 4.75, "", "#2563eb")

    draw_arrow(76, 34.25, 68, 37.5, "", "#2563eb")
    draw_arrow(76, 34.25, 90, 45.5, "", "#000000")
    draw_arrow(76, 34.25, 90, 37.5, "", "#000000")
    draw_arrow(76, 34.25, 90, 29.5, "", "#000000")

    # Loop Backs (Y / N Branch Diamonds) (y = -7 to -30)
    draw_diamond(30, 24, 14, 3.5, "找到可以使其加速并可以\nmoving back的标识需求")
    draw_arrow(30, 22.25, 35, 64.25, "Y,标记Schedule Line为F", "#2563eb")
    draw_poly_arrow([[52, 32.5], [40, 32.5], [40, 35.75]], "", "#2563eb")

    draw_diamond(60, -7, 14, 3.5, "是否为根物料")
    draw_arrow(60, 1.25, 60, -5.25, "", "#2563eb")

    draw_diamond(52, -15, 14, 3.5, "是否为F重算需求")
    draw_diamond(68, -15, 14, 3.5, "是否晚期订单\n已算完")
    draw_arrow(60, -8.75, 52, -13.25, "Y", "#2563eb")
    draw_arrow(60, -8.75, 68, -13.25, "Y", "#2563eb")

    draw_diamond(60, -22, 14, 3.5, "是否满足需求日期")
    draw_arrow(52, -16.75, 60, -20.25, "N", "#2563eb")
    draw_arrow(68, -16.75, 60, -20.25, "Y", "#2563eb")

    draw_rect(30, -18, 14, 3.5, "交期为根物料\n可用日期")
    draw_diamond(18, -18, 12, 3.2, "是否满足\n交期")

    draw_rect(60, -29, 14, 3.5, "交期为需求日期")
    draw_diamond(74, -29, 12, 3.2, "是否为配套组")
    draw_rect(88, -29, 14, 3.5, "组齐套日期为交期")

    draw_arrow(60, -23.75, 60, -27.25, "Y", "#2563eb")
    draw_arrow(60, -29, 74, -29, "Y", "#2563eb")
    draw_arrow(74, -29, 88, -29, "Y", "#2563eb")

    draw_diamond(60, -35, 14, 3.5, "是否为Planned\nOrder满足")
    draw_arrow(60, -30.75, 60, -33.25, "N", "#2563eb")

    # Bottom Stage: Backward Alignment & Changeover Minimization
    draw_rect(60, -42, 16, 3.5, "Backward分配约束\n和库存")
    draw_arrow(60, -36.75, 60, -40.25, "", "#2563eb")

    draw_parallelogram(35, -49, 14, 3.2, "需求交付的\nschedule line")
    draw_parallelogram(60, -49, 16, 3.2, "物料对应的\nPlanned Order结构\n日期和开始日期")
    draw_parallelogram(85, -49, 14, 3.2, "Allocation/即相关\n需求")

    draw_arrow(60, -43.75, 35, -47.4, "", "#2563eb")
    draw_arrow(60, -43.75, 60, -47.4, "", "#2563eb")
    draw_arrow(60, -43.75, 85, -47.4, "", "#2563eb")

    draw_diamond(60, -56, 14, 3.5, "是否继续向下\n展开")
    draw_arrow(60, -50.6, 60, -54.25, "", "#2563eb")

    draw_diamond(60, -63, 14, 3.5, "是否需求\n处理完毕")
    draw_arrow(60, -57.75, 60, -61.25, "N", "#2563eb")

    # Final Milestone / Goal Oval (Purple)
    draw_rect(45, -70, 14, 3.5, "Forward 补产能前\n边的空缺")
    draw_parallelogram(60, -70, 14, 3.2, "需求为F标识的\nschedule Line")
    draw_rect(45, -78, 16, 3.5, "Forward 需要合单\n来减少change over\n的需求")

    draw_oval(85, -67, 20, 4.5, "到此为最大化交付和资源利用", font_size=8.5)

    draw_arrow(60, -64.75, 45, -68.25, "Y", "#2563eb")
    draw_arrow(60, -64.75, 85, -64.75, "Y", "#8b5cf6")
    draw_arrow(45, -71.75, 45, -76.25, "", "#2563eb")

    # Save exact ProcessOn/Visio diagram
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    plt.tight_layout()
    plt.savefig(output_path, dpi=300, bbox_inches='tight', facecolor=bg_color)
    plt.close()
    print(f"Exact Visio/ProcessOn flowchart successfully generated and saved to: {output_path}")

if __name__ == "__main__":
    output_png = r"h:\IPC\方案\images\ibp_to_scheduling_master_flowchart.png"
    draw_exact_visio_flowchart(output_png)
