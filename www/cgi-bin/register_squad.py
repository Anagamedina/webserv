#!/usr/bin/env python3
"""CGI que recibe POST del formulario de escuadrones y devuelve página LaserWeb con los datos."""
import os
import sys
import urllib.parse

def parse_form(body):
    data = urllib.parse.parse_qs(body)
    team = data.get("team_name", [""])[0]
    members = data.get("members", [""])[0]
    return team, members

def html_escape(s):
    return (s.replace("&", "&amp;").replace("<", "&lt;")
            .replace(">", "&gt;").replace('"', "&quot;"))

length = int(os.environ.get("CONTENT_LENGTH", "0") or "0")
body = sys.stdin.read(length) if length > 0 else ""
team_name, members = parse_form(body)

team_esc = html_escape(team_name)
members_esc = html_escape(members).replace("\n", "<br>") if members else "(sin especificar)"

print("Content-Type: text/html; charset=utf-8")
print("")
print("""<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>LaserWeb — Escuadrón registrado</title>
    <link rel="stylesheet" href="/css/laserweb.css">
    <style>
        .success-badge{color:#39ff14;font-size:2rem;margin-bottom:1rem}
        .squad-card{margin:1rem 0;padding:1rem;border:1px solid #39ff14;border-radius:4px;background:rgba(57,255,20,0.05)}
        .squad-card h3{color:#00f5ff;font-size:0.95rem}
        .squad-card .members{color:#808090;font-size:0.9rem;margin-top:0.5rem}
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
            <h2>Escuadrones registrados — Rayos en el sistema</h2>
            <p class="back-row"><a href="/" class="btn btn-back">&larr; Volver al HUD</a></p>
            <div class="success-badge">&#9670; NUEVO ESCUADRÓN AÑADIDO</div>
            <div class="squad-card">
                <h3>""")
print(team_esc if team_esc else "(sin nombre)")
print("""</h3>
                <p class="members"><strong>Miembros:</strong><br>""")
print(members_esc)
print("""</p>
            </div>
            <p style="margin-top:1.5rem;"><a href="/about.html" class="btn">Registrar otro escuadrón</a></p>
        </main>
    </div>
</body>
</html>""")
