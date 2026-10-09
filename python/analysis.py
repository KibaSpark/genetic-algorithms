"""analysis.py — Resume tus corridas y las compara contra la Tabla 1 del paper.

Para cada combinación (K_pop, K_mut) calcula lo que el paper NO reporta: dispersión,
mediana, rango y cuántas corridas quedaron censuradas (no llegaron a NL 104).

Correr:  python python/analysis.py results/paper_runs.csv

Salidas (en la misma carpeta que el CSV de entrada):
  <tag>_summary.csv     estadísticas por celda
  <tag>_vs_paper.csv    tu promedio contra el del paper, con diferencia relativa
"""

import argparse
import sys
from pathlib import Path

import pandas as pd

ROOT = Path(__file__).resolve().parent.parent
PAPER_TABLE = ROOT / "data" / "paper_table1.csv"


def load_paper() -> pd.DataFrame:
    wide = pd.read_csv(PAPER_TABLE)
    long = wide.melt(id_vars="k_mut", var_name="k_pop", value_name="paper_mean")
    long["k_pop"] = long["k_pop"].astype(int)
    return long


def summarize(runs: pd.DataFrame) -> pd.DataFrame:
    g = runs.groupby(["k_pop", "k_mut"])
    out = g["k_sbox"].agg(
        n="count",
        mean="mean",
        median="median",
        sd="std",
        q1=lambda x: x.quantile(0.25),
        q3=lambda x: x.quantile(0.75),
        min="min",
        max="max",
    )
    out["censored"] = g["success"].apply(lambda s: int((s == 0).sum()))
    out["success_rate"] = g["success"].mean()
    out["mean_seconds"] = g["seconds"].mean()
    return out.reset_index()


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("runs_csv", help="archivo <tag>_runs.csv generado por sbox_ga")
    args = ap.parse_args()

    path = Path(args.runs_csv)
    runs = pd.read_csv(path)
    tag = path.name.removesuffix("_runs.csv")

    summary = summarize(runs)
    summary.to_csv(path.with_name(f"{tag}_summary.csv"), index=False)

    comp = summary.merge(load_paper(), on=["k_pop", "k_mut"], how="left")
    comp["diff_pct"] = 100 * (comp["mean"] - comp["paper_mean"]) / comp["paper_mean"]
    comp = comp[["k_pop", "k_mut", "n", "censored", "paper_mean", "mean", "diff_pct", "median", "sd", "q1", "q3"]]
    comp.to_csv(path.with_name(f"{tag}_vs_paper.csv"), index=False)

    pd.set_option("display.width", 160)
    print(f"Corridas: {len(runs)}  |  combinaciones: {len(summary)}  |  censuradas: {int(summary['censored'].sum())}\n")
    print(comp.round(1).to_string(index=False))
    if summary["censored"].sum() > 0:
        print("\nOjo: hay corridas censuradas; su k_sbox es el presupuesto gastado, no el tiempo hasta el éxito."
              " El promedio de esas celdas subestima el costo real.")
    print(f"\nGuardado: {tag}_summary.csv y {tag}_vs_paper.csv")
    return 0


if __name__ == "__main__":
    sys.exit(main())
