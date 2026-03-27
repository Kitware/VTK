#!/usr/bin/env bash
# --------------------------------------------------------------------------
# backfill_doc_versions.sh
#
# One-time script to deploy historical VTK documentation (v8.0 – v9.6.1)
# to the versioned doc server directories and seed the shared
# vtk_versions.json index so the version selector covers all past releases.
#
# For each major.minor release series the script:
#   1. Downloads vtkDocHtml-<latest-patch>.tar.gz from vtk.org.
#   2. Extracts the html/ directory into a temp location.
#   3. Rsyncs it to VTKDoxygen/release/<major.minor>/html/ on the server.
#   4. Does NOT touch VTKDoxygen/html/ (that slot is reserved for nightly).
#
# After all series are deployed it writes a fresh vtk_versions.json and
# uploads it to VTKDoxygen/ so all deployed pages share the same version
# list.
#
# Usage:
#   RSYNC_KEY_PATH=/path/to/ssh_key ./backfill_doc_versions.sh
#
# Optional overrides (shown with their defaults):
#   DOC_SERVER=kitware@web.kitware.com
#   DOC_SERVER_ROOT=VTKDoxygen
#   VTK_DOC_BASE_URL=https://vtk.org/doc
#   VTK_RELEASE_DOWNLOAD_BASE=https://www.vtk.org/files/release
#   DRY_RUN=0       # set to 1 to print what would be done without uploading
#   KEEP_DOWNLOADS=0  # set to 1 to keep downloaded tarballs in WORKDIR
#   WORKDIR=        # scratch directory; defaults to a temp dir that is
#                   # cleaned up on exit unless KEEP_DOWNLOADS=1
# --------------------------------------------------------------------------
set -euo pipefail

: "${RSYNC_KEY_PATH:?RSYNC_KEY_PATH must point to the SSH private key for web.kitware.com}"
: "${DOC_SERVER:=kitware@web.kitware.com}"
: "${DOC_SERVER_ROOT:=VTKDoxygen}"
: "${VTK_DOC_BASE_URL:=https://vtk.org/doc}"
: "${VTK_RELEASE_DOWNLOAD_BASE:=https://www.vtk.org/files/release}"
: "${DRY_RUN:=0}"
: "${KEEP_DOWNLOADS:=0}"

# --------------------------------------------------------------------------
# Release series: "major.minor" -> "latest full version"
# Derived from git tags; update when new patch releases are made.
# --------------------------------------------------------------------------
declare -A LATEST_PATCH=(
    ["8.0"]="8.0.1"
    ["8.1"]="8.1.2"
    ["8.2"]="8.2.0"
    ["9.0"]="9.0.3"
    ["9.1"]="9.1.0"
    ["9.2"]="9.2.6"
    ["9.3"]="9.3.1"
    ["9.4"]="9.4.2"
    ["9.5"]="9.5.2"
    ["9.6"]="9.6.1"
)

# Ordered newest-first so the server upload order matches the JSON order.
SERIES_DESCENDING=(
    "9.6" "9.5" "9.4" "9.3" "9.2" "9.1" "9.0"
    "8.2" "8.1" "8.0"
)

# --------------------------------------------------------------------------
# Scratch space
# --------------------------------------------------------------------------
if [[ -n "${WORKDIR:-}" ]]; then
    mkdir -p "${WORKDIR}"
    TMPDIR_OWNED=0
else
    WORKDIR="$(mktemp -d)"
    TMPDIR_OWNED=1
fi

cleanup() {
    if [[ "${KEEP_DOWNLOADS}" == "1" ]]; then
        echo "Keeping work directory: ${WORKDIR}"
    elif [[ "${TMPDIR_OWNED}" == "1" ]]; then
        rm -rf "${WORKDIR}"
    fi
}
trap cleanup EXIT

chmod 400 "${RSYNC_KEY_PATH}"
SSH_OPTS="-i ${RSYNC_KEY_PATH} -o StrictHostKeyChecking=no"
RSYNC_SSH_OPTS="-e ssh ${SSH_OPTS}"

# --------------------------------------------------------------------------
# Helper: run a command or just print it when DRY_RUN=1
# --------------------------------------------------------------------------
run() {
    if [[ "${DRY_RUN}" == "1" ]]; then
        echo "[dry-run] $*"
    else
        "$@"
    fi
}

