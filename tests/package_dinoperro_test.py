#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Check distribution identity, stale-build rejection and original-editor packaging."""
import importlib.util
import json
from pathlib import Path
import shutil
import tempfile
import unittest
from unittest.mock import patch
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("package_dinoperro", ROOT / "tools/package_dinoperro.py")
pack = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pack)


class DistributionTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        (self.root / "build").mkdir()
        for name in pack.DOCUMENTS:
            shutil.copyfile(ROOT / name, self.root / name)
        (self.root / "assets/dino").mkdir(parents=True)
        for name in pack.PREVIEWS:
            shutil.copyfile(ROOT / "assets/dino" / name, self.root / "assets/dino" / name)
        (self.root / "build/felucca.bin").write_bytes(b"SLOOP 2.8.1+DINO")
        raw = bytearray(4096)
        for i, ch in enumerate("FM-1_900"):
            raw[i * pack.make_site.BLK + pack.make_site.KEEP] = ord(ch) + i + 1
        for i in range(8, pack.make_site.BLOCKS):
            raw[i * pack.make_site.BLK + pack.make_site.KEEP] = 0x7D
        raw[2000:2016] = b"FELUCCA-LOADER-1"
        (self.root / "build/felucca.fwsc").write_bytes(raw)
        self.root_patch = patch.object(pack, "ROOT", self.root)
        self.root_patch.start()

    def tearDown(self):
        self.root_patch.stop()
        self.tmp.cleanup()

    def test_original_editor_and_fork_installer_link(self):
        with patch.object(pack.subprocess, "check_output", side_effect=["a" * 40 + "\n", ""]):
            pack.main()
        out = self.root / "build/sloop-2.8.1-dinoperro"
        src = (ROOT / "web/editor.html").read_text(encoding="utf-8")
        editor = (out / "webapp/editor/index.html").read_text(encoding="utf-8")
        self.assertEqual(editor, src.replace('download="sloop-editor.html"',
                                             'download="sloop-editor-2.8.1-dinoperro.html"'))
        offline = (out / "webapp/editor/sloop-editor.html").read_text(encoding="utf-8")
        self.assertIn(pack.INSTALL_GUIDE, offline)
        self.assertNotIn(pack.make_site.ONLINE + "webapp/installer/", offline)
        self.assertIn('"version": "2.8.1-dinoperro"',
                      (out / "webapp/installer/index.html").read_text(encoding="utf-8"))
        manifest = json.loads((out / "manifest.json").read_text(encoding="utf-8"))
        self.assertFalse(manifest["source_modified"])
        self.assertEqual(manifest["version"], "2.8.1-dinoperro")
        self.assertEqual(manifest["upstream"],
                         "isod89/sloop-fm1 v2.8.1 d89364253b79a5c6002c0d90c5f16b3c3c16ea9d")
        self.assertIn("firmware/sloop-2.8.1-dinoperro.fwsc", manifest["sha256"])
        with zipfile.ZipFile(str(out) + ".zip") as archive:
            self.assertIsNone(archive.testzip())

    def test_old_firmware_binary_rejected(self):
        for binary in (b"DINO SYNTH 0.2", b"SLOOP 2.7+DINO"):
            with self.subTest(binary=binary):
                (self.root / "build/felucca.bin").write_bytes(binary)
                with self.assertRaisesRegex(SystemExit, "Build SLOOP 2.8.1"):
                    pack.main()

    def test_newer_source_rejected(self):
        (self.root / "firmware").mkdir()
        newer = self.root / "firmware/test.c"
        newer.write_text("/* changed after build */", encoding="utf-8")
        import os
        changed = (self.root / "build/felucca.bin").stat().st_mtime + 10
        os.utime(newer, (changed, changed))
        with self.assertRaisesRegex(SystemExit, "sources changed"):
            pack.main()


if __name__ == "__main__":
    unittest.main()
