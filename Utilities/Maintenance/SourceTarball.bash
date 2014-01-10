#!/usr/bin/env bash
#==========================================================================
#
#   Copyright Insight Software Consortium
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#          http://www.apache.org/licenses/LICENSE-2.0.txt
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#==========================================================================*/

usage() {
  die 'USAGE: SourceTarball.bash [(--tgz|--txz|--zip)...] \
        [--verbose] [-v <version>] [<tag>|<commit>]'
}

info() {
  echo "$@" 1>&2
}

die() {
  echo "$@" 1>&2; exit 1
}

return_pipe_status() {
  echo ${PIPESTATUS[@]} |grep -q -v "[1-9]"
}

find_data_objects() {
  # TODO: Somehow mark large data in the tree instead of hard-coding it here.
  large='Testing/Data/(LSDyna|MFIXReader|NetCDF|SLAC|WindBladeReader)/'
  if test "$2" = "VTK_USE_LARGE_DATA"; then
    v=''
  else
    v='-v'
  fi
  git ls-tree --full-tree -r "$1" |
  egrep $v "$large" |
  egrep '\.(md5)$' |
  while read mode type obj path; do
    case "$path" in
      *.md5)  echo MD5/$(git cat-file blob $obj) ;;
      *)      die "Unknown ExternalData content link: $path" ;;
    esac
  done | sort | uniq
  return_pipe_status
}

if md5sum_type=$(type -p md5sum); then
  compute_MD5() {
    md5sum "$1" | sed 's/ .*//'
  }
elif md5_type=$(type -p md5); then
  compute_MD5() {
    md5 "$1" | sed 's/.*= //'
  }
else
  die 'Neither "md5sum" nor "md5" tool is available.'
fi

validate_MD5() {
  md5sum=$(compute_MD5 "$1") &&
  if test "$md5sum" != "$2"; then
    die "Object MD5/$2 is corrupt: $1"
  fi
}

download_object() {
  algo="$1" ; hash="$2" ; path="$3"
  mkdir -p $(dirname "$path") &&
  if wget "http://www.vtk.org/files/ExternalData/$algo/$hash" -O "$path.tmp$$" 1>&2; then
    mv "$path.tmp$$" "$path"
  else
    rm -f "$path.tmp$$"
    false
  fi
}

index_data_objects() {
  # Input lines have format <algo>/<hash>
  while IFS=/ read algo hash; do
    # Final path in source tarball
    path=".ExternalData/$algo/$hash"
    # Find the object file on disk
    if test -f "$path"; then
      file="$path" # available in place
    elif test -f ~/"$path" ; then
      file=~/"$path" # available in home dir
    else
      download_object "$algo" "$hash" "$path" &&
      file="$path"
    fi &&
    validate_$algo "$file" "$hash" &&
    obj=$(git hash-object -t blob -w "$file") &&
    echo "100644 blob $obj	$path" ||
    return
  done |
  git update-index --index-info
  return_pipe_status
}

load_data_objects() {
  find_data_objects "$@" |
  index_data_objects
  return_pipe_status
}

load_data_files() {
  git ls-tree -r "$1" -- '.ExternalData' |
  git update-index --index-info
  return_pipe_status
}

git_archive_tgz() {
  out="$2.tar.gz" && tmp="$out.tmp$$" &&
  if test -n "$3"; then prefix="$3"; else prefix="$2"; fi &&
  git -c core.autocrlf=false archive $verbose --format=tar --prefix=$prefix/ $1 |
  gzip -9 > "$tmp" &&
  mv "$tmp" "$out" &&
  info "Wrote $out"
}

git_archive_txz() {
  out="$2.tar.xz" && tmp="$out.tmp$$" &&
  if test -n "$3"; then prefix="$3"; else prefix="$2"; fi &&
  git -c core.autocrlf=false archive $verbose --format=tar --prefix=$prefix/ $1 |
  xz -9 > "$tmp" &&
  mv "$tmp" "$out" &&
  info "Wrote $out"
}

git_archive_zip() {
  out="$2.zip" && tmp="$out.tmp$$" &&
  if test -n "$3"; then prefix="$3"; else prefix="$2"; fi &&
  git -c core.autocrlf=true archive $verbose --format=zip --prefix=$prefix/ $1 > "$tmp" &&
  mv "$tmp" "$out" &&
  info "Wrote $out"
}

#-----------------------------------------------------------------------------

formats=
commit=
version=
verbose=

# Parse command line options.
while test $# != 0; do
  case "$1" in
    --tgz) formats="$formats tgz" ;;
    --txz) formats="$formats txz" ;;
    --zip) formats="$formats zip" ;;
    --verbose) verbose=-v ;;
    --) shift; break ;;
    -v) shift; version="$1" ;;
    -*) usage ;;
    *) test -z "$commit" && commit="$1" || usage ;;
  esac
  shift
done
test $# = 0 || usage
test -n "$commit" || commit=HEAD
test -n "$formats" || formats=tgz

if ! git rev-parse --verify -q "$commit" >/dev/null ; then
  die "'$commit' is not a valid commit"
fi
if test -z "$version"; then
  desc=$(git describe $commit) &&
  if test "${desc:0:1}" != "v"; then
    die "'git describe $commit' is '$desc'; use -v <version>"
  fi &&
  version=${desc#v} &&
  echo "$commit is version $version"
fi

# Create temporary git index to construct source tree
export GIT_INDEX_FILE="$(pwd)/tmp-$$-index" &&
trap "rm -f '$GIT_INDEX_FILE'" EXIT &&

result=0 &&

info "Loading source tree from $commit..." &&
rm -f "$GIT_INDEX_FILE" &&
git read-tree -m -i $commit &&
git rm -rf -q --cached '.ExternalData' &&
tree=$(git write-tree) &&

info "Generating source archive(s)..." &&
for fmt in $formats; do
  git_archive_$fmt $tree "VTK-$version" || result=1
done &&

info "Loading normal data for $commit..." &&
rm -f "$GIT_INDEX_FILE" &&
load_data_objects $commit &&
load_data_files $commit &&
tree=$(git write-tree) &&

info "Generating normal data archive(s)..." &&
for fmt in $formats; do
  git_archive_$fmt $tree "VTKData-$version" "VTK-$version" || result=1
done &&

info "Loading large data for $commit..." &&
rm -f "$GIT_INDEX_FILE" &&
load_data_objects $commit VTK_USE_LARGE_DATA &&
tree=$(git write-tree) &&

info "Generating large data archive(s)..." &&
for fmt in $formats; do
  git_archive_$fmt $tree "VTKLargeData-$version" "VTK-$version" || result=1
done &&

exit $result
