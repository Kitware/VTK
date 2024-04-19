#!/bin/bash

set -e

readonly build_path="$1"
shift

readonly yellow=$'\033'"[1;33m"
readonly clear=$'\033'"[0m"

# Check for "Warning" in the last configuration log
conf_file="$( ls -t build/Testing/Temporary/LastConfigure* | head -1 )"
if grep -q -e "Warning" "$conf_file"; then
  echo "${yellow}Configuration warnings detected, please check cdash-commit job${clear}"
  exit 47
fi

# Check that the number of build warnings is zero
num_warnings="$( cat "$build_path/compile_num_warnings.log" )"
readonly num_warnings
if [ "$num_warnings" -gt "0" ]; then
  echo "${yellow}Build warnings detected, please check cdash-commit job${clear}"
  exit 47
fi

:
