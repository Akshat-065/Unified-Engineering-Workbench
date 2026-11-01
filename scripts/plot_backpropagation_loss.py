#!/usr/bin/env python3
"""Render an SVG loss plot from the history exported by the C++ Task F implementation."""
import argparse
import csv
import html
import math
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, default=Path("results/backpropagation_loss.csv"))
    parser.add_argument("--output", type=Path, default=Path("results/backpropagation_loss.svg"))
    args = parser.parse_args()

    steps, losses = [], []
    with args.input.open(newline="", encoding="utf-8") as source:
        reader = csv.DictReader(source)
        required = {"training_step", "loss"}
        if not reader.fieldnames or not required.issubset(reader.fieldnames):
            raise ValueError("CSV must contain training_step and loss columns")
        for row_number, row in enumerate(reader, start=2):
            step, loss = int(row["training_step"]), float(row["loss"])
            if not math.isfinite(loss):
                raise ValueError(f"CSV row {row_number} has a non-finite loss")
            steps.append(step)
            losses.append(loss)
    if not steps:
        raise ValueError("CSV contains no training rows")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    width, height = 960, 520
    left, right, top, bottom = 78, 28, 72, 62
    x_min, x_max = min(steps), max(steps)
    y_min, y_max = min(0.0, min(losses)), max(losses)
    if y_max <= y_min:
        y_max = y_min + 1.0
    def sx(step: int) -> float:
        return left + (step - x_min) * (width - left - right) / max(1, x_max - x_min)
    def sy(loss: float) -> float:
        return height - bottom - (loss - y_min) * (height - top - bottom) / (y_max - y_min)
    polyline = " ".join(f"{sx(step):.2f},{sy(loss):.2f}" for step, loss in zip(steps, losses))
    title = html.escape("Task F: loss versus training step")
    svg = f'''<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">
<rect width="100%" height="100%" fill="#ffffff"/>
<text x="{width/2}" y="34" text-anchor="middle" font-family="Segoe UI,Arial,sans-serif" font-size="20" fill="#172b43">{title}</text>
<line x1="{left}" y1="{top}" x2="{left}" y2="{height-bottom}" stroke="#8795a5"/><line x1="{left}" y1="{height-bottom}" x2="{width-right}" y2="{height-bottom}" stroke="#8795a5"/>
<line x1="{left}" y1="{sy(y_max)}" x2="{width-right}" y2="{sy(y_max)}" stroke="#e4e9ef"/><line x1="{left}" y1="{sy(y_min)}" x2="{width-right}" y2="{sy(y_min)}" stroke="#e4e9ef"/>
<text x="{left-10}" y="{sy(y_max)+4}" text-anchor="end" font-family="Segoe UI,Arial,sans-serif" font-size="12" fill="#526477">{y_max:.4g}</text>
<text x="{left-10}" y="{sy(y_min)+4}" text-anchor="end" font-family="Segoe UI,Arial,sans-serif" font-size="12" fill="#526477">{y_min:.4g}</text>
<text x="{left}" y="{height-bottom+22}" font-family="Segoe UI,Arial,sans-serif" font-size="12" fill="#526477">{x_min}</text>
<text x="{width-right}" y="{height-bottom+22}" text-anchor="end" font-family="Segoe UI,Arial,sans-serif" font-size="12" fill="#526477">{x_max}</text>
<polyline points="{polyline}" fill="none" stroke="#2076bd" stroke-width="2.5" stroke-linejoin="round" stroke-linecap="round"/>
<line x1="{width-245}" y1="58" x2="{width-215}" y2="58" stroke="#2076bd" stroke-width="3"/><text x="{width-207}" y="62" font-family="Segoe UI,Arial,sans-serif" font-size="12" fill="#526477">C++ training loss</text>
<text x="{width/2}" y="{height-16}" text-anchor="middle" font-family="Segoe UI,Arial,sans-serif" font-size="14" fill="#344b62">Training step</text>
<text transform="translate(20 {height/2}) rotate(-90)" text-anchor="middle" font-family="Segoe UI,Arial,sans-serif" font-size="14" fill="#344b62">Loss J</text>
</svg>\n'''
    args.output.write_text(svg, encoding="utf-8")
    print(f"Wrote labelled SVG plot from {len(steps)} CSV rows to {args.output}")


if __name__ == "__main__":
    main()
