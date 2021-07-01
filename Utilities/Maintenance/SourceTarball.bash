#!/usr/bin/env bash

# Bail out when commands fail.
set -e

# The URLs where data is stored for VTK.
readonly urlbases="https://www.vtk.org/files/ExternalData/ALGO/HASH"

# Move to the top of the VTK tree.
readonly output_base="$( pwd )"
cd "${BASH_SOURCE%/*}/../.."

info () {
    echo >&2 "$@"
}

die () {
    echo >&2 "$@"
    exit 1
}

check_pipeline () {
    echo "${PIPESTATUS[@]}" | grep -q -v "[1-9]"
}

usage () {
    die "$0: [(--tgz|--txz|--zip)...]" \
        "[--verbose] [-v <version>] [<tag>|<commit>]"
}

# Check for a tool to get MD5 sums from.
if type -p md5sum >/dev/null; then
    readonly md5tool="md5sum"
    readonly md5regex="s/ .*//"
elif type -p md5 >/dev/null; then
    readonly md5tool="md5"
    readonly md5regex="s/.*= //"
elif type -p cmake >/dev/null; then
    readonly md5tool="cmake -E md5sum"
    readonly md5regex="s/ .*//"
else
    die "No 'md5sum' or 'md5' tool found."
fi

compute_MD5 () {
    local file="$1"
    readonly file
    shift

    $md5tool "$file" | sed -e "$md5regex"
}

# Check for a tool to get SHA512 sums from.
if type -p sha512sum >/dev/null; then
    readonly sha512tool="sha512sum"
    readonly sha512regex="s/ .*//"
else
    die "No 'sha512sum' tool found."
fi

compute_SHA512 () {
    local file="$1"; readonly file; shift

    $sha512tool "$file" | sed -e "$sha512regex"
}

validate () {
    local algo="$1"
    readonly algo
    shift

    local file="$1"
    readonly file
    shift

    local expected="$1"
    readonly expected
    shift

    local actual="$( "compute_$algo" "$file" )"
    readonly actual

    if ! [ "$actual" = "$expected" ]; then
        die "Object $expected is corrupt: $file"
    fi
}

download () {
    local algo="$1"
    readonly algo
    shift

    local hash="$1"
    readonly hash
    shift

    local path="$1"
    readonly path
    shift

    local temppath="$path.tmp$$"
    readonly temppath

    mkdir -p "$( dirname "$path" )"

    for urlbase in $urlbases; do
        url="$( echo "$urlbase" | sed -e "s/ALGO/$algo/;s/HASH/$hash/" )"

        if wget --no-verbose "$url" -O "$temppath" >&2; then
            mv "$temppath" "$path"
            return
        else
            rm -f "$temppath"
        fi
    done

    die "failed to download data for object '$hash': '$path'"
}

find_data_objects () {
    local revision="$1"
    readonly revision
    shift
    local largedata="$1"
    readonly largedata
    shift

    # Find all content links in the tree.
    git ls-tree --full-tree -r "$revision" | \
        egrep '\.(md5|sha512)$' | \
        while read mode type obj path; do
            case "$path" in
                *.md5) algo="MD5" ;;
                *.sha512) algo="SHA512" ;;
                *)
                    die "unknown ExternalData content link: $path"
                    ;;
            esac
            # Include large data iff we are making the large data archive
            # Include the normal data iff we are making the normal data archive
            if git check-attr vtk-is-large-data -- $path | grep -q -v -e " set$"; then
                if test "$largedata" != "VTK_USE_LARGE_DATA"; then
                    # Build the path to the object.
                    echo "$algo/$( git cat-file blob $obj )"
                fi
            elif test "$largedata" = "VTK_USE_LARGE_DATA"; then
                echo "$algo/$( git cat-file blob $obj )"
            fi
        done | \
            sort | \
            uniq

    check_pipeline
}

index_data_objects () {
    local algo
    local hash
    local path
    local file
    local obj

    while IFS=/ read algo hash; do
        # Final path in the source tarball.
        path=".ExternalData/$algo/$hash"

        # Download it if it doesn't exist.
        if ! [ -f "$path" ]; then
            download "$algo" "$hash" "$path"
        fi
        file="$path"

        # Validate the file (catches 404 pages and the like).
        validate "$algo" "$file" "$hash"
        obj="$( git hash-object -t blob -w "$file" )"
        echo "100644 blob $obj	$path"
    done | \
        git update-index --index-info

    check_pipeline
}

# Puts data objects into an index file.
load_data_objects () {
    find_data_objects "$@" | \
        index_data_objects
}

# Loads existing data files into an index file.
load_data_files () {
    local revision="$1"
    readonly revision
    shift

    git ls-tree -r "$revision" -- ".ExternalData" | \
        git update-index --index-info

    check_pipeline
}

