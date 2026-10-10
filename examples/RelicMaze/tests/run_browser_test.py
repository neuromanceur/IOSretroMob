#!/usr/bin/env python3
"""Original browser logic check; optional Node, no web service or guest run. MIT."""
import shutil,subprocess
from pathlib import Path
node=shutil.which('node')
if node is None:
    print('SKIP: Node unavailable; browser logic not executed')
else:
    raise SystemExit(subprocess.call([node,str(Path(__file__).resolve().parents[1]/'web/test_game.cjs')]))
