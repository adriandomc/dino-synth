#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Package SLOOP 2.8.1 with four Dinoperro visualisers and the original editor."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "web"))
import make_site

VERSION = "2.8.1-dinoperro"
DOCUMENTS = ("README.md", "SLOOP-DINOPERRO.md", "BUILDING.md", "GUIDE.md",
             "SLOOP-README.md", "LICENSE", "LICENSING.md")
PREVIEWS = ("cosmic-preview.png", "cosmic-animation.gif", "conway-preview.png", "conway-animation.gif")
INSTALL_GUIDE = "https://github.com/adriandomc/dino-synth/blob/dino-synth/SLOOP-DINOPERRO.md#instalación"


def main():
    package = ROOT / "build/felucca.fwsc"
    app = ROOT / "build/felucca.bin"
    if not app.is_file() or b"SLOOP 2.8.1+DINO" not in app.read_bytes() or b"DINO SYNTH" in app.read_bytes():
        raise SystemExit("Build SLOOP 2.8.1+DINO before packaging.")
    if not package.is_file() or package.stat().st_mtime < app.stat().st_mtime:
        raise SystemExit("The firmware package is missing or older than the app; rebuild first.")
    sources = list((ROOT / "firmware").rglob("*.c")) + list((ROOT / "firmware").rglob("*.h"))
    if any(path.stat().st_mtime > app.stat().st_mtime for path in sources):
        raise SystemExit("Firmware sources changed since the build; rebuild before packaging.")
    out = ROOT / "build" / f"sloop-{VERSION}"
    make_site.main(package, VERSION, out)
    offline = out / "webapp/editor/sloop-editor.html"
    html = offline.read_text(encoding="utf-8")
    old = make_site.ONLINE + "webapp/installer/"
    if html.count(old) != 1:
        raise SystemExit("Expected one offline installer link; update the packager.")
    offline.write_text(html.replace(old, INSTALL_GUIDE), encoding="utf-8")
    for name in DOCUMENTS:
        shutil.copyfile(ROOT / name, out / name)
    readme = (out / "README.md").read_text(encoding="utf-8")
    (out / "README.md").write_text(readme.replace("docs/firmware/", "firmware/").replace("docs/webapp/", "webapp/")
                                  .replace("assets/dino/", "previews/"), encoding="utf-8")
    previews = out / "previews"
    previews.mkdir(exist_ok=True)
    for name in PREVIEWS:
        shutil.copyfile(ROOT / "assets/dino" / name, previews / name)
    commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    dirty = bool(subprocess.check_output(["git", "status", "--porcelain", "--", "firmware", "web", "tools"],
                                        cwd=ROOT, text=True).strip())
    (out / "SOURCE.txt").write_text(
        "Source repository (GPL-3.0-only): https://github.com/adriandomc/dino-synth\n"
        f"Commit at packaging: {commit}\nUncommitted source changes: {dirty}\n", encoding="utf-8")
    manifest = {
        "version": VERSION,
        "source_commit": commit,
        "source_modified": dirty,
        "upstream": "isod89/sloop-fm1 v2.8.1 d89364253b79a5c6002c0d90c5f16b3c3c16ea9d",
        "sha256": {path.relative_to(out).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
                   for path in sorted(out.rglob("*")) if path.is_file() and path.name != "manifest.json"},
    }
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    archive = Path(str(out) + ".zip")
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as z:
        for path in sorted(out.rglob("*")):
            if path.is_file():
                z.write(path, str(path.relative_to(out.parent)))
    with zipfile.ZipFile(archive) as z:
        assert z.testzip() is None
        assert f"{out.name}/firmware/sloop-{VERSION}.fwsc" in z.namelist()
    print(f"Distribution: {archive}")
    print(f"Firmware SHA256: {hashlib.sha256(package.read_bytes()).hexdigest()}")


if __name__ == "__main__":
    main()
