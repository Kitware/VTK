#!/usr/bin/env bash
# --------------------------------------------------------------------------
# prepare-docs-upload-env.sh
#
# Sets up SSH configuration for documentation uploads to the server.
#
# This script configures SSH with a Host stanza to simplify credential
# management and allow scripts to use "vtk.doc" as a host alias instead of
# passing SSH options through multiple layers.
#
# Required arguments:
#   <ssh_key_path>        – Path to SSH private key for web.kitware.com
#
# Optional arguments (shown with defaults):
#   [--host ALIAS]        – SSH host alias to configure (default: vtk.doc)
#   [--user USER]         – Remote user name (default: kitware)
#   [--hostname HOSTNAME] – Remote hostname (default: web.kitware.com)
# --------------------------------------------------------------------------

set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <ssh_key_path> [--host ALIAS] [--user USER] [--hostname HOSTNAME]" >&2
    exit 1
fi

SSH_KEY_PATH="$1"
shift

# Defaults
SSH_HOST_ALIAS="vtk.doc"
SSH_USER="kitware"
SSH_HOSTNAME="web.kitware.com"

# Parse optional arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --host)
            SSH_HOST_ALIAS="$2"
            shift 2
            ;;
        --user)
            SSH_USER="$2"
            shift 2
            ;;
        --hostname)
            SSH_HOSTNAME="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1" >&2
            exit 1
            ;;
    esac
done

# Create SSH directory if needed
if [[ ! -d ~/.ssh ]]; then
    mkdir -p ~/.ssh
    chmod 700 ~/.ssh
fi

# Ensure the key file has correct permissions
chmod 400 "${SSH_KEY_PATH}"

# Set up SSH config entry for the documentation server.
# This allows scp/rsync/ssh commands to reference the host alias
# instead of needing to know the actual hostname or manage SSH options.
cat >> ~/.ssh/config << EOF
Host ${SSH_HOST_ALIAS}
    User               ${SSH_USER}
    HostName           ${SSH_HOSTNAME}
    IdentityFile       ${SSH_KEY_PATH}
    IdentitiesOnly     yes
    StrictHostKeyChecking no
EOF
