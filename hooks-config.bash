#=============================================================================
# Copyright 2010-2012 Kitware, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#=============================================================================

# Make sure GIT_DIR is set.
if test -z "$GIT_DIR"; then
	export GIT_DIR=$(git rev-parse --git-dir)
fi

# Load hooks configuration from source tree.
hooks_config=".hooks-config"
if test -r "$hooks_config"; then
	hooks_config() {
		git config -f "$hooks_config" "$@"
	}
else
	hooks_config() {
		false
	}
fi
config=".hooks-config.bash" && test -r "$config" && . "$config"

# Set up the location for "this" set of hooks.
HOOKS_DIR="${BASH_SOURCE%/*}"

hooks_chain() {
	hook="$1" ; shift
	chain=$(git config --get hooks.chain-$hook) ||
	chain="$(hooks_config --get hooks.chain.$hook)" ||
	eval chain="\${hooks_chain_${hook//-/_}}"
	test -n "$chain" || return 0
	case "$chain" in
	'/'*) prefix="" ;;
	'[A-Za-z]:/'*) prefix="" ;;
	'.'*) prefix="" ;;
	*) prefix="./" ;;
	esac
	if test -x "$prefix$chain" ; then
		exec "$prefix$chain" "$@"
	fi
}

# vim: set filetype=sh tabstop=8 softtabstop=8 shiftwidth=8 noexpandtab :
