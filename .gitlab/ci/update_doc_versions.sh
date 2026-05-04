#!/usr/bin/env bash
# --------------------------------------------------------------------------
# update_doc_versions.sh
#
# Maintains a shared vtk_versions.json on the documentation server.
# Called during the release documentation upload CI job.
#
# The script:
#   1. Downloads the current vtk_versions.json from the server (if any).
#   2. Adds the newly released version if it is not already present.
#   3. Uploads the updated JSON back to the server.
#
# Usage:
#   ./update_doc_versions.sh <version> [--host HOST] [--base-url URL] [--root ROOT]
#
# Required argument:
#   version             – The "major.minor" version string being released.
#
# Optional arguments (with defaults):
#   --host HOST         – SSH host alias (default: vtk.doc)
#   --base-url URL      – Base URL pattern for versioned docs (default: https://vtk.org/doc)
#   --root ROOT         – Root directory on server (default: VTKDoxygen)
#
# The script expects SSH to be configured with a Host stanza (e.g., in ~/.ssh/config):
#   Host vtk.doc
#       User         kitware
#       HostName     web.kitware.com
#       IdentityFile ~/.local/share/ssh/my-key
#       IdentitiesOnly  yes
# --------------------------------------------------------------------------
set -euo pipefail

# Parse arguments
if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <version> [--host HOST] [--base-url URL] [--root ROOT]" >&2
    exit 1
fi

VERSION="$1"
shift

# Defaults
SSH_HOST="vtk.doc"
BASE_URL="https://vtk.org/doc"
SERVER_ROOT="VTKDoxygen"

# Parse optional arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --host)
            SSH_HOST="$2"
            shift 2
            ;;
        --base-url)
            BASE_URL="$2"
            shift 2
            ;;
        --root)
            SERVER_ROOT="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1" >&2
            exit 1
            ;;
    esac
done

REMOTE_JSON="${SERVER_ROOT}/vtk_versions.json"
LOCAL_JSON="$(mktemp)"

trap 'rm -f "${LOCAL_JSON}"' EXIT

# ---- 1. Fetch the current versions file (if it exists) ------------------
echo "Fetching existing vtk_versions.json from server..."
if ! scp "${SSH_HOST}:${REMOTE_JSON}" "${LOCAL_JSON}" 2>/dev/null; then
    echo "No existing vtk_versions.json found; creating a new one."
    cat > "${LOCAL_JSON}" <<SEED
{
  "versions": [
    {
      "name": "nightly",
      "version": "nightly",
      "baseUrl": "${BASE_URL}/nightly/html"
    }
  ]
}
SEED
fi

# ---- 2. Insert the new version if absent --------------------------------
echo "Ensuring version ${VERSION} is listed..."
python3 - "${LOCAL_JSON}" "${VERSION}" "${BASE_URL}" <<'PYEOF'
import json, sys

json_path, version, base_url = sys.argv[1], sys.argv[2], sys.argv[3]

with open(json_path) as f:
    data = json.load(f)

versions = data.get("versions", [])

# Check if this version is already present.
if any(v.get("version") == version for v in versions):
    print(f"Version {version} already present – no changes needed.")
    sys.exit(0)

new_entry = {
    "name": version,
    "version": version,
    "baseUrl": f"{base_url}/release/{version}/html"
}

# Insert after "nightly" (index 0 if present) but before older releases,
# keeping releases sorted in descending order.
release_versions = [v for v in versions if v["version"] != "nightly"]
nightly = [v for v in versions if v["version"] == "nightly"]

release_versions.append(new_entry)
# Sort descending by version string (works for "major.minor" format).
release_versions.sort(key=lambda v: list(map(int, v["version"].split("."))), reverse=True)

# Mark the latest release.
for v in release_versions:
    if "(latest release)" in v.get("name", ""):
        v["name"] = v["version"]
release_versions[0]["name"] = f'{release_versions[0]["version"]} (latest release)'

data["versions"] = nightly + release_versions

with open(json_path, "w") as f:
    json.dump(data, f, indent=2)
    f.write("\n")

print(f"Added version {version}.")
PYEOF

# ---- 3. Upload the updated file back to the server ----------------------
echo "Uploading updated vtk_versions.json..."
scp "${LOCAL_JSON}" "${SSH_HOST}:${REMOTE_JSON}"
echo "Done."
