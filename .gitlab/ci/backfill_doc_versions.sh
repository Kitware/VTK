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
#   ./backfill_doc_versions.sh [--host HOST] [--base-url URL] [--root ROOT] \
#                               [--download-base URL] [--workdir DIR] [--dry-run] [--keep-downloads]
#
# Optional arguments (shown with their defaults):
#   --host HOST             – SSH host alias (default: vtk.doc)
#   --base-url URL          – Base URL for docs (default: https://vtk.org/doc)
#   --root ROOT             – Root directory on server (default: VTKDoxygen)
#   --download-base URL     – Base URL for downloads (default: https://www.vtk.org/files/release)
#   --workdir DIR           – Scratch directory (default: temp dir auto-cleaned up on exit)
#   --dry-run               – Print commands without uploading
#   --keep-downloads        – Keep downloaded tarballs in workdir
#
# The script expects SSH to be configured with a Host stanza (e.g., in ~/.ssh/config):
#   Host vtk.doc
#       User         kitware
#       HostName     web.kitware.com
#       IdentityFile ~/.local/share/ssh/my-key
#       IdentitiesOnly  yes
# --------------------------------------------------------------------------
set -euo pipefail

# Defaults
SSH_HOST="vtk.doc"
BASE_URL="https://vtk.org/doc"
SERVER_ROOT="VTKDoxygen"
DOWNLOAD_BASE="https://www.vtk.org/files/release"
DRY_RUN=0
KEEP_DOWNLOADS=0
WORKDIR=""

# Parse arguments
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
        --download-base)
            DOWNLOAD_BASE="$2"
            shift 2
            ;;
        --workdir)
            WORKDIR="$2"
            shift 2
            ;;
        --dry-run)
            DRY_RUN=1
            shift
            ;;
        --keep-downloads)
            KEEP_DOWNLOADS=1
            shift
            ;;
        *)
            echo "Unknown option: $1" >&2
            exit 1
            ;;
    esac
done

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
if [[ -n "${WORKDIR}" ]]; then
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
echo "  Server : ${SSH_HOST}:${SERVER_ROOT}"
echo "  DRY_RUN: ${DRY_RUN}"
echo "========================================"
echo ""

FAILED_SERIES=()

for series in "${SERIES_DESCENDING[@]}"; do
    full="${LATEST_PATCH[$series]}"
    tarball="vtkDocHtml-${full}.tar.gz"
    download_url="${DOWNLOAD_BASE}/${series}/${tarball}"
    local_tarball="${WORKDIR}/${tarball}"
    extract_dir="${WORKDIR}/extract-${series}"
    remote_dest="${SERVER_ROOT}/release/${series}/html"

    echo "----------------------------------------"
    echo "Series ${series}  (latest patch: ${full})"
    echo "  Download : ${download_url}"
    echo "  Remote   : ${SSH_HOST}:${remote_dest}/"

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
    # Use rsync with --delete to handle directory creation and file synchronization
    # in a single pass. The trailing slash on the source causes rsync to copy the
    # contents of html/ into the destination; without it, rsync would copy html itself.
    run rsync --recursive --times --compress --delete --mkpath \
        -e ssh \
        "${extract_dir}/html/" \
        "${SSH_HOST}:${remote_dest}/"

    echo "  Deployed ${series} -> ${SSH_HOST}:${remote_dest}/"

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

python3 "$(dirname "$0")/update_vtk_versions.py" create-from-series \
    "${LOCAL_JSON}" "${BASE_URL}" "${SERIES_DESCENDING[@]}"

echo ""
cat "${LOCAL_JSON}"
echo ""

run scp \
    "${LOCAL_JSON}" \
    "${SSH_HOST}:${SERVER_ROOT}/vtk_versions.json"

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
