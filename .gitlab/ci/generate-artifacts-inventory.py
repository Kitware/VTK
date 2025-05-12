#!/usr/bin/env python3

from pathlib import Path

import argparse
import hashlib
import json

parser = argparse.ArgumentParser(
    prog="generate-artifacts-inventory",
    description="Generate JSON file containing the SHA256sum of the files in a given directory.",
    epilog="Kitware, inc",
)

parser.add_argument(
    "--artifacts-dir", type=str, required=True, help="Path to the artifacts directory"
)
args = parser.parse_args()


def sha256sum(file_path):
    with open(file_path, "rb") as f:
        return hashlib.sha256(f.read()).hexdigest()


artifacts = [
    {"name": str(path), "sha256sum": sha256sum(path)}
    for path in Path(args.artifacts_dir).glob("*")
    if path.is_file()
]

print(json.dumps(artifacts))
