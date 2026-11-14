#!/usr/bin/env python3
"""Remove glslang's version-only comment; keep every SPIR-V word unchanged."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1] / 'src/gen'
for header in root.rglob('*.h'):
    original = header.read_bytes()
    normalized = re.sub(rb'\A[\t ]*// \d+\.\d+\.\d+\r?\n',
                        b'// Generated SPIR-V; compiler pinned in ci/build-lock.json.\n',
                        original, count=1)
    if normalized != original:
        header.write_bytes(normalized)
