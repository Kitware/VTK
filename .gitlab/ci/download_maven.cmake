##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

set(maven_sha256_sum 7ebee30817faef009c7352a876616457c718bccc3be57fc3a0182155ce69d360)
set(maven_url https://dlcdn.apache.org/maven/maven-3/3.9.7/binaries/apache-maven-3.9.7-bin.zip)
set(maven_ext zip)

message("Downloading ${maven_url}")
set(outdir "${CMAKE_SOURCE_DIR}/.gitlab/")
set(archive_path "${outdir}/maven.${maven_ext}")

# Download and extract
file(DOWNLOAD ${maven_url} "${archive_path}" EXPECTED_HASH SHA256=${maven_sha256_sum})
file(ARCHIVE_EXTRACT INPUT "${archive_path}" DESTINATION "${outdir}" VERBOSE)

# Rename subdir to standard naming
file(GLOB maven_path "${outdir}/*maven*-*/")
file(RENAME ${maven_path} "${outdir}/maven/")
