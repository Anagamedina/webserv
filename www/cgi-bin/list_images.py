#!/usr/bin/env python3
"""
CGI: TACTICAL SCANNER — Lista imágenes en www/images/ con estilo LaserWeb.
El servidor hace fork+execve; la stdout se envía como body HTTP.
"""
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
IMAGES_DIR = os.path.join(SCRIPT_DIR, "..", "images")
IMAGE_EXT = (".png", ".jpg", ".jpeg", ".gif", ".webp", ".svg", ".bmp")


def html_escape(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace('"', "&quot;")


def is_image(name):
    return name.lower().endswith(IMAGE_EXT)


def list_images():
    items = []
    if not os.path.isdir(IMAGES_DIR):
        return items
    for name in sorted(os.listdir(IMAGES_DIR)):
        if name.startswith("."):
            continue
        full = os.path.join(IMAGES_DIR, name)
        if os.path.isdir(full):
            items.append(("/images/" + name + "/", name, True))
        elif is_image(name):
            items.append(("/images/" + name, name, False))
    return items


def render_items(items):
    html = []
    for href, name, is_dir in items:
        name_esc = html_escape(name)
        if is_dir:
            html.append(f'          <li class="dir"><a href="{href}">{name_esc}/</a></li>')
        else:
            html.append(
                f'          <li class="ray-img"><a href="{href}">'
                f'<img src="{href}" alt="{name_esc}"></a><span>{name_esc}</span></li>'
            )
    return "\n".join(html) if html else '          <li style="color:#808090;">(ninguna imagen)</li>'


items = list_images()
items_html = render_items(items)

# 1. Cabeceras HTTP obligatorias (separadas del cuerpo por línea vacía)
print("Content-Type: text/html; charset=utf-8")
print()

# 2. Cuerpo HTML (lo que verá el navegador)
print("""<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>LaserWeb — Tactical Scanner</title>
  <link rel="stylesheet" href="/css/laserweb.css">
  <style>
    .scanner-grid{display:grid;grid-template-columns:minmax(0,1.2fr) minmax(0,1fr);gap:1.5rem;align-items:start}
    .radar-wrap{display:flex;justify-content:center}
    .radar{position:relative;width:260px;height:260px;border-radius:50%;background:radial-gradient(circle,rgba(0,255,0,0.25) 0,transparent 60%);border:2px solid #39ff14;box-shadow:0 0 25px rgba(57,255,20,0.6);overflow:hidden}
    .radar::before{content:'';position:absolute;width:100%;height:100%;background:conic-gradient(from 0deg,rgba(57,255,20,0) 0deg,rgba(57,255,20,0.5) 40deg,rgba(57,255,20,0) 80deg);animation:sweep 4s linear infinite}
    .radar-grid-line{position:absolute;top:50%;left:0;right:0;height:1px;background:rgba(57,255,20,0.3)}
    .radar-grid-line.vert{width:1px;height:100%;left:50%;top:0}
    @keyframes sweep{from{transform:rotate(0deg)}to{transform:rotate(360deg)}}
    .autoindex-list{list-style:none;margin:0;padding:0}
    .rays-gallery{display:grid;grid-template-columns:repeat(auto-fill,minmax(120px,1fr));gap:1rem;margin-top:0.8rem}
    .rays-gallery .ray-img{display:flex;flex-direction:column;align-items:center;padding:0.5rem;border:1px solid #39ff14;border-radius:4px;background:rgba(57,255,20,0.05)}
    .rays-gallery .ray-img img{width:100px;height:100px;object-fit:cover;border:1px solid #00f5ff}
    .rays-gallery .ray-img img:hover{box-shadow:0 0 12px rgba(0,245,255,0.6)}
    .rays-gallery .ray-img span{font-size:0.7rem;color:#808090;margin-top:0.3rem}
    .rays-gallery li:not(.ray-img){padding:0.5rem;border:1px solid #1a1a2e;border-radius:4px}
  </style>
</head>
<body>
  <div class="page">
    <header><h1 class="logo">L<span>Λ</span>S<span>Ξ</span>RW<span>Ξ</span>B</h1></header>
    <nav class="nav-hud">
      <a href="/">THE HUD</a>
      <a href="/upload.html">LOAD LASER</a>
      <a href="/about.html">THE SQUAD</a>
      <a href="/target-list.html">TARGET LIST</a>
      <a href="/cgi-bin/list_images.py">TACTICAL SCANNER</a>
    </nav>
    <main class="panel">
      <h2>TACTICAL SCANNER — Rayos lanzados</h2>
      <p class="back-row"><a href="/" class="btn btn-back">&larr; Volver al HUD</a></p>
      <div class="scanner-grid">
        <div class="radar-wrap">
          <div class="radar">
            <div class="radar-grid-line"></div>
            <div class="radar-grid-line vert"></div>
          </div>
        </div>
        <div>
          <h3 style="font-size:0.9rem;text-transform:uppercase;letter-spacing:0.15em;color:#00f5ff;">Rayos lanzados</h3>
          <ul class="autoindex-list rays-gallery">
""")
print(items_html)
print("""
          </ul>
        </div>
      </div>
    </main>
  </div>
</body>
</html>""")
