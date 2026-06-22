from fastapi import APIRouter, HTTPException
from fastapi.responses import FileResponse, Response
from pathlib import Path
from typing import Optional

router = APIRouter()

TILE_DIR = Path(__file__).resolve().parent.parent.parent.parent / "frontend" / "public" / "map_tiles"

TILE_SERVER = "https://tile.openstreetmap.org"
USER_AGENT = "MySmartHome/1.0 (tile proxy)"


@router.get("/map/tiles/{z}/{x}/{y}")
async def get_map_tile(z: int, x: int, y: int):
    local_path = TILE_DIR / str(z) / str(x) / f"{y}.png"

    if local_path.exists() and local_path.stat().st_size > 0:
        return FileResponse(str(local_path), media_type="image/png")

    raise HTTPException(status_code=404, detail="Tile not found locally")


@router.get("/map/tiles/{z}/{x}/{y}.png")
async def get_map_tile_ext(z: int, x: int, y: int):
    local_path = TILE_DIR / str(z) / str(x) / f"{y}.png"

    if local_path.exists() and local_path.stat().st_size > 0:
        return FileResponse(str(local_path), media_type="image/png")

    raise HTTPException(status_code=404, detail="Tile not found locally")


@router.get("/map/tile-stats")
async def get_tile_stat():
    if not TILE_DIR.exists():
        return {"total": 0, "zooms": []}

    result = []
    total = 0
    for zoom_dir in sorted(TILE_DIR.iterdir(), key=lambda p: int(p.name) if p.is_dir() and p.name.isdigit() else -1):
        if not zoom_dir.is_dir() or not zoom_dir.name.isdigit():
            continue
        count = 0
        for x_dir in zoom_dir.iterdir():
            if not x_dir.is_dir():
                continue
            count += sum(1 for f in x_dir.iterdir() if f.suffix.lower() in (".png", ".jpg", ".webp"))
        total += count
        result.append({"zoom": int(zoom_dir.name), "tiles": count})

    return {"total": total, "zooms": result}
