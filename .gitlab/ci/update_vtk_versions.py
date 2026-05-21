#!/usr/bin/env python3
"""
Utility to maintain vtk_versions.json used by the documentation version selector.

Supports two operations:
  1. add-version: Add a single version to an existing JSON file
  2. create-from-series: Create new JSON file from a list of release series
"""

import json
import sys


def add_version(json_path, version, base_url):
    """Add a release version to the existing vtk_versions.json.

    Reads the current versions file, adds the new version if not already present,
    sorts releases in descending order, and marks the latest release.
    """
    with open(json_path) as f:
        data = json.load(f)

    versions = data.get("versions", [])

    # Check if this version is already present.
    if any(v.get("version") == version for v in versions):
        print(f"Version {version} already present – no changes needed.")
        return

    new_entry = {
        "name": version,
        "version": version,
        "baseUrl": f"{base_url}/release/{version}/html"
    }

    # Separate nightly from release versions
    release_versions = [v for v in versions if v["version"] != "nightly"]
    nightly = [v for v in versions if v["version"] == "nightly"]

    # Add the new version and sort descending
    release_versions.append(new_entry)
    release_versions.sort(
        key=lambda v: list(map(int, v["version"].split("."))),
        reverse=True
    )

    # Mark the latest release
    for v in release_versions:
        if "(latest release)" in v.get("name", ""):
            v["name"] = v["version"]
    release_versions[0]["name"] = f'{release_versions[0]["version"]} (latest release)'

    data["versions"] = nightly + release_versions

    with open(json_path, "w") as f:
        json.dump(data, f, indent=2)
        f.write("\n")

    print(f"Added version {version}.")


def create_from_series(json_path, base_url, series_list):
    """Create a new vtk_versions.json from a list of release series.

    Creates the versions structure with nightly as the first entry, followed by
    release series in descending order, with the first release marked as latest.
    """
    versions = [
        {
            "name": "nightly",
            "version": "nightly",
            "baseUrl": f"{base_url}/html",
        }
    ]

    for i, series in enumerate(series_list):
        entry = {
            "name": series if i > 0 else f"{series} (latest release)",
            "version": series,
            "baseUrl": f"{base_url}/release/{series}/html",
        }
        versions.append(entry)

    with open(json_path, "w") as f:
        json.dump({"versions": versions}, f, indent=2)
        f.write("\n")

    print(f"  {len(versions)} entries written ({len(series_list)} releases + nightly).")


def main():
    if len(sys.argv) < 2:
        print("Usage:", file=sys.stderr)
        print(f"  {sys.argv[0]} add-version <json_path> <version> <base_url>", file=sys.stderr)
        print(f"  {sys.argv[0]} create-from-series <json_path> <base_url> <series> [<series> ...]", file=sys.stderr)
        sys.exit(1)

    command = sys.argv[1]

    if command == "add-version":
        if len(sys.argv) != 5:
            print("add-version requires 3 arguments: json_path, version, base_url", file=sys.stderr)
            sys.exit(1)
        add_version(sys.argv[2], sys.argv[3], sys.argv[4])
    elif command == "create-from-series":
        if len(sys.argv) < 4:
            print("create-from-series requires at least 2 arguments: json_path, base_url, series...", file=sys.stderr)
            sys.exit(1)
        create_from_series(sys.argv[2], sys.argv[3], sys.argv[4:])
    else:
        print(f"Unknown command: {command}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
