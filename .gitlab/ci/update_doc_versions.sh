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
# Environment variables (set by CI):
#   VTK_DOC_VERSION    – The "major.minor" version string being released.
#   VTK_DOC_BASE_URL   – Base URL pattern for versioned docs.
#   RSYNC_KEY_PATH     – SSH private key for the server.
#   DOC_SERVER          – SSH destination (user@host).
#   DOC_SERVER_ROOT     – Root directory on the server for VTK docs.
# --------------------------------------------------------------------------
set -euo pipefail

: "${VTK_DOC_VERSION:?VTK_DOC_VERSION is required}"
: "${RSYNC_KEY_PATH:?RSYNC_KEY_PATH is required}"
: "${DOC_SERVER:=kitware@web.kitware.com}"
: "${DOC_SERVER_ROOT:=VTKDoxygen}"
: "${VTK_DOC_BASE_URL:=https://vtk.org/doc}"

SSH_OPTS="-i ${RSYNC_KEY_PATH} -o StrictHostKeyChecking=no"
REMOTE_JSON="${DOC_SERVER_ROOT}/vtk_versions.json"
LOCAL_JSON="$(mktemp)"

trap 'rm -f "${LOCAL_JSON}"' EXIT

# ---- 1. Fetch the current versions file (if it exists) ------------------
echo "Fetching existing vtk_versions.json from server..."
if ! scp ${SSH_OPTS} "${DOC_SERVER}:${REMOTE_JSON}" "${LOCAL_JSON}" 2>/dev/null; then
    echo "No existing vtk_versions.json found; creating a new one."
    cat > "${LOCAL_JSON}" <<SEED
{
  "versions": [
    {
      "name": "nightly",
      "version": "nightly",
      "baseUrl": "${VTK_DOC_BASE_URL}/nightly/html"
    }
  ]
}
SEED
fi

# ---- 2. Insert the new version if absent --------------------------------
echo "Ensuring version ${VTK_DOC_VERSION} is listed..."
python3 - "${LOCAL_JSON}" "${VTK_DOC_VERSION}" "${VTK_DOC_BASE_URL}" <<'PYEOF'
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
scp ${SSH_OPTS} "${LOCAL_JSON}" "${DOC_SERVER}:${REMOTE_JSON}"
echo "Done."
