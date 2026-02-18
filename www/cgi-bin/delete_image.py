#!/usr/bin/env python3
import html
import os
import sys
import urllib.parse
from pathlib import Path

def resolve_images_dir():
    # Buscamos la carpeta images de forma robusta
    script_path = Path(__file__).resolve()
    images_dir = script_path.parent.parent / "images"
    if not images_dir.exists():
        images_dir.mkdir(parents=True, exist_ok=True)
    return images_dir

def parse_target_filename():
    query = os.environ.get("QUERY_STRING", "")
    params = urllib.parse.parse_qs(query)
    raw_name = params.get("file", [""])[0]
    # os.path.basename limpia cualquier intento de ../../
    return os.path.basename(raw_name)

def try_delete(images_dir, filename):
    if not filename:
        return False, "NO TARGET ACQUIRED."

    # Usamos la ruta directa sin forzar .resolve() en la comparación inicial
    # para evitar problemas de montado de discos en Mac
    target = images_dir / filename

    if not target.exists():
        return False, f"RAY SIGNATURE NOT FOUND: {filename}"
    
    try:
        target.unlink()
        return True, "RAY DESTROYED SUCCESSFULLY."
    except Exception as exc:
        return False, f"SYSTEM FAILURE: {str(exc)}"

# Lógica principal
images_dir = resolve_images_dir()
filename = parse_target_filename()
ok, message = try_delete(images_dir, filename)

# Respuesta HTML con estética LaserTag
sys.stdout.write("Content-Type: text/html; charset=utf-8\r\n\r\n")
print(f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>LaserWeb - Deletion Terminal</title>
    <link rel="stylesheet" href="/css/laserweb.css">
    <link rel="stylesheet" href="/css/laserweb-ops.css">
    <style>
        .result-box {{
            border: 2px solid {"#39ff14" if ok else "#ff073a"};
            background: rgba(0,0,0,0.8);
            padding: 30px;
            text-align: center;
            position: relative;
            overflow: hidden;
            box-shadow: 0 0 20px {"#39ff14" if ok else "#ff073a"};
        }}
        /* Animación de rayo láser cruzando el mensaje */
        .laser-flash {{
            position: absolute;
            top: 50%; left: -100%;
            width: 100%; height: 4px;
            background: white;
            box-shadow: 0 0 15px white, 0 0 30px {"#39ff14" if ok else "#ff073a"};
            animation: shoot-ray 0.6s ease-out forwards;
        }}
        @keyframes shoot-ray {{
            0% {{ left: -100%; opacity: 1; }}
            100% {{ left: 100%; opacity: 0; }}
        }}
    </style>
</head>
<body>
    <div class="page">
        <header><h1 class="logo">L<span>A</span>S<span>E</span>RW<span>E</span>B</h1></header>
        <main class="panel">
            <div class="result-box">
                <div class="laser-flash"></div>
                <h2 style="color: {"#39ff14" if ok else "#ff073a"}; letter-spacing: 2px;">
                    { "TARGET NEUTRALIZED" if ok else "DESTRUCTION ERROR" }
                </h2>
                <p style="font-family: monospace; font-size: 1.2rem; margin: 20px 0;">
                    {html.escape(message)}
                </p>
                <p style="color: #888;">Object ID: {html.escape(filename)}</p>
            </div>

            <div class="armory-actions" style="margin-top: 30px; display: flex; gap: 20px; justify-content: center;">
                <a class="btn" href="/cgi-bin/list_images.py" style="text-decoration: none;">RETURN TO SCANNER</a>
                <a class="btn btn-back" href="/upload.html" style="text-decoration: none;">NEW BLUEPRINT</a>
            </div>
        </main>
    </div>
</body>
</html>""")