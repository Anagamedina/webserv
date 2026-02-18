#!/usr/bin/env python3
import html
import os
import sys
import urllib.parse
from pathlib import Path


def storage_file():
    root_data = Path(os.getcwd()) / "www" / "data"
    fallback_data = Path(__file__).resolve().parent.parent / "data"
    target_data = root_data if root_data.exists() else fallback_data
    target_data.mkdir(parents=True, exist_ok=True)
    return target_data / "squads.db"


def read_squads(db_path):
    squads = []
    if not db_path.exists():
        return squads
    with open(str(db_path), "r") as db_file:
        for raw in db_file:
            line = raw.strip()
            if not line:
                continue
            parts = line.split("|", 1)
            team = urllib.parse.unquote(parts[0]) if parts else ""
            members = urllib.parse.unquote(parts[1]) if len(parts) > 1 else ""
            squads.append((team, members))
    return squads


def render_squad_items(squads):
    if not squads:
        return "<li>No junior squads registered yet.</li>"

    rows = []
    for index, (team, members) in enumerate(reversed(squads), start=1):
        team_esc = html.escape(team or "(no name)")
        members_esc = html.escape(members).replace("\n", "<br>") if members else "(no members)"
        rows.append(
            "<li><strong>J-%d | %s</strong><br><span>%s</span></li>"
            % (index, team_esc, members_esc)
        )
    return "\n".join(rows)


db = storage_file()
squads = read_squads(db)
items_html = render_squad_items(squads)

sys.stdout.write("Content-Type: text/html; charset=utf-8\r\n\r\n")
print("""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>LaserWeb - Junior Squads</title>
    <link rel="stylesheet" href="/css/laserweb.css">
    <link rel="stylesheet" href="/css/laserweb-ops.css">
</head>
<body>
    <div class="page">
        <header><h1 class="logo">L<span>A</span>S<span>E</span>RW<span>E</span>B</h1></header>
        <nav class="nav-hud">
            <a href="/">THE HUD</a>
            <a href="/upload.html">THE ARMORY</a>
            <a href="/squad_staff.html">ELITE SQUAD</a>
            <a href="/cgi-bin/junior_squads.py">JUNIOR SQUADS</a>
            <a href="/cgi-bin/list_images.py">TACTICAL SCANNER</a>
        </nav>
        <main class="panel">
            <h2>Junior Squads - Generated in mission</h2>
            <p class="back-row"><a href="/squads.html" class="btn btn-back">&larr; Register new squad</a></p>
            <div class="squad-result-card">
                <h3>Summary</h3>
                <p><strong>Total detected:</strong> %d squad(s)</p>
                <p><strong>Source:</strong> /www/data/squads.db</p>
            </div>
            <h3 class="page-title">Current list</h3>
            <ul class="squad-result-list">
""" % len(squads))
print(items_html)
print("""
            </ul>
        </main>
    </div>
</body>
</html>""")
