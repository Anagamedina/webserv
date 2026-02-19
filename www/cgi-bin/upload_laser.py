#!/usr/bin/env python3
import cgi
import os
from pathlib import Path

# RUTA ABSOLUTA BASADA EN EL SCRIPT:
# Estamos en www/cgi-bin -> subimos uno -> www/images
SCRIPT_DIR = Path(__file__).resolve().parent
UPLOAD_DIR = SCRIPT_DIR.parent / "images"

# Crear la carpeta si no existe
UPLOAD_DIR.mkdir(parents=True, exist_ok=True)

form = cgi.FieldStorage()
print("Content-Type: text/html; charset=utf-8\r\n\r\n")

success = False
filename = ""

if "file" in form:
    fileitem = form["file"]
    if fileitem.filename:
        filename = os.path.basename(fileitem.filename)
        filepath = UPLOAD_DIR / filename
        
        with open(str(filepath), 'wb') as f:
            f.write(fileitem.file.read())
        success = True

if success:
    print(f"""
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta http-equiv="refresh" content="3;url=/cgi-bin/list_images.py">
    <title>LaserWeb - Carga Exitosa</title>
    <link rel="stylesheet" href="/css/laserweb.css">
    <link rel="stylesheet" href="/css/laserweb-ops.css">
</head>
<body>
    <div class="page">
        <main class="panel" style="text-align: center; margin-top: 50px;">
            <h2 style="color: #39ff14; text-shadow: 0 0 10px #39ff14;">SISTEMA ACTUALIZADO</h2>
            <div class="doc-block" style="border-color: #39ff14;">
                <p>El láser <strong>{filename}</strong> ha sido transferido.</p>
                <p style="color: #808090; font-size: 0.85rem;">Redirigiendo al Tactical Scanner...</p>
            </div>
            <a href="/cgi-bin/list_images.py" class="btn">ACCESO INMEDIATO</a>
        </main>
    </div>
</body>
</html>
""")
else:
    print("<h2>Error en la carga. Revisa el formulario.</h2>")