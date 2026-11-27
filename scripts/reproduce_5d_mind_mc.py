#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Deterministic Reproducible Multi-Agent Monte Carlo Simulation
for Volume 4 Section 4.5 of 《秩序的生成、存续和进化》:
Hidden-Rule-Shift Game under the Conscience-Driven 5D Mind Model.

Run directly with Python 3 standard library (zero third-party dependencies):
    python scripts/reproduce_5d_mind_mc.py
"""

import math
import random
import statistics


BASE_SEED = 42

# Nine benchmark agent / organizational configurations:
#   (Label, (R_sens, E_supp, S_buff, A_fluid, Omega_meta), M, silicon_coupled)
CONFIGS = [
    ("1. 五维完备单脑总师 (0.90, 0.90, 0.90, 0.90, 0.90)",   (0.90, 0.90, 0.90, 0.90, 0.90), 64, False),
    ("2. 一阶同维死磕者 (Omega=0.00, A=0.95)",               (0.90, 0.90, 0.90, 0.95, 0.00), 64, False),
    ("3. 中算力无良知套利者 (E=0.00, A=0.50)",               (0.90, 0.00, 0.90, 0.50, 0.90), 64, False),
    ("4. 高算力无良知套利者 (E=0.00, A=0.95)",               (0.90, 0.00, 0.90, 0.95, 0.90), 64, False),
    ("5. 缺缓存带宽过载者 (S=0.00, A=0.95)",                 (0.90, 0.90, 0.00, 0.95, 0.90), 64, False),
    ("6. 有底线无算盘守夜人 (A=0.00, E=0.90)",               (0.90, 0.90, 0.90, 0.00, 0.90), 64, False),
    ("7. 亚临界失察麻木者 (R=0.00, A=0.95)",                 (0.00, 0.90, 0.90, 0.95, 0.90), 64, False),
    ("8. 无硅基代偿单脑总师 (0.90x5, M=1000)",               (0.90, 0.90, 0.90, 0.90, 0.90), 1000, False),
    ("9. 碳硅双层正交总体设计部 (Phi_H (x) A_S, M=1000)",    (0.90, 0.90, 0.90, 0.90, 0.90), 1000, True),
]


def simulate_episode(
    rng: random.Random,
    params: tuple,
    M: int,
    silicon_coupled: bool,
    T: int = 80,
    t_shift: int = 20,
    W_max: int = 4,
):
    """
    Explicit discrete-time five-gate neurocybernetic dynamics:
      - System health initializes at H_0 = 100.0; collapse occurs when H_t <= 0.0.
      - Effective coupled variables entering biological working memory:
          M_eff = 1  (when coupled with silicon orthogonal DAG pruning A_Silicon)
                = max(1, ceil(M * max(0.0, 1.0 - 1.045 * S_buff)))  (unassisted brain)
    """
    R_sens, E_supp, S_buff, A_fluid, Omega_meta = params
    H = 100.0
    detected = False
    redefined = False
    tau_undet = 0
    residuals = []

    M_eff = 1 if silicon_coupled else max(1, math.ceil(M * max(0.0, 1.0 - 1.045 * S_buff)))
    sigma_base = 0.655 if silicon_coupled else 0.955
    p_sense = 1.0 if silicon_coupled else R_sens

    for t in range(1, T + 1):
        # 1) Pre-shift (t < t_shift) or post-redefinition feasible steady state
        if t < t_shift or redefined:
            dx = rng.gauss(0.0, sigma_base)
            H = min(100.0, H + 0.15 - 0.05 * abs(dx))
            residuals.append(dx)
            continue

        # 2) Post-shift (t >= t_shift), Gate 1: Residual sensitivity p_sense
        if not detected:
            tau_undet += 1
            if rng.random() < p_sense:
                detected = True
                dx = rng.gauss(0.0, sigma_base)
                residuals.append(dx)
                continue
            else:
                dx = rng.gauss(0.0, 2.08)
                H -= (1.55 + 0.112 * tau_undet) + rng.gauss(0.0, 0.65)
                residuals.append(dx)
                if H <= 0.0:
                    return redefined, False, t, residuals
                continue

        # 3) Gate 2: Embodied conscience compact-support brake b_t ~ Bernoulli(E_supp)
        if rng.random() >= E_supp:
            drain_arb = 1.78 + 4.12 * A_fluid
            sigma_arb = 1.73 + 3.914 * A_fluid
            dx = rng.gauss(0.0, sigma_arb)
            H -= drain_arb + rng.gauss(0.0, 0.85)
            residuals.append(dx)
        else:
            # 4) Gate 3: Working-memory slot ceiling (M_eff <= W_max = 4)
            if M_eff > W_max:
                drain_over = 2.24 - 0.22 * S_buff
                sigma_over = 2.15 - 0.42 * S_buff
                dx = rng.gauss(0.0, sigma_over)
                H -= drain_over + rng.gauss(0.0, 0.70)
                residuals.append(dx)
            else:
                # 5) Gates 4 & 5: Joint causal synthesis & meta-language rule redefinition
                kappa = 0.65 if silicon_coupled else 0.170
                if rng.random() < kappa * A_fluid * Omega_meta:
                    redefined = True
                    dx = rng.gauss(0.0, sigma_base)
                    residuals.append(dx)
                else:
                    drain_search = 1.96 + 1.40 * A_fluid * (1.0 - Omega_meta)
                    sigma_search = (0.655 if silicon_coupled else 0.99) + 1.48 * A_fluid * (1.0 - Omega_meta)
                    dx = rng.gauss(0.0, sigma_search)
                    H -= drain_search + rng.gauss(0.0, 0.50)
                    residuals.append(dx)

        if H <= 0.0:
            return redefined, False, t, residuals

    return redefined, True, None, residuals


def main():
    N = 1000
    print(f"{'Configuration':<50} | {'M':>4} | {'Redef%':>6} | {'Surv%':>6} | {'T_coll':>6} | {'n_c':>4} | {'Var(dX)':>7}")
    print("-" * 101)
    post_mod = post_high = None
    for idx, (label, params, M, sil) in enumerate(CONFIGS):
        rng = random.Random(BASE_SEED + idx * 1009)
        redef_cnt = 0
        surv_cnt = 0
        coll_steps = []
        vars_list = []
        for _ in range(N):
            redef, surv, t_coll, res = simulate_episode(rng, params, M, sil)
            redef_cnt += int(redef)
            surv_cnt += int(surv)
            if t_coll is not None:
                coll_steps.append(t_coll)
            vars_list.append(statistics.pvariance(res))
        mean_coll = statistics.mean(coll_steps) if coll_steps else None
        if "中算力无良知套利者" in label:
            post_mod = mean_coll - 20.0
        if "高算力无良知套利者" in label:
            post_high = mean_coll - 20.0
        coll_str = f"{mean_coll:.1f}" if mean_coll is not None else ">80"
        print(
            f"{label:<50} | {M:>4} | {100.0 * redef_cnt / N:>5.1f}% | "
            f"{100.0 * surv_cnt / N:>5.1f}% | {coll_str:>6} | {len(coll_steps):>4} | {statistics.mean(vars_list):>7.2f}"
        )
    if post_mod and post_high:
        shortening = 100.0 * (post_mod - post_high) / post_mod
        print(
            f"\n[Theorem 3.3 Check] Post-shift survival under E_supp=0: "
            f"{post_mod:.1f} -> {post_high:.1f} steps (-{shortening:.1f}%)"
        )


if __name__ == "__main__":
    main()