# --------------------------------------------------------------------------
# Step 1 & 2 & 3: for each series, download, extract, and upload HTML
# --------------------------------------------------------------------------
echo "========================================"
echo "  Deploying historical VTK Doxygen docs"
echo "  Server : ${DOC_SERVER}:${DOC_SERVER_ROOT}"
echo "  DRY_RUN: ${DRY_RUN}"
echo "========================================"
echo ""

FAILED_SERIES=()

for series in "${SERIES_DESCENDING[@]}"; do
    full="${LATEST_PATCH[$series]}"
    tarball="vtkDocHtml-${full}.tar.gz"
    download_url="${VTK_RELEASE_DOWNLOAD_BASE}/${series}/${tarball}"
    local_tarball="${WORKDIR}/${tarball}"
    extract_dir="${WORKDIR}/extract-${series}"
    remote_dest="${DOC_SERVER_ROOT}/release/${series}/html"

    echo "----------------------------------------"
    echo "Series ${series}  (latest patch: ${full})"
    echo "  Download : ${download_url}"
    echo "  Remote   : ${DOC_SERVER}:${remote_dest}/"

    # -- Download -----------------------------------------------------------
    if [[ -f "${local_tarball}" ]]; then
        echo "  Tarball already present in WORKDIR – skipping download."
    elif [[ "${DRY_RUN}" == "1" ]]; then
        echo "  [dry-run] Would download ${download_url}"
    else
        echo "  Downloading..."
        if ! curl --fail --silent --show-error --location \
                  --output "${local_tarball}" \
                  "${download_url}"; then
            echo "  WARNING: download failed for ${series} – skipping."
            FAILED_SERIES+=("${series}")
            continue
        fi
        echo "  Downloaded $(du -sh "${local_tarball}" | cut -f1)."
    fi

    # -- Extract ------------------------------------------------------------
    if [[ "${DRY_RUN}" == "1" ]]; then
        echo "  [dry-run] Would extract ${tarball} -> ${extract_dir}/html/"
    else
        rm -rf "${extract_dir}"
        mkdir -p "${extract_dir}"
        echo "  Extracting..."
        tar -xzf "${local_tarball}" -C "${extract_dir}"
        if [[ ! -d "${extract_dir}/html" ]]; then
            echo "  WARNING: html/ directory not found in tarball for ${series} – skipping."
            FAILED_SERIES+=("${series}")
            continue
        fi
    fi

    # -- Upload -------------------------------------------------------------
    # Create the remote directory first, then rsync the contents.
    run ssh ${SSH_OPTS} "${DOC_SERVER}" \
        "mkdir -p ${remote_dest}"

    run rsync --recursive --times --compress --delete \
        -e "ssh ${SSH_OPTS}" \
        "${extract_dir}/html/" \
        "${DOC_SERVER}:${remote_dest}/"

    echo "  Deployed ${series} -> ${DOC_SERVER}:${remote_dest}/"

    # Discard the extracted tree to keep disk use low; keep the tarball
    # only if the caller requested it.
    if [[ "${DRY_RUN}" != "1" ]]; then
        rm -rf "${extract_dir}"
        if [[ "${KEEP_DOWNLOADS}" != "1" ]]; then
            rm -f "${local_tarball}"
        fi
    fi
done

echo ""

# --------------------------------------------------------------------------
# Step 4: Generate and upload vtk_versions.json
# --------------------------------------------------------------------------
echo "----------------------------------------"
echo "Generating vtk_versions.json..."

LOCAL_JSON="${WORKDIR}/vtk_versions.json"

python3 - "${LOCAL_JSON}" "${VTK_DOC_BASE_URL}" \
        "${SERIES_DESCENDING[@]}" <<'PYEOF'
import json, sys

json_path = sys.argv[1]
base_url  = sys.argv[2]
# All remaining args are series in descending order.
series_list = sys.argv[3:]

versions = [
    {
        "name": "nightly",
        "version": "nightly",
        "baseUrl": f"{base_url}/nightly/html",
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
PYEOF

echo ""
cat "${LOCAL_JSON}"
echo ""

run scp ${SSH_OPTS} \
    "${LOCAL_JSON}" \
    "${DOC_SERVER}:${DOC_SERVER_ROOT}/vtk_versions.json"

echo ""

# --------------------------------------------------------------------------
# Summary
# --------------------------------------------------------------------------
echo "========================================"
if [[ ${#FAILED_SERIES[@]} -eq 0 ]]; then
    echo "  All series deployed successfully."
else
    echo "  Completed with failures for: ${FAILED_SERIES[*]}"
    echo "  These series were skipped (tarball not available or download error)."
    echo "  vtk_versions.json was still uploaded with all series listed."
fi
echo "========================================"
