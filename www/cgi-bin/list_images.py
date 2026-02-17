#!/usr/bin/env python3
import html, os, sys
from pathlib import Path

IMAGE_EXT = {".png", ".jpg", ".jpeg", ".gif", ".webp", ".svg"}

def resolve_images_dir():
    script_path = Path(__file__).resolve()
    images_dir = script_path.parent.parent / "images"
    images_dir.mkdir(parents=True, exist_ok=True)
    return images_dir

def list_image_files(images_dir):
    files = []
    if images_dir.is_dir():
        for entry in sorted(images_dir.iterdir(), key=lambda p: p.name.lower()):
            if entry.is_file() and not entry.name.startswith(".") and entry.suffix.lower() in IMAGE_EXT:
                files.append(entry.name)
    return files

def gallery_items(names):
    if not names:
        return '<li class="no-data" style="color:var(--neon-red);">NO RAYS DETECTED IN ARMORY</li>'
    items = []
    for filename in names:
        safe_name = html.escape(filename)
        src = f"/images/{safe_name}"
        items.append(f"""
            <li class="ray-img">
                <a href="{src}" target="_blank"><img src="{src}" alt="{safe_name}"></a>
                <p style="font-size:0.7rem; margin:10px 0;">{safe_name}</p>
                <form action="/cgi-bin/delete_image.py" method="GET">
                    <input type="hidden" name="file" value="{safe_name}">
                    <button type="submit" class="btn-destroy">DESTROY RAY</button>
                </form>
            </li>""")
    return "\n".join(items)

images_dir = resolve_images_dir()
image_names = list_image_files(images_dir)
items_html = gallery_items(image_names)

sys.stdout.write("Content-Type: text/html; charset=utf-8\r\n\r\n")
print(f"""<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <title>LaserWeb - Tactical Scanner</title>
    <link rel="stylesheet" href="/css/laserweb.css">
    <link rel="stylesheet" href="/css/laserweb-ops.css">
</head>
<body>
    <div class="laser-beam" style="left: 20%; background: var(--neon-cyan); animation-delay: 0s;"></div>
    <div class="laser-beam" style="left: 50%; background: var(--neon-green); animation-delay: 1.5s;"></div>
    <div class="laser-beam" style="left: 80%; background: var(--neon-red); animation-delay: 3s;"></div>

    <div class="page">
        <header><h1 class="logo">T<span>A</span>CTICAL SC<span>A</span>NNER</h1></header>
        <nav class="nav-hud">
            <a href="/">THE HUD</a>
            <a href="/upload.html">THE ARMORY</a>
            <a href="/cgi-bin/list_images.py">TACTICAL SCANNER</a>
        </nav>
        <main class="panel">
            <section class="system-card">
                <h3 style="color:var(--neon-green); font-size:0.8rem;">[ SCANNER LOG ]</h3>
                <p>STATUS: <span style="animation: blink 1s infinite;">OPERATIONAL</span></p>
                <p>RAYS LOADED: {len(image_names)}</p>
                <p style="margin-top:0.6rem;">
                    <a href="/upload.html" class="btn">LOAD MORE LASERS</a>
                </p>
            </section>
            <ul class="rays-gallery">{items_html}</ul>
        </main>
    </div>
</body>
</html>""")