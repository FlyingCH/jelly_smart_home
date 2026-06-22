#!/usr/bin/env python3
"""
地图瓦片下载工具

从 OpenStreetMap 免费瓦片服务下载指定区域的瓦片图，用于离线地图展示。
可根据经纬度范围计算所需瓦片坐标，递归下载所有层级。

用法:
    # 下载北京市范围 1-15 级瓦片
    python map_tile_downloader.py --min-lat 39.8 --min-lon 116.1 --max-lat 40.1 --max-lon 116.7 --min-zoom 1 --max-zoom 15

    # 下载默认范围（中国中部）1-12 级
    python map_tile_downloader.py

    # 查看已下载统计
    python map_tile_downloader.py --stats
"""

import argparse
import json
import os
import sys
import time
from datetime import datetime
from pathlib import Path
from typing import Optional, Tuple
from urllib.parse import urlparse

import requests

TILE_CACHE_DIR = Path(__file__).resolve().parent.parent.parent / "frontend" / "public" / "map_tiles"
TILE_SERVER = "https://tile.openstreetmap.org"
USER_AGENT = "MySmartHome/1.0 (map tile downloader)"
MAX_RETRIES = 3
RETRY_DELAY = 2
DOWNLOAD_DELAY = 0.1


def lat_lon_to_tile(lat: float, lon: float, zoom: int) -> Tuple[int, int]:
    import math
    lat_rad = math.radians(lat)
    n = 2.0 ** zoom
    x = int((lon + 180.0) / 360.0 * n)
    y = int((1.0 - math.asinh(math.tan(lat_rad)) / math.pi) / 2.0 * n)
    return x, y


def download_tile(url: str, filepath: Path) -> bool:
    if filepath.exists() and filepath.stat().st_size > 0:
        return True

    filepath.parent.mkdir(parents=True, exist_ok=True)

    for attempt in range(1, MAX_RETRIES + 1):
        try:
            resp = requests.get(
                url,
                headers={"User-Agent": USER_AGENT},
                timeout=10,
            )
            if resp.status_code == 200:
                filepath.write_bytes(resp.content)
                return True
            elif resp.status_code == 404:
                blank = Path(__file__).parent / "blank.png"
                if blank.exists():
                    filepath.write_bytes(blank.read_bytes())
                return False
            else:
                print(f"  HTTP {resp.status_code}, retry {attempt}/{MAX_RETRIES}")
        except requests.RequestException as exc:
            print(f"  Network error: {exc}, retry {attempt}/{MAX_RETRIES}")

        if attempt < MAX_RETRIES:
            time.sleep(RETRY_DELAY)

    return False


def download_area(
    min_lat: float,
    min_lon: float,
    max_lat: float,
    max_lon: float,
    min_zoom: int,
    max_zoom: int,
    tile_server: str = TILE_SERVER,
    output_dir: Path = TILE_CACHE_DIR,
) -> dict:
    stats = {"total": 0, "downloaded": 0, "skipped": 0, "failed": 0, "zooms": {}}

    for zoom in range(min_zoom, max_zoom + 1):
        x_min, y_max = lat_lon_to_tile(max_lat, min_lon, zoom)
        x_max, y_min = lat_lon_to_tile(min_lat, max_lon, zoom)

        if x_min > x_max:
            x_min, x_max = x_max, x_min
        if y_min > y_max:
            y_min, y_max = y_max, y_min

        zoom_tiles = (x_max - x_min + 1) * (y_max - y_min + 1)
        stats["zooms"][zoom] = {"x_range": (x_min, x_max), "y_range": (y_min, y_max), "count": zoom_tiles}
        stats["total"] += zoom_tiles

        print(f"\nZoom {zoom}: x[{x_min}..{x_max}] y[{y_min}..{y_max}] = {zoom_tiles} tiles")

        zoom_dir = output_dir / str(zoom)
        zoom_dir.mkdir(parents=True, exist_ok=True)

        for x in range(x_min, x_max + 1):
            for y in range(y_min, y_max + 1):
                tile_path = zoom_dir / str(x) / f"{y}.png"
                tile_url = f"{tile_server}/{zoom}/{x}/{y}.png"

                if tile_path.exists() and tile_path.stat().st_size > 0:
                    stats["skipped"] += 1
                else:
                    ok = download_tile(tile_url, tile_path)
                    if ok:
                        stats["downloaded"] += 1
                    else:
                        stats["failed"] += 1

                if stats["downloaded"] % 50 == 0 and stats["downloaded"] > 0:
                    print(f"  Progress: {stats['downloaded']}/{stats['total']} tiles")

                time.sleep(DOWNLOAD_DELAY)

    return stats


def print_stats(output_dir: Path = TILE_CACHE_DIR):
    if not output_dir.exists():
        print("Tile cache directory does not exist.")
        return

    total_files = 0
    total_size = 0
    zoom_stats = {}

    for zoom_dir in sorted(output_dir.iterdir(), key=lambda p: int(p.name) if p.is_dir() and p.name.isdigit() else -1):
        if not zoom_dir.is_dir() or not zoom_dir.name.isdigit():
            continue
        zoom = int(zoom_dir.name)
        count = 0
        size = 0
        for x_dir in zoom_dir.iterdir():
            if not x_dir.is_dir():
                continue
            for tile_file in x_dir.iterdir():
                if tile_file.suffix.lower() in (".png", ".jpg", ".webp"):
                    count += 1
                    size += tile_file.stat().st_size
        zoom_stats[zoom] = {"count": count, "size": size}
        total_files += count
        total_size += size

    print(f"\nTile cache statistics:")
    print(f"  Total tiles: {total_files}")
    print(f"  Total size:  {total_size / 1024 / 1024:.2f} MB")
    print(f"  Cache path:  {output_dir}")
    print()

    for zoom in sorted(zoom_stats.keys()):
        s = zoom_stats[zoom]
        print(f"  Zoom {zoom:2d}: {s['count']:6d} tiles, {s['size'] / 1024:.1f} KB")


def main():
    parser = argparse.ArgumentParser(description="OSM Map Tile Downloader")
    parser.add_argument("--min-lat", type=float, default=30.0, help="Minimum latitude (default: 30.0)")
    parser.add_argument("--min-lon", type=float, default=110.0, help="Minimum longitude (default: 110.0)")
    parser.add_argument("--max-lat", type=float, default=40.0, help="Maximum latitude (default: 40.0)")
    parser.add_argument("--max-lon", type=float, default=120.0, help="Maximum longitude (default: 120.0)")
    parser.add_argument("--min-zoom", type=int, default=1, help="Minimum zoom level (default: 1)")
    parser.add_argument("--max-zoom", type=int, default=12, help="Maximum zoom level (default: 12)")
    parser.add_argument("--output", type=str, default=str(TILE_CACHE_DIR), help=f"Output directory (default: {TILE_CACHE_DIR})")
    parser.add_argument("--tile-server", type=str, default=TILE_SERVER, help=f"Tile server URL (default: {TILE_SERVER})")
    parser.add_argument("--stats", action="store_true", help="Show download statistics and exit")
    parser.add_argument("--delay", type=float, default=DOWNLOAD_DELAY, help=f"Delay between downloads in seconds (default: {DOWNLOAD_DELAY})")

    args = parser.parse_args()

    output_dir = Path(args.output)

    if args.stats:
        print_stats(output_dir)
        return

    print(f"Map Tile Downloader")
    print(f"  Area:       lat[{args.min_lat}..{args.max_lat}], lon[{args.min_lon}..{args.max_lon}]")
    print(f"  Zoom:       {args.min_zoom}..{args.max_zoom}")
    print(f"  Tile URL:   {args.tile_server}/{{z}}/{{x}}/{{y}}.png")
    print(f"  Output:     {output_dir}")
    print(f"  Started at: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print()

    start = time.time()
    stats = download_area(
        min_lat=args.min_lat,
        min_lon=args.min_lon,
        max_lat=args.max_lat,
        max_lon=args.max_lon,
        min_zoom=args.min_zoom,
        max_zoom=args.max_zoom,
        tile_server=args.tile_server,
        output_dir=output_dir,
    )
    elapsed = time.time() - start

    print(f"\nDownload complete!")
    print(f"  Total:      {stats['total']} tiles")
    print(f"  Downloaded: {stats['downloaded']}")
    print(f"  Skipped:    {stats['skipped']}")
    print(f"  Failed:     {stats['failed']}")
    print(f"  Time:       {elapsed:.1f}s")
    print(f"  Finished:   {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")


if __name__ == "__main__":
    main()
