#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="KWIML"
readonly ownership="KWIML Upstream <kwrobot@kitware.com>"
readonly subtree="Utilities/KWIML/vtkkwiml"
readonly repo="https://gitlab.kitware.com/utils/kwiml.git"
readonly tag="master"
readonly paths="
"

extract_source () {
    git_archive
    cat > "$extractdir/$name-reduced/abi.h" <<EOF
/* Forward include for source-tree layout.  */
#include "include/kwiml/abi.h"
EOF
    cat > "$extractdir/$name-reduced/int.h" <<EOF
/* Forward include for source-tree layout.  */
#include "include/kwiml/int.h"
EOF
}

. "${BASH_SOURCE%/*}/../../ThirdParty/update-common.sh"
