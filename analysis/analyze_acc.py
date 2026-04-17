#!/usr/bin/env python3
"""Analyze skateboard accelerometer CSVs — stdlib only."""
import csv, glob, math, os, statistics

DATA_DIR = os.path.join(os.path.dirname(__file__), '..', 'acc-data')

def load(path):
    rows = []
    with open(path) as f:
        for r in csv.reader(f):
            x, y, z = float(r[0]), float(r[1]), float(r[2])
            mag = math.sqrt(x*x + y*y + z*z)
            rows.append((x, y, z, mag, abs(mag - 9.81)))
    return rows  # [(x, y, z, mag, delta), ...]

def rolling_avg(vals, w=10):
    out, s = [], sum(vals[:w])
    for i in range(w, len(vals)):
        out.append(s / w)
        s += vals[i] - vals[i - w]
    return out

def percentiles(vals, ps=(25, 50, 75, 90, 95, 99)):
    s = sorted(vals)
    n = len(s)
    return {p: s[int(n * p / 100)] for p in ps}

def analyze(name, rows):
    n = len(rows)
    deltas = [r[4] for r in rows]
    mags = [r[3] for r in rows]

    print(f"\n{'='*60}")
    print(f"FILE: {name}  ({n} samples, ~{n*0.1:.0f}s)")
    print(f"{'='*60}")

    print(f"\nRaw magnitude: mean={statistics.mean(mags):.2f}  std={statistics.stdev(mags):.2f}  min={min(mags):.2f}  max={max(mags):.2f}")

    d_pct = percentiles(deltas)
    print(f"\nDelta from gravity (|mag - 9.81|):")
    print(f"  mean={statistics.mean(deltas):.2f}  std={statistics.stdev(deltas):.2f}  max={max(deltas):.2f}")
    print(f"  percentiles: " + "  ".join(f"p{p}={v:.2f}" for p, v in d_pct.items()))

    for t in [0.5, 1.0, 2.0, 3.0, 4.0, 6.0, 8.0, 10.0]:
        pct = sum(1 for d in deltas if d > t) / n * 100
        print(f"  delta > {t:4.1f}: {pct:5.1f}%")

    roll = rolling_avg(deltas)
    r_pct = percentiles(roll)
    print(f"\nRolling avg (w=10): mean={statistics.mean(roll):.2f}  max={max(roll):.2f}")
    print(f"  percentiles: " + "  ".join(f"p{p}={v:.2f}" for p, v in r_pct.items()))

    # Jerk
    jerk = [abs(mags[i] - mags[i-1]) for i in range(1, n)]
    j_pct = percentiles(jerk)
    print(f"\nJerk (|Δmag|): mean={statistics.mean(jerk):.2f}  std={statistics.stdev(jerk):.2f}  max={max(jerk):.2f}")
    print(f"  percentiles: " + "  ".join(f"p{p}={v:.2f}" for p, v in j_pct.items()))

    # Per-axis
    print(f"\nPer-axis:")
    for i, ax in enumerate('xyz'):
        vals = [r[i] for r in rows]
        print(f"  {ax}: mean={statistics.mean(vals):.2f}  std={statistics.stdev(vals):.2f}  range=[{min(vals):.2f}, {max(vals):.2f}]")

    # Motion states from rolling avg
    still = sum(1 for v in roll if v < 1.0) / len(roll) * 100
    cruise = sum(1 for v in roll if 1.0 <= v < 3.0) / len(roll) * 100
    active = sum(1 for v in roll if 3.0 <= v < 6.0) / len(roll) * 100
    intense = sum(1 for v in roll if v >= 6.0) / len(roll) * 100
    print(f"\nMotion states (rolling avg):")
    print(f"  still (<1):    {still:5.1f}%")
    print(f"  cruising (1-3):{cruise:5.1f}%")
    print(f"  active (3-6):  {active:5.1f}%")
    print(f"  intense (>6):  {intense:5.1f}%")

    # Detect spikes (potential tricks/impacts) — jerk > p99
    spike_thresh = j_pct[99]
    spikes = [(i, jerk[i]) for i in range(len(jerk)) if jerk[i] > spike_thresh]
    # Cluster spikes within 5 samples of each other
    clusters = []
    for idx, val in spikes:
        if clusters and idx - clusters[-1][-1][0] <= 5:
            clusters[-1].append((idx, val))
        else:
            clusters.append([(idx, val)])
    print(f"\nImpact/trick spikes (jerk > {spike_thresh:.2f}, {len(clusters)} clusters):")
    for c in clusters[:10]:
        peak = max(c, key=lambda x: x[1])
        print(f"  sample {peak[0]} (t≈{peak[0]*0.1:.1f}s): jerk={peak[1]:.2f}, delta={deltas[peak[0]]:.2f}")

if __name__ == '__main__':
    for f in sorted(glob.glob(os.path.join(DATA_DIR, '*.csv'))):
        analyze(os.path.basename(f), load(f))