read_all_submodules () {
    # `git submodule foreach` clears GIT_INDEX_FILE from then environment
    # inside its command.
    local git_index="$GIT_INDEX_FILE"
    export git_index

    git submodule foreach --quiet '
        gitdir="$( git rev-parse --git-dir )"
        cd "$toplevel"
        GIT_INDEX_FILE="$git_index"
        export GIT_INDEX_FILE
        git rm --cached "$path" 2>/dev/null
        GIT_ALTERNATE_OBJECT_DIRECTORIES="$gitdir/objects" git read-tree -i --prefix="$path/" "$sha1"
        echo "$gitdir/objects"
    ' | \
        tr '\n' ':'
}

read_submodules_into_index () {
    local object_dirs=""
    local new_object_dirs

    while git ls-files -s | grep -q -e '^160000'; do
        new_object_dirs="$( read_all_submodules )"
        object_dirs="$object_dirs:$new_object_dirs"
    done

    object_dirs="$( echo "$object_dirs" | sed -e 's/:$//;s/^://' )"
    readonly object_dirs

    GIT_ALTERNATE_OBJECT_DIRECTORIES="$object_dirs"
    export GIT_ALTERNATE_OBJECT_DIRECTORIES
}

# Creates an archive of a git tree object.
git_archive () {
    local archive_format="$1"
    readonly archive_format
    shift

    local revision="$1"
    readonly revision
    shift

    local destination="$1"
    readonly destination
    shift

    local prefix
    if [ "$#" -gt 0 ]; then
        prefix="$1"
        shift
    else
        prefix="$destination"
    fi
    readonly prefix

    local ext
    local format
    local compress
    case "$archive_format" in
        tgz)
            ext=".tar.gz"
            format="tar"
            compress="gzip --best"
            ;;
        txz)
            ext=".tar.xz"
            format="tar"
            compress="xz --best"
            ;;
        zip)
            ext=".zip"
            format="zip"
            compress="cat"
            ;;
        *)
            die "unknown archive format: $format"
            ;;
    esac

    local output="$output_base/$destination$ext"
    readonly output

    local temppath="$output.tmp$$"
    readonly temppath

    git -c core.autocrlf=false archive $verbose "--format=$format" "--prefix=$prefix/" "$revision" | \
        $compress > "$temppath"
    mv "$temppath" "$output"

    info "Wrote $output"
}

#------------------------------------------------------------------------------

formats=
commit=
verbose=
version=

while [ "$#" -gt 0 ]; do
    case "$1" in
        --tgz)
            formats="$formats tgz"
            ;;
        --txz)
            formats="$formats txz"
            ;;
        --zip)
            formats="$formats zip"
            ;;
        --verbose)
            verbose="-v"
            ;;
        -v)
            shift
            version="$1"
            ;;
        --)
            shift
            break
            ;;
        -*)
            usage
            ;;
        *)
            if [ -z "$commit" ]; then
                commit="$1"
            else
                usage
            fi
            ;;
    esac
    shift
done

[ "$#" -eq 0 ] || \
    usage
[ -z "$commit" ] && \
    commit="HEAD"
[ -z "$formats" ] && \
    formats="tgz"

readonly formats
readonly verbose
readonly commit

git rev-parse --verify -q "$commit" >/dev/null || \
    die "'$commit' is not a valid commit"
if [ -z "$version" ]; then
    readonly desc="$( git describe "$commit" )"
    if ! [ "${desc:0:1}" = "v" ]; then
        die "'git describe $commit' is '$desc'; use -v <version>"
    fi
    version="${desc#v}"
    echo "$commit is version $version"
fi

readonly version

GIT_INDEX_FILE="$output_base/tmp-$$-index" && \
    trap "rm -f '$GIT_INDEX_FILE'" EXIT
export GIT_INDEX_FILE

result=0

info "Loading source tree from $commit..."
rm -f "$GIT_INDEX_FILE"
git read-tree -m -i "$commit"
git rm -rf -q --cached ".ExternalData"
git submodule sync
git submodule update --init
read_submodules_into_index
tree="$( git write-tree )"

info "Generating source archive(s)..."
for format in $formats; do
    git_archive "$format" "$tree" "VTK-$version" || \
        result=1
done

info "Loading normal data for $commit..."
rm -f "$GIT_INDEX_FILE"
load_data_objects "$commit" ""
load_data_files "$commit"
tree="$( git write-tree )"

info "Generating normal data archive(s)..."
for format in $formats; do
    git_archive "$format" "$tree" "VTKData-$version" "VTK-$version" || \
        result=1
done


info "Loading large data for $commit..."
rm -f "$GIT_INDEX_FILE"
load_data_objects "$commit" VTK_USE_LARGE_DATA
load_data_files "$commit"
tree="$( git write-tree )"

info "Generating large data archive(s)..."
for format in $formats; do
    git_archive "$format" "$tree" "VTKLargeData-$version" "VTK-$version" || \
        result=1
done

exit "$result"
