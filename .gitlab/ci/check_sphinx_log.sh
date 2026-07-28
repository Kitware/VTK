#!/bin/bash

set -e

readonly sphinx_log_path="$1"
shift

readonly red=$'\033'"[1;31m"
readonly yellow=$'\033'"[1;33m"
readonly clear=$'\033'"[0m"

if grep -q -e "ERROR:" "$sphinx_log_path"; then
  echo "${red}Sphinx errors detected${clear}"
  exit 1
fi

if grep -q -e "WARNING:" "$sphinx_log_path"; then
  echo "${yellow}Sphinx warnings detected${clear}"
  # This is the hardcoded value for warnings used in .warning_policy.yml
  exit 47
fi

:
