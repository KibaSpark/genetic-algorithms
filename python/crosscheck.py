"""crosscheck.py — Verificación cruzada: ¿el C++ calcula lo mismo que la referencia de Python?

Toma S-boxes que guardó el C++ (results/<tag>_sboxes.csv), les recalcula NL y WHS con
reference.py y compara. Si todo cuadra, tu C++ es confiable.

Correr:  python python/crosscheck.py results/paper_sboxes.csv --sample 50
"""

import argparse
import sys
from pathlib import Path

import numpy as np
import pandas as pd

sys.path.insert(0, str(Path(__file__).resolve().parent))
import reference  # noqa: E402


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("sboxes_csv", help="archivo <tag>_sboxes.csv generado por sbox_ga")
    ap.add_argument("--sample", type=int, default=50, help="cuántas S-boxes revisar (default 50)")
    ap.add_argument("--seed", type=int, default=0)
    args = ap.parse_args()

    df = pd.read_csv(args.sboxes_csv)
    if df.empty:
        print("El archivo no tiene S-boxes.")
        return 1
    rows = df.sample(n=min(args.sample, len(df)), random_state=args.seed)

    bad = 0
    for _, r in rows.iterrows():
        s = reference.sbox_from_hex(r["sbox_hex"])
        if sorted(s.tolist()) != list(range(256)):
            print(f"NO BIYECTIVA: k_pop={r.k_pop} k_mut={r.k_mut} run={r.run}")
            bad += 1
            continue
        nl, cost = reference.evaluate(s)
        rel = abs(cost - r["whs"]) / max(abs(cost), 1e-300)
        if nl != r["nl"] or rel > 1e-9:
            print(f"DIFERENCIA k_pop={r.k_pop} k_mut={r.k_mut} run={r.run}: "
                  f"C++ NL={r['nl']} WHS={r['whs']:.6e} | Python NL={nl} WHS={cost:.6e}")
            bad += 1

    print(f"\nRevisadas {len(rows)} S-boxes: {len(rows) - bad} coinciden, {bad} con diferencias.")
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
