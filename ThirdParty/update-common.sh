########################################################################
# Script for updating third party packages.
#
# This script should be sourced in a project-specific script which sets
# the following variables:
#
#   name
#       The name of the project.
#   ownership
#       A git author name/email for the commits.
#   subtree
#       The location of the thirdparty package within the main source
#       tree.
#   update
#       The location of the update script within the main source tree.
#   repo
#       The git repository to use as upstream.
#   tag
#       The tag, branch or commit hash to use for upstream.
#
# The script must also contain a line the following:
#
#   readonly basehash='<optional git hash>' # NEWHASH
#
# where the current tracking branch hash will be stored. Leaving it
# empty will cause a new tracking branch to be created for the project.
# It will be updated by this script.
#
# Additionally, an "extract_source" function must be defined. It will be
# run within the checkout of the project on the requested tag. It should
# should place the desired tree into $extractdir/$name-reduced. This
# directory will be used as the newest commit for the project.
#
# For convenience, the function may use the "git_archive" function which
# does a standard "git archive" extraction using the (optional) "paths"
# variable to only extract a subset of the source tree.
########################################################################

########################################################################
# Utility functions
########################################################################
git_archive () {
    git archive --prefix="$name-reduced/" HEAD -- $paths | \
        tar -C "$extractdir" -x
}

die () {
    echo >&2 "$@"
    exit 1
}

warn () {
    echo >&2 "warning: $@"
}

########################################################################
# Sanity checking
########################################################################
[ -n "$name" ] || \
    die "'name' is empty"
[ -n "$ownership" ] || \
    die "'ownership' is empty"
[ -n "$subtree" ] || \
    die "'subtree' is empty"
[ -n "$update" ] || \
    die "'update' is empty"
[ -n "$repo" ] || \
    die "'repo' is empty"
[ -n "$tag" ] || \
    die "'tag' is empty"
[ -n "$basehash" ] || \
    warn "'basehash' is empty; performing initial import"

readonly workdir="$PWD/work"
readonly upstreamdir="$workdir/upstream"
readonly extractdir="$workdir/extract"

[ -d "$workdir" ] && \
    die "error: workdir '$workdir' already exists"

trap "rm -rf '$workdir'" EXIT

# Get upstream
git clone "$repo" "$upstreamdir"

if [ -n "$basehash" ]; then
    # Use the existing package's history
    git worktree add "$extractdir" "$basehash"
    # Clear out the working tree
    pushd "$extractdir"
    git ls-files | xargs rm -v
    popd
else
    # Create a repo to hold this package's history
    mkdir -p "$extractdir"
    git -C "$extractdir" init
fi

# Extract the subset of upstream we care about
pushd "$upstreamdir"
git checkout "$tag"
readonly upstream_hash="$( git rev-parse HEAD )"
readonly upstream_date="$( git show -s --format=%cd HEAD )"
extract_source || \
    die "failed to extract source"
popd

[ -d "$extractdir/$name-reduced" ] || \
    die "expected directory to extract does not exist"
if [ -z "$commit_summary" ]; then
    warn "summary not set; using default"
    readonly commit_summary="$name: update to $tag"
fi

# Commit the subset
pushd "$extractdir"
mv -v "$name-reduced/"* .
rmdir "$name-reduced/"
git add -A .
git commit -n --author="$ownership" --date="$upstream_date" -F - <<-EOF
$commit_summary

Code extracted from:

    $repo

at commit $upstream_hash.
EOF
readonly newhash="$( git rev-parse HEAD )"
popd

# Merge the subset into this repository
if [ -z "$basehash" ]; then
    git fetch "$extractdir"
fi
git merge -s recursive "-Xsubtree=$subtree/" --no-commit "$newhash"
sed -i -e "/NEWHASH$/s/='.*'/='$newhash'/" "$update"
git add "$update"
git commit -m "$name: update to $tag"
