#!/usr/bin/env python3
"""Create the benchmark plot using only the Python standard library."""

import csv
import html
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INPUT = ROOT / "results" / "benchmark.csv"
OUTPUT = ROOT / "results" / "performance.svg"


def series_name(row):
    return row["algorithm"] if row["algorithm"] != "tiled" else f'tiled-{row["tile"]}'


with INPUT.open(newline="", encoding="utf-8") as source:
    rows = list(csv.DictReader(source))

data = {}
naive = {}
for row in rows:
    size = int(row["size"])
    value = float(row["median_ms"])
    data.setdefault(series_name(row), {})[size] = value
    if row["algorithm"] == "naive":
        naive[size] = value

width, height = 1100, 480
margin_left, margin_right, margin_top, margin_bottom = 72, 28, 50, 62
gap = 74
panel_width = (width - margin_left - margin_right - gap) / 2
panel_height = height - margin_top - margin_bottom
sizes = sorted(naive)
colors = {
    "naive": "#333333",
    "ikj": "#d55e00",
    "tiled-16": "#56b4e9",
    "tiled-32": "#0072b2",
    "tiled-64": "#009e73",
}


def x_position(size, panel):
    start = margin_left + panel * (panel_width + gap)
    return start + sizes.index(size) * panel_width / (len(sizes) - 1)


def add_panel(parts, panel, title, values, maximum, unit):
    x0 = margin_left + panel * (panel_width + gap)
    y0 = margin_top + panel_height
    parts.append(f'<text x="{x0 + panel_width / 2:.1f}" y="25" class="title">{title}</text>')
    for tick in range(6):
        value = maximum * tick / 5
        y = y0 - panel_height * tick / 5
        parts.append(f'<line x1="{x0}" y1="{y:.1f}" x2="{x0 + panel_width:.1f}" y2="{y:.1f}" class="grid"/>')
        parts.append(f'<text x="{x0 - 10}" y="{y + 4:.1f}" class="tick" text-anchor="end">{value:g}</text>')
    for size in sizes:
        x = x_position(size, panel)
        parts.append(f'<text x="{x:.1f}" y="{y0 + 24}" class="tick" text-anchor="middle">{size}</text>')
    parts.append(f'<text x="{x0 + panel_width / 2:.1f}" y="{height - 10}" class="label">Matrix dimension N</text>')
    parts.append(f'<text transform="translate({x0 - 53:.1f},{margin_top + panel_height / 2:.1f}) rotate(-90)" class="label">{unit}</text>')

    for name, points in values.items():
        coordinates = []
        for size in sizes:
            if size in points:
                x = x_position(size, panel)
                y = y0 - panel_height * points[size] / maximum
                coordinates.append((x, y))
        path = " ".join(f"{x:.1f},{y:.1f}" for x, y in coordinates)
        color = colors[name]
        parts.append(f'<polyline points="{path}" fill="none" stroke="{color}" stroke-width="2.5"/>')
        for x, y in coordinates:
            parts.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="3.5" fill="{color}"/>')


speedups = {
    name: {size: naive[size] / value for size, value in points.items()}
    for name, points in data.items() if name != "naive"
}
runtime_max = 16000
speedup_max = 20
svg = [f'''<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">
<style>
text {{ font-family: Arial, sans-serif; fill: #222; }}
.title {{ font-size: 18px; font-weight: 600; text-anchor: middle; }}
.label {{ font-size: 13px; text-anchor: middle; }}
.tick {{ font-size: 12px; fill: #555; }}
.grid {{ stroke: #ddd; stroke-width: 1; }}
</style>
<rect width="100%" height="100%" fill="white"/>''']
add_panel(svg, 0, "Runtime vs. matrix size", data, runtime_max, "Median runtime (ms)")
add_panel(svg, 1, "Speedup relative to naive", speedups, speedup_max, "Speedup (×)")

legend_x, legend_y = margin_left + 8, margin_top + 16
for index, name in enumerate(data):
    x = legend_x + (index % 3) * 125
    y = legend_y + (index // 3) * 21
    svg.append(f'<line x1="{x}" y1="{y}" x2="{x + 20}" y2="{y}" stroke="{colors[name]}" stroke-width="3"/>')
    svg.append(f'<text x="{x + 27}" y="{y + 4}" class="tick">{html.escape(name)}</text>')
svg.append("</svg>\n")
OUTPUT.write_text("\n".join(svg), encoding="utf-8")
print(OUTPUT)
