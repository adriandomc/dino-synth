#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Package the compiled DINO firmware, local installer/editor, previews and guides."""
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


def main():
    version = "0.2-dev"
    package = ROOT / "build/felucca.fwsc"
    app = ROOT / "build/felucca.bin"
    if not app.exists() or b"DINO SYNTH" not in app.read_bytes():
        raise SystemExit("Build DINO first; the app must identify itself as DINO SYNTH.")
    sources = list((ROOT / "firmware").rglob("*.c")) + list((ROOT / "firmware").rglob("*.h"))
    if any(path.stat().st_mtime > app.stat().st_mtime for path in sources):
        raise SystemExit("Firmware sources changed since the build; rebuild before packaging.")
    out = ROOT / "build" / f"dino-synth-{version}"
    make_site.main(package, version, out, dino=True)
    for name in ("README.md", "DINO.md", "DINO-VALIDATION.md", "BUILDING.md", "GUIDE.md", "SLOOP-README.md", "LICENSE", "LICENSING.md"):
        shutil.copyfile(ROOT / name, out / name)
    readme = (out / "README.md").read_text(encoding="utf-8")
    (out / "README.md").write_text(readme.replace("docs/firmware/", "firmware/").replace("docs/webapp/", "webapp/").replace("assets/dino/", "previews/"), encoding="utf-8")
    previews = out / "previews"
    previews.mkdir(exist_ok=True)
    for directory, names in (("dino-visuals", ("cosmic-preview.png", "cosmic-contact-sheet.png", "cosmic-animation.gif", "conway-preview.png", "conway-animation.gif")),
                             ("dino-ui", ("ui-contact-sheet.png",))):
        for name in names:
            shutil.copyfile(ROOT / "build" / directory / name, previews / name)
    manifest = {
        "version": version,
        "source_commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        "upstream": "isod89/sloop-fm1 v2.7 ba54ba5f0e7f6e0a1a3499497809e1e6c1ad7e8c",
        "hardware_tested": False,
        "sha256": {str(path.relative_to(out)).replace("\\", "/"): hashlib.sha256(path.read_bytes()).hexdigest()
                   for path in sorted(out.rglob("*")) if path.is_file() and path.name != "manifest.json"},
    }
    (out / "SOURCE.txt").write_text("Corresponding source (GPL-3.0-only):\nhttps://github.com/adriandomc/dino-synth/tree/" + manifest["source_commit"] + "\n", encoding="utf-8")
    manifest["sha256"]["SOURCE.txt"] = hashlib.sha256((out / "SOURCE.txt").read_bytes()).hexdigest()
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    archive = Path(str(out) + ".zip")
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as z:
        for path in sorted(out.rglob("*")):
            if path.is_file():
                z.write(path, str(path.relative_to(out.parent)))
    with zipfile.ZipFile(archive) as z:
        assert z.testzip() is None
        assert f"{out.name}/firmware/dino-synth-{version}.fwsc" in z.namelist()
    print(f"Distribution: {archive}")
    print(f"Firmware SHA256: {hashlib.sha256(package.read_bytes()).hexdigest()}")


if __name__ == "__main__":
    main()
